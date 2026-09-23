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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Str Resolve(Text Folder, Text Source)
    {
        // An absolute path stands on its own; anything else hangs off the tracker's folder.
        const Bool Absolute = (Source.GetSize() > 1 && Source[1] == ':') || (!Source.IsEmpty() && Source[0] == '/');

        if (Absolute || Folder.IsEmpty())
        {
            return Str(Source);
        }

        Str Result(Folder);
        Result.Append('/');
        Result.Append(Source);
        return Result;
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
            const ConstPtr<Canvas> Source = Fetch(Entry.Source);

            if (Source == nullptr)
            {
                return false;
            }

            if (!Entry.From.IsValid())
            {
                Entry.From = Area(0, 0, Source->GetWidth(), Source->GetHeight());
            }

            if (Entry.From.X + Entry.From.Width > Source->GetWidth() || Entry.From.Y + Entry.From.Height > Source->GetHeight())
            {
                LOG_E("Texture: '{0}' takes {1}x{2} at {3},{4}, which leaves its {5}x{6} source", Entry.Name,
                    Entry.From.Width, Entry.From.Height, Entry.From.X, Entry.From.Y,
                    Source->GetWidth(), Source->GetHeight());

                return false;
            }

            if (!Entry.Rect.IsValid())
            {
                Entry.Rect.Width  = Entry.From.Width;
                Entry.Rect.Height = Entry.From.Height;
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

        Sequence<Canvas> Pages;

        for (UInt16 Slice = 0; Slice < Atlas.Slices; ++Slice)
        {
            Pages.Append(Canvas(Atlas.Width, Atlas.Height));
        }

        for (ConstRef<Region> Entry : Atlas.Regions)
        {
            ConstRef<Area> Place = Entry.Rect;

            if (Entry.Slice >= Atlas.Slices || Place.X + Place.Width > Atlas.Width || Place.Y + Place.Height > Atlas.Height)
            {
                LOG_E("Texture: '{0}' sits outside its {1}x{2} slice", Entry.Name, Atlas.Width, Atlas.Height);

                return false;
            }

            ConstRef<Canvas> Source = * Fetch(Entry.Source);

            Canvas Cut(Entry.From.Width, Entry.From.Height);
            Cut.Blit(Source, Entry.From.X, Entry.From.Y, Entry.From.Width, Entry.From.Height, 0, 0);

            // A frame gathered into an array stands at the array's size, whatever it was drawn at.
            if (Cut.GetWidth() != Place.Width || Cut.GetHeight() != Place.Height)
            {
                Cut = Canvas::From(Resampler::Resize(Cut.To(Canvas::kFormat), Place.Width, Place.Height));
            }

            Ref<Canvas> Page = Pages[Entry.Slice];
            Page.Blit(Cut, 0, 0, Place.Width, Place.Height, Place.X, Place.Y);

            // The edge is repeated outward, so a filter reading past the region finds the region and not a neighbour.
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

        // The slices are written in the tracker's format, or, when it names none, in the first source's.
        if (Atlas.Format == ZyGraphic::TextureFormat::Unspecified)
        {
            Atlas.Format = (mProfile.Format != ZyGraphic::TextureFormat::Unspecified) ? mProfile.Format : mFirst;
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

    ConstPtr<Canvas> Composer::Fetch(Text Source)
    {
        if (const ConstPtr<Canvas> Found = mSources.Find(Str(Source)))
        {
            return Found;
        }

        const Surface Decoded = mBaker.Load(Resolve(mFolder, Source), mProfile);

        if (!Decoded.IsValid())
        {
            return nullptr;
        }

        if (mFirst == ZyGraphic::TextureFormat::Unspecified)
        {
            mFirst = Decoded.Slices.GetFront().GetFormat();
        }

        mSources.Assign(Str(Source), Canvas::From(Decoded.Slices.GetFront()));
        return mSources.Find(Str(Source));
    }
}
