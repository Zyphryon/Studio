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

#include "Composer.hpp"
#include "Studio.Texture/Process/Resampler.hpp"
#include <Zyphryon.Graphic/Metadata.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Canvas Fit(AnyRef<Canvas> Source, UInt16 Width, UInt16 Height)
    {
        if (Source.GetWidth() == Width && Source.GetHeight() == Height)
        {
            return Move(Source);
        }
        return Canvas::From(Resampler::Resize(Source.To(Canvas::kFormat), Width, Height));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Transfer(Ref<Canvas> Target, ConstRef<Canvas> Source, UInt8 Written, UInt8 Read)
    {
        for (UInt32 Y = 0; Y < Target.GetHeight(); ++Y)
        {
            for (UInt32 X = 0; X < Target.GetWidth(); ++X)
            {
                const Color From = Source.Get(X, Y);
                const Color Into = Target.Get(X, Y);

                const Array<Real32, 4> Taken(From.GetRed(), From.GetGreen(), From.GetBlue(), From.GetAlpha());
                Array<Real32, 4>       Kept(Into.GetRed(), Into.GetGreen(), Into.GetBlue(), Into.GetAlpha());
                Kept[Written] = Taken[Read];

                Target.Set(X, Y, Color(Kept[0], Kept[1], Kept[2], Kept[3]));
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Composer::Composer(ConstRef<Baker> Baker, Text Folder, ConstRef<Profile> Profile)
        : mBaker   { Baker },
          mFolder  { Folder },
          mProfile { Profile },
          mFirst   { ZyGraphic::TextureFormat::Unspecified }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Composer::Measure(Ref<Tracker> Atlas)
    {
        for (Ref<Region> Entry : Atlas.Regions)
        {
            const ConstPtr<Surface> Source = Fetch(Entry.Source, false);

            if (Source == nullptr)
            {
                return false;
            }

            if (Entry.Layer >= Source->Slices.GetSize())
            {
                LOG_E("Texture: '{0}' takes slice {1} of '{2}', which holds {3}",
                    Entry.Name, Entry.Layer, Entry.Source, Source->Slices.GetSize());

                return false;
            }

            ConstRef<Bitmap> Layer = Source->Slices[Entry.Layer];

            if (!Entry.From.IsValid())
            {
                Entry.From = Area(0, 0, Layer.GetWidth(), Layer.GetHeight());
            }

            if (   Entry.From.X + Entry.From.Width  > Layer.GetWidth()
                || Entry.From.Y + Entry.From.Height > Layer.GetHeight())
            {
                LOG_E("Texture: '{0}' takes {1}x{2} at {3},{4}, which leaves its {5}x{6} source",
                    Entry.Name,
                    Entry.From.Width,
                    Entry.From.Height,
                    Entry.From.X,
                    Entry.From.Y,
                    Layer.GetWidth(),
                    Layer.GetHeight());

                return false;
            }

            if (!Entry.Rect.IsValid())
            {
                Entry.Rect.Width  = Entry.From.Width;
                Entry.Rect.Height = Entry.From.Height;
            }

            for (ConstRef<Splice> Graft : Entry.Channels)
            {
                if (Fetch(Graft.Source, true) == nullptr)
                {
                    return false;
                }
            }
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Composer::Compose(Ref<Tracker> Atlas, Ref<Sequence<Bitmap>> Output)
    {
        if (Atlas.Width == 0 || Atlas.Height == 0 || Atlas.Slices == 0)
        {
            LOG_E("Texture: the tracker has no extent to draw into; pack it first");

            return false;
        }

        if (!Measure(Atlas))
        {
            return false;
        }

        // The format is settled before drawing, since the fill is given as that format stores it.
        if (Atlas.Format == ZyGraphic::TextureFormat::Unspecified)
        {
            Atlas.Format = (mProfile.Format != ZyGraphic::TextureFormat::Unspecified) ? mProfile.Format : mFirst;
        }

        // Only an array can grow; a flat atlas with a region past its one slice is refused below.
        if (Atlas.Layout == ZyGraphic::TextureLayout::Texture2DArray)
        {
            Atlas.Slices = Atlas.GetDepth();
        }

        const Color Fill = ZyGraphic::GetTextureMetadata(Atlas.Format).IsSRGB
            ? Color::FromColor8(Atlas.Fill).ToLinear()
            : Color::FromColor8(Atlas.Fill);

        Sequence<Canvas> Pages;

        for (UInt16 Slice = 0; Slice < Atlas.Slices; ++Slice)
        {
            Ref<Canvas> Page = Pages.Append(Canvas(Atlas.Width, Atlas.Height));

            // A new canvas is already transparent, the default fill.
            if (Atlas.Fill == IntColor8::Transparent())
            {
                continue;
            }

            for (UInt32 Y = 0; Y < Atlas.Height; ++Y)
            {
                for (UInt32 X = 0; X < Atlas.Width; ++X)
                {
                    Page.Set(X, Y, Fill);
                }
            }
        }

        for (ConstRef<Region> Entry : Atlas.Regions)
        {
            ConstRef<Area> Place = Entry.Rect;

            if (Entry.Slice >= Atlas.Slices)
            {
                LOG_E("Texture: '{0}' sits on slice {1} of a flat texture, which has only one",
                    Entry.Name, Entry.Slice);

                return false;
            }

            if (Place.X + Place.Width > Atlas.Width || Place.Y + Place.Height > Atlas.Height)
            {
                LOG_E("Texture: '{0}' sits outside its {1}x{2} slice", Entry.Name, Atlas.Width, Atlas.Height);

                return false;
            }

            const Canvas Source = Canvas::From(Fetch(Entry.Source, false)->Slices[Entry.Layer]);

            Canvas Cut(Entry.From.Width, Entry.From.Height);
            Cut.Blit(Source, Entry.From.X, Entry.From.Y, Entry.From.Width, Entry.From.Height, 0, 0);

            // A region placed at another size, as in an array, is resized to fit.
            Cut = Fit(Move(Cut), Place.Width, Place.Height);

            // A channel is taken from the whole of its source, fitted to the region the same way.
            for (ConstRef<Splice> Graft : Entry.Channels)
            {
                Canvas Plane = Canvas::From(Fetch(Graft.Source, true)->Slices.GetFront());

                Transfer(Cut, Fit(Move(Plane), Place.Width, Place.Height), Graft.Target, Graft.Origin);
            }

            Ref<Canvas> Page = Pages[Entry.Slice];
            Page.Blit(Cut, 0, 0, Place.Width, Place.Height, Place.X, Place.Y);

            // The edge is repeated outward, so a filter reading past the region samples its own edge, not a neighbour.
            const SInt32 Reach = Atlas.Extrude;

            for (SInt32 Y = -Reach; Y < Place.Height + Reach; ++Y)
            {
                for (SInt32 X = -Reach; X < Place.Width + Reach; ++X)
                {
                    const Bool   Inside = X >= 0 && Y >= 0 && X < Place.Width && Y < Place.Height;
                    const SInt32 ToX    = Place.X + X;
                    const SInt32 ToY    = Place.Y + Y;

                    if (!Inside && ToX >= 0 && ToY >= 0 && ToX < Atlas.Width && ToY < Atlas.Height)
                    {
                        Page.Set(ToX, ToY, Cut.Fetch(X, Y, false));
                    }
                }
            }
        }

        Output.Clear();

        for (ConstRef<Canvas> Page : Pages)
        {
            Bitmap Slice = Page.To(Atlas.Format);

            if (Slice.GetPixels().IsEmpty())
            {
                return false;
            }
            Output.Append(Move(Slice));
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Composer::Bake(Ref<Tracker> Atlas)
    {
        Sequence<Bitmap> Slices;

        if (!Compose(Atlas, Slices))
        {
            return Blob();
        }

        Atlas.Layout = mProfile.GetLayout(Atlas.Layout);

        // The slices are already in the tracker's format, so the texture is written in it rather than inferred again.
        Profile Written = mProfile;
        Written.Format = Atlas.Format;

        return mBaker.Encode(Move(Slices), Atlas.Layout, Written);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt16 Composer::Count(Text Source)
    {
        const ConstPtr<Surface> Decoded = Fetch(Source, false);

        return Decoded ? static_cast<UInt16>(Decoded->Slices.GetSize()) : 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Surface> Composer::Fetch(Text Source, Bool Data)
    {
        Ref<Table<Str, Surface>> Cache = Data ? mData : mSources;

        if (const ConstPtr<Surface> Found = Cache.Find(Str(Source)))
        {
            return Found;
        }

        // A channel holds data such as a height, so it is never read through the sRGB curve.
        Profile Settings = mProfile;
        Settings.Linear |= Data;

        Surface Decoded = mBaker.Load(Tracker::Resolve(mFolder, Source), Settings);

        if (!Decoded.IsValid())
        {
            return nullptr;
        }

        if (!Data && mFirst == ZyGraphic::TextureFormat::Unspecified)
        {
            mFirst = Decoded.Slices.GetFront().GetFormat();
        }

        Cache.Assign(Str(Source), Move(Decoded));
        return Cache.Find(Str(Source));
    }
}