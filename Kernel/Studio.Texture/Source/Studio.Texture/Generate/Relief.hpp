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

#include "Field.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Draws a greyscale relief map, white at the surface and darker as it sinks, out of an image.
    class Relief final
    {
    public:

        /// \brief Specifies how a bevel rises from the outline of the art to its full height.
        enum class Shape : UInt8
        {
            Linear,     ///< A straight ramp, as a chamfer.
            Round,      ///< A quarter circle, as a pillow.
            Sharp,      ///< A curve that stays low and rises late, as a blade.
            Plateau,    ///< A smooth step with a flat top, as a coin.
        };

        /// \brief Represents the knobs a relief map is drawn with.
        struct Settings final
        {
            /// The part of each pixel read as its height.
            Channel Source     = Channel::Luminance;

            /// Whether the height is turned upside down, so dark reads as raised.
            Bool    Invert     = false;

            /// The spread the height is smoothed by, in pixels.
            Real32  Blur       = 0.0f;

            /// The height read as the bottom; anything below sinks to it.
            Real32  Low        = 0.0f;

            /// The height read as the top; anything above rises to it.
            Real32  High       = 1.0f;

            /// How far the heights spread about their middle; above one hardens the relief, below softens it.
            Real32  Contrast   = 1.0f;

            /// How far every height is lifted, before the contrast is applied.
            Real32  Brightness = 0.0f;

            /// The curve the heights are bent by; above one sinks the middle tones, below lifts them.
            Real32  Gamma      = 1.0f;

            /// The distance from the outline at which the art reaches its full height, in pixels; zero for none.
            Real32  Bevel      = 0.0f;

            /// How the bevel rises.
            Shape   Profile    = Shape::Round;

            /// How much of the image's own height rides on the bevel; zero for the bevel's shape alone.
            Real32  Detail     = 1.0f;

            /// Whether the edges wrap around, as they do for a texture that tiles.
            Bool    Wrap       = false;

            /// Whether transparent pixels sink to the bottom.
            Bool    Masked     = true;

            /// \brief Reads the settings from a command line or any bag with the same accessors.
            ///
            /// \param Environment The parsed switches.
            /// \return The settings, left at their defaults where a switch is missing.
            static Settings From(ConstRef<Environment> Environment);
        };

    public:

        /// \brief Draws the relief map of an image.
        ///
        /// \param Source   The image read as a height.
        /// \param Settings The knobs the map is drawn with.
        /// \return The map, its height in RGB alike and the source's alpha kept.
        static Canvas Generate(ConstRef<Canvas> Source, ConstRef<Settings> Settings);
    };
}
