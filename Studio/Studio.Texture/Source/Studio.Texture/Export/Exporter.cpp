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
#include "Studio.Texture/Process/Mipmapper.hpp"
#include "Studio.Texture/Process/Transcoder.hpp"
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
        const ZyGraphic::TextureMetadata Description = ZyGraphic::GetTextureMetadata(Format);

        // TODO: Block-compressed

        // Compressed, packed, depth and unsampleable formats each need an encoder this exporter does not have.
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

    Blob Exporter::Export(Ref<ZyJob::Service> Scheduler, AnyRef<Bitmap> Source, ConstRef<Profile> Profile)
    {
        Sequence<Bitmap> Slices(1);
        Slices.Append(Move(Source));

        return Export(Scheduler, Move(Slices), ZyGraphic::TextureLayout::Texture2D, Profile);
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

        const UInt16 Width  = Slices[0].GetWidth();
        const UInt16 Height = Slices[0].GetHeight();

        const ZyGraphic::TextureFormat Format
            = (Profile.Format == ZyGraphic::TextureFormat::Unspecified) ? Slices[0].GetFormat() : Profile.Format;

        if (!IsSupported(Format))
        {
            LOG_E("Texture: '{0}' is not a format this exporter can write", ZyEnum::GetName(Format));

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

        // Filtering runs at each surface's own depth, so the conversion to the target format happens last.
        const UInt8 Levels = Profile.Mipmaps ? ZyGraphic::GetLevelCount(Width, Height) : 1;

        Sequence<Bitmap> Baked;
        Baked.Resize(Slices.GetSize());

        // TODO: Resizer

        Scheduler.Parallel(ZyJob::Lane::Compute, static_cast<UInt32>(Slices.GetSize()), [&](UInt32 Start, UInt32 End)
        {
            for (UInt32 Index = Start; Index < End; ++Index)
            {
                Baked[Index] = Prepare(Move(Slices[Index]), Levels, Format);
            }
        });

        UInt32 Length = 0;

        for (ConstRef<Bitmap> Result : Baked)
        {
            if (Result.GetPixels().IsEmpty())
            {
                return Blob();
            }
            Length += Result.GetPixels().GetSize();
        }

        // Gathered slice-major, which is the order the loader and both drivers read a sliced payload in.
        Blob      Payload = Blob::Allocate<Byte>(Length);
        Ptr<Byte> Cursor  = Payload.GetData<Byte>();

        for (ConstRef<Bitmap> Result : Baked)
        {
            const ConstSpan<Byte> Bytes = Result.GetPixels();

            Copy(Cursor, Bytes.GetSize(), Bytes.GetData());
            Cursor += Bytes.GetSize();
        }

        const ConstSpan<Byte> Bytes = ConstSpan(Payload.GetData<Byte>(), Length);

        Writer Output(Length + 32);
        Output.Write<UInt32>(kMagic);
        Output.Write<UInt16>(kVersion);
        Output.Write<ZyGraphic::TextureLayout>(Layout);
        Output.Write<ZyGraphic::TextureFormat>(Format);
        Output.Write<UInt16>(Width);
        Output.Write<UInt16>(Height);
        Output.Write<UInt16>(static_cast<UInt16>(Baked.GetSize()));
        Output.Write<UInt8>(Baked[0].GetLevels());
        Output.Write<UInt32>(Length);

        // A payload as long as the raw count reads as uncompressed, so only one that shrank is kept.
        if (Profile.Compress)
        {
            Blob         Scratch = Blob::Allocate<Byte>(LZ4Bound(Length));
            const UInt32 Size    = LZ4Encode(Bytes, Scratch.GetData<Byte>(), LZ4Bound(Length), kCompression);

            if (Size > 0 && Size < Length)
            {
                Output.WriteBlock<UInt32, Byte>(ConstSpan(Scratch.GetData(), Size));
                return Output.Detach();
            }
        }

        Output.WriteBlock<UInt32, Byte>(Bytes);
        return Output.Detach();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bitmap Exporter::Prepare(AnyRef<Bitmap> Source, UInt8 Levels, ZyGraphic::TextureFormat Format)
    {
        return Transcoder::Transcode(Mipmapper::Generate(Move(Source), Levels), Format);
    }
}