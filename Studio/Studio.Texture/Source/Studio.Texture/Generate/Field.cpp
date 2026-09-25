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

#include "Field.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Field::Field(UInt16 Width, UInt16 Height)
        : mWidth  { Width },
          mHeight { Height }
    {
        mValues.Fill(0.0f, static_cast<UInt>(Width) * Height);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Field::Fetch(SInt32 X, SInt32 Y, Bool Wrap) const
    {
        return Get(Canvas::Reach(X, mWidth, Wrap), Canvas::Reach(Y, mHeight, Wrap));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Field::Blur(Real32 Sigma, Bool Wrap)
    {
        if (Sigma <= 0.0f)
        {
            return;
        }

        // Three sigmas either side hold more than 99% of a gaussian's weight.
        const SInt32 Radius = Max(1, static_cast<SInt32>(Ceil(Sigma * 3.0f)));

        Sequence<Real32> Kernel;
        Kernel.Fill(0.0f, static_cast<UInt>(Radius) * 2 + 1);

        Real32 Total = 0.0f;

        for (SInt32 Offset = -Radius; Offset <= Radius; ++Offset)
        {
            const Real32 Weight = Pow(2.718281828f, -static_cast<Real32>(Offset * Offset) / (2.0f * Sigma * Sigma));

            Kernel[Offset + Radius] = Weight;
            Total += Weight;
        }

        for (Ref<Real32> Weight : Kernel)
        {
            Weight /= Total;
        }

        // A gaussian is separable, so a pass across and then a pass down equal the full 2D kernel at a fraction
        // of the cost.
        Field Across(mWidth, mHeight);

        for (SInt32 Y = 0; Y < mHeight; ++Y)
        {
            for (SInt32 X = 0; X < mWidth; ++X)
            {
                Real32 Sum = 0.0f;

                for (SInt32 Offset = -Radius; Offset <= Radius; ++Offset)
                {
                    Sum += Kernel[Offset + Radius] * Fetch(X + Offset, Y, Wrap);
                }
                Across.Set(X, Y, Sum);
            }
        }

        for (SInt32 Y = 0; Y < mHeight; ++Y)
        {
            for (SInt32 X = 0; X < mWidth; ++X)
            {
                Real32 Sum = 0.0f;

                for (SInt32 Offset = -Radius; Offset <= Radius; ++Offset)
                {
                    Sum += Kernel[Offset + Radius] * Across.Fetch(X, Y + Offset, Wrap);
                }
                Set(X, Y, Sum);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Field Field::From(ConstRef<Canvas> Source, Channel From, Bool Invert)
    {
        Field Result(Source.GetWidth(), Source.GetHeight());

        for (UInt32 Y = 0; Y < Source.GetHeight(); ++Y)
        {
            for (UInt32 X = 0; X < Source.GetWidth(); ++X)
            {
                const Color Pixel = Source.Get(X, Y);

                Real32 Value;

                switch (From)
                {
                case Channel::Red:
                    Value = Pixel.GetRed();
                    break;
                case Channel::Green:
                    Value = Pixel.GetGreen();
                    break;
                case Channel::Blue:
                    Value = Pixel.GetBlue();
                    break;
                case Channel::Alpha:
                    Value = Pixel.GetAlpha();
                    break;
                default:
                    Value = Pixel.GetRed() * 0.2126f + Pixel.GetGreen() * 0.7152f + Pixel.GetBlue() * 0.0722f;
                    break;
                }

                Result.Set(X, Y, Invert ? 1.0f - Value : Value);
            }
        }
        return Result;
    }
}