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

#include "Indexer.hpp"
#include <meshoptimizer.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Indexer::Unify(Ref<Scene> Source)
    {
        const UInt Corners = Source.Vertices.GetSize();

        Sequence<UInt32> Remap;
        Remap.Resize(Corners);

        const UInt Unique = meshopt_generateVertexRemap(
            Remap.GetData(),
            Source.Indices.GetData(),
            Source.Indices.GetSize(),
            Source.Vertices.GetData(),
            Corners,
            sizeof(Scene::Vertex));

        Sequence<Scene::Vertex> Welded;
        Welded.Resize(Unique);

        meshopt_remapVertexBuffer(
            Welded.GetData(),
            Source.Vertices.GetData(),
            Corners,
            sizeof(Scene::Vertex),
            Remap.GetData());

        meshopt_remapIndexBuffer(
            Source.Indices.GetData(),
            Source.Indices.GetData(),
            Source.Indices.GetSize(),
            Remap.GetData());

        Source.Vertices = Move(Welded);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Indexer::Optimize(Ref<Scene> Source, ConstSpan<Simplifier::Level> Levels)
    {
        const UInt Count = Source.Vertices.GetSize();

        // A level that gained nothing draws the run above it again, so each run is ordered once.
        Table<UInt32, Bool> Ordered;

        for (ConstRef<Simplifier::Level> Level : Levels)
        {
            for (ConstRef<Scene::Surface> Surface : Level.Surfaces)
            {
                if (Ordered.Contains(Surface.Offset))
                {
                    continue;
                }

                Ordered.Assign(Surface.Offset, true);

                const Ptr<UInt32> Run = Source.Indices.GetData() + Surface.Offset;
                meshopt_optimizeVertexCache(Run, Run, Surface.Count, Count);
            }
        }

        meshopt_optimizeVertexFetch(
            Source.Vertices.GetData(),
            Source.Indices.GetData(),
            Source.Indices.GetSize(),
            Source.Vertices.GetData(),
            Count,
            sizeof(Scene::Vertex));
    }
}