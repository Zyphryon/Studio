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

#include "Tracker.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Places the regions of a tracker, either packed together on pages or one to a slice.
    class Packer final
    {
    public:

        /// \brief Specifies how regions are laid out.
        enum class Mode : UInt8
        {
            Atlas,      ///< Packed together, spilling onto further slices only when allowed.
            Array,      ///< One region to a slice, each filling it.
        };

        /// \brief Represents the knobs a layout is worked out with.
        struct Settings final
        {
            /// How the regions are laid out.
            Mode   Layout     = Mode::Atlas;

            /// The widest a slice may grow, in pixels.
            UInt16 Width      = 2048;

            /// The tallest a slice may grow, in pixels.
            UInt16 Height     = 2048;

            /// The empty pixels kept between neighbouring regions.
            UInt16 Padding    = 1;

            /// The pixels each region's edge is repeated outward by.
            UInt16 Extrude    = 0;

            /// Whether each side of a slice is rounded up to a power of two.
            Bool   PowerOfTwo = true;

            /// Whether a slice is kept square.
            Bool   Square     = false;

            /// Whether regions that do not fit spill onto further slices, making the texture an array.
            Bool   Pages      = false;

            /// \brief Reads the settings from a command line or any bag with the same accessors.
            ///
            /// \param Environment The parsed switches.
            /// \return The settings, left at their defaults where a switch is missing.
            static Settings From(ConstRef<Environment> Environment);
        };

    public:

        /// \brief Works out where every region of a tracker sits.
        ///
        /// \note Every region must already know its size, which is the extent of what it takes from its source.
        ///
        /// \param Atlas    The tracker whose regions are placed, and whose extent, slices and layout are set.
        /// \param Settings The knobs the layout is worked out with.
        /// \return `true` when every region found a place, otherwise `false`.
        static Bool Pack(Ref<Tracker> Atlas, ConstRef<Settings> Settings);
    };
}
