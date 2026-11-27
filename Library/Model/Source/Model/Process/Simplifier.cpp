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

#include "Simplifier.hpp"
#include <meshoptimizer.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sequence<Simplifier::Level> Simplifier::Generate(
        Ref<ZyJob::Service> Scheduler, Ref<Scene> Source, ConstSpan<Profile::Detail> Details, Real32 Drift)
    {
        // Attributes are weighted rather than their seams locked, the normal lower since shading forgives more drift.
        static constexpr Array kWeights(0.2f, 0.2f, 0.2f, 0.6f, 0.6f);

        Sequence<Level> Result;

        ConstSpan<Scene::Vertex>  Vertices = Source.Vertices;
        ConstSpan<Scene::Surface> Surfaces = Source.Surfaces;

        if (Vertices.IsEmpty() || Surfaces.IsEmpty())
        {
            return Result;
        }

        // The source geometry is always the first level, so the coarser ones only describe what they reduced.
        Ref<Level> Base = Result.Append();
        Base.Coverage = Details.IsEmpty() ? 0.0f : Details[0].Coverage;
        Base.Surfaces.Append(Surfaces);

        if (Details.GetSize() < 2)
        {
            return Result;
        }

        Sequence<Real32> Positions;
        Sequence<Real32> Attributes;
        Positions.Resize(Vertices.GetSize() * 3);
        Attributes.Resize(Vertices.GetSize() * kAttributes);

        for (UInt Index = 0; Index < Vertices.GetSize(); ++Index)
        {
            ConstRef<Scene::Vertex> Vertex = Vertices[Index];

            const Ptr<Real32> Point = Positions.GetData() + Index * 3;
            Point[0] = Vertex.Position.GetX();
            Point[1] = Vertex.Position.GetY();
            Point[2] = Vertex.Position.GetZ();

            const Ptr<Real32> Target = Attributes.GetData() + Index * kAttributes;
            Target[0] = Vertex.Normal.GetX();
            Target[1] = Vertex.Normal.GetY();
            Target[2] = Vertex.Normal.GetZ();
            Target[3] = Vertex.TexCoord0.GetX();
            Target[4] = Vertex.TexCoord0.GetY();
        }

        const UInt   Levels = Details.GetSize() - 1;
        const Real32 Scale  = meshopt_simplifyScale(Positions.GetData(), Vertices.GetSize(), 3 * sizeof(Real32));

        // One entry per surface per coarser level, so a reduction's place follows from its index, not its finish.
        const auto Reduce = [&](UInt32 Options, Real32 Limit, Ref<Sequence<Real32>> Strays)
        {
            Strays.Resize(Levels * Surfaces.GetSize());

            Sequence<Sequence<UInt32>> Reductions;
            Reductions.Resize(Levels * Surfaces.GetSize());

            // Every reduction reads the same arrays and writes only its own entry, and nothing grows the indices.
            const UInt32 Total = static_cast<UInt32>(Reductions.GetSize());

            Scheduler.Parallel(ZyJob::Lane::Compute, Total, [&](UInt32 Start, UInt32 End)
            {
                Sequence<UInt32> Scratch;

                for (UInt32 Slot = Start; Slot < End; ++Slot)
                {
                    ConstRef<Profile::Detail> Setting = Details[Slot / Surfaces.GetSize() + 1];
                    ConstRef<Scene::Surface>  Surface = Surfaces[Slot % Surfaces.GetSize()];

                    // Rounded down to a whole triangle and never below one.
                    const UInt Target = Max<UInt>(3, static_cast<UInt>(Surface.Count * Setting.Ratio) / 3 * 3);

                    if (Target >= Surface.Count)
                    {
                        continue;
                    }

                    Scratch.Resize(Surface.Count);

                    Real32 Stray = 0.0f;

                    const UInt Count = meshopt_simplifyWithAttributes(
                        Scratch.GetData(),
                        Source.Indices.GetData() + Surface.Offset,
                        Surface.Count,
                        Positions.GetData(),
                        Vertices.GetSize(),
                        3 * sizeof(Real32),
                        Attributes.GetData(),
                        kAttributes * sizeof(Real32),
                        kWeights.GetData(),
                        kAttributes,
                        nullptr,
                        Target,
                        Setting.Error * Limit,
                        Options,
                        AddressOf(Stray));

                    if (Count > 0 && Count < Surface.Count)
                    {
                        Reductions[Slot].Append(ConstSpan(Scratch.GetData(), Count));
                        Strays[Slot] = Stray;
                    }
                }
            });
            return Reductions;
        };

        constexpr UInt32 kOptions = meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute;

        Sequence<Real32> Strays;
        Sequence<Real32> Ignored;

        const Sequence<Sequence<UInt32>> Drawn = Reduce(kOptions, Scale, Strays);
        const Sequence<Sequence<UInt32>> Cast  = Reduce(kOptions | meshopt_SimplifyPermissive, Scale * kLoose, Ignored);

        // Nothing gained leaves the run it was asked to reduce, rather than a copy of it.
        const auto Place = [&](ConstRef<Sequence<UInt32>> Produced, ConstRef<Scene::Surface> Fallback)
        {
            if (Produced.IsEmpty())
            {
                return Fallback;
            }

            Scene::Surface Reduced;
            Reduced.Offset   = static_cast<UInt32>(Source.Indices.GetSize());
            Reduced.Count    = static_cast<UInt32>(Produced.GetSize());
            Reduced.Material = Fallback.Material;

            Source.Indices.Append(Produced);
            return Reduced;
        };

        // Gathered back in level order, then surface order, as they would have landed one after another.
        for (UInt Step = 0; Step < Levels; ++Step)
        {
            Ref<Level> Entry = Result.Append();
            Entry.Coverage = Details[Step + 1].Coverage;

            for (UInt Index = 0; Index < Surfaces.GetSize(); ++Index)
            {
                const UInt Slot = Step * Surfaces.GetSize() + Index;

                // A surface its casters could not shrink casts as the level draws it.
                ConstRef<Scene::Surface> Own = Entry.Surfaces.Append(Place(Drawn[Slot], Surfaces[Index]));
                Entry.Casters.Append(Place(Cast[Slot], Own));
            }
        }

        // A finer level is kept only while the next would stray past the drift, seen at the size it covers.
        if (Drift > 0.0f)
        {
            const Vector3 Size  = Source.GetBounds().GetSize();
            const Real32  Reach = Max(Size.GetX(), Size.GetY(), Size.GetZ());

            for (UInt Step = 0; Step < Levels; ++Step)
            {
                Real32 Worst = 0.0f;

                for (UInt Index = 0; Index < Surfaces.GetSize(); ++Index)
                {
                    Worst = Max(Worst, Strays[Step * Surfaces.GetSize() + Index]);
                }

                const Real32 Coverage = Drift * Reach / (kLines * Max(Worst, kEpsilon<Real32>));
                Result[Step].Coverage = (Step > 0 ? Min(Coverage, Result[Step - 1].Coverage) : Coverage);
            }
        }
        return Result;
    }
}