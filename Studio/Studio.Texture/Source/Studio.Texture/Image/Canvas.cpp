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

#include "Canvas.hpp"
#include "Texel.hpp"
#include "Studio.Texture/Process/Transcoder.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas::Canvas(UInt16 Width, UInt16 Height)
        : mWidth  { Width },
          mHeight { Height },
          mPixels { Blob::Allocate<Byte>(ZyGraphic::GetLevelSize(kFormat, Width, Height, 0)) }
    {
        Zero(mPixels.GetData<Byte>(), mPixels.GetSize());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Color Canvas::Get(UInt32 X, UInt32 Y) const
    {
        return Load<Real32, 4, false>(mPixels.GetData<Byte>(), Y * mWidth + X);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Color Canvas::Fetch(SInt32 X, SInt32 Y, Bool Wrap) const
    {
        const SInt32 Width  = mWidth;
        const SInt32 Height = mHeight;

        if (Wrap)
        {
            X = (X % Width  + Width)  % Width;
            Y = (Y % Height + Height) % Height;
        }
        else
        {
            X = Clamp(X, 0, Width  - 1);
            Y = Clamp(Y, 0, Height - 1);
        }
        return Get(static_cast<UInt32>(X), static_cast<UInt32>(Y));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Set(UInt32 X, UInt32 Y, ConstRef<Color> Value)
    {
        Store<Real32, 4, false>(mPixels.GetData<Byte>(), Y * mWidth + X, Value);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Blit(ConstRef<Canvas> Source, UInt32 FromX, UInt32 FromY, UInt32 Width, UInt32 Height, UInt32 ToX, UInt32 ToY)
    {
        ZY_ASSERT(FromX + Width <= Source.mWidth && FromY + Height <= Source.mHeight, "Blit reads past its source");
        ZY_ASSERT(ToX + Width <= mWidth && ToY + Height <= mHeight, "Blit writes past its target");

        constexpr UInt32 kTexel = sizeof(Real32) * 4;

        // Both canvases share one format, so each row of the rectangle is a single copy.
        for (UInt32 Row = 0; Row < Height; ++Row)
        {
            const ConstPtr<Byte> From = Source.mPixels.GetData<Byte>() + ((FromY + Row) * Source.mWidth + FromX) * kTexel;
            const Ptr<Byte>      To   = mPixels.GetData<Byte>()        + ((ToY + Row) * mWidth + ToX) * kTexel;

            ::Blit(To, Width * kTexel, From);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bitmap Canvas::To(ZyGraphic::TextureFormat Format) const
    {
        if (!IsValid())
        {
            return Bitmap();
        }

        Bitmap Floats(kFormat, mWidth, mHeight, 1, Blob::Copy(ConstSpan(mPixels.GetData<Byte>(), mPixels.GetSize())));

        return Transcoder::Transcode(Move(Floats), Format);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas Canvas::From(ConstRef<Bitmap> Source)
    {
        if (Source.GetPixels().IsEmpty() || Source.GetWidth() == 0 || Source.GetHeight() == 0)
        {
            return Canvas();
        }

        // Only the base level is converted, so the transcoder is handed a copy of that level alone.
        const UInt32 Length = ZyGraphic::GetLevelSize(Source.GetFormat(), Source.GetWidth(), Source.GetHeight(), 0);
        Bitmap       Level(Source.GetFormat(), Source.GetWidth(), Source.GetHeight(), 1,
            Blob::Copy(Source.GetPixels().Slice(0, Length)));

        const Bitmap Floats = Transcoder::Transcode(Move(Level), kFormat);

        if (Floats.GetPixels().IsEmpty())
        {
            return Canvas();
        }

        Canvas Result;
        Result.mWidth  = Floats.GetWidth();
        Result.mHeight = Floats.GetHeight();
        Result.mPixels = Blob::Copy(Floats.GetPixels());
        return Result;
    }
}