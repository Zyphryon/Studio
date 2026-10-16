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

    static void Pack(ZyGraphic::TextureFormat Format, ConstPtr<Byte> Texels, Ptr<Byte> Block)
    {
        using enum ZyGraphic::TextureFormat;

        switch (Format)
        {
        case BC1UIntNorm:
        case BC1UIntNorm_sRGB:
            rgbcx::encode_bc1(rgbcx::MAX_LEVEL, Block, Texels, false, false);
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
        {
            bc7enc_compress_block_params Parameters;
            bc7enc_compress_block_params_init(AddressOf(Parameters));
            Parameters.m_uber_level = 2;

            if (Format == BC7UIntNorm)
            {
                bc7enc_compress_block_params_init_linear_weights(AddressOf(Parameters));
            }
            bc7enc_compress_block(Block, Texels, AddressOf(Parameters));
            break;
        }
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

        Blob Output = Blob::Allocate<Byte>(ZyGraphic::GetLevelOffset(Format, Width, Height, Levels));

        const ConstPtr<Byte> Input  = Source.GetPixels().GetData();
        const Ptr<Byte>      Target = Output.GetData<Byte>();

        for (UInt8 Level = 0; Level < Levels; ++Level)
        {
            const UInt32 Across = ZyGraphic::GetLevelExtent(Width,  Level);
            const UInt32 Down   = ZyGraphic::GetLevelExtent(Height, Level);

            const ConstPtr<Byte> Pixels = Input  + ZyGraphic::GetLevelOffset(Plain,  Width, Height, Level);
            const Ptr<Byte>      Blocks = Target + ZyGraphic::GetLevelOffset(Format, Width, Height, Level);

            const UInt32 Columns = (Across + 3) / 4;
            const UInt32 Rows    = (Down   + 3) / 4;

            // Every row of blocks is packed on its own, so one large image is spread over every worker.
            Scheduler.Parallel(ZyJob::Lane::Compute, Rows, [&](UInt32 First, UInt32 Last)
            {
                for (UInt32 Row = First; Row < Last; ++Row)
                {
                    for (UInt32 Column = 0; Column < Columns; ++Column)
                    {
                        // A level smaller than a block, or an edge that does not fill one, repeats its last texel.
                        Array<Byte, 128> Texels;

                        for (UInt32 Y = 0; Y < 4; ++Y)
                        {
                            const UInt32 SourceY = Min(Row * 4 + Y, Down - 1);

                            for (UInt32 X = 0; X < 4; ++X)
                            {
                                const UInt32         SourceX = Min(Column * 4 + X, Across - 1);
                                const ConstPtr<Byte> Texel   = Pixels + (SourceY * Across + SourceX) * Stride;

                                Copy(Texels.GetData() + (Y * 4 + X) * Stride, Stride, Texel);
                            }
                        }
                        Pack(Format, Texels.GetData(), Blocks + (Row * Columns + Column) * Size);
                    }
                }
            });
        }
        return Bitmap(Format, Width, Height, Levels, Move(Output));
    }
}