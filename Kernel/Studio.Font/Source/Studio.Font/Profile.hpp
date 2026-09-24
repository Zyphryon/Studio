// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    /// \brief Represents a closed range of codepoints to bake.
    struct Interval final
    {
        /// The first codepoint in the range.
        UInt32 Minimum = 0;

        /// The last codepoint in the range, inclusive.
        UInt32 Maximum = 0;
    };

    /// \brief Represents the settings a typeface is baked into a native font with.
    struct Profile final
    {
        /// The em size the glyph fields are generated at, in pixels.
        Real32             Size      = 40.0f;

        /// The width of the band the distance fades across, in atlas texels.
        Real32             Range     = 20.0f;

        /// The angle past which the join between two edges counts as a corner, in radians.
        Real32             Angle     = 3.0f;

        /// The total vertical space an underline takes, in em units.
        Real32             Underline = 1.2f;

        /// The index of the typeface to read out of a collection.
        UInt32             Face      = 0;

        /// The gap left between neighbouring glyphs in the atlas, in texels.
        UInt32             Padding   = 1;

        /// The largest side an atlas page may take before another page opens, in texels.
        UInt32             Limit     = 8192;

        /// The codepoint ranges to bake, which may overlap and need not be in order.
        Sequence<Interval> Charset;

        /// \brief Reads the settings from a command line; a charset that cannot be read is left empty.
        ///
        /// \param Environment The parsed switches.
        /// \return The settings, defaulted where a switch is missing.
        static Profile From(ConstRef<Environment> Environment);

        /// \brief Reads a charset description into a list of ranges.
        ///
        /// \param Description The charset description.
        /// \param Output      Receives the ranges.
        /// \return `true` if every entry was understood, otherwise `false`.
        static Bool Parse(Text Description, Ref<Sequence<Interval>> Output);
    };
}