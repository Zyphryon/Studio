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

#include "Model/Process/Simplifier.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    /// \brief Welds and orders a scene's vertices, through meshoptimizer.
    class Indexer final
    {
    public:

        /// \brief Welds corners alike in every attribute and material, and drops triangles that draw nothing new.
        ///
        /// \param Source The scene, whose vertices, indices and runs shrink and are rewritten in place.
        static void Unify(Ref<Scene> Source);

        /// \brief Welds, in the runs every level casts, the corners a shadow cannot tell apart.
        ///
        /// \param Source The scene, whose indices grow by a copy of the runs of every level that casts what it draws.
        /// \param Levels The levels, whose casters are welded, and laid first where a level has none.
        static void Shadow(Ref<Scene> Source, Span<Simplifier::Level> Levels);

        /// \brief Orders every run for the vertex cache and for overdraw, then the vertices from the coarsest level.
        ///
        /// \param Scheduler The pool the runs are spread over.
        /// \param Source    The scene, whose indices and vertices are rewritten in place.
        /// \param Levels    The levels, whose runs are the ones reordered.
        static void Optimize(Ref<ZyJob::Service> Scheduler, Ref<Scene> Source, ConstSpan<Simplifier::Level> Levels);

    private:

        /// \brief The most a run's front to back order may cost its vertex cache, as a ratio of its transforms.
        static constexpr Real32 kCache = 1.05f;
    };
}