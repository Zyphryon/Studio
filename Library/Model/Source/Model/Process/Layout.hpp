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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    /// \brief Packs a scene's vertices into the one buffer the engine binds every vertex stream from.
    class Layout final
    {
    public:

        /// \brief Represents where one attribute sits inside the packed buffer.
        struct Binding final
        {
            /// The semantic the attribute is bound to.
            ZyGraphic::VertexSlot   Slot   = ZyGraphic::VertexSlot::None;

            /// The element type the attribute is stored as.
            ZyGraphic::VertexFormat Format = ZyGraphic::VertexFormat::Float32x3;

            /// The bytes between one vertex's element and the next, the whole record of its stream.
            UInt16                  Stride = 0;

            /// The byte offset of the first vertex's element within the buffer.
            UInt32                  Offset = 0;
        };

        /// \brief Represents a packed scene.
        struct Result final
        {
            /// The placement of every attribute written.
            Sequence<Binding> Bindings;

            /// The packed vertices, one stream after another, or empty on failure.
            Blob              Vertices;

            /// The indices, 16-bit unless the vertices outnumber them.
            Blob              Indices;

            /// `true` when the indices are 32-bit, `false` when they are 16-bit.
            Bool              Extended = false;

            /// The number of streams the attributes were grouped into.
            UInt              Streams  = 0;
        };

    public:

        /// \brief Packs the attributes requested that the scene carries, interleaving those that share a stream.
        ///
        /// \param Source     The scene.
        /// \param Attributes The attributes to write and the streams they are grouped into.
        /// \return The packed scene, whose vertices are empty when no attribute survived.
        static Result Pack(ConstRef<Scene> Source, ConstSpan<Profile::Channel> Attributes);

    private:

        /// \brief Writes one vertex's element of one attribute.
        ///
        /// \param Vertex The vertex.
        /// \param Slot   The attribute.
        /// \param Target Receives the element.
        static void Write(ConstRef<Scene::Vertex> Vertex, ZyGraphic::VertexSlot Slot, Ptr<Byte> Target);
    };
}