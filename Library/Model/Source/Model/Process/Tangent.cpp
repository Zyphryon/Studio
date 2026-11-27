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

#include "Tangent.hpp"
#include <meshoptimizer.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Tangent::Generate(Ref<Scene> Source)
    {
        if (!Source.Has(Scene::Attribute::Normal) || !Source.Has(Scene::Attribute::TexCoord0))
        {
            return false;
        }

        Ref<Sequence<Scene::Vertex>> Vertices = Source.Vertices;
        Ref<Sequence<UInt32>>        Indices  = Source.Indices;

        // One frame per corner, weighed the way MikkTSpace does, which is what a normal map is baked against.
        Sequence<Real32> Frames;
        Frames.Resize(Indices.GetSize() * 4);

        meshopt_generateTangents(
            Frames.GetData(),
            Indices.GetData(),
            Indices.GetSize(),
            reinterpret_cast<ConstPtr<Real32>>(AddressOf(Vertices[0].Position)),
            Vertices.GetSize(),
            sizeof(Scene::Vertex),
            reinterpret_cast<ConstPtr<Real32>>(AddressOf(Vertices[0].Normal)),
            sizeof(Scene::Vertex),
            reinterpret_cast<ConstPtr<Real32>>(AddressOf(Vertices[0].TexCoord0)),
            sizeof(Scene::Vertex),
            meshopt_TangentCompatible | meshopt_TangentZeroFallback);

        const auto Take = [&Frames](Ref<Scene::Vertex> Vertex, UInt Corner)
        {
            const ConstPtr<Real32> Frame = Frames.GetData() + Corner * 4;

            // A corner whose texture coordinates pin no direction takes any one off its normal.
            const Vector3 Aside = Steady(Vector3::Perpendicular(Vertex.Normal), Vector3::UnitX());

            // The engine winds its fronts clockwise, which turns the bitangent over from where the sign puts it.
            Vertex.Tangent    = Steady(Vector3(Frame[0], Frame[1], Frame[2]), Aside);
            Vertex.Handedness = -Frame[3];
        };

        for (UInt Corner = 0; Corner < Indices.GetSize(); ++Corner)
        {
            Take(Vertices[Indices[Corner]], Corner);
        }

        // A corner whose frame is not its vertex's takes a copy of the vertex, as either side of a mirrored seam does.
        Sequence<UInt32> Copies;
        Copies.Fill(kNone, Vertices.GetSize());

        for (UInt Corner = 0; Corner < Indices.GetSize(); ++Corner)
        {
            const UInt32 Origin = Indices[Corner];

            Scene::Vertex Wanted = Vertices[Origin];
            Take(Wanted, Corner);

            const auto Differs = [&](UInt32 Other)
            {
                return Vertices[Other].Tangent != Wanted.Tangent || Vertices[Other].Handedness != Wanted.Handedness;
            };

            UInt32 Match = Origin;

            while (Match != kNone && Differs(Match))
            {
                Match = Copies[Match];
            }

            if (Match == kNone)
            {
                Match = static_cast<UInt32>(Vertices.GetSize());

                Vertices.Append(Wanted);
                Copies.Append(Exchange(Copies[Origin], Match));
            }
            Indices[Corner] = Match;
        }

        Source.Attributes |= Scene::Attribute::Tangent;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Vector3 Tangent::Steady(ConstRef<Vector3> Vector, ConstRef<Vector3> Fallback)
    {
        const Real32 Length = Vector.GetLength();
        return IsAlmostZero(Length) ? Fallback : Vector * (1.0f / Length);
    }
}