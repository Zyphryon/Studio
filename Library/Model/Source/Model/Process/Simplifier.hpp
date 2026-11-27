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

#include "Model/Profile.hpp"
#include <Zyphryon.Job/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    /// \brief Builds the reduced copies that stand in for a scene as it shrinks on screen, over the same vertices.
    class Simplifier final
    {
    public:

        /// \brief Represents one copy of every surface, drawn in place of the finer ones below a coverage.
        struct Level final
        {
            /// The smallest fraction of the viewport's height the level is drawn at.
            Real32                   Coverage = 0.0f;

            /// The runs the level draws, one per material it still shows, indexing the scene's vertices.
            Sequence<Scene::Surface> Surfaces;

            /// The runs shadow maps draw in the level's place, reduced or welded further, or empty to draw its own.
            Sequence<Scene::Surface> Casters;
        };

    public:

        /// \brief Generates every level for a scene, each reduced from the one before and appended to its indices.
        ///
        /// \param Scheduler The pool each level is checked against the source on.
        /// \param Source    The welded scene, whose indices grow by the runs of every level past the first.
        /// \param Details   The reduction and coverage of every level, coarsest last.
        /// \param Drift     The pixels a level may stray by before a finer one is drawn, 0 to keep its coverage.
        /// \return The levels worth drawing, the first being the scene's own geometry.
        static Sequence<Level> Generate(
            Ref<ZyJob::Service> Scheduler, Ref<Scene> Source, ConstSpan<Profile::Detail> Details, Real32 Drift);

    private:

        /// \brief The floats per vertex the reduction weighs, the three of its normal.
        static constexpr UInt   kAttributes = 3;

        /// \brief The lines of the view a drift is measured on.
        static constexpr Real32 kLines      = 1080.0f;

        /// \brief The times further a caster may stray than what its level draws.
        static constexpr Real32 kLoose      = 3.0f;

        /// \brief The weight of a unit normal against the mesh's extent.
        static constexpr Real32 kNormal     = 0.5f;

        /// \brief The most of the level before that a level may keep, since shedding less is not worth its indices.
        static constexpr Real32 kGain       = 0.75f;

        /// \brief The most of the source's surface a level may leave further from itself than its error.
        static constexpr Real32 kLost       = 0.05f;

        /// \brief The times a level that left too much of the source out is reduced again, at half the error.
        static constexpr UInt   kTries      = 3;

        /// \brief The cells the widest side of a level's bounds is cut into, at the finest.
        static constexpr UInt32 kCells      = 64;

        /// \brief Measures how far a point is from a triangle, to the nearest spot on its face, edges or corners.
        ///
        /// \param Point The point to measure from.
        /// \param A     The first corner of the triangle.
        /// \param B     The second corner of the triangle.
        /// \param C     The third corner of the triangle.
        /// \return The squared distance to the nearest point of the triangle.
        static Real32 Apart(Vector3 Point, Vector3 A, Vector3 B, Vector3 C);

        /// \brief Measures how much of the source's surface a level leaves out, as what lies beyond a reach of it.
        ///
        /// \param Scheduler The pool the vertices are spread over.
        /// \param Vertices  The scene's vertices, which the source and every level index.
        /// \param Spans     The surface of the source every vertex stands for.
        /// \param Triangles The triangles of the level.
        /// \param Reach     The furthest a vertex may lie from the level and still count as drawn by it.
        /// \return The surface of the vertices further than the reach from every triangle of the level.
        static Real32 Miss(Ref<ZyJob::Service> Scheduler, ConstSpan<Scene::Vertex> Vertices,
            ConstSpan<Real32> Spans, ConstSpan<UInt32> Triangles, Real32 Reach);

        /// \brief Lays a level's triangles at the end of the indices, gathered into one run per material.
        ///
        /// \param Triangles The triangles the reduction left, in any order.
        /// \param Owners    The surface every vertex belongs to, which is that of any triangle it is a corner of.
        /// \param Surfaces  The scene's own runs, which name the material of each surface.
        /// \param Indices   Receives the triangles, appended run after run.
        /// \return The runs laid, leaving out a surface the reduction left no triangle of.
        static Sequence<Scene::Surface> Lay(ConstSpan<UInt32> Triangles,
            ConstSpan<UInt32> Owners, ConstSpan<Scene::Surface> Surfaces, Ref<Sequence<UInt32>> Indices);
    };
}