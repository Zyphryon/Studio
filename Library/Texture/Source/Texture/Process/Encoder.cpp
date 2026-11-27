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

#include "Encoder.hpp"
#include <bc7enc.h>
#include <rgbcx.h>
#include <Zyphryon.Graphic/Metadata.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Pack(
        ZyGraphic::TextureFormat               Format,
        ConstRef<bc7enc_compress_block_params> Parameters,
        ConstPtr<Byte>                         Texels,
        Ptr<Byte>                              Block)
    {
        using enum ZyGraphic::TextureFormat;

        switch (Format)
        {
        case BC1UIntNorm:
        case BC1UIntNorm_sRGB:
            // Three-colour blocks are allowed, which never pick the transparent black since alpha is not asked for.
            rgbcx::encode_bc1(rgbcx::MAX_LEVEL, Block, Texels, true, false);
            break;
        case BC2UIntNorm:
        case BC2UIntNorm_sRGB:
            for (UInt32 Index = 0; Index < 16; Index += 2)
            {
                const UInt32 Low  = (Texels[Index * 4 + 3]       * 15 + 127) / 255;
                const UInt32 High = (Texels[(Index + 1) * 4 + 3] * 15 + 127) / 255;

                Block[Index / 2] = static_cast<Byte>(Low | (High << 4));
            }
            rgbcx::encode_bc1(rgbcx::MAX_LEVEL, Block + 8, Texels, false, false);
            break;
        case BC3UIntNorm:
        case BC3UIntNorm_sRGB:
            rgbcx::encode_bc3_hq(rgbcx::MAX_LEVEL, Block, Texels);
            break;
        case BC4UIntNorm:
            rgbcx::encode_bc4_hq(Block, Texels, 1);
            break;
        case BC5UIntNorm:
            rgbcx::encode_bc5_hq(Block, Texels, 0, 1, 2);
            break;
        case BC7UIntNorm:
        case BC7UIntNorm_sRGB:
            bc7enc_compress_block(Block, Texels, AddressOf(Parameters));
            break;
        default:
            break;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ZyGraphic::TextureFormat Encoder::GetSource(ZyGraphic::TextureFormat Format)
    {
        using enum ZyGraphic::TextureFormat;

        switch (Format)
        {
        case BC1UIntNorm:
        case BC2UIntNorm:
        case BC3UIntNorm:
        case BC7UIntNorm:
            return RGBA8UIntNorm;
        case BC1UIntNorm_sRGB:
        case BC2UIntNorm_sRGB:
        case BC3UIntNorm_sRGB:
        case BC7UIntNorm_sRGB:
            return RGBA8UIntNorm_sRGB;
        case BC4UIntNorm:
            return R8UIntNorm;
        case BC5UIntNorm:
            return RG8UIntNorm;
        default:
            return Unspecified;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Encoder::IsSupported(ZyGraphic::TextureFormat Format)
    {
        return GetSource(Format) != ZyGraphic::TextureFormat::Unspecified;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bitmap Encoder::Encode(Ref<ZyJob::Service> Scheduler, ConstRef<Bitmap> Source, ZyGraphic::TextureFormat Format)
    {
        const ZyGraphic::TextureFormat Plain = Source.GetFormat();

        if (!IsSupported(Format) || Plain != GetSource(Format))
        {
            LOG_E("Texture: '{0}' cannot be packed into '{1}'", ZyEnum::GetName(Plain), ZyEnum::GetName(Format));

            return Bitmap();
        }

        // Each encoder builds its tables once, before the first block, and none of them guards its own.
        static const Bool kReady = []
        {
            rgbcx::init();
            bc7enc_compress_block_init();
            return true;
        }();
        (void) kReady;

        const UInt16 Width  = Source.GetWidth();
        const UInt16 Height = Source.GetHeight();
        const UInt8  Levels = Max<UInt8>(Source.GetLevels(), 1);
        const UInt32 Stride = ZyGraphic::GetTextureMetadata(Plain).BitsPerPixel / 8;
        const UInt32 Size   = ZyGraphic::GetTextureMetadata(Format).BitsPerPixel * 2;

        ZY_ASSERT(Levels <= kLevels, "More levels than a 16-bit extent can halve into");

        // The BC7 settings are made once, perceptual for sRGB colour and uniform for anything else.
        bc7enc_compress_block_params Parameters;
        bc7enc_compress_block_params_init(AddressOf(Parameters));
        Parameters.m_uber_level = 2;

        if (Format == ZyGraphic::TextureFormat::BC7UIntNorm)
        {
            bc7enc_compress_block_params_init_linear_weights(AddressOf(Parameters));
        }

        // Blocks are numbered across the whole chain, so the small levels share the workers with the large ones.
        Array<UInt32, kLevels + 1> First;
        Array<UInt32, kLevels + 1> Origin;
        First[0]  = 0;
        Origin[0] = 0;

        for (UInt8 Level = 0; Level < Levels; ++Level)
        {
            First[Level + 1]  = First[Level]  + ZyGraphic::GetLevelSize(Format, Width, Height, Level) / Size;
            Origin[Level + 1] = Origin[Level] + ZyGraphic::GetLevelSize(Plain,  Width, Height, Level);
        }

        Blob Output = Blob::Allocate<Byte>(First[Levels] * Size);

        const ConstPtr<Byte> Input  = Source.GetPixels().GetData();
        const Ptr<Byte>      Target = Output.GetData<Byte>();

        Scheduler.Parallel(ZyJob::Lane::Compute, First[Levels], [&](UInt32 Start, UInt32 End)
        {
            UInt8 Level = 0;

            for (UInt32 Index = Start; Index < End; ++Index)
            {
                while (Index >= First[Level + 1])
                {
                    ++Level;
                }

                const UInt32 Across  = ZyGraphic::GetLevelExtent(Width,  Level);
                const UInt32 Down    = ZyGraphic::GetLevelExtent(Height, Level);
                const UInt32 Columns = (Across + 3) / 4;
                const UInt32 Row     = (Index - First[Level]) / Columns;
                const UInt32 Column  = (Index - First[Level]) % Columns;

                // A level smaller than a block, or an edge that does not fill one, repeats its last texel.
                Array<Byte, 128> Texels;

                for (UInt32 Y = 0; Y < 4; ++Y)
                {
                    const UInt32 SourceY = Min(Row * 4 + Y, Down - 1);

                    for (UInt32 X = 0; X < 4; ++X)
                    {
                        const UInt32         SourceX = Min(Column * 4 + X, Across - 1);
                        const ConstPtr<Byte> Texel   = Input + Origin[Level] + (SourceY * Across + SourceX) * Stride;

                        Copy(Texels.GetData() + (Y * 4 + X) * Stride, Stride, Texel);
                    }
                }
                Pack(Format, Parameters, Texels.GetData(), Target + static_cast<UInt64>(Index) * Size);
            }
        });
        return Bitmap(Format, Width, Height, Levels, Move(Output));
    }
}