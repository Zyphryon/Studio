// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Shape.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    /// \brief Represents a decoded typeface, as it travels from an importer to the generator.
    class Typeface final
    {
    public:

        /// \brief Represents the metrics the whole typeface shares, in em units.
        struct Metrics final
        {
            /// The distance from the baseline to the top of typical ascenders.
            Real32 Ascender           = 0.0f;

            /// The distance from the baseline to the bottom of typical descenders, which is negative.
            Real32 Descender          = 0.0f;

            /// The offset from the baseline to the top of the underline.
            Real32 UnderlineOffset    = 0.0f;

            /// The thickness of the underline stroke.
            Real32 UnderlineThickness = 0.0f;
        };

        /// \brief Represents one decoded glyph.
        struct Glyph final
        {
            /// The Unicode codepoint the glyph draws.
            UInt32 Codepoint = 0;

            /// The distance the pen advances after the glyph, in em units.
            Real32 Advance   = 0.0f;

            /// The outline, in pixels, with the origin on the baseline and Y pointing up.
            Shape  Outline;
        };

        /// \brief The kerning table, keyed by both codepoints packed into one integer.
        using Kerning = Table<UInt64, Real32>;

    public:

        /// \brief Constructs an empty typeface.
        ZY_INLINE Typeface() = default;

        /// \brief Constructs a typeface from decoded data.
        ///
        /// \param Metrics The metrics the whole typeface shares.
        /// \param Glyphs  The decoded glyphs.
        /// \param Kerning The kerning pairs between those glyphs.
        ZY_INLINE Typeface(AnyRef<Metrics> Metrics, AnyRef<Sequence<Glyph>> Glyphs, AnyRef<Kerning> Kerning)
            : mMetrics { Move(Metrics) },
              mGlyphs  { Move(Glyphs) },
              mKerning { Move(Kerning) }
        {
        }

        /// \brief Checks whether the typeface holds no glyph.
        ///
        /// \return `true` if nothing was decoded, otherwise `false`.
        ZY_INLINE Bool IsEmpty() const
        {
            return mGlyphs.IsEmpty();
        }

        /// \brief Gets the metrics the whole typeface shares.
        ///
        /// \return The metrics, in em units.
        ZY_INLINE ConstRef<Metrics> GetMetrics() const
        {
            return mMetrics;
        }

        /// \brief Gets the decoded glyphs.
        ///
        /// \return The glyphs.
        ZY_INLINE ConstSpan<Glyph> GetGlyphs() const
        {
            return mGlyphs;
        }

        /// \brief Gets the kerning pairs.
        ///
        /// \return The kerning table.
        ZY_INLINE ConstRef<Kerning> GetKerning() const
        {
            return mKerning;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Metrics         mMetrics;
        Sequence<Glyph> mGlyphs;
        Kerning         mKerning;
    };
}