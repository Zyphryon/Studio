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

#include <Zyphryon.Render/2D/Font.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    /// \brief Writes a baked typeface in the engine's native font format.
    class Exporter final
    {
    public:

        /// \brief The file extension a baked font is written with.
        static constexpr Text   kOutput      = "fnt";

        /// \brief The four-character code every baked font starts with, stored little-endian as `ZFNT`.
        static constexpr UInt32 kMagic       = 'Z' | ('F' << 8) | ('N' << 16) | ('T' << 24);

        /// \brief The layout revision of the header this exporter writes.
        static constexpr UInt16 kVersion     = 1;

        /// \brief The tag of the chunk that holds the kerning table.
        static constexpr UInt32 kKerning     = 'K' | ('E' << 8) | ('R' << 16) | ('N' << 24);

        /// \brief The tag of the chunk that holds one atlas page.
        static constexpr UInt32 kAtlas       = 'A' | ('T' << 8) | ('L' << 16) | ('S' << 24);

        /// \brief The four-character code of the native texture an atlas chunk carries.
        static constexpr UInt32 kTexture     = 'Z' | ('T' << 8) | ('E' << 16) | ('X' << 24);

        /// \brief The LZ4 search effort the atlas pages are encoded at.
        static constexpr UInt32 kCompression = 3;

    public:

        /// \brief Serializes a baked typeface into a native font blob.
        ///
        /// \param Metrics The typeface metrics.
        /// \param Glyphs  The glyph table, keyed by codepoint.
        /// \param Kerning The kerning table, left out if empty.
        /// \param Pages   The RGBA texels of every page, in order.
        /// \param Side    The side of every page, in texels.
        /// \return The font bytes, or an empty blob on failure.
        static Blob Export(
            Ref<ZyRender::Font::Metrics> Metrics,
            Ref<ZyRender::Font::Glyphs>  Glyphs,
            Ref<ZyRender::Font::Kerning> Kerning,
            ConstSpan<Blob>              Pages,
            UInt16                       Side);
    };
}