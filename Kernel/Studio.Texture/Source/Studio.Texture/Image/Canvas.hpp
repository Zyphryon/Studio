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
    /// \brief Represents one level of pixels as linear RGBA floats, the form generators and composers work in.
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
        /// \return `true` when the canvas has an extent, otherwise `false`.
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

        /// \brief Reads one pixel, with coordinates outside the canvas clamped to its edge or wrapped around it.
        ///
        /// \param X    The column, which may fall outside the canvas.
        /// \param Y    The row, which may fall outside the canvas.
        /// \param Wrap `true` to wrap around the edges as a tiling texture does, `false` to clamp to them.
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
        /// \param FromX  The left edge of the rectangle in \p Source.
        /// \param FromY  The top edge of the rectangle in \p Source.
        /// \param Width  The width of the rectangle.
        /// \param Height The height of the rectangle.
        /// \param ToX    The left edge the rectangle lands at in this canvas.
        /// \param ToY    The top edge the rectangle lands at in this canvas.
        void Blit(ConstRef<Canvas> Source, UInt32 FromX, UInt32 FromY, UInt32 Width, UInt32 Height, UInt32 ToX, UInt32 ToY);

        /// \brief Converts the base level of a bitmap into a canvas.
        ///
        /// \param Source The bitmap to read, in any format the transcoder reads.
        /// \return The canvas, or an empty one when the bitmap cannot be read.
        static Canvas From(ConstRef<Bitmap> Source);

        /// \brief Converts the canvas into a single-level bitmap of another format.
        ///
        /// \param Format The format to write, which decides whether the colour is stored as linear or as sRGB.
        /// \return The bitmap, or an empty one when the format cannot be written.
        Bitmap To(ZyGraphic::TextureFormat Format) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16 mWidth;
        UInt16 mHeight;
        Blob   mPixels;
    };
}
