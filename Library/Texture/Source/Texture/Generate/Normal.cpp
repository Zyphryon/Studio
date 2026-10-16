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
#include <Zyphryon.Math/Vector2.hpp>
#include <Zyphryon.Math/Vector3.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Vector2 Measure(ConstRef<Field> Height, SInt32 X, SInt32 Y, Real32 Side, Real32 Middle, Bool Wrap)
    {
        const auto At = [&](SInt32 OffsetX, SInt32 OffsetY)
        {
            return Height.Fetch(X + OffsetX, Y + OffsetY, Wrap);
        };

        // Normalized so every kernel reads a unit ramp as a slope of one.
        const Real32 Scale = 1.0f / (2.0f * (2.0f * Side + Middle));

        return Vector2(
            ((At(1, -1) - At(-1, -1)) * Side + (At(1, 0) - At(-1, 0)) * Middle + (At(1, 1) - At(-1, 1)) * Side) * Scale,
            ((At(-1, 1) - At(-1, -1)) * Side + (At(0, 1) - At(0, -1)) * Middle + (At(1, 1) - At(1, -1)) * Side) * Scale);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Normal::Settings Normal::Settings::From(ConstRef<Environment> Environment)
    {
        Settings Result;
        Result.Input    = Elevation::From(Environment);
        Result.Filter   = Environment.GetEnum<Kernel>("filter",    Result.Filter);
        Result.Strength = Environment.GetNumber<Real32>("strength", Result.Strength);
        Result.Octaves  = Clamp(Environment.GetNumber<UInt8>("octaves", Result.Octaves), UInt8(1), UInt8(8));
        Result.Falloff  = Environment.GetNumber<Real32>("falloff",  Result.Falloff);
        Result.FlipX    = Environment.GetBool("flip-x",  Result.FlipX);
        Result.FlipY    = Environment.GetBool("flip-y",  Result.FlipY);
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

        Field Base = Field::From(Source, Settings.Input);

        // Transparent pixels are ground level, so the outline of the art reads as the edge of a raised shape.
        if (Settings.Input.Masked)
        {
            for (UInt32 Y = 0; Y < Height; ++Y)
            {
                for (UInt32 X = 0; X < Width; ++X)
                {
                    if (!Source.IsSolid(X, Y))
                    {
                        Base.Set(X, Y, 0.0f);
                    }
                }
            }
        }

        Base.Blur(Settings.Input.Blur, Settings.Input.Wrap);

        // How much the outer rows count against the centre one; the central difference is the one with no outer rows.
        static constexpr Real32 kWeights[][2] =
        {
            { 0.0f, 1.0f  },    // Central
            { 1.0f, 2.0f  },    // Sobel
            { 3.0f, 10.0f },    // Scharr
            { 1.0f, 1.0f  },    // Prewitt
        };

        const Real32 Side   = kWeights[ZyEnum::Cast(Settings.Filter)][0];
        const Real32 Middle = kWeights[ZyEnum::Cast(Settings.Filter)][1];

        Sequence<Vector2> Slopes;
        Slopes.Fill(Vector2(), static_cast<UInt>(Width) * Height);

        // Each octave measures a copy blurred twice as wide, so broad shapes and fine grain both show.
        Real32 Weight = 1.0f;

        for (UInt8 Octave = 0; Octave < Settings.Octaves; ++Octave)
        {
            Field Scale = Base;

            if (Octave > 0)
            {
                Scale.Blur(static_cast<Real32>(1u << Octave) * 0.5f, Settings.Input.Wrap);
            }

            for (SInt32 Y = 0; Y < Height; ++Y)
            {
                for (SInt32 X = 0; X < Width; ++X)
                {
                    Slopes[Y * Width + X] += Measure(Scale, X, Y, Side, Middle, Settings.Input.Wrap) * Weight;
                }
            }
            Weight *= Settings.Falloff;
        }

        Canvas Result(Width, Height);

        for (UInt32 Y = 0; Y < Height; ++Y)
        {
            for (UInt32 X = 0; X < Width; ++X)
            {
                // A normal leans away from rising height, hence the negated X. Image rows run downward while the
                // map's +Y points up, so the vertical slope keeps its sign.
                Real32 NX = -Slopes[Y * Width + X].GetX() * Settings.Strength;
                Real32 NY =  Slopes[Y * Width + X].GetY() * Settings.Strength;

                if (Settings.FlipX)
                {
                    NX = -NX;
                }
                if (Settings.FlipY)
                {
                    NY = -NY;
                }

                if (Settings.Input.Masked && !Source.IsSolid(X, Y))
                {
                    NX = 0.0f;
                    NY = 0.0f;
                }

                const Vector3 Direction = Vector3::Normalize(Vector3(NX, NY, 1.0f)) * 0.5f + Vector3(0.5f);

                Result.Set(X, Y, Color(Direction.GetX(), Direction.GetY(), Direction.GetZ(), Source.Get(X, Y).GetAlpha()));
            }
        }
        return Result;
    }
}