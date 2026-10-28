// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Scene.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Box Scene::GetBounds() const
    {
        if (Vertices.IsEmpty())
        {
            return Box::Invalid();
        }

        Vector3 Minimum = Vertices[0].Position;
        Vector3 Maximum = Vertices[0].Position;

        for (ConstRef<Vertex> Entry : Vertices)
        {
            Minimum = Vector3::Min(Minimum, Entry.Position);
            Maximum = Vector3::Max(Maximum, Entry.Position);
        }
        return Box(Minimum, Maximum);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt64 Scene::Identify(Text Name)
    {
        return Name.Hash();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Scene::Retarget(Text Path)
    {
        // A source records whatever path the authoring machine had, so only the file name survives.
        const SInt Slash = Max(StrFindLast(Path, '/'), StrFindLast(Path, '\\'));
        const Text Name  = Path.Slice(static_cast<UInt>(Slash + 1));
        const SInt Dot   = StrFindLast(Name, '.');

        return Str::Print<"{0}.{1}">(Dot > 0 ? Name.Slice(0, static_cast<UInt>(Dot)) : Name, kTexture);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Scene::Encoding> Scene::Describe(ZyGraphic::VertexSlot Slot)
    {
        // Positions keep full precision since they set the scale everything else is judged by, and a colour packs down.
        static constexpr Encoding kPosition(Attribute::Position,   ZyGraphic::VertexFormat::Float32x3,    12);
        static constexpr Encoding kNormal(Attribute::Normal,       ZyGraphic::VertexFormat::Float32x3,    12);
        static constexpr Encoding kTangent(Attribute::Tangent,     ZyGraphic::VertexFormat::Float32x4,    16);
        static constexpr Encoding kColor(Attribute::Color,         ZyGraphic::VertexFormat::UIntNorm8x4,   4);
        static constexpr Encoding kTexCoord0(Attribute::TexCoord0, ZyGraphic::VertexFormat::Float32x2,     8);
        static constexpr Encoding kTexCoord1(Attribute::TexCoord1, ZyGraphic::VertexFormat::Float32x2,     8);
        static constexpr Encoding kJoints(Attribute::Skin,         ZyGraphic::VertexFormat::UInt16x4,      8);
        static constexpr Encoding kWeights(Attribute::Skin,        ZyGraphic::VertexFormat::UIntNorm16x4,  8);

        switch (Slot)
        {
        case ZyGraphic::VertexSlot::Position:
            return AddressOf(kPosition);
        case ZyGraphic::VertexSlot::Normal:
            return AddressOf(kNormal);
        case ZyGraphic::VertexSlot::Tangent:
            return AddressOf(kTangent);
        case ZyGraphic::VertexSlot::Color0:
            return AddressOf(kColor);
        case ZyGraphic::VertexSlot::TexCoord0:
            return AddressOf(kTexCoord0);
        case ZyGraphic::VertexSlot::TexCoord1:
            return AddressOf(kTexCoord1);
        case ZyGraphic::VertexSlot::BlendIndices:
            return AddressOf(kJoints);
        case ZyGraphic::VertexSlot::BlendWeights:
            return AddressOf(kWeights);
        default:
            return nullptr;
        }
    }
}