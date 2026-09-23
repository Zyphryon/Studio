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

#include "Studio.Texture/Image/Canvas.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Writes a canvas as a PNG, for art that leaves Studio to be edited elsewhere.
    class Picture final
    {
    public:

        /// \brief The file extension a picture is written with.
        static constexpr Text kOutput = "png";

    public:

        /// \brief Encodes a canvas as an eight-bit RGBA PNG.
        ///
        /// \param Source The canvas to encode.
        /// \param sRGB   `true` to store colour sRGB-encoded, as art is; `false` to store the values as they are,
        ///               as data such as a normal or relief map is.
        /// \return The PNG bytes, or an empty blob if the canvas is empty.
        static Blob Encode(ConstRef<Canvas> Source, Bool sRGB);

        /// \brief Encodes a canvas as a PNG and writes it to disk.
        ///
        /// \param Path   The file to write; any folder in it that does not exist yet is created.
        /// \param Source The canvas to encode.
        /// \param sRGB   `true` to store colour sRGB-encoded, `false` to store the values as they are.
        /// \return `true` if the file was written, otherwise `false`.
        static Bool Write(Text Path, ConstRef<Canvas> Source, Bool sRGB);
    };
}