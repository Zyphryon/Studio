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
    /// \brief Draws a greyscale relief map out of an image: white where it is raised, darker as it sinks.
    class Relief final
    {
    public:

        /// \brief Specifies how a bevel rises from the outline of the art to its full height.
        enum class Shape : UInt8
        {
            Linear,     ///< A straight ramp, like a chamfer.
            Round,      ///< A quarter circle, like a pillow.
            Sharp,      ///< A curve that stays low and rises late, like a blade.
            Plateau,    ///< A smooth step with a flat top, like a coin.
        };

        /// \brief Represents the settings a relief map is drawn with.
        struct Settings final
        {
            /// The part of each pixel read as its height.
            Channel Source     = Channel::Luminance;

            /// Whether the height is inverted, so dark reads as raised.
            Bool    Invert     = false;

            /// The blur applied to the height, in pixels.
            Real32  Blur       = 0.0f;

            /// The height read as the bottom; anything below is clamped to it.
            Real32  Low        = 0.0f;

            /// The height read as the top; anything above is clamped to it.
            Real32  High       = 1.0f;

            /// The spread of the heights about their middle; above one hardens.
            Real32  Contrast   = 1.0f;

            /// The amount added to every height, before the contrast is applied.
            Real32  Brightness = 0.0f;

            /// The exponent the heights are raised to; above one sinks the middle.
            Real32  Gamma      = 1.0f;

            /// The distance from the outline to full height, in pixels; zero for none.
            Real32  Bevel      = 0.0f;

            /// The shape the bevel rises in.
            Shape   Profile    = Shape::Round;

            /// The share of the image's own height kept on the bevel.
            Real32  Detail     = 1.0f;

            /// Whether the edges wrap around, as they do for a texture that tiles.
            Bool    Wrap       = false;

            /// Whether transparent pixels sink to the bottom.
            Bool    Masked     = true;

            /// \brief Reads the settings from a command line.
            ///
            /// \param Environment The parsed switches.
            /// \return The settings, with defaults where a switch is missing.
            static Settings From(ConstRef<Environment> Environment);
        };

    public:

        /// \brief Draws the relief map of an image.
        ///
        /// \param Source   The image read as a height.
        /// \param Settings The settings to draw with.
        /// \return The map, the height in RGB and the alpha kept.
        static Canvas Generate(ConstRef<Canvas> Source, ConstRef<Settings> Settings);
    };
}