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

#include "Importer.hpp"
#include "Texture/Export/Exporter.hpp"
#include <Zyphryon.Graphic/Metadata.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static ZyGraphic::TextureFormat Resolve(SInt32 Channels, Bool Real, Bool Wide, Bool sRGB)
    {
        switch (Channels)
        {
        case 1:
            return Real ? ZyGraphic::TextureFormat::R32Float
                 : Wide ? ZyGraphic::TextureFormat::R16UIntNorm
                        : ZyGraphic::TextureFormat::R8UIntNorm;
        case 2:
            return Real ? ZyGraphic::TextureFormat::RG32Float
                 : Wide ? ZyGraphic::TextureFormat::RG16UIntNorm
                        : ZyGraphic::TextureFormat::RG8UIntNorm;
        default:
            return Real ? ZyGraphic::TextureFormat::RGBA32Float
                 : Wide ? ZyGraphic::TextureFormat::RGBA16UIntNorm
                 : sRGB ? ZyGraphic::TextureFormat::RGBA8UIntNorm_sRGB
                        : ZyGraphic::TextureFormat::RGBA8UIntNorm;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Surface ImportSTB(ConstSpan<Byte> Source, ConstRef<Profile> Profile)
    {
        const ConstPtr<stbi_uc> Encoded = Source.GetData();
        const SInt32            Length  = static_cast<SInt32>(Source.GetSize());

        SInt32 Width    = 0;
        SInt32 Height   = 0;
        SInt32 Channels = 0;

        if (!stbi_info_from_memory(Encoded, Length, AddressOf(Width), AddressOf(Height), AddressOf(Channels)))
        {
            LOG_E("Texture: failed to read source image header ({0})", StrConvert(stbi_failure_reason()));

            return Surface();
        }

        // Each depth gets its own loader, since the 8-bit one would tone-map or truncate a wider source.
        const Bool Real = stbi_is_hdr_from_memory(Encoded, Length);
        const Bool Wide = !Real && stbi_is_16_bit_from_memory(Encoded, Length);

        // The engine stores sRGB only at 8 bits, so a wider source stays linear.
        const Bool sRGB = !Profile.Linear && !Real && !Wide;

        if (!Profile.Linear && (Real || Wide))
        {
            LOG_W("Texture: sRGB was requested but the source is {0}, which the engine only stores as linear",
                Real ? "floating-point"_Text : "16-bit"_Text);
        }

        // The engine has no three-channel format and sRGB only as RGBA, so both cases expand to four channels.
        const Bool   Expand  = Channels == 3 || (sRGB && Channels < 4);
        const SInt32 Request = Expand ? STBI_rgb_alpha : Channels;

        if (sRGB && Channels < 3)
        {
            LOG_W("Texture: expanding a {0}-channel sRGB source to RGBA, the only sRGB format the engine has", Channels);
        }

        Ptr<void> Pixels;

        if (Real)
        {
            Pixels = static_cast<Ptr<void>>(
                stbi_loadf_from_memory(Encoded, Length, AddressOf(Width), AddressOf(Height), AddressOf(Channels), Request));
        }
        else if (Wide)
        {
            Pixels = static_cast<Ptr<void>>(
                stbi_load_16_from_memory(Encoded, Length, AddressOf(Width), AddressOf(Height), AddressOf(Channels), Request));
        }
        else
        {
            Pixels = static_cast<Ptr<void>>(
                stbi_load_from_memory(Encoded, Length, AddressOf(Width), AddressOf(Height), AddressOf(Channels), Request));
        }

        if (Pixels == nullptr)
        {
            LOG_E("Texture: failed to decode source image ({0})", StrConvert(stbi_failure_reason()));

            return Surface();
        }

        if (Width > kMaximum<UInt16> || Height > kMaximum<UInt16>)
        {
            LOG_E("Texture: source image is {0}x{1}, which exceeds the {2} pixel limit", Width, Height, kMaximum<UInt16>);

            stbi_image_free(Pixels);
            return Surface();
        }

        const ZyGraphic::TextureFormat Format = Resolve(Request, Real, Wide, sRGB);

        // Adopt the decoder's buffer rather than copying it; the blob releases it through the matching free.
        const UInt32 Size = ZyGraphic::GetLevelSize(Format, static_cast<UInt16>(Width), static_cast<UInt16>(Height), 0);

        Blob Data(static_cast<Ptr<Byte>>(Pixels), Size, Blob::Deleter([](Ptr<Byte> Address)
        {
            stbi_image_free(Address);
        }));
        return Surface::From(Bitmap(Format, static_cast<UInt16>(Width), static_cast<UInt16>(Height), 1, Move(Data)));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Surface ImportTEX(ConstSpan<Byte> Source)
    {
        Reader Input(Source);

        if (Input.Read<UInt32>() != Exporter::kMagic)
        {
            LOG_E("Texture: source is not a ZTEX file (bad magic)");

            return Surface();
        }

        if (const UInt16 Version = Input.Read<UInt16>(); Version != Exporter::kVersion)
        {
            LOG_E("Texture: ZTEX version {0} is not one this importer reads", Version);

            return Surface();
        }

        const ZyGraphic::TextureLayout Layout  = Input.Read<ZyGraphic::TextureLayout>();
        const ZyGraphic::TextureFormat Format  = Input.Read<ZyGraphic::TextureFormat>();
        const UInt16                   Width   = Input.Read<UInt16>();
        const UInt16                   Height  = Input.Read<UInt16>();
        const UInt16                   Layers  = Input.Read<UInt16>();
        const UInt8                    Levels  = Input.Read<UInt8>();
        const Blob                     Payload = Input.ReadBlockCompressed<UInt32>();

        if (Width == 0 || Height == 0 || Layers == 0 || Levels == 0 || !Payload)
        {
            LOG_E("Texture: ZTEX header describes no surface to read back");

            return Surface();
        }

        // Later stages read one texel at a time, which a compressed or packed payload cannot give.
        const ZyGraphic::TextureMetadata Description = ZyGraphic::GetTextureMetadata(Format);

        if (Description.IsCompressed() || Description.IsPacked || !Description.IsSampler())
        {
            LOG_E("Texture: '{0}' stores no plain interleaved texels, so it cannot be read back", ZyEnum::GetName(Format));

            return Surface();
        }

        // Slices follow one another with their whole chain, so the header fixes the payload's length.
        const UInt32 Stride = ZyGraphic::GetLevelOffset(Format, Width, Height, Levels);

        if (static_cast<UInt64>(Stride) * Layers != Payload.GetSize())
        {
            LOG_E("Texture: ZTEX payload is {0} bytes, but {1}x{2} over {3} slice(s) needs {4}",
                Payload.GetSize(), Width, Height, Layers, static_cast<UInt64>(Stride) * Layers);

            return Surface();
        }

        const ConstPtr<Byte> Pixels = Payload.GetData<Byte>();

        // Later stages take a single level, so each slice keeps its base level and the chain is built again.
        const UInt32 Length = ZyGraphic::GetLevelSize(Format, Width, Height, 0);

        Surface Result;
        Result.Layout = Layout;
        Result.Slices.Reserve(Layers);

        for (UInt32 Slice = 0; Slice < Layers; ++Slice)
        {
            Blob Data = Blob::Copy(ConstSpan(Pixels + static_cast<UInt>(Slice) * Stride, Length));

            Result.Slices.Append(Format, Width, Height, 1, Move(Data));
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Importer::Accepts(Text Type)
    {
        for (const Text Entry : kTypes)
        {
            if (StrEqualCaseInsensitive(Entry, Type))
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Surface Importer::Import(ConstSpan<Byte> Source, Text Type, ConstRef<Profile> Profile)
    {
        if (!Accepts(Type))
        {
            LOG_E("Texture: '{0}' is not a source format this baker understands", Type);

            return Surface();
        }

        if (Source.IsEmpty())
        {
            LOG_E("Texture: source image is empty");

            return Surface();
        }
        return StrEqualCaseInsensitive(Type, "tex") ? ImportTEX(Source) : ImportSTB(Source, Profile);
    }
}