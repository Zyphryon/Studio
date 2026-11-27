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

    static void Sweep(Ref<Field> Grid, UInt32 Line, Bool Down, Bool Wrap)
    {
        const SInt32 Count = Down ? Grid.GetHeight() : Grid.GetWidth();

        // A tile repeats, so the line is read once more on either side and every nearest copy of a pixel is seen.
        const SInt32 Start = Wrap ? -Count : 0;
        const SInt32 End   = Wrap ? Count * 2 : Count;

        const auto At = [&](SInt32 Position)
        {
            const UInt32 Along = Canvas::Reach(Position, Count, Wrap);
            return static_cast<Real64>(Down ? Grid.Get(Line, Along) : Grid.Get(Along, Line));
        };

        // The lower envelope of the parabolas the reached pixels cast, each with its height and where it takes over.
        Sequence<SInt32> Apex;
        Sequence<Real64> Lift;
        Sequence<Real64> Onset;

        for (SInt32 Position = Start; Position < End; ++Position)
        {
            const Real64 Height = At(Position);
            const Real64 Here   = Position;

            if (Height == kInfinity<Real64>)
            {
                continue;
            }

            Real64 Cross = -kInfinity<Real64>;

            while (!Apex.IsEmpty())
            {
                const Real64 Last = Apex.GetBack();

                Cross = (Height + Here * Here - Lift.GetBack() - Last * Last) / (2.0 * (Here - Last));

                if (Cross > Onset.GetBack())
                {
                    break;
                }
                Apex.RemoveLast();
                Lift.RemoveLast();
                Onset.RemoveLast();
            }

            Apex.Append(Position);
            Lift.Append(Height);
            Onset.Append(Cross);
        }

        UInt Taken = 0;

        for (SInt32 Position = 0; Position < Count && !Apex.IsEmpty(); ++Position)
        {
            while (Taken + 1 < Apex.GetSize() && Onset[Taken + 1] < Position)
            {
                ++Taken;
            }

            const Real64 Offset = Position - Apex[Taken];
            const Real32 Value  = static_cast<Real32>(Offset * Offset + Lift[Taken]);

            if (Down)
            {
                Grid.Set(Line, Position, Value);
            }
            else
            {
                Grid.Set(Position, Line, Value);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Field Distance(ConstRef<Canvas> Source, Bool Wrap)
    {
        const UInt32 Width  = Source.GetWidth();
        const UInt32 Height = Source.GetHeight();

        Field Result(Source.GetWidth(), Source.GetHeight());

        // Empty pixels are the outline at distance zero, and every drawn pixel starts out unreached.
        for (UInt32 Y = 0; Y < Height; ++Y)
        {
            for (UInt32 X = 0; X < Width; ++X)
            {
                Result.Set(X, Y, Source.IsSolid(X, Y) ? kInfinity<Real32> : 0.0f);
            }
        }

        // The exact squared Euclidean distance is separable, a sweep along every row and then down every column.
        for (UInt32 Y = 0; Y < Height; ++Y)
        {
            Sweep(Result, Y, false, Wrap);
        }

        for (UInt32 X = 0; X < Width; ++X)
        {
            Sweep(Result, X, true, Wrap);
        }

        for (UInt32 Y = 0; Y < Height; ++Y)
        {
            for (UInt32 X = 0; X < Width; ++X)
            {
                Real32 Reach = Sqrt(Result.Get(X, Y));

                // A texture that does not tile ends in empty space, so its border counts as outline too.
                if (!Wrap)
                {
                    Reach = Min(Reach, static_cast<Real32>(Min(Min(X, Width - 1 - X), Min(Y, Height - 1 - Y)) + 1));
                }

                // The distance runs between pixel centres, so half a pixel is taken off to measure from the edge.
                Result.Set(X, Y, Max(Reach - 0.5f, 0.0f));
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