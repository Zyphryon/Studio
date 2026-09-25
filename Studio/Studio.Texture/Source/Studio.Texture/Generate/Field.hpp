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
    /// \brief Specifies which part of a pixel is read as its height.
    enum class Channel : UInt8
    {
        Luminance,  ///< The perceived brightness of the colour.
        Red,        ///< The red channel alone.
        Green,      ///< The green channel alone.
        Blue,       ///< The blue channel alone.
        Alpha,      ///< The alpha channel alone.
    };

    /// \brief Represents one height per pixel, which the normal and relief generators start from.
    class Field final
    {
    public:

        /// \brief Constructs a field of the given extent, every value zero.
        ///
        /// \param Width  The width, in pixels.
        /// \param Height The height, in pixels.
        Field(UInt16 Width, UInt16 Height);

        /// \brief Gets the width of the field.
        ///
        /// \return The width, in pixels.
        ZY_INLINE UInt16 GetWidth() const
        {
            return mWidth;
        }

        /// \brief Gets the height of the field.
        ///
        /// \return The height, in pixels.
        ZY_INLINE UInt16 GetHeight() const
        {
            return mHeight;
        }

        /// \brief Reads one value.
        ///
        /// \param X The column, from the left edge.
        /// \param Y The row, from the top edge.
        /// \return The value.
        ZY_INLINE Real32 Get(UInt32 X, UInt32 Y) const
        {
            return mValues[Y * mWidth + X];
        }

        /// \brief Writes one value.
        ///
        /// \param X     The column, from the left edge.
        /// \param Y     The row, from the top edge.
        /// \param Value The value.
        ZY_INLINE void Set(UInt32 X, UInt32 Y, Real32 Value)
        {
            mValues[Y * mWidth + X] = Value;
        }

        /// \brief Reads one value, clamping or wrapping coordinates outside the field.
        ///
        /// \param X    The column.
        /// \param Y    The row.
        /// \param Wrap `true` to wrap around the edges, `false` to clamp to them.
        /// \return The value.
        Real32 Fetch(SInt32 X, SInt32 Y, Bool Wrap) const;

        /// \brief Smooths the field with a gaussian of the given spread.
        ///
        /// \param Sigma The spread, in pixels; zero leaves the field as it is.
        /// \param Wrap  `true` to wrap around the edges, `false` to clamp to them.
        void Blur(Real32 Sigma, Bool Wrap);

    public:

        /// \brief Reads the height of every pixel of a canvas.
        ///
        /// \param Source The canvas to read.
        /// \param From   The part of each pixel read as its height.
        /// \param Invert `true` to turn the height upside down.
        /// \return The field, one value per pixel.
        static Field From(ConstRef<Canvas> Source, Channel From, Bool Invert);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16           mWidth;
        UInt16           mHeight;
        Sequence<Real32> mValues;
    };
}