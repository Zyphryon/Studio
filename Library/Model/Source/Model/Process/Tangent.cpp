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
        ConstRef<Sequence<UInt32>>   Indices  = Source.Indices;

        Sequence<Vector3> Along;
        Sequence<Vector3> Across;
        Along.Fill(Vector3(0.0f, 0.0f, 0.0f), Vertices.GetSize());
        Across.Fill(Vector3(0.0f, 0.0f, 0.0f), Vertices.GetSize());

        for (UInt Cursor = 0; Cursor + 2 < Indices.GetSize(); Cursor += 3)
        {
            const Array Corners(Indices[Cursor], Indices[Cursor + 1], Indices[Cursor + 2]);

            const Vector3 Edge0  = Vertices[Corners[1]].Position  - Vertices[Corners[0]].Position;
            const Vector3 Edge1  = Vertices[Corners[2]].Position  - Vertices[Corners[0]].Position;
            const Vector2 Delta0 = Vertices[Corners[1]].TexCoord0 - Vertices[Corners[0]].TexCoord0;
            const Vector2 Delta1 = Vertices[Corners[2]].TexCoord0 - Vertices[Corners[0]].TexCoord0;

            // Collinear or collapsed texture coordinates pin no direction, so the triangle adds nothing.
            const Real32 Determinant = Delta0.GetX() * Delta1.GetY() - Delta1.GetX() * Delta0.GetY();

            if (IsAlmostZero(Determinant))
            {
                continue;
            }

            const Real32  Scale     = 1.0f / Determinant;
            const Vector3 Tangent   = (Edge0 * Delta1.GetY() - Edge1 * Delta0.GetY()) * Scale;
            const Vector3 Bitangent = (Edge1 * Delta0.GetX() - Edge0 * Delta1.GetX()) * Scale;

            for (const UInt32 Corner : Corners)
            {
                Along[Corner]  = Along[Corner] + Tangent;
                Across[Corner] = Across[Corner] + Bitangent;
            }
        }

        for (UInt Index = 0; Index < Vertices.GetSize(); ++Index)
        {
            Ref<Scene::Vertex> Vertex = Vertices[Index];

            // Kept orthogonal to the normal however the contributing triangles were angled.
            const Vector3 Projected = Along[Index] - Vertex.Normal * Vector3::Dot(Vertex.Normal, Along[Index]);

            // A vertex whose texture coordinates pin no direction takes any one off its normal.
            const Vector3 Aside = Steady(Vector3::Perpendicular(Vertex.Normal), Vector3::UnitX());

            Vertex.Tangent    = Steady(Projected, Aside);
            Vertex.Handedness = Vector3::Dot(
                Vector3::Cross(Vertex.Normal, Vertex.Tangent), Across[Index]) < 0.0f ? -1.0f : 1.0f;
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