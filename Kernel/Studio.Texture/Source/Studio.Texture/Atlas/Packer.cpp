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

#include "Packer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief A free rectangle of one page, in whole pixels.
    struct Space final
    {
        /// The left edge.
        UInt32 X      = 0;

        /// The top edge.
        UInt32 Y      = 0;

        /// The width.
        UInt32 Width  = 0;

        /// The height.
        UInt32 Height = 0;
    };

    /// \brief The free space left on one page, kept as the largest rectangles that still fit (the MaxRects method).
    using Page = Sequence<Space>;

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static UInt16 RoundUp(UInt32 Value, UInt16 Limit)
    {
        UInt32 Result = 1;

        while (Result < Value)
        {
            Result <<= 1;
        }
        return static_cast<UInt16>(Min<UInt32>(Result, Limit));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool Contains(ConstRef<Space> Outer, ConstRef<Space> Inner)
    {
        return Inner.X >= Outer.X && Inner.Y >= Outer.Y
            && Inner.X + Inner.Width  <= Outer.X + Outer.Width
            && Inner.Y + Inner.Height <= Outer.Y + Outer.Height;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool Find(ConstRef<Page> Free, UInt32 Width, UInt32 Height, Ref<Space> Output, Ref<UInt32> Score)
    {
        Bool Found = false;

        // Best short side fit: the free rectangle leaving the thinnest sliver along its tighter side.
        for (ConstRef<Space> Candidate : Free)
        {
            if (Candidate.Width < Width || Candidate.Height < Height)
            {
                continue;
            }

            const UInt32 Short = Min(Candidate.Width - Width, Candidate.Height - Height);

            if (!Found || Short < Score)
            {
                Output = Space(Candidate.X, Candidate.Y, Width, Height);
                Score  = Short;
                Found  = true;
            }
        }
        return Found;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Occupy(Ref<Page> Free, ConstRef<Space> Used)
    {
        Page Next;

        // Every free rectangle the placement overlaps is split into what is left of it on each of its four sides.
        for (ConstRef<Space> Candidate : Free)
        {
            const Bool Apart = Used.X >= Candidate.X + Candidate.Width  || Used.X + Used.Width  <= Candidate.X
                            || Used.Y >= Candidate.Y + Candidate.Height || Used.Y + Used.Height <= Candidate.Y;

            if (Apart)
            {
                Next.Append(Candidate);
                continue;
            }

            if (Used.X > Candidate.X)
            {
                Next.Append(Space(Candidate.X, Candidate.Y, Used.X - Candidate.X, Candidate.Height));
            }
            if (Used.X + Used.Width < Candidate.X + Candidate.Width)
            {
                Next.Append(Space(Used.X + Used.Width, Candidate.Y,
                    Candidate.X + Candidate.Width - (Used.X + Used.Width), Candidate.Height));
            }
            if (Used.Y > Candidate.Y)
            {
                Next.Append(Space(Candidate.X, Candidate.Y, Candidate.Width, Used.Y - Candidate.Y));
            }
            if (Used.Y + Used.Height < Candidate.Y + Candidate.Height)
            {
                Next.Append(Space(Candidate.X, Used.Y + Used.Height,
                    Candidate.Width, Candidate.Y + Candidate.Height - (Used.Y + Used.Height)));
            }
        }

        // A rectangle wholly inside another adds nothing the other does not already offer.
        Free.Clear();

        for (UInt Index = 0; Index < Next.GetSize(); ++Index)
        {
            Bool Covered = false;

            for (UInt Other = 0; Other < Next.GetSize() && !Covered; ++Other)
            {
                if (Other != Index && Contains(Next[Other], Next[Index]))
                {
                    // Of two identical rectangles, only the first survives.
                    Covered = !Contains(Next[Index], Next[Other]) || Other < Index;
                }
            }

            if (!Covered)
            {
                Free.Append(Next[Index]);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Packer::Settings Packer::Settings::From(ConstRef<Environment> Environment)
    {
        Settings Result;
        Result.Layout     = Environment.GetEnum<Mode>("mode",           Result.Layout);
        Result.Width      = Environment.GetNumber<UInt16>("width",      Result.Width);
        Result.Height     = Environment.GetNumber<UInt16>("height",     Result.Height);
        Result.Padding    = Environment.GetNumber<UInt16>("padding",    Result.Padding);
        Result.Extrude    = Environment.GetNumber<UInt16>("extrude",    Result.Extrude);
        Result.PowerOfTwo = Environment.GetBool("pot",                  Result.PowerOfTwo);
        Result.Square     = Environment.GetBool("square",               Result.Square);
        Result.Pages      = Environment.GetBool("pages",                Result.Pages);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Packer::Pack(Ref<Tracker> Atlas, ConstRef<Settings> Settings)
    {
        if (Atlas.Regions.IsEmpty())
        {
            LOG_E("Texture: an atlas needs at least one region to lay out");

            return false;
        }

        Atlas.Padding = Settings.Padding;
        Atlas.Extrude = Settings.Extrude;

        // An array stands every region on a slice of its own, all at the size of the largest.
        if (Settings.Layout == Mode::Array)
        {
            UInt16 Width  = 0;
            UInt16 Height = 0;

            for (ConstRef<Region> Entry : Atlas.Regions)
            {
                Width  = Max(Width,  Entry.Rect.Width);
                Height = Max(Height, Entry.Rect.Height);
            }

            for (UInt Index = 0; Index < Atlas.Regions.GetSize(); ++Index)
            {
                Ref<Region> Entry = Atlas.Regions[Index];
                Entry.Slice = static_cast<UInt16>(Index);
                Entry.Rect  = Area(0, 0, Width, Height);
            }

            Atlas.Layout = ZyGraphic::TextureLayout::Texture2DArray;
            Atlas.Width  = Width;
            Atlas.Height = Height;
            Atlas.Slices = static_cast<UInt16>(Atlas.Regions.GetSize());
            Atlas.Padding = 0;
            Atlas.Extrude = 0;
            return true;
        }

        // Largest first packs tightest, while the tracker keeps the order the regions were authored in.
        Sequence<UInt> Order;

        for (UInt Index = 0; Index < Atlas.Regions.GetSize(); ++Index)
        {
            Order.Append(Index);
        }

        const auto Covering = [&](UInt Index)
        {
            ConstRef<Area> Size = Atlas.Regions[Index].Rect;
            return static_cast<UInt32>(Size.Width) * Size.Height;
        };

        for (UInt Index = 1; Index < Order.GetSize(); ++Index)
        {
            const UInt Moving = Order[Index];
            UInt       Slot   = Index;

            while (Slot > 0 && Covering(Order[Slot - 1]) < Covering(Moving))
            {
                Order[Slot] = Order[Slot - 1];
                --Slot;
            }
            Order[Slot] = Moving;
        }

        const UInt32 Border = Settings.Extrude * 2u + Settings.Padding;

        Sequence<Page> Pages;
        Pages.Append().Append(Space(0, 0, Settings.Width, Settings.Height));

        UInt32 Right  = 0;
        UInt32 Bottom = 0;

        for (const UInt Index : Order)
        {
            Ref<Region> Entry = Atlas.Regions[Index];

            const UInt32 Width  = Entry.Rect.Width  + Border;
            const UInt32 Height = Entry.Rect.Height + Border;

            Space  Best;
            UInt32 BestScore = 0;
            UInt   BestPage  = 0;
            Bool   Found     = false;

            for (UInt Number = 0; Number < Pages.GetSize(); ++Number)
            {
                Space  Candidate;
                UInt32 Score = 0;

                if (Find(Pages[Number], Width, Height, Candidate, Score) && (!Found || Score < BestScore))
                {
                    Best      = Candidate;
                    BestScore = Score;
                    BestPage  = Number;
                    Found     = true;
                }
            }

            if (!Found && Settings.Pages)
            {
                Pages.Append().Append(Space(0, 0, Settings.Width, Settings.Height));

                BestPage = Pages.GetSize() - 1;
                Found    = Find(Pages[BestPage], Width, Height, Best, BestScore);
            }

            if (!Found)
            {
                LOG_E("Texture: '{0}' ({1}x{2}) does not fit a {3}x{4} slice{5}", Entry.Name,
                    Entry.Rect.Width, Entry.Rect.Height, Settings.Width, Settings.Height,
                    Settings.Pages ? ""_Text : ", and '--pages' is off"_Text);

                return false;
            }

            Occupy(Pages[BestPage], Best);

            Entry.Slice  = static_cast<UInt16>(BestPage);
            Entry.Rect.X = static_cast<UInt16>(Best.X + Settings.Extrude);
            Entry.Rect.Y = static_cast<UInt16>(Best.Y + Settings.Extrude);

            Right  = Max(Right,  Best.X + Best.Width);
            Bottom = Max(Bottom, Best.Y + Best.Height);
        }

        // Every slice shares one extent, so it shrinks to what the fullest page used.
        UInt32 Width  = (Pages.GetSize() > 1) ? Settings.Width  : Right;
        UInt32 Height = (Pages.GetSize() > 1) ? Settings.Height : Bottom;

        if (Settings.PowerOfTwo)
        {
            Width  = RoundUp(Width,  Settings.Width);
            Height = RoundUp(Height, Settings.Height);
        }

        if (Settings.Square)
        {
            Width  = Max(Width, Height);
            Height = Width;
        }

        Atlas.Width  = static_cast<UInt16>(Width);
        Atlas.Height = static_cast<UInt16>(Height);
        Atlas.Slices = static_cast<UInt16>(Pages.GetSize());
        Atlas.Layout = (Pages.GetSize() > 1)
            ? ZyGraphic::TextureLayout::Texture2DArray
            : ZyGraphic::TextureLayout::Texture2D;
        return true;
    }
}
