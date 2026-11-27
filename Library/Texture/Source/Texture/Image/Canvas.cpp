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
#include "Texture/Process/Transcoder.hpp"

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
        ZY_ASSERT(Fits(Width, Height), "A canvas must fit a 32-bit size, which its callers check first");

        Zero(mPixels.GetData<Byte>(), mPixels.GetSize());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Color Canvas::Get(UInt32 X, UInt32 Y) const
    {
        return Load<Real32, false>(mPixels.GetData<Byte>(), Y * mWidth + X, 4);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Color Canvas::Fetch(SInt32 X, SInt32 Y, Bool Wrap) const
    {
        return Get(Reach(X, mWidth, Wrap), Reach(Y, mHeight, Wrap));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Set(UInt32 X, UInt32 Y, ConstRef<Color> Value)
    {
        Store<Real32, false>(mPixels.GetData<Byte>(), Y * mWidth + X, 4, Value);
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

        const UInt32 Texels = static_cast<UInt32>(mWidth) * mHeight;
        Blob         Output = Blob::Allocate<Byte>(ZyGraphic::GetLevelSize(Format, mWidth, mHeight, 0));

        if (!Transcoder::Convert(mPixels.GetData<Byte>(), kFormat, Output.GetData<Byte>(), Format, Texels))
        {
            return Bitmap();
        }
        return Bitmap(Format, mWidth, mHeight, 1, Move(Output));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas Canvas::From(ConstRef<Bitmap> Source)
    {
        if (Source.GetPixels().IsEmpty() || Source.GetWidth() == 0 || Source.GetHeight() == 0)
        {
            return Canvas();
        }

        // Only the base level is read, and it is converted straight into the canvas.
        Canvas Result;
        Result.mWidth  = Source.GetWidth();
        Result.mHeight = Source.GetHeight();
        Result.mPixels = Blob::Allocate<Byte>(ZyGraphic::GetLevelSize(kFormat, Result.mWidth, Result.mHeight, 0));

        const UInt32    Texels = static_cast<UInt32>(Result.mWidth) * Result.mHeight;
        const Ptr<Byte> Target = Result.mPixels.GetData<Byte>();

        if (!Transcoder::Convert(Source.GetPixels().GetData(), Source.GetFormat(), Target, kFormat, Texels))
        {
            return Canvas();
        }
        return Result;
    }
}