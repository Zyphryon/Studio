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

#include "Transcoder.hpp"
#include "Texture/Image/Texel.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename From, Bool FromSRGB, typename To, Bool ToSRGB>
    static void Process(
        ConstPtr<Byte> Source, UInt32 SourceChannels,
        Ptr<Byte>      Target, UInt32 TargetChannels, UInt32 Texels)
    {
        // A wider target fills the channels the source never carried, leaving alpha opaque.
        for (UInt32 Index = 0; Index < Texels; ++Index)
        {
            Store<To, ToSRGB>(Target, Index, TargetChannels, Load<From, FromSRGB>(Source, Index, SourceChannels));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static auto Pick(ConstRef<ZyGraphic::TextureMetadata> Source, ConstRef<ZyGraphic::TextureMetadata> Target)
    {
        // The source's colour space, then the target's storage and colour space, flatten into one index.
        const UInt32 Slot = (Source.IsSRGB ? ZyEnum::Count<Component>() * 2 : 0)
                          + ZyEnum::Cast(GetComponent(Target)) * 2 + (Target.IsSRGB ? 1 : 0);

        // Every pairing for one source type, in the order the index above walks them.
        const auto Select = []<typename From>(UInt32 Index)
        {
            using Function = void (*)(ConstPtr<Byte>, UInt32, Ptr<Byte>, UInt32, UInt32);

            static constexpr Function kTable[] =
            {
                Process<From, false, UInt8,  false>, Process<From, false, UInt8,  true>,
                Process<From, false, SInt8,  false>, Process<From, false, SInt8,  true>,
                Process<From, false, UInt16, false>, Process<From, false, UInt16, true>,
                Process<From, false, SInt16, false>, Process<From, false, SInt16, true>,
                Process<From, false, Half,   false>, Process<From, false, Half,   true>,
                Process<From, false, Real32, false>, Process<From, false, Real32, true>,
                Process<From, true,  UInt8,  false>, Process<From, true,  UInt8,  true>,
                Process<From, true,  SInt8,  false>, Process<From, true,  SInt8,  true>,
                Process<From, true,  UInt16, false>, Process<From, true,  UInt16, true>,
                Process<From, true,  SInt16, false>, Process<From, true,  SInt16, true>,
                Process<From, true,  Half,   false>, Process<From, true,  Half,   true>,
                Process<From, true,  Real32, false>, Process<From, true,  Real32, true>,
            };
            return kTable[Index];
        };

        return Dispatch(GetComponent(Source), Select, Slot);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bitmap Transcoder::Transcode(AnyRef<Bitmap> Source, ZyGraphic::TextureFormat Format)
    {
        const ZyGraphic::TextureMetadata Target = ZyGraphic::GetTextureMetadata(Format);
        const ZyGraphic::TextureMetadata Origin = ZyGraphic::GetTextureMetadata(Source.GetFormat());

        if (!IsInterleaved(Target))
        {
            LOG_E("Texture: '{0}' cannot be written as interleaved components", ZyEnum::GetName(Format));

            return Bitmap();
        }

        if (!IsInterleaved(Origin))
        {
            LOG_E("Texture: '{0}' must be unpacked before it can be converted", ZyEnum::GetName(Source.GetFormat()));

            return Bitmap();
        }

        // Already in the target format, so the bitmap is handed back as it is.
        if (Format == Source.GetFormat())
        {
            return Move(Source);
        }

        const UInt16 Width  = Source.GetWidth();
        const UInt16 Height = Source.GetHeight();
        const UInt8  Levels = Max<UInt8>(Source.GetLevels(), 1);

        Blob Output = Blob::Allocate<Byte>(ZyGraphic::GetLevelOffset(Format, Width, Height, Levels));

        const auto Process = Pick(Origin, Target);

        for (UInt8 Level = 0; Level < Levels; ++Level)
        {
            Process(
                Source.GetPixels().GetData() + ZyGraphic::GetLevelOffset(Source.GetFormat(), Width, Height, Level),
                Origin.Components,
                Output.GetData()             + ZyGraphic::GetLevelOffset(Format, Width, Height, Level),
                Target.Components,
                ZyGraphic::GetLevelExtent(Width, Level) * ZyGraphic::GetLevelExtent(Height, Level));
        }
        return Bitmap(Format, Width, Height, Levels, Move(Output));
    }
}