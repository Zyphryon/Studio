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

    static Bool Contains(ConstRef<Area> Outer, ConstRef<Area> Inner)
    {
        return Inner.X >= Outer.X && Inner.Y >= Outer.Y
            && Inner.X + Inner.Width  <= Outer.X + Outer.Width
            && Inner.Y + Inner.Height <= Outer.Y + Outer.Height;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool Find(ConstRef<Sequence<Area>> Free, UInt32 Width, UInt32 Height, Ref<Area> Output, Ref<UInt32> Score)
    {
        Bool Found = false;

        // Best short side fit: the free rectangle leaving the thinnest sliver along its tighter side.
        for (ConstRef<Area> Candidate : Free)
        {
            if (Candidate.Width < Width || Candidate.Height < Height)
            {
                continue;
            }

            if (const UInt32 Short = Min(Candidate.Width - Width, Candidate.Height - Height); !Found || Short < Score)
            {
                Output = Area(Candidate.X, Candidate.Y, static_cast<UInt16>(Width), static_cast<UInt16>(Height));
                Score  = Short;
                Found  = true;
            }
        }
        return Found;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Occupy(Ref<Sequence<Area>> Free, ConstRef<Area> Used)
    {
        Sequence<Area> Next;

        // Every free rectangle the placement overlaps is split into what is left of it on each of its four sides.
        for (ConstRef<Area> Candidate : Free)
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
                Next.Append(Candidate.X, Candidate.Y, Used.X - Candidate.X, Candidate.Height);
            }
            if (Used.X + Used.Width < Candidate.X + Candidate.Width)
            {
                const UInt32 Right = Used.X + Used.Width;
                Next.Append(Right, Candidate.Y, Candidate.X + Candidate.Width - Right, Candidate.Height);
            }
            if (Used.Y > Candidate.Y)
            {
                Next.Append(Candidate.X, Candidate.Y, Candidate.Width, Used.Y - Candidate.Y);
            }
            if (Used.Y + Used.Height < Candidate.Y + Candidate.Height)
            {
                const UInt32 Bottom = Used.Y + Used.Height;
                Next.Append(Candidate.X, Bottom, Candidate.Width, Candidate.Y + Candidate.Height - Bottom);
            }
        }

        // A rectangle lying wholly inside another is dropped, since the larger one already offers that space.
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

    static Bool Place(Ref<Tracker> Atlas,
        ConstRef<Sequence<UInt>>   Order,
        ConstRef<Packer::Settings> Settings, UInt32 Width, UInt32 Height, Bool Spill, Ref<UInt> Failed)
    {
        const UInt32 Border = Settings.Extrude * 2u + Settings.Padding;
        const Area   Blank(0, 0, static_cast<UInt16>(Width), static_cast<UInt16>(Height));

        // Each page holds its free space as the largest rectangles that still fit (the MaxRects method).
        Sequence<Sequence<Area>> Pages;
        Pages.Append().Append(Blank);

        for (const UInt Index : Order)
        {
            Ref<Region> Entry = Atlas.Regions[Index];

            const UInt32 Across = Entry.Rect.Width  + Border;
            const UInt32 Down   = Entry.Rect.Height + Border;

            Area   Best;
            UInt32 BestScore = 0;
            UInt   BestPage  = 0;
            Bool   Found     = false;

            for (UInt Number = 0; Number < Pages.GetSize(); ++Number)
            {
                Area   Candidate;
                UInt32 Score = 0;

                if (Find(Pages[Number], Across, Down, Candidate, Score) && (!Found || Score < BestScore))
                {
                    Best      = Candidate;
                    BestScore = Score;
                    BestPage  = Number;
                    Found     = true;
                }
            }

            if (!Found && Spill)
            {
                Pages.Append().Append(Blank);

                BestPage = Pages.GetSize() - 1;
                Found    = Find(Pages[BestPage], Across, Down, Best, BestScore);
            }

            if (!Found)
            {
                Failed = Index;
                return false;
            }

            Occupy(Pages[BestPage], Best);

            Entry.Slice  = static_cast<UInt16>(BestPage);
            Entry.Rect.X = static_cast<UInt16>(Best.X + Settings.Extrude);
            Entry.Rect.Y = static_cast<UInt16>(Best.Y + Settings.Extrude);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool Grow(Ref<UInt32> Width, Ref<UInt32> Height, ConstRef<Packer::Settings> Settings)
    {
        if (Width >= Settings.Width && Height >= Settings.Height)
        {
            return false;
        }

        // The shorter side doubles, so the bin stays close to square instead of growing into a strip.
        if (Settings.Square)
        {
            Width  = Min<UInt32>(Width  * 2, Settings.Width);
            Height = Min<UInt32>(Height * 2, Settings.Height);
        }
        else if ((Width <= Height && Width < Settings.Width) || Height >= Settings.Height)
        {
            Width  = Min<UInt32>(Width  * 2, Settings.Width);
        }
        else
        {
            Height = Min<UInt32>(Height * 2, Settings.Height);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Packer::Settings Packer::Settings::From(ConstRef<Environment> Environment)
    {
        Settings Result;
        Result.Layout     = Environment.GetEnum<Mode>("mode",           Result.Layout);
        Result.Width      = Environment.GetNumber<UInt16>("width",      Result.Width);
        Result.Height     = Environment.GetNumber<UInt16>("height",     Result.Height);
        Result.Extent     = Profile::Extent::From(Environment, "extent");
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

        // An array gives every region a slice of its own, each the size asked for or else the largest region's.
        if (Settings.Layout == Mode::Array)
        {
            UInt16 Width  = Settings.Extent.Width;
            UInt16 Height = Settings.Extent.Height;

            if (!Settings.Extent.IsValid())
            {
                for (ConstRef<Region> Entry : Atlas.Regions)
                {
                    Width  = Max(Width,  Entry.Rect.Width);
                    Height = Max(Height, Entry.Rect.Height);
                }
            }

            for (UInt Index = 0; Index < Atlas.Regions.GetSize(); ++Index)
            {
                Ref<Region> Entry = Atlas.Regions[Index];
                Entry.Slice = static_cast<UInt16>(Index);
                Entry.Rect  = Area(0, 0, Width, Height);
            }

            Atlas.Layout  = ZyGraphic::TextureLayout::Texture2DArray;
            Atlas.Width   = Width;
            Atlas.Height  = Height;
            Atlas.Slices  = static_cast<UInt16>(Atlas.Regions.GetSize());
            Atlas.Padding = 0;
            Atlas.Extrude = 0;
            return true;
        }

        const auto Covering = [&](UInt Index)
        {
            ConstRef<Area> Size = Atlas.Regions[Index].Rect;
            return static_cast<UInt32>(Size.Width) * Size.Height;
        };

        // Regions are placed largest first, which packs tightest, while the tracker keeps the order they were
        // authored in. Equal areas keep that order too, so the same input always packs the same way.
        Sequence<UInt> Order;

        for (UInt Index = 0; Index < Atlas.Regions.GetSize(); ++Index)
        {
            Order.Append(Index);
        }

        Order.Sort([&](UInt Left, UInt Right)
        {
            const UInt32 First  = Covering(Left);
            const UInt32 Second = Covering(Right);

            return First > Second || (First == Second && Left < Right);
        });

        const UInt32 Border = Settings.Extrude * 2u + Settings.Padding;

        UInt64 Total   = 0;
        UInt32 Widest  = 0;
        UInt32 Tallest = 0;

        for (ConstRef<Region> Entry : Atlas.Regions)
        {
            Total  += static_cast<UInt64>(Entry.Rect.Width + Border) * (Entry.Rect.Height + Border);
            Widest  = Max<UInt32>(Widest,  Entry.Rect.Width  + Border);
            Tallest = Max<UInt32>(Tallest, Entry.Rect.Height + Border);
        }

        // The search starts from the smallest bin that could hold every region, and grows it until they all fit.
        UInt32 Width  = Min<UInt32>(RoundUp(Widest,  0xFFFF), Settings.Width);
        UInt32 Height = Min<UInt32>(RoundUp(Tallest, 0xFFFF), Settings.Height);

        if (Settings.Square)
        {
            Width  = Min<UInt32>(Max(Width, Height), Settings.Width);
            Height = Min<UInt32>(Max(Width, Height), Settings.Height);
        }

        while (static_cast<UInt64>(Width) * Height < Total && Grow(Width, Height, Settings))
        {
        }

        UInt Failed = 0;
        Bool Placed = Place(Atlas, Order, Settings, Width, Height, false, Failed);

        while (!Placed && Grow(Width, Height, Settings))
        {
            Placed = Place(Atlas, Order, Settings, Width, Height, false, Failed);
        }

        // Regions spill onto further slices only once one slice at its largest cannot hold them all.
        if (!Placed && Settings.Pages)
        {
            Placed = Place(Atlas, Order, Settings, Settings.Width, Settings.Height, true, Failed);
        }

        if (!Placed)
        {
            ConstRef<Region> Entry = Atlas.Regions[Failed];

            LOG_E("Texture: '{0}' ({1}x{2}) does not fit a {3}x{4} slice{5}", Entry.Name,
                Entry.Rect.Width, Entry.Rect.Height, Settings.Width, Settings.Height,
                Settings.Pages ? ""_Text : ", and '--pages' is off"_Text);

            return false;
        }

        // The placed regions give the slice count and how far their space reaches, border included.
        UInt32 Pages  = 0;
        UInt32 Right  = 0;
        UInt32 Bottom = 0;

        for (ConstRef<Region> Entry : Atlas.Regions)
        {
            Pages  = Max<UInt32>(Pages,  Entry.Slice + 1u);
            Right  = Max<UInt32>(Right,  Entry.Rect.X + Entry.Rect.Width  + Settings.Extrude + Settings.Padding);
            Bottom = Max<UInt32>(Bottom, Entry.Rect.Y + Entry.Rect.Height + Settings.Extrude + Settings.Padding);
        }

        // Every slice of an array shares one extent, so only a lone slice shrinks to what it used.
        const Bool Paged = Pages > 1;

        Width  = Paged ? Settings.Width  : Right;
        Height = Paged ? Settings.Height : Bottom;

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

        Atlas.Layout  = Paged ? ZyGraphic::TextureLayout::Texture2DArray : ZyGraphic::TextureLayout::Texture2D;
        Atlas.Width   = static_cast<UInt16>(Width);
        Atlas.Height  = static_cast<UInt16>(Height);
        Atlas.Slices  = static_cast<UInt16>(Pages);
        Atlas.Padding = Settings.Padding;
        Atlas.Extrude = Settings.Extrude;
        return true;
    }
}