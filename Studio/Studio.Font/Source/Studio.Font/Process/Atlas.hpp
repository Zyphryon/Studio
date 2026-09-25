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

#include "Generator.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    /// \brief Represents one glyph's place in the atlas.
    struct Cell final
    {
        /// The codepoint the cell holds.
        UInt32 Codepoint = 0;

        /// The page the cell was placed on.
        UInt32 Page      = 0;

        /// The column the cell starts at, in texels.
        UInt32 X         = 0;

        /// The row the cell starts at, in texels.
        UInt32 Y         = 0;

        /// The generated field, which is empty for a blank such as the space.
        Field  Data;
    };

    /// \brief Places glyph fields on one or more square pages, in rows (shelf packing).
    class Atlas final
    {
    public:

        /// \brief Represents the pages the glyphs were placed on.
        struct Layout final
        {
            /// The side every page shares, in texels.
            UInt32 Side  = 0;

            /// The number of pages the glyphs needed, or zero if they could not be placed at all.
            UInt32 Pages = 0;
        };

    public:

        /// \brief Gives every cell a page and a position on it.
        ///
        /// \param Cells   The cells to place, reordered tallest first.
        /// \param Padding The gap between cells, in texels.
        /// \param Limit   The largest side of a page, in texels.
        /// \return The pages the cells were placed on.
        static Layout Arrange(Ref<Sequence<Cell>> Cells, UInt32 Padding, UInt32 Limit);

        /// \brief Draws the cells of one page into a four-channel texture.
        ///
        /// \param Cells The placed cells.
        /// \param Side  The side of the page, in texels.
        /// \param Page  The page to draw.
        /// \return The interleaved texels of the page.
        static Blob Compose(ConstRef<Sequence<Cell>> Cells, UInt32 Side, UInt32 Page);
    };
}