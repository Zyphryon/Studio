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
#include "Studio.Texture/Profile.hpp"

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
            Array,      ///< One region to a slice, sized to it as \ref Fitting says.
        };

        /// \brief Specifies how a region is sized to its array slice.
        enum class Fitting : UInt8
        {
            Stretch,    ///< Resized to fill the slice, whatever its shape.
            Center,     ///< Kept at its own size, centred; one larger than the slice does not fit.
            Contain,    ///< Kept at its own size, centred, and shrunk with its shape kept when larger than the slice.
        };

        /// \brief Represents the settings a layout is worked out with.
        struct Settings final
        {
            /// The way the regions are laid out.
            Mode            Layout     = Mode::Atlas;

            /// The widest a slice may grow, in pixels.
            UInt16          Width      = 2048;

            /// The tallest a slice may grow, in pixels.
            UInt16          Height     = 2048;

            /// The size of every array slice, or invalid for the largest region's.
            Profile::Extent Extent;

            /// The way each region of an array is sized to its slice.
            Fitting         Fit        = Fitting::Stretch;

            /// The empty pixels kept between neighbouring regions.
            UInt16          Padding    = 1;

            /// The pixels each region's edge is repeated outward by.
            UInt16          Extrude    = 0;

            /// Whether each side of a slice is rounded up to a power of two.
            Bool            PowerOfTwo = true;

            /// Whether a slice is kept square.
            Bool            Square     = false;

            /// Whether regions that do not fit spill onto further slices.
            Bool            Pages      = false;

            /// \brief Reads the settings from a command line.
            ///
            /// \param Environment The parsed switches.
            /// \return The settings, with defaults where a switch is missing.
            static Settings From(ConstRef<Environment> Environment);
        };

    public:

        /// \brief Works out where every measured region of a tracker sits.
        ///
        /// \param Atlas    The tracker to place.
        /// \param Settings The settings to lay it out with.
        /// \return `true` if every region fit, otherwise `false`.
        static Bool Pack(Ref<Tracker> Atlas, ConstRef<Settings> Settings);
    };
}