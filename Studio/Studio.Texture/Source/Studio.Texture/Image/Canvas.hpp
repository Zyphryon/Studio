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
    /// \brief Represents one level of pixels as linear RGBA floats, for drawing on.
    class Canvas final
    {
    public:

        /// \brief The format a canvas is held in.
        static constexpr ZyGraphic::TextureFormat kFormat = ZyGraphic::TextureFormat::RGBA32Float;

    public:

        /// \brief Constructs an empty canvas holding no pixels.
        ZY_INLINE Canvas()
            : mWidth  { 0 },
              mHeight { 0 }
        {
        }

        /// \brief Constructs a canvas of the given extent, every pixel transparent black.
        ///
        /// \param Width  The width, in pixels.
        /// \param Height The height, in pixels.
        Canvas(UInt16 Width, UInt16 Height);

        /// \brief Checks whether the canvas holds any pixels.
        ///
        /// \return `true` if the canvas has an extent, otherwise `false`.
        ZY_INLINE Bool IsValid() const
        {
            return mWidth > 0 && mHeight > 0;
        }

        /// \brief Gets the width of the canvas.
        ///
        /// \return The width, in pixels.
        ZY_INLINE UInt16 GetWidth() const
        {
            return mWidth;
        }

        /// \brief Gets the height of the canvas.
        ///
        /// \return The height, in pixels.
        ZY_INLINE UInt16 GetHeight() const
        {
            return mHeight;
        }

        /// \brief Reads one pixel.
        ///
        /// \param X The column, from the left edge.
        /// \param Y The row, from the top edge.
        /// \return The pixel, in linear space.
        Color Get(UInt32 X, UInt32 Y) const;

        /// \brief Reads one pixel, clamping or wrapping coordinates that fall outside the canvas.
        ///
        /// \param X    The column, which may fall outside the canvas.
        /// \param Y    The row, which may fall outside the canvas.
        /// \param Wrap `true` to wrap around the edges, `false` to clamp.
        /// \return The pixel, in linear space.
        Color Fetch(SInt32 X, SInt32 Y, Bool Wrap) const;

        /// \brief Writes one pixel.
        ///
        /// \param X     The column, from the left edge.
        /// \param Y     The row, from the top edge.
        /// \param Value The pixel, in linear space.
        void Set(UInt32 X, UInt32 Y, ConstRef<Color> Value);

        /// \brief Copies a rectangle of another canvas into this one.
        ///
        /// \param Source The canvas to copy from.
        /// \param FromX  The left edge in \p Source.
        /// \param FromY  The top edge in \p Source.
        /// \param Width  The width of the rectangle.
        /// \param Height The height of the rectangle.
        /// \param ToX    The left edge in this canvas.
        /// \param ToY    The top edge in this canvas.
        void Blit(ConstRef<Canvas> Source, UInt32 FromX, UInt32 FromY, UInt32 Width, UInt32 Height, UInt32 ToX, UInt32 ToY);

        /// \brief Converts the canvas into a single-level bitmap of another format.
        ///
        /// \param Format The format to write.
        /// \return The bitmap, or an empty one if the format cannot be written.
        Bitmap To(ZyGraphic::TextureFormat Format) const;

    public:

        /// \brief Converts the base level of a bitmap into a canvas.
        ///
        /// \param Source The bitmap to read.
        /// \return The canvas, or an empty one if the bitmap cannot be read.
        static Canvas From(ConstRef<Bitmap> Source);

        /// \brief Brings a coordinate that may lie past the edge of an image back onto it.
        ///
        /// \param Coordinate The column or row, which may be negative or past the extent.
        /// \param Extent     The width or height of the image.
        /// \param Wrap       Whether the image tiles, so a coordinate past one edge comes back in at the other.
        /// \return The coordinate on the image: wrapped around if \p Wrap, otherwise held to the nearest edge.
        ZY_INLINE static UInt32 Reach(SInt32 Coordinate, SInt32 Extent, Bool Wrap)
        {
            if (Wrap)
            {
                return static_cast<UInt32>((Coordinate % Extent + Extent) % Extent);
            }
            return static_cast<UInt32>(Clamp(Coordinate, 0, Extent - 1));
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16 mWidth;
        UInt16 mHeight;
        Blob   mPixels;
    };
}