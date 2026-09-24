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
    /// \brief Draws a tangent-space normal map out of an image read as a height.
    class Normal final
    {
    public:

        /// \brief Specifies the filter a slope is measured with.
        enum class Kernel : UInt8
        {
            Central,    ///< The two neighbours along the axis; the sharpest, and the noisiest.
            Sobel,      ///< A 3x3 filter weighted toward the centre row; the usual choice.
            Scharr,     ///< A 3x3 filter tuned to respond the same in every direction.
            Prewitt,    ///< A 3x3 filter that weighs its three rows alike; the softest.
        };

        /// \brief Represents the settings a normal map is drawn with.
        struct Settings final
        {
            /// The part of each pixel read as its height.
            Channel Source   = Channel::Luminance;

            /// The filter slopes are measured with.
            Kernel  Filter   = Kernel::Sobel;

            /// The steepness one unit of height reads as.
            Real32  Strength = 2.0f;

            /// The blur applied to the height before it is measured, in pixels.
            Real32  Blur     = 0.0f;

            /// The number of scales measured and summed, each twice as coarse as the one before.
            UInt8   Octaves  = 1;

            /// The weight of each coarser scale relative to the one before it.
            Real32  Falloff  = 0.5f;

            /// Whether the height is inverted, so dark reads as raised.
            Bool    Invert   = false;

            /// Whether the X axis is mirrored.
            Bool    FlipX    = false;

            /// Whether the Y axis is mirrored, for programs that expect green to point down.
            Bool    FlipY    = false;

            /// Whether the edges wrap around, as they do for a texture that tiles.
            Bool    Wrap     = false;

            /// Whether transparent pixels are ground, so the outline stands up.
            Bool    Masked   = true;

            /// \brief Reads the settings from a command line.
            ///
            /// \param Environment The parsed switches.
            /// \return The settings, with defaults where a switch is missing.
            static Settings From(ConstRef<Environment> Environment);
        };

    public:

        /// \brief Draws the normal map of an image.
        ///
        /// \param Source   The image read as a height.
        /// \param Settings The settings to draw with.
        /// \return The map, normals packed into RGB and the alpha kept.
        static Canvas Generate(ConstRef<Canvas> Source, ConstRef<Settings> Settings);
    };
}