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

#include "Resampler.hpp"
#include "Texture/Image/Texel.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Type, Bool sRGB>
    static void Process(
        ConstPtr<Byte> Source, UInt32 SourceWidth, UInt32 SourceHeight,
        Ptr<Byte>      Target, UInt32 TargetWidth, UInt32 TargetHeight, UInt32 Channels)
    {
        for (UInt32 Y = 0; Y < TargetHeight; ++Y)
        {
            const UInt32 MinY = Y * SourceHeight / TargetHeight;
            const UInt32 MaxY = Max<UInt32>((Y + 1) * SourceHeight / TargetHeight, MinY + 1);

            for (UInt32 X = 0; X < TargetWidth; ++X)
            {
                const UInt32 MinX = X * SourceWidth / TargetWidth;
                const UInt32 MaxX = Max<UInt32>((X + 1) * SourceWidth / TargetWidth, MinX + 1);

                Color  Total(0.0f, 0.0f, 0.0f, 0.0f);
                UInt32 Count = 0;

                for (UInt32 Row = MinY; Row < MaxY; ++Row)
                {
                    for (UInt32 Column = MinX; Column < MaxX; ++Column)
                    {
                        Total += Load<Type, sRGB>(Source, Row * SourceWidth + Column, Channels);
                        ++Count;
                    }
                }

                Store<Type, sRGB>(Target, Y * TargetWidth + X, Channels, Total * (1.0f / static_cast<Real32>(Count)));
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static auto Pick(ConstRef<ZyGraphic::TextureMetadata> Format)
    {
        const auto Select = []<typename Type>(UInt32 Index)
        {
            using Return = void (*)(ConstPtr<Byte>, UInt32, UInt32, Ptr<Byte>, UInt32, UInt32, UInt32);

            static constexpr Return kTable[] =
            {
                Process<Type, false>, Process<Type, true>,
            };
            return kTable[Index];
        };

        return Dispatch(GetComponent(Format), Select, Format.IsSRGB ? 1 : 0);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bitmap Resampler::Resize(ConstRef<Bitmap> Source, UInt16 Width, UInt16 Height)
    {
        const ZyGraphic::TextureFormat   Format   = Source.GetFormat();
        const ZyGraphic::TextureMetadata Metadata = ZyGraphic::GetTextureMetadata(Format);

        if (!IsInterleaved(Metadata))
        {
            LOG_E("Texture: '{0}' must have its texels unpacked before it can be filtered", ZyEnum::GetName(Format));

            return Bitmap();
        }

        if (Width == 0 || Height == 0 || Source.GetWidth() == 0 || Source.GetHeight() == 0)
        {
            LOG_E("Texture: neither end of a filter can be empty");

            return Bitmap();
        }

        Blob Output = Blob::Allocate<Byte>(ZyGraphic::GetLevelSize(Format, Width, Height, 0));

        if (Source.GetWidth() == Width && Source.GetHeight() == Height)
        {
            Copy(Output.GetData<Byte>(), Output.GetSize(), Source.GetPixels().GetData());
        }
        else
        {
            Pick(Metadata)(
                Source.GetPixels().GetData(), Source.GetWidth(), Source.GetHeight(),
                Output.GetData<Byte>(), Width, Height, Metadata.Components);
        }
        return Bitmap(Format, Width, Height, 1, Move(Output));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bitmap Resampler::Mipmap(AnyRef<Bitmap> Source, UInt8 Levels)
    {
        const ZyGraphic::TextureFormat   Format   = Source.GetFormat();
        const ZyGraphic::TextureMetadata Metadata = ZyGraphic::GetTextureMetadata(Format);

        if (!IsInterleaved(Metadata))
        {
            LOG_E("Texture: '{0}' must have its texels unpacked before it can be filtered", ZyEnum::GetName(Format));

            return Bitmap();
        }

        const UInt16 Width  = Source.GetWidth();
        const UInt16 Height = Source.GetHeight();

        ZY_ASSERT(Source.GetLevels() == 1, "A chain can only be built from a single level");
        ZY_ASSERT(Levels <= ZyGraphic::GetLevelCount(Width, Height), "More levels than the extent can halve into");

        // With nothing to filter, the bitmap is handed back as it is rather than copied.
        if (Levels <= 1)
        {
            return Move(Source);
        }

        Blob Output = Blob::Allocate<Byte>(ZyGraphic::GetLevelOffset(Format, Width, Height, Levels));

        const Ptr<Byte> Target = Output.GetData<Byte>();

        // The base level is copied as it is, and each later level is averaged down from the one above it.
        Copy(Target, ZyGraphic::GetLevelSize(Format, Width, Height, 0), Source.GetPixels().GetData());

        const auto Process = Pick(Metadata);

        for (UInt8 Level = 1; Level < Levels; ++Level)
        {
            Process(
                Target + ZyGraphic::GetLevelOffset(Format, Width, Height, Level - 1),
                ZyGraphic::GetLevelExtent(Width,  Level - 1),
                ZyGraphic::GetLevelExtent(Height, Level - 1),
                Target + ZyGraphic::GetLevelOffset(Format, Width, Height, Level),
                ZyGraphic::GetLevelExtent(Width,  Level),
                ZyGraphic::GetLevelExtent(Height, Level),
                Metadata.Components);
        }
        return Bitmap(Format, Width, Height, Levels, Move(Output));
    }
}