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

#include "Studio.Font/Shape.hpp"
#include <Zyphryon.Math/Geometry/Rect.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    /// \brief Represents the generated distance field of one glyph.
    struct Field final
    {
        /// The width of the field, in texels.
        UInt32         Width  = 0;

        /// The height of the field, in texels.
        UInt32         Height = 0;

        /// The box the field covers, in pixels, with the origin on the baseline and Y pointing up.
        Rect           Bounds;

        /// The texels, four interleaved channels each, with the first row along the bottom of the glyph.
        Sequence<Byte> Texels;
    };

    /// \brief Turns a glyph outline into the multi-channel distance field the engine's text shader samples.
    class Generator final
    {
    public:

        /// \brief Generates the field of one outline.
        ///
        /// \param Outline The glyph outline, in pixels.
        /// \param Range   The width of the distance band, in texels.
        /// \param Angle   The corner angle, in radians.
        /// \param Output  Receives the field, empty if nothing is enclosed.
        static void Generate(ConstRef<Shape> Outline, Real32 Range, Real32 Angle, Ref<Field> Output);
    };
}