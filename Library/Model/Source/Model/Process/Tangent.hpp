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

#include "Model/Scene.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    /// \brief Builds the tangent frame a normal map is sampled through, the way the tools that bake one do.
    class Tangent final
    {
    public:

        /// \brief Builds the tangent and handedness of every corner, splitting a vertex its corners disagree on.
        ///
        /// \param Source The welded scene, which needs normals and first texture coordinates and may gain vertices.
        /// \return `true` if the tangents were built, `false` when the scene lacks what they are built from.
        static Bool Generate(Ref<Scene> Source);

    private:

        /// \brief The link of a vertex no copy was made of.
        static constexpr UInt32 kNone = kMaximum<UInt32>;

        /// \brief Normalizes a direction, or falls back when it has no length.
        ///
        /// \param Vector   The direction to normalize.
        /// \param Fallback The direction taken when the vector has no length.
        /// \return The unit direction.
        static Vector3 Steady(ConstRef<Vector3> Vector, ConstRef<Vector3> Fallback);
    };
}