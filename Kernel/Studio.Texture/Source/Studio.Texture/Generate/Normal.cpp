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

#include "Normal.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief The alpha below which a pixel counts as empty.
    static constexpr Real32 kOpaque = 0.5f;

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Measure(ConstRef<Field> Height, SInt32 X, SInt32 Y, Normal::Kernel Filter, Bool Wrap, Ref<Real32> DX, Ref<Real32> DY)
    {
        const auto At = [&](SInt32 OffsetX, SInt32 OffsetY)
        {
            return Height.Fetch(X + OffsetX, Y + OffsetY, Wrap);
        };

        if (Filter == Normal::Kernel::Central)
        {
            DX = (At(1, 0) - At(-1, 0)) * 0.5f;
            DY = (At(0, 1) - At(0, -1)) * 0.5f;
            return;
        }

        // The three 3x3 kernels differ only in how much the centre row counts against the outer two.
        Real32 Side;
        Real32 Middle;

        switch (Filter)
        {
        case Normal::Kernel::Scharr:
            Side   = 3.0f;
            Middle = 10.0f;
            break;
        case Normal::Kernel::Prewitt:
            Side   = 1.0f;
            Middle = 1.0f;
            break;
        default:
            Side   = 1.0f;
            Middle = 2.0f;
            break;
        }

        // Normalized so every kernel reads a unit ramp as a slope of one.
        const Real32 Scale = 1.0f / (2.0f * (2.0f * Side + Middle));

        DX = ((At(1, -1) - At(-1, -1)) * Side + (At(1, 0) - At(-1, 0)) * Middle + (At(1, 1) - At(-1, 1)) * Side) * Scale;
        DY = ((At(-1, 1) - At(-1, -1)) * Side + (At(0, 1) - At(0, -1)) * Middle + (At(1, 1) - At(1, -1)) * Side) * Scale;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Normal::Settings Normal::Settings::From(ConstRef<Environment> Environment)
    {
        Settings Result;
        Result.Source   = Environment.GetEnum<Channel>("channel",  Result.Source);
        Result.Filter   = Environment.GetEnum<Kernel>("filter",    Result.Filter);
        Result.Strength = Environment.GetNumber<Real32>("strength", Result.Strength);
        Result.Blur     = Environment.GetNumber<Real32>("blur",     Result.Blur);
        Result.Octaves  = Clamp(Environment.GetNumber<UInt8>("octaves", Result.Octaves), UInt8(1), UInt8(8));
        Result.Falloff  = Environment.GetNumber<Real32>("falloff",  Result.Falloff);
        Result.Invert   = Environment.GetBool("invert",  Result.Invert);
        Result.FlipX    = Environment.GetBool("flip-x",  Result.FlipX);
        Result.FlipY    = Environment.GetBool("flip-y",  Result.FlipY);
        Result.Wrap     = Environment.GetBool("wrap",    Result.Wrap);
        Result.Masked   = Environment.GetBool("masked",  Result.Masked);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas Normal::Generate(ConstRef<Canvas> Source, ConstRef<Settings> Settings)
    {
        if (!Source.IsValid())
        {
            return Canvas();
        }

        const UInt16 Width  = Source.GetWidth();
        const UInt16 Height = Source.GetHeight();

        Field Base = Field::From(Source, Settings.Source, Settings.Invert);

        // Where nothing is drawn is ground, so the art's outline reads as the edge of a raised shape.
        if (Settings.Masked)
        {
            for (UInt32 Y = 0; Y < Height; ++Y)
            {
                for (UInt32 X = 0; X < Width; ++X)
                {
                    if (Source.Get(X, Y).GetAlpha() < kOpaque)
                    {
                        Base.Set(X, Y, 0.0f);
                    }
                }
            }
        }

        Base.Blur(Settings.Blur, Settings.Wrap);

        Sequence<Real32> SlopeX;
        Sequence<Real32> SlopeY;
        SlopeX.Resize(static_cast<UInt>(Width) * Height, 0.0f);
        SlopeY.Resize(static_cast<UInt>(Width) * Height, 0.0f);

        // Each octave measures a copy blurred twice as wide, so broad shapes and fine grain both show.
        Real32 Weight = 1.0f;

        for (UInt8 Octave = 0; Octave < Settings.Octaves; ++Octave)
        {
            Field Scale = Base;

            if (Octave > 0)
            {
                Scale.Blur(static_cast<Real32>(1u << Octave) * 0.5f, Settings.Wrap);
            }

            for (SInt32 Y = 0; Y < Height; ++Y)
            {
                for (SInt32 X = 0; X < Width; ++X)
                {
                    Real32 DX;
                    Real32 DY;
                    Measure(Scale, X, Y, Settings.Filter, Settings.Wrap, DX, DY);

                    SlopeX[Y * Width + X] += DX * Weight;
                    SlopeY[Y * Width + X] += DY * Weight;
                }
            }
            Weight *= Settings.Falloff;
        }

        Canvas Result(Width, Height);

        for (UInt32 Y = 0; Y < Height; ++Y)
        {
            for (UInt32 X = 0; X < Width; ++X)
            {
                const Real32 Alpha = Source.Get(X, Y).GetAlpha();

                // A normal faces away from where the height climbs. Rows run down the image while the map's up
                // runs up the card, so the vertical slope enters with its sign kept.
                Real32 NX = -SlopeX[Y * Width + X] * Settings.Strength;
                Real32 NY =  SlopeY[Y * Width + X] * Settings.Strength;
                Real32 NZ = 1.0f;

                if (Settings.FlipX)
                {
                    NX = -NX;
                }
                if (Settings.FlipY)
                {
                    NY = -NY;
                }

                if (Settings.Masked && Alpha < kOpaque)
                {
                    NX = 0.0f;
                    NY = 0.0f;
                }

                const Real32 Length = Sqrt(NX * NX + NY * NY + NZ * NZ);

                NX /= Length;
                NY /= Length;
                NZ /= Length;

                Result.Set(X, Y, Color(NX * 0.5f + 0.5f, NY * 0.5f + 0.5f, NZ * 0.5f + 0.5f, Alpha));
            }
        }
        return Result;
    }
}
