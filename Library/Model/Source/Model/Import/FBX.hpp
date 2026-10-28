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
#include <ufbx.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    /// \brief Reads an FBX scene through ufbx into the engine's left-handed axes, flattening every node it draws.
    class FBX final
    {
    public:

        /// \brief Reads an FBX scene, with its materials, its deform bones and every take resampled.
        ///
        /// \param Scheduler The pool the clips are resampled on.
        /// \param Source    The encoded source bytes.
        /// \param Profile   The settings the scene is read with.
        /// \return The scene, or an empty one on failure.
        static Scene Read(Ref<ZyJob::Service> Scheduler, ConstSpan<Byte> Source, ConstRef<Profile> Profile);

    private:

        /// \brief The fraction of a tick a Hermite slope is measured across.
        static constexpr Real64 kDerivative = 1.0 / 32.0;

        /// \brief Converts a loaded scene.
        ///
        /// \param Scheduler The pool the clips are resampled on.
        /// \param Native    The scene ufbx loaded.
        /// \param Profile   The settings the scene is read with.
        /// \return The scene.
        static Scene Convert(Ref<ZyJob::Service> Scheduler, ConstRef<ufbx_scene> Native, ConstRef<Profile> Profile);

        /// \brief Builds the joints from the bones that carry weight, each hanging from its nearest such ancestor.
        ///
        /// \param Native  The scene ufbx loaded.
        /// \param Profile The settings giving the scale.
        /// \param Lookup  Receives the joint of every node by its typed id, -1 for a node that is not one.
        /// \return The joints, parents first, or empty when nothing deforms.
        static Sequence<Scene::Joint> Build(
            ConstRef<ufbx_scene> Native, ConstRef<Profile> Profile, Ref<Sequence<SInt32>> Lookup);

        /// \brief Writes the bones deforming one vertex, keeping the heaviest and renormalizing their weights.
        ///
        /// \param Vertex Receives the bones and weights.
        /// \param Skin   The skin binding the vertex, or `nullptr` for a mesh no skin binds.
        /// \param Index  The vertex within the mesh, which the skin addresses rather than the corner.
        /// \param Lookup The joint of every node by its typed id.
        static void Bind(
            Ref<Scene::Vertex> Vertex, ConstPtr<ufbx_skin_deformer> Skin, UInt32 Index, ConstSpan<SInt32> Lookup);

        /// \brief Resamples one take of every joint onto the profile's cadence, measuring slopes for Hermite.
        ///
        /// \param Scheduler The pool the ticks are spread over.
        /// \param Native    The scene ufbx loaded.
        /// \param Joints    The joints built from the scene.
        /// \param Lookup    The joint of every node by its typed id.
        /// \param Profile   The settings giving the cadence, the interpolation and the scale.
        /// \param Stack     The take, which has to last longer than an instant.
        /// \return The clip.
        static Scene::Clip Resample(
            Ref<ZyJob::Service>       Scheduler,
            ConstRef<ufbx_scene>      Native,
            ConstSpan<Scene::Joint>   Joints,
            ConstSpan<SInt32>         Lookup,
            ConstRef<Profile>         Profile,
            ConstRef<ufbx_anim_stack> Stack);

        /// \brief Resolves one corner of a mesh into a vertex in the engine's axes.
        ///
        /// \param Mesh       The mesh.
        /// \param Corner     The corner within the mesh.
        /// \param Geometry   The transform placing the mesh in the world.
        /// \param Rotation   The transform turning the mesh's directions into the world.
        /// \param Profile    The settings giving the scale.
        /// \param Attributes The attributes to read.
        /// \return The vertex, its undefined attributes left at their defaults.
        static Scene::Vertex Resolve(
            ConstRef<ufbx_mesh>   Mesh,
            UInt32                Corner,
            ConstRef<ufbx_matrix> Geometry,
            ConstRef<ufbx_matrix> Rotation,
            ConstRef<Profile>     Profile,
            Scene::Attribute      Attributes);

        /// \brief Records one texture slot of a material, its constant and its image each only when stated.
        ///
        /// \param Output Receives the slot.
        /// \param Slot   The texture slot.
        /// \param Map    The map ufbx translated the source's shading model into.
        static void Attach(Ref<Scene::Material> Output, Scene::Slot Slot, ConstRef<ufbx_material_map> Map);

        /// \brief Turns a direction into the world and the engine's axes.
        ///
        /// \param Basis     The transform turning directions into the world.
        /// \param Direction The direction.
        /// \return The unit direction, or zero for one without length.
        static Vector3 Orient(ConstRef<ufbx_matrix> Basis, ConstRef<ufbx_vec3> Direction);

        /// \brief Carries a world transform into the engine's axes and the space the positions were baked in.
        ///
        /// \param World The world transform.
        /// \param Scale The factor the positions were scaled by.
        /// \return The transform, mirrored across Z on both sides and scaled.
        static ufbx_matrix Place(ConstRef<ufbx_matrix> World, Real32 Scale);

        /// \brief Splits a transform into the position, scale and rotation a bone rests at.
        ///
        /// \param Matrix The transform.
        /// \param Joint  Receives the position, scale and rotation.
        static void Decompose(ConstRef<ufbx_matrix> Matrix, Ref<Scene::Joint> Joint);

        /// \brief Converts a ufbx matrix, whose columns are the engine's rows, into the engine's matrix.
        ///
        /// \param Matrix The ufbx matrix.
        /// \return The engine matrix.
        static Matrix4x3 Adopt(ConstRef<ufbx_matrix> Matrix);

        /// \brief Views a ufbx string as text.
        ///
        /// \param Value The ufbx string.
        /// \return The text, valid while the scene lives.
        static Text Wrap(ConstRef<ufbx_string> Value);
    };
}