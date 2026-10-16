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

#include "Relief.hpp"
#include <Zyphryon.Math/Motion/Easing.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Field Distance(ConstRef<Canvas> Source, Bool Wrap)
    {
        const SInt32 Width  = Source.GetWidth();
        const SInt32 Height = Source.GetHeight();

        Field Result(Source.GetWidth(), Source.GetHeight());

        // Empty pixels are the outline, at distance zero; every drawn pixel starts out unreached.
        for (SInt32 Y = 0; Y < Height; ++Y)
        {
            for (SInt32 X = 0; X < Width; ++X)
            {
                Result.Set(X, Y, Source.IsSolid(X, Y) ? 1.0e9f : 0.0f);
            }
        }

        // A texture that does not tile ends in empty space, so its border counts as outline too.
        const auto At = [&](SInt32 X, SInt32 Y)
        {
            if (!Wrap && (X < 0 || Y < 0 || X >= Width || Y >= Height))
            {
                return 0.0f;
            }
            return Result.Fetch(X, Y, true);
        };

        constexpr Real32 kStraight = 1.0f;
        constexpr Real32 kDiagonal = 1.41421356f;

        // A chamfer distance transform: one pass down and one back up, each pixel taking the nearest distance its
        // already visited neighbours offer.
        for (SInt32 Y = 0; Y < Height; ++Y)
        {
            for (SInt32 X = 0; X < Width; ++X)
            {
                Real32 Best = Result.Get(X, Y);
                Best = Min(Best, At(X - 1, Y)     + kStraight);
                Best = Min(Best, At(X,     Y - 1) + kStraight);
                Best = Min(Best, At(X - 1, Y - 1) + kDiagonal);
                Best = Min(Best, At(X + 1, Y - 1) + kDiagonal);
                Result.Set(X, Y, Best);
            }
        }

        for (SInt32 Y = Height - 1; Y >= 0; --Y)
        {
            for (SInt32 X = Width - 1; X >= 0; --X)
            {
                Real32 Best = Result.Get(X, Y);
                Best = Min(Best, At(X + 1, Y)     + kStraight);
                Best = Min(Best, At(X,     Y + 1) + kStraight);
                Best = Min(Best, At(X + 1, Y + 1) + kDiagonal);
                Best = Min(Best, At(X - 1, Y + 1) + kDiagonal);
                Result.Set(X, Y, Best);
            }
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Real32 Rise(Real32 Along, Relief::Shape Profile)
    {
        const Real32 T = Clamp(Along, 0.0f, 1.0f);

        switch (Profile)
        {
        case Relief::Shape::Linear:
            return T;
        case Relief::Shape::Sharp:
            return InQuad(T);
        case Relief::Shape::Plateau:
            return Smoothstep(T);
        default:
            return OutCirc(T);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Relief::Settings Relief::Settings::From(ConstRef<Environment> Environment)
    {
        Settings Result;
        Result.Input      = Elevation::From(Environment);
        Result.Low        = Environment.GetNumber<Real32>("low",          Result.Low);
        Result.High       = Environment.GetNumber<Real32>("high",         Result.High);
        Result.Contrast   = Environment.GetNumber<Real32>("contrast",     Result.Contrast);
        Result.Brightness = Environment.GetNumber<Real32>("brightness",   Result.Brightness);
        Result.Gamma      = Environment.GetNumber<Real32>("gamma",        Result.Gamma);
        Result.Bevel      = Environment.GetNumber<Real32>("bevel",        Result.Bevel);
        Result.Profile    = Environment.GetEnum<Shape>("shape",           Result.Profile);
        Result.Detail     = Environment.GetNumber<Real32>("detail",       Result.Detail);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas Relief::Generate(ConstRef<Canvas> Source, ConstRef<Settings> Settings)
    {
        if (!Source.IsValid())
        {
            return Canvas();
        }

        const UInt16 Width  = Source.GetWidth();
        const UInt16 Height = Source.GetHeight();

        Field Detail = Field::From(Source, Settings.Input);
        Detail.Blur(Settings.Input.Blur, Settings.Input.Wrap);

        const Real32 Span  = Max(Settings.High - Settings.Low, 1.0e-4f);
        const Bool   Bevel = Settings.Bevel > 0.0f;
        const Field  Reach = Bevel ? Distance(Source, Settings.Input.Wrap) : Field(0, 0);

        Canvas Result(Width, Height);

        for (UInt32 Y = 0; Y < Height; ++Y)
        {
            for (UInt32 X = 0; X < Width; ++X)
            {
                const Real32 Alpha = Source.Get(X, Y).GetAlpha();

                // Levels first, then tone, in the order an image editor applies them.
                Real32 Value = Clamp((Detail.Get(X, Y) - Settings.Low) / Span, 0.0f, 1.0f);
                Value = Clamp((Value + Settings.Brightness - 0.5f) * Settings.Contrast + 0.5f, 0.0f, 1.0f);
                Value = Pow(Value, Max(Settings.Gamma, 1.0e-3f));

                if (Bevel)
                {
                    const Real32 Shape = Rise(Reach.Get(X, Y) / Settings.Bevel, Settings.Profile);
                    Value = Shape * (1.0f - Settings.Detail + Settings.Detail * Value);
                }

                if (Settings.Input.Masked && !Source.IsSolid(X, Y))
                {
                    Value = 0.0f;
                }

                Result.Set(X, Y, Color(Value, Value, Value, Alpha));
            }
        }
        return Result;
    }
}