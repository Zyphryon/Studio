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

    static Typeface::Metrics ReadMetrics(Ref<stbtt_fontinfo> Info, UInt Size)
    {
        const Real32 Normalize = stbtt_ScaleForMappingEmToPixels(AddressOf(Info), 1.0f);

        int Ascent  = 0;
        int Descent = 0;
        int LineGap = 0;
        stbtt_GetFontVMetrics(AddressOf(Info), &Ascent, &Descent, &LineGap);

        Typeface::Metrics Result;
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
        return Result;
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

    Typeface Importer::Import(ConstSpan<Byte> Source, ConstRef<Profile> Profile)
    {
        if (Source.GetSize() < 12)
        {
            LOG_E("Font: the source is too small to be a typeface");

            return Typeface();
        }

        const ConstPtr<unsigned char> Bytes = Source.GetData();

        // A collection holds several typefaces in one file, so the one asked for is found by index.
        if (const int Faces = stbtt_GetNumberOfFonts(Bytes); Faces > 0 && Profile.Face >= static_cast<UInt32>(Faces))
        {
            LOG_E("Font: the source holds {0} typeface(s), so index {1} does not exist", Faces, Profile.Face);

            return Typeface();
        }

        const int Offset = stbtt_GetFontOffsetForIndex(Bytes, static_cast<int>(Profile.Face));

        stbtt_fontinfo Info;

        if (Offset < 0 || !stbtt_InitFont(&Info, Bytes, Offset))
        {
            LOG_E("Font: the source is not a readable TrueType or OpenType typeface");

            return Typeface();
        }

        const Real32 Normalize = stbtt_ScaleForMappingEmToPixels(&Info, 1.0f);
        const Real32 Scale     = stbtt_ScaleForMappingEmToPixels(&Info, Profile.Size);

        // A codepoint the typeface has no glyph for is skipped rather than baked blank, so the engine's fallback
        // glyph can stand in for it when text is drawn.
        Sequence<Typeface::Glyph> Glyphs;
        Sequence<int>             Indices;

        // Ranges may overlap or repeat, so a codepoint already decoded is not decoded twice.
        Table<UInt32, Bool> Seen;

        for (ConstRef<Interval> Block : Profile.Charset)
        {
            for (UInt32 Codepoint = Block.GetMinimum(); Codepoint <= Block.GetMaximum(); ++Codepoint)
            {
                const int Index = stbtt_FindGlyphIndex(&Info, static_cast<int>(Codepoint));

                if (Index == 0 || Seen.Contains(Codepoint))
                {
                    continue;
                }
                Seen.Assign(Codepoint, true);

                int Advance = 0;
                int Bearing = 0;
                stbtt_GetGlyphHMetrics(&Info, Index, &Advance, &Bearing);

                Ref<Typeface::Glyph> Glyph = Glyphs.Append();
                Glyph.Codepoint = Codepoint;
                Glyph.Advance   = static_cast<Real32>(Advance) * Normalize;

                ReadOutline(Info, Index, Scale, Glyph.Outline);

                Indices.Append(Index);
            }
        }

        // stb_truetype reads kerning from `GPOS` if the typeface has one, and from the older `kern` table if not.
        Typeface::Kerning Kerning;

        for (UInt First = 0; First < Indices.GetSize(); ++First)
        {
            for (UInt Second = 0; Second < Indices.GetSize(); ++Second)
            {
                if (const int Adjust = stbtt_GetGlyphKernAdvance(&Info, Indices[First], Indices[Second]); Adjust != 0)
                {
                    const UInt64 Key = static_cast<UInt64>(Glyphs[First].Codepoint) << 32 | Glyphs[Second].Codepoint;

                    Kerning.Assign(Key, static_cast<Real32>(Adjust) * Normalize);
                }
            }
        }

        return Typeface(ReadMetrics(Info, Source.GetSize()), Move(Glyphs), Move(Kerning));
    }
}