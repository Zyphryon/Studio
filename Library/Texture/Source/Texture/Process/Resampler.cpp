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
#include "Texture/Process/Transcoder.hpp"
#include <Zyphryon.Math/Vector3.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Filter(ConstRef<Canvas> Source, Ref<Canvas> Target, UInt32 Y, Bool Opacity, Bool Normal, Bool Flat)
    {
        const UInt32 SourceWidth  = Source.GetWidth();
        const UInt32 SourceHeight = Source.GetHeight();
        const UInt32 TargetWidth  = Target.GetWidth();
        const UInt32 TargetHeight = Target.GetHeight();

        // Spans are counted in parts of a source texel, so an edge texel weighs exactly the share a target covers.
        const UInt32 Top    = Y * SourceHeight;
        const UInt32 Bottom = Top + SourceHeight;

        for (UInt32 X = 0; X < TargetWidth; ++X)
        {
            const UInt32 Left  = X * SourceWidth;
            const UInt32 Right = Left + SourceWidth;

            Color  Total(0.0f, 0.0f, 0.0f, 0.0f);
            Color  Tinted(0.0f, 0.0f, 0.0f, 0.0f);
            Real32 Weight = 0.0f;

            for (UInt32 Row = Top / TargetHeight; Row * TargetHeight < Bottom; ++Row)
            {
                const UInt32 Down = Min((Row + 1) * TargetHeight, Bottom) - Max(Row * TargetHeight, Top);
                const Real32 Tall = static_cast<Real32>(Down) / static_cast<Real32>(SourceHeight);

                for (UInt32 Column = Left / TargetWidth; Column * TargetWidth < Right; ++Column)
                {
                    const UInt32 Across = Min((Column + 1) * TargetWidth, Right) - Max(Column * TargetWidth, Left);
                    const Real32 Share  = Tall * (static_cast<Real32>(Across) / static_cast<Real32>(SourceWidth));

                    Color Texel = Source.Get(Column, Row);

                    // A format that keeps no Z leaves the shader to rebuild it, so the filter rebuilds it the same way.
                    if (Flat)
                    {
                        const Real32 NX = Texel.GetRed()   * 2.0f - 1.0f;
                        const Real32 NY = Texel.GetGreen() * 2.0f - 1.0f;
                        const Real32 NZ = Sqrt(Max(1.0f - NX * NX - NY * NY, 0.0f));

                        Texel = Color(Texel.GetRed(), Texel.GetGreen(), NZ * 0.5f + 0.5f, Texel.GetAlpha());
                    }

                    Total  += Texel * Share;
                    Weight += Share;

                    if (Opacity)
                    {
                        Tinted += Texel * (Share * Texel.GetAlpha());
                    }
                }
            }

            Color Value = Total * (1.0f / Weight);

            // Colour weighed by alpha keeps what see-through texels hold out of the visible ones beside them.
            if (Opacity && Total.GetAlpha() > 0.0f)
            {
                Value = Color::WithAlpha(Tinted * (1.0f / Total.GetAlpha()), Value.GetAlpha());
            }

            // An averaged normal comes out shorter than a unit one, so it is lengthened back before it is stored.
            if (Normal)
            {
                const Vector3 Encoded(Value.GetRed(), Value.GetGreen(), Value.GetBlue());
                const Vector3 Direction = Encoded * 2.0f - Vector3(1.0f);

                if (!IsAlmostZero(Direction.GetLength()))
                {
                    const Vector3 Unit = Vector3::Normalize(Direction) * 0.5f + Vector3(0.5f);

                    Value = Color(Unit.GetX(), Unit.GetY(), Unit.GetZ(), Value.GetAlpha());
                }
            }

            Target.Set(X, Y, Value);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Real32 Cover(ConstRef<Canvas> Level, Real32 Cutoff)
    {
        const UInt32 Count  = static_cast<UInt32>(Level.GetWidth()) * Level.GetHeight();
        UInt32       Passed = 0;

        for (UInt32 Y = 0; Y < Level.GetHeight(); ++Y)
        {
            for (UInt32 X = 0; X < Level.GetWidth(); ++X)
            {
                Passed += (Level.Get(X, Y).GetAlpha() >= Cutoff) ? 1 : 0;
            }
        }
        return static_cast<Real32>(Passed) / static_cast<Real32>(Count);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Keep(Ref<Canvas> Level, Real32 Cutoff, Real32 Coverage)
    {
        // Alphas are counted once by step, so the texels any cut lets through are a running total over the steps.
        constexpr UInt32 kSteps = 1024;

        Array<UInt32, kSteps + 1> Counts { };

        for (UInt32 Y = 0; Y < Level.GetHeight(); ++Y)
        {
            for (UInt32 X = 0; X < Level.GetWidth(); ++X)
            {
                ++Counts[static_cast<UInt32>(Clamp(Level.Get(X, Y).GetAlpha(), 0.0f, 1.0f) * kSteps)];
            }
        }

        // The cut whose share lands nearest the base level's is taken from the middle of the steps that tie for it.
        const SInt64 Wanted = static_cast<SInt64>(Coverage * Level.GetWidth() * Level.GetHeight());

        SInt64 Passed = 0;
        SInt64 Least  = kMaximum<SInt64>;
        UInt32 First  = kSteps;
        UInt32 Last   = kSteps;

        for (UInt32 Step = kSteps; Step > 0; --Step)
        {
            Passed += Counts[Step];

            if (const SInt64 Gap = Abs(Passed - Wanted); Gap <= Least)
            {
                First = (Gap < Least) ? Step : First;
                Last  = Step;
                Least = Gap;
            }
        }

        // Alpha is scaled so that cut lands on the real one, which then lets the same share of texels through.
        const Real32 Scale = Cutoff * kSteps * 2.0f / static_cast<Real32>(First + Last);

        for (UInt32 Y = 0; Y < Level.GetHeight(); ++Y)
        {
            for (UInt32 X = 0; X < Level.GetWidth(); ++X)
            {
                const Color Texel = Level.Get(X, Y);

                Level.Set(X, Y, Color::WithAlpha(Texel, Min(Texel.GetAlpha() * Scale, 1.0f)));
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas Resampler::Resize(ConstRef<Canvas> Source, UInt16 Width, UInt16 Height)
    {
        if (!Source.IsValid() || Width == 0 || Height == 0)
        {
            LOG_E("Texture: neither end of a filter can be empty");

            return Canvas();
        }

        Canvas Result(Width, Height);

        for (UInt32 Row = 0; Row < Height; ++Row)
        {
            Filter(Source, Result, Row, false, false, false);
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bitmap Resampler::Mipmap(
        Ref<ZyJob::Service>      Scheduler,
        AnyRef<Bitmap>           Source,
        UInt8                    Levels,
        ZyGraphic::TextureFormat Format,
        ConstRef<Profile>        Profile)
    {
        const UInt16 Width  = Source.GetWidth();
        const UInt16 Height = Source.GetHeight();

        ZY_ASSERT(Source.GetLevels() == 1, "A chain can only be built from a single level");
        ZY_ASSERT(Levels <= ZyGraphic::GetLevelCount(Width, Height), "More levels than the extent can halve into");

        // With nothing to filter, the base level is converted on its own, or handed back when it is already in format.
        if (Levels <= 1)
        {
            return Transcoder::Transcode(Move(Source), Format);
        }

        const ZyGraphic::TextureMetadata Metadata = ZyGraphic::GetTextureMetadata(Format);

        if (!IsInterleaved(Metadata))
        {
            LOG_E("Texture: '{0}' cannot be written as interleaved components", ZyEnum::GetName(Format));

            return Bitmap();
        }

        // Each level is filtered from the one above it in linear floats, so it is rounded to the format only once.
        Canvas Level = Canvas::From(Source);

        if (!Level.IsValid())
        {
            return Bitmap();
        }

        const Bool   Flat     = Profile.Normal && Metadata.Components < 3;
        const Real32 Coverage = (Profile.Cutoff > 0.0f) ? Cover(Level, Profile.Cutoff) : 0.0f;
        const UInt32 Stride   = Metadata.BitsPerPixel / 8;

        Blob Output = Blob::Allocate<Byte>(ZyGraphic::GetLevelOffset(Format, Width, Height, Levels));

        for (UInt8 Step = 0; Step < Levels; ++Step)
        {
            if (Step > 0)
            {
                Canvas Next(
                    static_cast<UInt16>(ZyGraphic::GetLevelExtent(Width,  Step)),
                    static_cast<UInt16>(ZyGraphic::GetLevelExtent(Height, Step)));

                Scheduler.Parallel(ZyJob::Lane::Compute, Next.GetHeight(), [&](UInt32 First, UInt32 Last)
                {
                    for (UInt32 Row = First; Row < Last; ++Row)
                    {
                        Filter(Level, Next, Row, Profile.Opacity, Profile.Normal, Flat);
                    }
                });

                Level = Move(Next);

                if (Profile.Cutoff > 0.0f)
                {
                    Keep(Level, Profile.Cutoff, Coverage);
                }
            }

            const UInt32         Offset = ZyGraphic::GetLevelOffset(Format, Width, Height, Step);
            const ConstPtr<Byte> Pixels = Level.GetPixels().GetData();
            const Ptr<Byte>      Target = Output.GetData<Byte>() + Offset;
            const UInt32         Across = Level.GetWidth();

            // Rows are converted straight into their place in the chain.
            Scheduler.Parallel(ZyJob::Lane::Compute, Level.GetHeight(), [&](UInt32 First, UInt32 Last)
            {
                Transcoder::Convert(
                    Pixels + static_cast<UInt64>(First) * Across * sizeof(Real32) * 4,
                    Canvas::kFormat,
                    Target + static_cast<UInt64>(First) * Across * Stride,
                    Format,
                    (Last - First) * Across);
            });
        }
        return Bitmap(Format, Width, Height, Levels, Move(Output));
    }
}