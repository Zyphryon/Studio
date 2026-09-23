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
    /// \brief Represents decoded pixels in one texture format, as they travel from an importer to the exporter.
    class Bitmap final
    {
    public:

        /// \brief The texture format the pixels are stored in.
        using Format = ZyGraphic::TextureFormat;

    public:

        /// \brief Constructs an empty, invalid bitmap.
        ZY_INLINE Bitmap()
            : mFormat { Format::Unspecified },
              mWidth  { 0 },
              mHeight { 0 },
              mLevels { 0 }
        {
        }

        /// \brief Constructs a bitmap from decoded pixels.
        ///
        /// \param Format The format the pixels are stored in.
        /// \param Width  The width of the base level, in pixels.
        /// \param Height The height of the base level, in pixels.
        /// \param Levels The number of mip levels \p Data holds, including the base level.
        /// \param Data   The tightly packed pixels, each level following the one above it.
        ZY_INLINE Bitmap(Format Format, UInt16 Width, UInt16 Height, UInt8 Levels, AnyRef<Blob> Data)
            : mFormat { Format },
              mWidth  { Width },
              mHeight { Height },
              mLevels { Levels },
              mData   { Move(Data) }
        {
        }

        /// \brief Gets the format the pixels are stored in.
        ///
        /// \return The texture format of the pixels.
        ZY_INLINE Format GetFormat() const
        {
            return mFormat;
        }

        /// \brief Gets the width of the bitmap.
        ///
        /// \return The width, in pixels.
        ZY_INLINE UInt16 GetWidth() const
        {
            return mWidth;
        }

        /// \brief Gets the height of the bitmap.
        ///
        /// \return The height, in pixels.
        ZY_INLINE UInt16 GetHeight() const
        {
            return mHeight;
        }

        /// \brief Gets the number of mip levels the pixels hold, including the base level.
        ///
        /// \return The level count, or zero if the bitmap holds no pixels.
        ZY_INLINE UInt8 GetLevels() const
        {
            return mLevels;
        }

        /// \brief Gets the pixels as read-only bytes.
        ///
        /// \return The bytes of every level, empty if the bitmap holds none.
        ZY_INLINE ConstSpan<Byte> GetPixels() const
        {
            return mData;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Format mFormat;
        UInt16 mWidth;
        UInt16 mHeight;
        UInt8  mLevels;
        Blob   mData;
    };
}