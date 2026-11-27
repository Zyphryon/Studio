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

#include "Baker.hpp"
#include "Font/Process/Atlas.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Baker::Baker(Ref<ZyJob::Service> Scheduler)
        : mScheduler { Scheduler }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Baker::Bake(ConstSpan<Source> Sources, ConstRef<Profile> Profile) const
    {
        if (Profile.Size <= 0.0f || Profile.Range <= 0.0f)
        {
            LOG_E("Font: the em size and the range both have to be positive");

            return Blob();
        }

        if (Profile.Charset.IsEmpty())
        {
            LOG_E("Font: the charset selects no codepoint at all");

            return Blob();
        }

        if (Sources.IsEmpty())
        {
            LOG_E("Font: no typeface was given to bake");

            return Blob();
        }

        Typeface            Face;
        Table<UInt32, Bool> Claimed;

        for (ConstRef<Source> Each : Sources)
        {
            if (!Importer::Accepts(Each.Type))
            {
                LOG_E("Font: '{0}' is not a source format this baker understands", Each.Type);

                return Blob();
            }

            // A source may narrow what it is asked for, which is how a fallback gives its icons and nothing else.
            Font::Profile Asked = Profile;

            if (!Each.Charset.IsEmpty())
            {
                Asked.Charset = Each.Charset;
            }

            // The profile's face indexes the main typeface's collection, so every fallback is read at its first face.
            if (AddressOf(Each) != Sources.GetData())
            {
                Asked.Face = 0;
            }

            Importer::Import(mScheduler, Each.Data, Asked, Claimed, Face);
        }

        if (Face.Glyphs.IsEmpty())
        {
            LOG_E("Font: the typefaces carry none of the requested codepoints");

            return Blob();
        }

        // Every cell exists before the fields are generated in parallel: each glyph writes only its own cell, and
        // the sequence must not grow while that happens.
        Sequence<Cell> Cells(Face.Glyphs.GetSize());

        for (ConstRef<Typeface::Glyph> Glyph : Face.Glyphs)
        {
            Ref<Cell> Entry = Cells.Append();
            Entry.Codepoint = Glyph.Codepoint;
            Entry.Advance   = Glyph.Advance;
        }

        mScheduler.Parallel(ZyJob::Lane::Compute, static_cast<UInt32>(Cells.GetSize()), [&](UInt32 Start, UInt32 End)
        {
            for (UInt32 Index = Start; Index < End; ++Index)
            {
                Generator::Generate(Face.Glyphs[Index].Outline, Profile.Range, Profile.Angle, Cells[Index].Data);
            }
        });

        const Atlas::Layout Sheet = Atlas::Arrange(Cells, Profile.Padding, Profile.Limit);

        if (Sheet.Pages == 0 || Sheet.Side > 65535)
        {
            LOG_E("Font: a glyph is larger than a whole atlas page; lower the size or the range");

            return Blob();
        }

        Sequence<Blob> Pages(Sheet.Pages);

        for (UInt32 Page = 0; Page < Sheet.Pages; ++Page)
        {
            Pages.Append(Atlas::Compose(Cells, Sheet.Side, Page));
        }

        // The glyph table is the engine's own type, so the loader reads back exactly what is written here.
        ZyRender::Font::Glyphs Glyphs;
        Glyphs.Reserve(Cells.GetSize());

        const Real32 Extent = static_cast<Real32>(Sheet.Side);

        for (ConstRef<Cell> Entry : Cells)
        {
            // The glyph is value-initialized, so a blank is written with zero rectangles, not indeterminate ones.
            ZyRender::Font::Glyph Glyph { };
            Glyph.Advance = Entry.Advance;
            Glyph.Page    = static_cast<UInt16>(Entry.Page);

            // A blank has no field, and text drawing skips a zero-size rectangle.
            if (Entry.Data.Width > 0 && Entry.Data.Height > 0)
            {
                ConstRef<Rect> Bounds = Entry.Data.Bounds;

                // Everything is stored in em units, which is what lets one bake serve every size.
                Glyph.Local = Rect(
                    Bounds.GetMinimumX() / Profile.Size,
                    Bounds.GetMinimumY() / Profile.Size,
                    Bounds.GetMaximumX() / Profile.Size,
                    Bounds.GetMaximumY() / Profile.Size);

                // The quad's corners land on the centres of the outermost texels, not on their outer edges, to match
                // the half-texel inset the field was generated with.
                Glyph.Atlas = Rect(
                    (static_cast<Real32>(Entry.X) + 0.5f) / Extent,
                    (static_cast<Real32>(Entry.Y) + 0.5f) / Extent,
                    (static_cast<Real32>(Entry.X + Entry.Data.Width)  - 0.5f) / Extent,
                    (static_cast<Real32>(Entry.Y + Entry.Data.Height) - 0.5f) / Extent);
            }
            Glyphs.Assign(Entry.Codepoint, Glyph);
        }

        // The importer measured the typeface, and the rest of the metrics are the profile's.
        Face.Metrics.Size          = Profile.Size;
        Face.Metrics.Distance      = Profile.Range;
        Face.Metrics.UnderlineSize = Profile.Underline;

        LOG_I("Font: {0} glyph(s), {1} kerning pair(s), {2} page(s) of {3}x{3}",
            Glyphs.GetSize(), Face.Kerning.GetSize(), Sheet.Pages, Sheet.Side);

        return Exporter::Export(Face.Metrics, Glyphs, Face.Kerning, Pages, static_cast<UInt16>(Sheet.Side));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Baker::Bake(Text Source, Text Destination, ConstRef<Profile> Profile, ConstSpan<Fallback> Fallbacks) const
    {
        // Every typeface is read before the bake, since the bake walks them together and reads their bytes as it goes.
        Sequence<Blob>          Files;
        Sequence<Baker::Source> Faces;

        const auto Gather = [&](Text Path, Text Charset)
        {
            Ref<Blob> Bytes = Files.Append();

            if (Filesystem::Read(Path, Bytes) != Filesystem::Result::Success || Bytes == nullptr)
            {
                LOG_E("Font: failed to read '{0}'", Path);

                return false;
            }

            Ref<Baker::Source> Face = Faces.Append();
            Face.Data = Bytes;
            Face.Type = StrAfterLast(Path, '.');

            return Charset.IsEmpty() || Font::Profile::Parse(Charset, Face.Charset);
        };

        if (!Gather(Source, Text::Empty()))
        {
            return false;
        }

        for (ConstRef<Fallback> Entry : Fallbacks)
        {
            if (!Gather(Entry.Path, Entry.Charset))
            {
                return false;
            }
        }

        const Blob Output = Bake(Faces, Profile);

        if (Output == nullptr)
        {
            return false;
        }

        Filesystem::Ensure(Destination);

        if (Filesystem::Write(Destination, Output) != Filesystem::Result::Success)
        {
            LOG_E("Font: failed to write '{0}'", Destination);

            return false;
        }

        LOG_I("Font: '{0}' -> '{1}' ({2} bytes)", Source, Destination, Output.GetSize());
        return true;
    }
}