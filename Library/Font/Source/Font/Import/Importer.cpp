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

#include "Importer.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void ReadMetrics(Ref<stbtt_fontinfo> Info, UInt Size, Ref<ZyRender::Font::Metrics> Result)
    {
        const Real32 Normalize = stbtt_ScaleForMappingEmToPixels(AddressOf(Info), 1.0f);

        int Ascent  = 0;
        int Descent = 0;
        int LineGap = 0;
        stbtt_GetFontVMetrics(AddressOf(Info), &Ascent, &Descent, &LineGap);

        Result.Ascender           = static_cast<Real32>(Ascent)  * Normalize;
        Result.Descender          = static_cast<Real32>(Descent) * Normalize;
        Result.UnderlineOffset    = -0.1f;
        Result.UnderlineThickness =  0.05f;

        const stbtt_uint32 Post = stbtt__find_table(Info.data, Info.fontstart, "post");

        if (Post != 0 && Post + 12 <= Size)
        {
            const SInt16 Position  = ttSHORT(Info.data + Post + 8);
            const SInt16 Thickness = ttSHORT(Info.data + Post + 10);

            // A zero thickness means the field was left unfilled, not that the typeface wants a hairline.
            if (Thickness != 0)
            {
                Result.UnderlineOffset    = static_cast<Real32>(Position)  * Normalize;
                Result.UnderlineThickness = static_cast<Real32>(Thickness) * Normalize;
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void ReadOutline(Ref<stbtt_fontinfo> Info, int Index, Real32 Scale, Ref<Shape> Output)
    {
        Ptr<stbtt_vertex> Vertices = nullptr;
        const int         Count    = stbtt_GetGlyphShape(AddressOf(Info), Index, &Vertices);

        if (Count <= 0 || Vertices == nullptr)
        {
            return;
        }

        Contour Current;
        Vector2 Cursor;
        Vector2 Anchor;

        // A contour is closed back to where it began before it is handed over.
        const auto Close = [&]()
        {
            if (Current.IsEmpty())
            {
                return;
            }

            if (Cursor != Anchor)
            {
                Current.Append(Cursor, Anchor);
            }
            Output.Push(Move(Current));
            Current.Clear();
        };

        for (int Step = 0; Step < Count; ++Step)
        {
            ConstRef<stbtt_vertex> Vertex  = Vertices[Step];
            const Vector2          Target  = Vector2(Vertex.x, Vertex.y)   * Scale;
            const Vector2          Control = Vector2(Vertex.cx, Vertex.cy) * Scale;

            // A line or curve that ends where it starts is left out, as the field generator's own importer leaves it.
            if (Target == Cursor && (Vertex.type == STBTT_vline || Vertex.type == STBTT_vcurve))
            {
                continue;
            }

            switch (Vertex.type)
            {
            case STBTT_vmove:
                Close();
                Anchor = Target;
                break;
            case STBTT_vline:
                Current.Append(Cursor, Target);
                break;
            case STBTT_vcurve:
                Current.Append(Cursor, Control, Target);
                break;
            case STBTT_vcubic:
                Current.Append(Cursor, Control, Vector2(Vertex.cx1, Vertex.cy1) * Scale, Target);
                break;
            default:
                break;
            }
            Cursor = Target;
        }

        Close();
        stbtt_FreeShape(AddressOf(Info), Vertices);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Importer::Accepts(Text Type)
    {
        for (const Text Entry : kTypes)
        {
            if (StrEqualCaseInsensitive(Entry, Type))
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Importer::Import(
        Ref<ZyJob::Service>      Scheduler,
        ConstSpan<Byte>          Source,
        ConstRef<Profile>        Profile,
        Ref<Table<UInt32, Bool>> Claimed,
        Ref<Typeface>            Output)
    {
        if (Source.GetSize() < 12)
        {
            LOG_E("Font: the source is too small to be a typeface");

            return;
        }

        const ConstPtr<unsigned char> Bytes = Source.GetData();

        // A collection holds several typefaces in one file, so the one asked for is found by index.
        if (const int Faces = stbtt_GetNumberOfFonts(Bytes); Faces > 0 && Profile.Face >= static_cast<UInt32>(Faces))
        {
            LOG_E("Font: the source holds {0} typeface(s), so index {1} does not exist", Faces, Profile.Face);

            return;
        }

        const int Offset = stbtt_GetFontOffsetForIndex(Bytes, static_cast<int>(Profile.Face));

        stbtt_fontinfo Info;

        if (Offset < 0 || !stbtt_InitFont(&Info, Bytes, Offset))
        {
            LOG_E("Font: the source is not a readable TrueType or OpenType typeface");

            return;
        }

        const Real32 Normalize = stbtt_ScaleForMappingEmToPixels(&Info, 1.0f);
        const Real32 Scale     = stbtt_ScaleForMappingEmToPixels(&Info, Profile.Size);

        // A codepoint the typeface lacks is skipped, not baked blank, so a later face or the fallback glyph fills in.
        const UInt    Start = Output.Glyphs.GetSize();
        Sequence<int> Indices;

        for (ConstRef<Interval> Block : Profile.Charset)
        {
            for (UInt32 Codepoint = Block.GetMinimum(); Codepoint <= Block.GetMaximum(); ++Codepoint)
            {
                const int Index = stbtt_FindGlyphIndex(&Info, static_cast<int>(Codepoint));

                // An earlier typeface, or an overlapping range, may have given the codepoint already.
                if (Index == 0 || Claimed.Contains(Codepoint))
                {
                    continue;
                }
                Claimed.Assign(Codepoint, true);

                int Advance = 0;
                int Bearing = 0;
                stbtt_GetGlyphHMetrics(&Info, Index, &Advance, &Bearing);

                Ref<Typeface::Glyph> Glyph = Output.Glyphs.Append();
                Glyph.Codepoint = Codepoint;
                Glyph.Advance   = static_cast<Real32>(Advance) * Normalize;

                ReadOutline(Info, Index, Scale, Glyph.Outline);

                Indices.Append(Index);
            }
        }

        if (Indices.IsEmpty())
        {
            return;
        }

        // The metrics come from the first typeface that gives anything, which is the main one.
        if (Start == 0)
        {
            ReadMetrics(Info, Source.GetSize(), Output.Metrics);
        }

        const ConstPtr<Typeface::Glyph> Glyphs = Output.Glyphs.GetData() + Start;
        const UInt32                    Count  = static_cast<UInt32>(Indices.GetSize());

        // Each row is measured on its own and the rows are merged in order, so the table comes out the same every bake.
        Sequence<Sequence<ZyRender::Font::Kerning::Pair>> Rows;
        Rows.Advance(Count);

        // Kerning comes from `GPOS` pair adjustments that carry only an x advance, otherwise from the `kern` table.
        Scheduler.Parallel(ZyJob::Lane::Compute, Count, [&](UInt32 Begin, UInt32 End)
        {
            for (UInt32 First = Begin; First < End; ++First)
            {
                const UInt64 Lead = static_cast<UInt64>(Glyphs[First].Codepoint) << 32;

                for (UInt32 Second = 0; Second < Count; ++Second)
                {
                    const int Adjust = stbtt_GetGlyphKernAdvance(&Info, Indices[First], Indices[Second]);

                    if (Adjust != 0)
                    {
                        Rows[First].Append(Lead | Glyphs[Second].Codepoint, static_cast<Real32>(Adjust) * Normalize);
                    }
                }
            }
        });

        for (ConstRef<Sequence<ZyRender::Font::Kerning::Pair>> Row : Rows)
        {
            for (ConstRef<ZyRender::Font::Kerning::Pair> Pair : Row)
            {
                Output.Kerning.Assign(Pair.First, Pair.Second);
            }
        }
    }
}