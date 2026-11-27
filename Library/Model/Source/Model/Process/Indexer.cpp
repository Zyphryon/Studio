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

        // Welded within a material alone, so a reduced triangle's material is that of any of its vertices.
        Sequence<UInt32> Owners;
        Owners.Fill(0, Corners);

        for (ConstRef<Scene::Surface> Surface : Source.Surfaces)
        {
            for (UInt32 Index = Surface.Offset; Index < Surface.Offset + Surface.Count; ++Index)
            {
                Owners[Source.Indices[Index]] = Surface.Material;
            }
        }

        const Array Streams(
            meshopt_Stream { Source.Vertices.GetData(), sizeof(Scene::Vertex), sizeof(Scene::Vertex) },
            meshopt_Stream { Owners.GetData(), sizeof(UInt32), sizeof(UInt32) });

        // A triangle that repeats another corner for corner draws it twice, and one with two alike draws nothing.
        const UInt Kept = meshopt_filterIndexBufferMulti(
            Source.Indices.GetData(),
            Source.Indices.GetData(),
            Source.Indices.GetSize(),
            Corners,
            Streams.GetData(),
            Streams.GetSize());

        if (Kept < Source.Indices.GetSize())
        {
            Source.Indices.Remove(Kept, Source.Indices.GetSize() - Kept);
        }

        // The triangles left of a material still lie together and in order, so each run is counted off again.
        UInt32 Offset = 0;

        for (Ref<Scene::Surface> Surface : Source.Surfaces)
        {
            Surface.Offset = Offset;

            while (Offset < Kept && Owners[Source.Indices[Offset]] == Surface.Material)
            {
                Offset += 3;
            }
            Surface.Count = Offset - Surface.Offset;
        }

        // A run left with no triangle is left out, as a level leaves out one its reduction emptied.
        Source.Surfaces.RemoveSomeIf([](ConstRef<Scene::Surface> Surface)
        {
            return Surface.Count == 0;
        });

        // Corners no triangle is left to name are given no vertex, so they fall out of the weld.
        Sequence<UInt32> Remap;
        Remap.Resize(Corners);

        const UInt Unique = meshopt_generateVertexRemapMulti(
            Remap.GetData(),
            Source.Indices.GetData(),
            Source.Indices.GetSize(),
            Corners,
            Streams.GetData(),
            Streams.GetSize());

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

    void Indexer::Shadow(Ref<Scene> Source, Span<Simplifier::Level> Levels)
    {
        const UInt              Count = Source.Vertices.GetSize();
        ConstRef<Scene::Vertex> First = Source.Vertices[0];

        // A shadow reads where a vertex is, where its cutout is sampled and which bones bend it, and nothing else.
        const Array Streams(
            meshopt_Stream { AddressOf(First.Position),  sizeof(First.Position),  sizeof(Scene::Vertex) },
            meshopt_Stream { AddressOf(First.TexCoord0), sizeof(First.TexCoord0), sizeof(Scene::Vertex) },
            meshopt_Stream { AddressOf(First.Joints),    sizeof(First.Joints),    sizeof(Scene::Vertex) },
            meshopt_Stream { AddressOf(First.Weights),   sizeof(First.Weights),   sizeof(Scene::Vertex) });

        for (Ref<Simplifier::Level> Level : Levels)
        {
            // A level that casts what it draws is welded in a copy of its runs, which it goes on drawing as they are.
            if (Level.Casters.IsEmpty())
            {
                for (ConstRef<Scene::Surface> Surface : Level.Surfaces)
                {
                    const UInt32 Offset = static_cast<UInt32>(Source.Indices.GetSize());

                    Source.Indices.Expand(Offset + Surface.Count);
                    Copy(Source.Indices.GetData() + Offset, Surface.Count, Source.Indices.GetData() + Surface.Offset);

                    Level.Casters.Append(Offset, Surface.Count, Surface.Material);
                }
            }

            // Its casters lie run after run, so they are welded as one, and a corner may share another run's vertex.
            const UInt32      Head = Level.Casters.GetFront().Offset;
            const UInt32      Size = Level.Casters.GetBack().Offset + Level.Casters.GetBack().Count - Head;
            const Ptr<UInt32> Run  = Source.Indices.GetData() + Head;

            meshopt_generateShadowIndexBufferMulti(Run, Run, Size, Count, Streams.GetData(), Streams.GetSize());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Indexer::Optimize(Ref<ZyJob::Service> Scheduler, Ref<Scene> Source, ConstSpan<Simplifier::Level> Levels)
    {
        const UInt             Count     = Source.Vertices.GetSize();
        const ConstPtr<Real32> Positions = reinterpret_cast<ConstPtr<Real32>>(Source.Vertices.GetData());

        Sequence<Scene::Surface> Runs;

        for (ConstRef<Simplifier::Level> Level : Levels)
        {
            Runs.Append(Level.Surfaces);
            Runs.Append(Level.Casters);
        }

        // The library reads a front as wound the other way, so a run is turned over for it and turned back after.
        const auto Turn = [](Ptr<UInt32> Run, UInt32 Size)
        {
            for (UInt32 Corner = 0; Corner < Size; Corner += 3)
            {
                Swap(Run[Corner + 1], Run[Corner + 2]);
            }
        };

        // Every run is a draw of its own over indices no other run holds, so each is ordered apart.
        Scheduler.Parallel(ZyJob::Lane::Compute, static_cast<UInt32>(Runs.GetSize()), [&](UInt32 Start, UInt32 End)
        {
            for (UInt32 Index = Start; Index < End; ++Index)
            {
                const Ptr<UInt32> Run  = Source.Indices.GetData() + Runs[Index].Offset;
                const UInt32      Size = Runs[Index].Count;

                meshopt_optimizeVertexCache(Run, Run, Size, Count);

                // Its clusters are then drawn front to back, at little cost to the cache, so less is shaded twice.
                Turn(Run, Size);
                meshopt_optimizeOverdraw(Run, Run, Size, Positions, Count, sizeof(Scene::Vertex), kCache);
                Turn(Run, Size);
            }
        });

        // Numbered from the coarsest level up, so a coarse level reads a short range at the head of the vertices.
        Sequence<UInt32> Coarsest;
        Coarsest.Reserve(Source.Indices.GetSize());

        for (UInt Index = Runs.GetSize(); Index > 0; --Index)
        {
            Coarsest.Append(ConstSpan(Source.Indices.GetData() + Runs[Index - 1].Offset, Runs[Index - 1].Count));
        }

        Sequence<UInt32> Remap;
        Remap.Resize(Count);

        const UInt Used = meshopt_optimizeVertexFetchRemap(
            Remap.GetData(),
            Coarsest.GetData(),
            Coarsest.GetSize(),
            Count);

        Sequence<Scene::Vertex> Ordered;
        Ordered.Resize(Used);

        meshopt_remapVertexBuffer(
            Ordered.GetData(),
            Source.Vertices.GetData(),
            Count,
            sizeof(Scene::Vertex),
            Remap.GetData());

        meshopt_remapIndexBuffer(
            Source.Indices.GetData(),
            Source.Indices.GetData(),
            Source.Indices.GetSize(),
            Remap.GetData());

        Source.Vertices = Move(Ordered);
    }
}