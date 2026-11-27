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

#include "Texture/Image/Canvas.hpp"
#include "Texture/Profile.hpp"
#include <Zyphryon.Job/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Resizes linear pixels, weighing each source texel by the share of a target texel it covers.
    class Resampler final
    {
    public:

        /// \brief Filters a canvas into a new extent.
        ///
        /// \param Source The canvas to filter.
        /// \param Width  The target width, in pixels, at least one.
        /// \param Height The target height, in pixels, at least one.
        /// \return The resized canvas, or an empty one on failure.
        static Canvas Resize(ConstRef<Canvas> Source, UInt16 Width, UInt16 Height);

        /// \brief Filters a single-level bitmap down into a chain of successively halved levels, each written once.
        ///
        /// \param Scheduler The pool each level's rows are spread over.
        /// \param Source    The single-level bitmap to expand.
        /// \param Levels    The number of levels, the base included.
        /// \param Format    The interleaved format every level is written in.
        /// \param Profile   The settings that say how alpha, coverage and normals are filtered.
        /// \return The whole chain in \p Format, or an invalid bitmap on failure.
        static Bitmap Mipmap(
            Ref<ZyJob::Service>      Scheduler,
            AnyRef<Bitmap>           Source,
            UInt8                    Levels,
            ZyGraphic::TextureFormat Format,
            ConstRef<Profile>        Profile);
    };
}