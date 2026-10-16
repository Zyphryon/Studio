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

#include <Zyphryon.Graphic/Types.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Represents the settings a source image is baked into a native texture with.
    struct Profile final
    {
        /// \brief Represents a width and a height, both zero when the switch was not given.
        struct Extent final
        {
            /// The size across.
            UInt16 Width  = 0;

            /// The size down.
            UInt16 Height = 0;

            /// \brief Checks whether the extent was given at all.
            ///
            /// \return `true` if both sides are above zero, otherwise `false`.
            ZY_INLINE Bool IsValid() const
            {
                return Width > 0 && Height > 0;
            }

            /// \brief Reads an extent typed as `<width>x<height>`.
            ///
            /// \param Environment The parsed switches.
            /// \param Name        The switch name, without its dashes.
            /// \return The extent, or an invalid one if the switch is missing or malformed.
            static Extent From(ConstRef<Environment> Environment, Text Name);
        };

        /// The format to write, or `Unspecified` to keep the source's own depth and channel count.
        ZyGraphic::TextureFormat Format   = ZyGraphic::TextureFormat::Unspecified;

        /// Whether a full mip chain down to 1x1 is generated, rather than the base level alone.
        Bool                     Mipmaps  = false;

        /// Whether the source holds linear values; `false` for colour art authored in sRGB.
        Bool                     Linear   = true;

        /// Whether the payload is LZ4-compressed, which is only kept when it comes out smaller.
        Bool                     Compress = true;

        /// Whether a lone 2D image is written as a one-slice array, for shaders that only sample arrays.
        Bool                     Layered  = false;

        /// The grid a cube atlas packs its six faces in, counted in faces.
        Extent                   Cube;

        /// The size of one slice of an array atlas, counted in texels.
        Extent                   Slice;

        /// \brief Gets the layout to write, a lone 2D texture becoming an array if \ref Layered.
        ///
        /// \param Layout The layout the slices were decoded or packed as.
        /// \return The layout to write.
        ZY_INLINE ZyGraphic::TextureLayout GetLayout(ZyGraphic::TextureLayout Layout) const
        {
            if (Layered && Layout == ZyGraphic::TextureLayout::Texture2D)
            {
                return ZyGraphic::TextureLayout::Texture2DArray;
            }
            return Layout;
        }

        /// \brief Reads the settings from a command line, or from any settings bag with the same accessors.
        ///
        /// \param Environment The parsed switches.
        /// \return The settings, left at their defaults where a switch is missing.
        static Profile From(ConstRef<Environment> Environment);
    };
}