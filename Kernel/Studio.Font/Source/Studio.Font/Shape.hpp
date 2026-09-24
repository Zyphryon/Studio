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

#include <Zyphryon.Math/Vector2.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    /// \brief Represents one outline segment: a line, a quadratic Bézier, or a cubic Bézier.
    class Edge final
    {
    public:

        /// \brief The number of control points of the highest-order segment.
        static constexpr UInt32 kMaxPoints = 4;

    public:

        /// \brief Constructs an empty edge.
        ZY_INLINE constexpr Edge()
            : mOrder { 0 }
        {
        }

        /// \brief Constructs a line.
        ///
        /// \param P0 The start point.
        /// \param P1 The end point.
        ZY_INLINE constexpr Edge(Vector2 P0, Vector2 P1)
            : mOrder  { 1 },
              mPoints { P0, P1 }
        {
        }

        /// \brief Constructs a quadratic Bézier.
        ///
        /// \param P0 The start point.
        /// \param P1 The control point.
        /// \param P2 The end point.
        ZY_INLINE constexpr Edge(Vector2 P0, Vector2 P1, Vector2 P2)
            : mOrder  { 2 },
              mPoints { P0, P1, P2 }
        {
        }

        /// \brief Constructs a cubic Bézier.
        ///
        /// \param P0 The start point.
        /// \param P1 The first control point.
        /// \param P2 The second control point.
        /// \param P3 The end point.
        ZY_INLINE constexpr Edge(Vector2 P0, Vector2 P1, Vector2 P2, Vector2 P3)
            : mOrder  { 3 },
              mPoints { P0, P1, P2, P3 }
        {
        }

        /// \brief Gets the degree of the segment.
        ///
        /// \return The order, from 1 (line) to 3 (cubic), or 0 if empty.
        ZY_INLINE constexpr UInt8 GetOrder() const
        {
            return mOrder;
        }

        /// \brief Gets one of the segment's control points.
        ///
        /// \param Index The index of the point, up to the order.
        /// \return The control point.
        ZY_INLINE constexpr Vector2 GetPoint(UInt32 Index) const
        {
            ZY_ASSERT(Index <= mOrder, "Edge control index is out of bounds");

            return mPoints[Index];
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt8   mOrder;
        Vector2 mPoints[kMaxPoints];
    };

    /// \brief Represents a closed loop of edges.
    using Contour = Sequence<Edge>;

    /// \brief Represents a glyph outline as every closed loop the glyph is built from.
    class Shape final
    {
    public:

        /// \brief Checks whether the shape has no edge at all.
        ///
        /// \return `true` if every contour is empty, otherwise `false`.
        ZY_INLINE Bool IsEmpty() const
        {
            for (ConstRef<Contour> Loop : mContours)
            {
                if (!Loop.IsEmpty())
                {
                    return false;
                }
            }
            return true;
        }

        /// \brief Gets the contours the shape is built from.
        ///
        /// \return The contours.
        ZY_INLINE ConstSpan<Contour> GetContours() const
        {
            return mContours;
        }

        /// \brief Takes ownership of a finished contour.
        ///
        /// \param Loop The contour to add.
        ZY_INLINE void Push(AnyRef<Contour> Loop)
        {
            mContours.Append(Move(Loop));
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Sequence<Contour> mContours;
    };
}