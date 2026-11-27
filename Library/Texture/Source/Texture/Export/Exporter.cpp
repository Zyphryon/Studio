// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Exporter.hpp"
#include "Texture/Process/Encoder.hpp"
#include "Texture/Process/Resampler.hpp"
#include <Zyphryon.Graphic/Metadata.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Exporter::IsSupported(ZyGraphic::TextureFormat Format)
    {
        if (Encoder::IsSupported(Format))
        {
            return true;
        }

        const ZyGraphic::TextureMetadata Description = ZyGraphic::GetTextureMetadata(Format);

        // Other compressed, packed, depth and unsampleable formats each need an encoder this exporter does not have.
        if (Description.IsCompressed() ||  Description.IsPacked || Description.IsDepth
         || Description.IsStencil      || !Description.IsSampler())
        {
            return false;
        }

        // A raw integer format would have the sampler's values reinterpreted rather than scaled.
        if (!Description.IsFloat && !Description.IsNormalized)
        {
            return false;
        }

        const UInt32 Bits = Description.BitsPerComponent();
        return Description.IsFloat ? (Bits == 16 || Bits == 32) : (Bits == 8 || Bits == 16);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Exporter::Export(
        Ref<ZyJob::Service>      Scheduler,
        AnyRef<Sequence<Bitmap>> Slices,
        ZyGraphic::TextureLayout Layout,
        ConstRef<Profile>        Profile)
    {
        if (Slices.IsEmpty())
        {
            return Blob();
        }

        // The header counts layers in 16 bits, so more slices than that would be written as some other number.
        if (Slices.GetSize() > kMaximum<UInt16>)
        {
            LOG_E("Texture: {0} slices are more than the {1} one texture can hold", Slices.GetSize(), kMaximum<UInt16>);

            return Blob();
        }

        const UInt16 Width  = Slices[0].GetWidth();
        const UInt16 Height = Slices[0].GetHeight();

        const ZyGraphic::TextureFormat Format
            = (Profile.Format == ZyGraphic::TextureFormat::Unspecified) ? Slices[0].GetFormat() : Profile.Format;

        if (!IsSupported(Format))
        {
            LOG_E("Texture: '{0}' is not a format this exporter can write", ZyEnum::GetName(Format));

            return Blob();
        }

        if (Encoder::IsSupported(Format) && (Width % 4 != 0 || Height % 4 != 0))
        {
            LOG_E("Texture: '{0}' needs sides that are multiples of four, which {1}x{2} are not",
                ZyEnum::GetName(Format), Width, Height);

            return Blob();
        }

        for (ConstRef<Bitmap> Slice : Slices)
        {
            if (Slice.GetWidth() != Width || Slice.GetHeight() != Height)
            {
                LOG_E("Texture: every slice must share one extent ({0}x{1} against {2}x{3})",
                    Slice.GetWidth(), Slice.GetHeight(), Width, Height);

                return Blob();
            }
        }

        const UInt8 Levels = Profile.Mipmaps ? ZyGraphic::GetLevelCount(Width, Height) : 1;

        Sequence<Bitmap> Baked;
        Baked.Resize(Slices.GetSize());

        Scheduler.Parallel(ZyJob::Lane::Compute, static_cast<UInt32>(Slices.GetSize()), [&](UInt32 Start, UInt32 End)
        {
            for (UInt32 Index = Start; Index < End; ++Index)
            {
                Baked[Index] = Prepare(Scheduler, Move(Slices[Index]), Levels, Format, Profile);
            }
        });

        UInt64 Total = 0;

        for (ConstRef<Bitmap> Result : Baked)
        {
            if (Result.GetPixels().IsEmpty())
            {
                return Blob();
            }
            Total += Result.GetPixels().GetSize();
        }

        // The payload's length is written in 32 bits, so slices that add up past it cannot be stored.
        if (Total > kMaximum<UInt32>)
        {
            LOG_E("Texture: {0} slice(s) come to {1} bytes, past what one texture can hold", Baked.GetSize(), Total);

            return Blob();
        }

        const UInt32 Length = static_cast<UInt32>(Total);

        // Several slices are gathered slice-major, which is the order the loader and both drivers read a payload in.
        Blob Payload;

        if (Baked.GetSize() > 1)
        {
            Payload = Blob::Allocate<Byte>(Length);

            Ptr<Byte> Cursor = Payload.GetData<Byte>();

            for (ConstRef<Bitmap> Result : Baked)
            {
                const ConstSpan<Byte> Bytes = Result.GetPixels();

                Copy(Cursor, Bytes.GetSize(), Bytes.GetData());
                Cursor += Bytes.GetSize();
            }
        }

        // A lone slice is written from where it was baked.
        const ConstSpan<Byte> Bytes = Payload ? ConstSpan(Payload.GetData<Byte>(), Length) : Baked[0].GetPixels();

        Writer Output(Length + 32);
        Output.Write<UInt32>(kMagic);
        Output.Write<UInt16>(kVersion);
        Output.Write(Layout);
        Output.Write(Format);
        Output.Write<UInt16>(Width);
        Output.Write<UInt16>(Height);
        Output.Write<UInt16>(static_cast<UInt16>(Baked.GetSize()));
        Output.Write<UInt8>(Baked[0].GetLevels());

        if (Profile.Compress)
        {
            Output.WriteBlockCompressed<UInt32>(Bytes, 9);
        }
        else
        {
            Output.Write<UInt32>(Length);
            Output.WriteBlock<UInt32, Byte>(Bytes);
        }
        return Output.Detach();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bitmap Exporter::Prepare(
        Ref<ZyJob::Service>      Scheduler,
        AnyRef<Bitmap>           Source,
        UInt8                    Levels,
        ZyGraphic::TextureFormat Format)
    {
        return Prepare(Scheduler, Move(Source), Levels, Format, Profile());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bitmap Exporter::Prepare(
        Ref<ZyJob::Service>      Scheduler,
        AnyRef<Bitmap>           Source,
        UInt8                    Levels,
        ZyGraphic::TextureFormat Format,
        ConstRef<Profile>        Profile)
    {
        // A block format is packed from the plain one it decodes to, once every level is filtered and converted.
        const Bool                     Packed = Encoder::IsSupported(Format);
        const ZyGraphic::TextureFormat Plain  = Packed ? Encoder::GetSource(Format) : Format;

        Bitmap Chain = Resampler::Mipmap(Scheduler, Move(Source), Levels, Plain, Profile);

        if (!Packed || Chain.GetPixels().IsEmpty())
        {
            return Chain;
        }
        return Encoder::Encode(Scheduler, Chain, Format);
    }
}