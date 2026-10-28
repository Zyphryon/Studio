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
    /// \brief Represents the settings a source scene is baked into a native model with.
    struct Profile final
    {
        /// \brief Represents one vertex attribute to write, and the stream it is interleaved into.
        struct Channel final
        {
            /// The semantic the attribute is bound to.
            ZyGraphic::VertexSlot Slot  = ZyGraphic::VertexSlot::None;

            /// The stream the attribute shares with every other channel of the same number.
            UInt8                 Group = 0;
        };

        /// \brief Represents one level of detail to generate, and the coverage it is drawn down to.
        struct Detail final
        {
            /// The share of the source triangles the level aims to keep, 1 being the geometry unreduced.
            Real32 Ratio    = 1.0f;

            /// The smallest fraction of the viewport's height the level is drawn at, 0 for the last level.
            Real32 Coverage = 0.0f;

            /// The furthest the reduction may move the silhouette, as a fraction of the mesh's extent.
            Real32 Error    = 0.02f;
        };

        /// The uniform factor every position is scaled by, after any unit conversion.
        Real32             Scale      = 1.0f;

        /// The metres one baked unit stands for, or 0 to keep the source's units.
        Real32             Units      = 0.0f;

        /// `true` to generate normals for geometry that carries none, `false` to leave it without.
        Bool               Normals    = true;

        /// `true` to generate tangents for textured geometry that carries none, `false` to leave it without.
        Bool               Tangents   = true;

        /// `true` to carry a skin and its clips across, `false` to bake a rigged source as the prop it rests as.
        Bool               Rig        = true;

        /// The samples per second every clip is resampled onto.
        Real32             Rate       = 30.0f;

        /// The way the runtime reads between a clip's samples, Hermite storing the source's slope three times over.
        Interpolation      Blend      = Interpolation::Linear;

        /// The attributes to write and the streams they are grouped into, those the scene lacks being left out.
        Sequence<Channel>  Attributes = Standard();

        /// The levels of detail to generate, coarsest last.
        Sequence<Detail>   Details    = StandardDetail();

        /// \brief Checks whether an attribute is one of those requested.
        ///
        /// \param Slot The semantic to look for.
        /// \return `true` when the attribute is requested, otherwise `false`.
        Bool Wants(ZyGraphic::VertexSlot Slot) const;

        /// \brief Gets the attributes a bake writes unless told otherwise, all in one stream.
        ///
        /// \return The attribute list.
        static Sequence<Channel> Standard();

        /// \brief Gets the levels of detail a bake generates unless told otherwise.
        ///
        /// \return The detail table, a quarter of the triangles per step against half the coverage.
        static Sequence<Detail> StandardDetail();

        /// \brief Reads an attribute list, where a bare name is a stream of its own and a bracketed run shares one.
        ///
        /// \param Description The attribute list, such as `[Position],[Normal,TexCoord0]`.
        /// \param Output      Receives the attributes, left untouched when the list cannot be read.
        /// \return `true` if the list was read, otherwise `false`.
        static Bool Parse(Text Description, Ref<Sequence<Channel>> Output);

        /// \brief Reads the settings from a command line, keeping the default of any switch left out.
        ///
        /// \param Environment The parsed switches.
        /// \param Output      Receives the settings, left untouched when a switch cannot be read.
        /// \return `true` if every switch was read, otherwise `false`.
        static Bool From(ConstRef<Environment> Environment, Ref<Profile> Output);
    };
}