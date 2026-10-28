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

            /// The runs the level draws, one per material, indexing the scene's vertices.
            Sequence<Scene::Surface> Surfaces;
        };

        /// \brief The attributes the reduction is asked to keep, the normal and the first texture coordinates.
        static constexpr UInt kAttributes = 5;

    public:

        /// \brief Generates every level for a scene, appending the reduced runs to its index buffer.
        ///
        /// \param Scheduler The pool the reductions are spread over.
        /// \param Source    The scene, whose indices grow by one run per surface per level past the first.
        /// \param Details   The reduction and coverage of every level, coarsest last.
        /// \return The levels, the first being the scene's own geometry, or empty for a scene without surfaces.
        static Sequence<Level> Generate(
            Ref<ZyJob::Service> Scheduler, Ref<Scene> Source, ConstSpan<Profile::Detail> Details);
    };
}