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

#include "Model/Process/Layout.hpp"
#include "Model/Process/Simplifier.hpp"
#include <Zyphryon.Math/Geometry/Sphere.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    /// \brief Writes a baked scene in the engine's native mesh, skeleton, animation and model formats.
    class Exporter final
    {
    public:

        /// \brief The file extension a baked model manifest is written with.
        static constexpr Text   kOutput           = "mdl";

        /// \brief The file extension a baked mesh is written with.
        static constexpr Text   kMeshOutput       = "msh";

        /// \brief The four-character code every baked mesh starts with, stored little-endian as `ZMSH`.
        static constexpr UInt32 kMeshMagic        = 'Z' | ('M' << 8) | ('S' << 16) | ('H' << 24);

        /// \brief The layout revision of the mesh this exporter writes.
        static constexpr UInt16 kMeshVersion      = 1;

        /// \brief The file extension a baked skeleton is written with.
        static constexpr Text   kSkeletonOutput   = "skl";

        /// \brief The four-character code every baked skeleton starts with, stored little-endian as `ZSKL`.
        static constexpr UInt32 kSkeletonMagic    = 'Z' | ('S' << 8) | ('K' << 16) | ('L' << 24);

        /// \brief The layout revision of the skeleton this exporter writes.
        static constexpr UInt16 kSkeletonVersion  = 1;

        /// \brief The file extension a baked animation is written with.
        static constexpr Text   kAnimationOutput  = "anm";

        /// \brief The four-character code every baked animation starts with, stored little-endian as `ZANM`.
        static constexpr UInt32 kAnimationMagic   = 'Z' | ('A' << 8) | ('N' << 16) | ('M' << 24);

        /// \brief The layout revision of the animation this exporter writes.
        static constexpr UInt16 kAnimationVersion = 1;

    public:

        /// \brief Serializes packed geometry and its levels of detail into a native mesh.
        ///
        /// \param Geometry The packed vertices and indices.
        /// \param Extent   The local-space box enclosing every vertex.
        /// \param Levels   The levels, coarsest last, whose runs become the primitives.
        /// \return The mesh bytes, or an empty blob when the geometry is more than the format can name.
        static Blob ExportMesh(ConstRef<Layout::Result> Geometry, Box Extent, ConstSpan<Simplifier::Level> Levels);

        /// \brief Serializes the rig into a native skeleton.
        ///
        /// \param Joints The bones, parents first.
        /// \return The skeleton bytes, or an empty blob for a rig the format cannot hold.
        static Blob ExportSkeleton(ConstSpan<Scene::Joint> Joints);

        /// \brief Serializes a clip into a native animation, storing once every component that holds still.
        ///
        /// \param Clip   The clip.
        /// \param Joints The bones the clip drives, whose names key its lanes.
        /// \param Extent The local-space sphere every pose of the clip stays inside.
        /// \return The animation bytes.
        static Blob ExportAnimation(ConstRef<Scene::Clip> Clip, ConstSpan<Scene::Joint> Joints, Sphere Extent);

        /// \brief Serializes the manifest that ties the mesh, the skeleton and every material into one model.
        ///
        /// \param Mesh      The mesh, relative to the manifest.
        /// \param Skeleton  The skeleton relative to the manifest, or empty for a model that never deforms.
        /// \param Materials The material table, in slot order.
        /// \return The manifest, as JSON text.
        static Str ExportModel(Text Mesh, Text Skeleton, ConstSpan<Scene::Material> Materials);

    private:

        /// \brief Enumerates the kind of constant a texture slot is modulated by.
        enum class Constant : UInt8
        {
            None,   ///< Takes no constant a shading model agrees on.
            Tint,   ///< Takes a colour.
            Scalar, ///< Takes a single factor.
        };

        /// \brief The tag of the chunk that holds the mesh's bindings, levels and primitives.
        static constexpr UInt32 kMeshChunk     = 'M' | ('E' << 8) | ('S' << 16) | ('H' << 24);

        /// \brief The tag of the chunk that holds the vertex buffer.
        static constexpr UInt32 kVerticesChunk = 'V' | ('B' << 8) | ('U' << 16) | ('F' << 24);

        /// \brief The tag of the chunk that holds the index buffer.
        static constexpr UInt32 kIndicesChunk  = 'I' | ('B' << 8) | ('U' << 16) | ('F' << 24);

        /// \brief The bit an animation lane sets when it drives a bone's position.
        static constexpr UInt8  kPosition      = 1 << 0;

        /// \brief The bit an animation lane sets when it drives a bone's scale.
        static constexpr UInt8  kScale         = 1 << 1;

        /// \brief The bit an animation lane sets when it drives a bone's rotation.
        static constexpr UInt8  kRotation      = 1 << 2;

        /// \brief The name the manifest states its one sampler under, which the technique must declare.
        static constexpr Text   kSampler       = "Default";

        /// \brief Gets the kind of constant a texture slot is modulated by.
        ///
        /// \param Slot The texture slot.
        /// \return The kind, by what the slot means rather than how many components a scene wrote.
        static Constant Classify(Scene::Slot Slot);
    };
}