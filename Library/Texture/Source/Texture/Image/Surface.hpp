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

#include "Bitmap.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Represents the bitmaps one source decodes into, and the layout they make up together.
    struct Surface final
    {
        /// The layout the slices make up: a plain 2D texture, an array or a cube.
        ZyGraphic::TextureLayout Layout = ZyGraphic::TextureLayout::Texture2D;

        /// The bitmaps, one per array slice or cube face, in the order the layout addresses them.
        Sequence<Bitmap>         Slices;

        /// \brief Checks whether the decode produced anything at all.
        ///
        /// \return `true` if the surface holds at least one slice with pixels, otherwise `false`.
        ZY_INLINE Bool IsValid() const
        {
            return !Slices.IsEmpty() && !Slices.GetFront().GetPixels().IsEmpty();
        }

        /// \brief Wraps a single bitmap as a plain 2D surface.
        ///
        /// \param Source The bitmap the surface takes over.
        /// \return A surface holding \p Source as its only slice.
        ZY_INLINE static Surface From(AnyRef<Bitmap> Source)
        {
            Surface Result;
            Result.Slices.Append(Move(Source));
            return Result;
        }
    };
}