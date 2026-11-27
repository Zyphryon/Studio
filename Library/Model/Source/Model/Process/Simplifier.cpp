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

        // A vertex belongs to one surface, which the weld keeps apart, so a triangle's material is that of any corner.
        Sequence<UInt32> Owners;
        Owners.Fill(0, Vertices.GetSize());

        // A vertex stands for a third of each face around it, which is how the surface a level leaves out is added up.
        Sequence<Real32> Spans;
        Spans.Fill(0.0f, Vertices.GetSize());

        Real64 Area = 0.0;

        for (UInt Index = 0; Index < Surfaces.GetSize(); ++Index)
        {
            const ConstSpan<UInt32> Run(Source.Indices.GetData() + Surfaces[Index].Offset, Surfaces[Index].Count);

            for (UInt Corner = 0; Corner + 2 < Run.GetSize(); Corner += 3)
            {
                const Vector3 Along  = Vertices[Run[Corner + 1]].Position - Vertices[Run[Corner]].Position;
                const Vector3 Across = Vertices[Run[Corner + 2]].Position - Vertices[Run[Corner]].Position;
                const Real32  Third  = Vector3::Cross(Along, Across).GetLength() / 6.0f;

                for (UInt Each = Corner; Each < Corner + 3; ++Each)
                {
                    Owners[Run[Each]] = static_cast<UInt32>(Index);
                    Spans[Run[Each]] += Third;
                }
                Area += Third * 3.0;
            }
        }

        const ConstPtr<Real32> Positions = reinterpret_cast<ConstPtr<Real32>>(Vertices.GetData());

        // Every seam may be reduced across but one a triangle cannot straddle, where a material, chart or mirror ends.
        Sequence<UInt32> Twins;
        Twins.Resize(Vertices.GetSize());

        meshopt_generatePositionRemap(Twins.GetData(), Positions, Vertices.GetSize(), sizeof(Scene::Vertex));

        Sequence<UInt8> Locks;
        Locks.Fill(0, Vertices.GetSize());

        for (UInt Index = 0; Index < Vertices.GetSize(); ++Index)
        {
            ConstRef<Scene::Vertex> Vertex = Vertices[Index];
            ConstRef<Scene::Vertex> Twin   = Vertices[Twins[Index]];

            const Bool Split = Owners[Index] != Owners[Twins[Index]] || Vertex.Handedness != Twin.Handedness;
            const Bool Seam  = Vertex.TexCoord0 != Twin.TexCoord0 || Vertex.TexCoord1 != Twin.TexCoord1;

            Locks[Index] = Split || Seam ? meshopt_SimplifyVertex_Protect : 0;
        }

        // A normal of no weight is left out of the reduction, as one the scene lacks is, and a shadow weighs none.
        const Real32 Shade = Source.Has(Scene::Attribute::Normal) ? kNormal : 0.0f;
        const Array  Drawn(Shade, Shade, Shade);
        const Array  Cast(0.0f, 0.0f, 0.0f);

        // Small loose parts are dropped whole, and a skin is kept regular since a sliver bends badly between its bones.
        const UInt32 Options = meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute | meshopt_SimplifyErrorClamped
            | meshopt_SimplifyPermissive | meshopt_SimplifyPrune
            | (Source.Has(Scene::Attribute::Skin) ? meshopt_SimplifyRegularize : 0);

        // Each level is reduced from the one before, so it shares that one's vertices and adds to how far it strays.
        Sequence<UInt32> Chain = Source.Indices;
        Sequence<UInt32> Coarser;
        Real32           Stray = 0.0f;

        const auto Reduce = [&](UInt Target, Real32 Limit, ConstRef<Array<Real32, kAttributes>> Weights)
        {
            Sequence<UInt32> Scratch;
            Scratch.Resize(Chain.GetSize());

            Real32 Added = 0.0f;

            const UInt Count = meshopt_simplifyWithAttributes(
                Scratch.GetData(),
                Chain.GetData(),
                Chain.GetSize(),
                Positions,
                Vertices.GetSize(),
                sizeof(Scene::Vertex),
                reinterpret_cast<ConstPtr<Real32>>(AddressOf(Vertices[0].Normal)),
                sizeof(Scene::Vertex),
                Weights.GetData(),
                kAttributes,
                Locks.GetData(),
                Target,
                Limit,
                Options,
                AddressOf(Added));

            Coarser.Clear();
            Coarser.Append(ConstSpan(Scratch.GetData(), Count));
            return Added;
        };

        const Real32 Scale = meshopt_simplifyScale(Positions, Vertices.GetSize(), sizeof(Scene::Vertex));
        const UInt   Whole = Chain.GetSize();

        for (UInt Step = 1; Step < Details.GetSize(); ++Step)
        {
            // Rounded down to a whole triangle and never below one.
            const UInt   Target = Max<UInt>(3, static_cast<UInt>(Whole * Details[Step].Ratio) / 3 * 3);
            const Real32 Limit  = Details[Step].Error * Scale;

            Real32 Budget = Limit - Stray;
            Real32 Added  = 0.0f;
            Bool   Kept   = false;

            // A level that leaves too much of the source out is reduced again at half the error, which drops less.
            for (UInt Try = 0; Try < kTries && !Kept && Target < Chain.GetSize() && Budget > 0.0f; ++Try)
            {
                Added = Reduce(Target, Budget, Drawn);

                // Shedding too little is not worth a level, and less error would shed less still.
                if (Coarser.IsEmpty() || Coarser.GetSize() > Chain.GetSize() * kGain)
                {
                    break;
                }

                Kept    = Miss(Scheduler, Vertices, Spans, Coarser, Limit) <= Area * kLost;
                Budget *= 0.5f;
            }

            // A level that is not worth drawing is left out, and the one before stands in down to its coverage.
            if (!Kept)
            {
                Result.GetBack().Coverage = Details[Step].Coverage;
                continue;
            }

            Swap(Chain, Coarser);
            Stray += Added;

            // A finer level is kept only while this one would stray past the drift, seen at the size it covers.
            if (Drift > 0.0f)
            {
                Result.GetBack().Coverage = Drift * Scale / (kLines * Max(Stray, kEpsilon<Real32>));
            }

            Ref<Level> Entry = Result.Append();
            Entry.Coverage = Details[Step].Coverage;
            Entry.Surfaces = Lay(Chain, Owners, Surfaces, Source.Indices);

            // A level its error held above its share casts a looser reduction of itself, which a shadow forgives.
            if (Chain.GetSize() > Target)
            {
                const Real32 Loose = Limit * kLoose;

                Reduce(Target, Loose - Stray, Cast);

                const Bool Lighter = !Coarser.IsEmpty() && Coarser.GetSize() <= Chain.GetSize() * kGain;

                if (Lighter && Miss(Scheduler, Vertices, Spans, Coarser, Loose) <= Area * kLost)
                {
                    Entry.Casters = Lay(Coarser, Owners, Surfaces, Source.Indices);
                }
            }
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Simplifier::Apart(Vector3 Point, Vector3 A, Vector3 B, Vector3 C)
    {
        // The closest point, found from the corner, edge or face region the point projects into.
        const Vector3 Along  = B - A;
        const Vector3 Across = C - A;

        const Vector3 FromA = Point - A;
        const Real32  D1    = Vector3::Dot(Along,  FromA);
        const Real32  D2    = Vector3::Dot(Across, FromA);

        if (D1 <= 0.0f && D2 <= 0.0f)
        {
            return FromA.GetLengthSquared();
        }

        const Vector3 FromB = Point - B;
        const Real32  D3    = Vector3::Dot(Along,  FromB);
        const Real32  D4    = Vector3::Dot(Across, FromB);

        if (D3 >= 0.0f && D4 <= D3)
        {
            return FromB.GetLengthSquared();
        }

        if (D1 * D4 - D3 * D2 <= 0.0f && D1 >= 0.0f && D3 <= 0.0f)
        {
            return (FromA - Along * (D1 / (D1 - D3))).GetLengthSquared();
        }

        const Vector3 FromC = Point - C;
        const Real32  D5    = Vector3::Dot(Along,  FromC);
        const Real32  D6    = Vector3::Dot(Across, FromC);

        if (D6 >= 0.0f && D5 <= D6)
        {
            return FromC.GetLengthSquared();
        }

        if (D5 * D2 - D1 * D6 <= 0.0f && D2 >= 0.0f && D6 <= 0.0f)
        {
            return (FromA - Across * (D2 / (D2 - D6))).GetLengthSquared();
        }

        if (D3 * D6 - D5 * D4 <= 0.0f && D4 >= D3 && D5 >= D6)
        {
            return (FromB - (C - B) * ((D4 - D3) / ((D4 - D3) + (D5 - D6)))).GetLengthSquared();
        }

        const Real32 OnB = D5 * D2 - D1 * D6;
        const Real32 OnC = D1 * D4 - D3 * D2;
        const Real32 All = D3 * D6 - D5 * D4 + OnB + OnC;

        return (FromA - Along * (OnB / All) - Across * (OnC / All)).GetLengthSquared();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Real32 Simplifier::Miss(Ref<ZyJob::Service> Scheduler, ConstSpan<Scene::Vertex> Vertices,
        ConstSpan<Real32> Spans, ConstSpan<UInt32> Triangles, Real32 Reach)
    {
        // A vertex the level still draws is where it was, so only the ones it let go of are looked for.
        Sequence<Real32> Left(Spans);

        Vector3 Lower(kMaximum<Real32>);
        Vector3 Upper(-kMaximum<Real32>);

        for (const UInt32 Index : Triangles)
        {
            Left[Index] = 0.0f;

            Lower = Vector3::Min(Lower, Vertices[Index].Position);
            Upper = Vector3::Max(Upper, Vertices[Index].Position);
        }

        // Cut no finer than the reach, so whatever lies within it of a point is in the cells around the point's own.
        const Vector3 Size = Upper - Lower;
        const Real32  Side = Max(Reach, Max(Size.GetX(), Size.GetY(), Size.GetZ()) / kCells, kEpsilon<Real32>);

        const auto Slot = [Side](Real32 Value, Real32 Origin)
        {
            return static_cast<SInt32>(Floor((Value - Origin) / Side));
        };

        const SInt32 Wide = Slot(Upper.GetX(), Lower.GetX()) + 1;
        const SInt32 Tall = Slot(Upper.GetY(), Lower.GetY()) + 1;
        const SInt32 Deep = Slot(Upper.GetZ(), Lower.GetZ()) + 1;

        // A triangle is listed in every cell its box touches, counted first so each cell's list is one run.
        const auto Visit = [&](UInt Corner, auto Each)
        {
            ConstRef<Vector3> A = Vertices[Triangles[Corner]].Position;
            ConstRef<Vector3> B = Vertices[Triangles[Corner + 1]].Position;
            ConstRef<Vector3> C = Vertices[Triangles[Corner + 2]].Position;

            const Vector3 Least = Vector3::Min(A, Vector3::Min(B, C));
            const Vector3 Most  = Vector3::Max(A, Vector3::Max(B, C));

            const SInt32 First = Slot(Least.GetX(), Lower.GetX());
            const SInt32 Last  = Slot(Most.GetX(),  Lower.GetX());

            for (SInt32 Layer = Slot(Least.GetZ(), Lower.GetZ()); Layer <= Slot(Most.GetZ(), Lower.GetZ()); ++Layer)
            {
                for (SInt32 Row = Slot(Least.GetY(), Lower.GetY()); Row <= Slot(Most.GetY(), Lower.GetY()); ++Row)
                {
                    for (SInt32 Column = First; Column <= Last; ++Column)
                    {
                        Each(static_cast<UInt32>((Layer * Tall + Row) * Wide + Column));
                    }
                }
            }
        };

        Sequence<UInt32> Starts;
        Starts.Fill(0, static_cast<UInt>(Wide) * Tall * Deep + 1);

        for (UInt Corner = 0; Corner < Triangles.GetSize(); Corner += 3)
        {
            Visit(Corner, [&Starts](UInt32 Cell)
            {
                ++Starts[Cell + 1];
            });
        }

        for (UInt Cell = 1; Cell < Starts.GetSize(); ++Cell)
        {
            Starts[Cell] += Starts[Cell - 1];
        }

        Sequence<UInt32> Cursors = Starts;
        Sequence<UInt32> Held;
        Held.Resize(Starts.GetBack());

        for (UInt Corner = 0; Corner < Triangles.GetSize(); Corner += 3)
        {
            Visit(Corner, [&Cursors, &Held, Corner](UInt32 Cell)
            {
                Held[Cursors[Cell]++] = static_cast<UInt32>(Corner);
            });
        }

        // Every vertex writes its own entry alone, so the pool shares nothing but what it reads.
        Scheduler.Parallel(ZyJob::Lane::Compute, static_cast<UInt32>(Vertices.GetSize()), [&](UInt32 Start, UInt32 End)
        {
            for (UInt32 Index = Start; Index < End; ++Index)
            {
                if (Left[Index] == 0.0f)
                {
                    continue;
                }

                const Vector3 Point  = Vertices[Index].Position;
                const SInt32  Layer  = Slot(Point.GetZ(), Lower.GetZ());
                const SInt32  Row    = Slot(Point.GetY(), Lower.GetY());
                const SInt32  Column = Slot(Point.GetX(), Lower.GetX());

                Bool Found = false;

                for (SInt32 Z = Max(Layer - 1, 0); Z <= Min(Layer + 1, Deep - 1) && !Found; ++Z)
                {
                    for (SInt32 Y = Max(Row - 1, 0); Y <= Min(Row + 1, Tall - 1) && !Found; ++Y)
                    {
                        for (SInt32 X = Max(Column - 1, 0); X <= Min(Column + 1, Wide - 1) && !Found; ++X)
                        {
                            const UInt32 Cell = static_cast<UInt32>((Z * Tall + Y) * Wide + X);

                            for (UInt32 Item = Starts[Cell]; Item < Starts[Cell + 1] && !Found; ++Item)
                            {
                                const ConstPtr<UInt32> Corners = Triangles.GetData() + Held[Item];

                                Found = Apart(Point,
                                    Vertices[Corners[0]].Position,
                                    Vertices[Corners[1]].Position,
                                    Vertices[Corners[2]].Position) <= Reach * Reach;
                            }
                        }
                    }
                }

                if (Found)
                {
                    Left[Index] = 0.0f;
                }
            }
        });

        Real64 Missed = 0.0;

        for (const Real32 Span : Left)
        {
            Missed += Span;
        }
        return static_cast<Real32>(Missed);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sequence<Scene::Surface> Simplifier::Lay(ConstSpan<UInt32> Triangles,
        ConstSpan<UInt32> Owners, ConstSpan<Scene::Surface> Surfaces, Ref<Sequence<UInt32>> Indices)
    {
        // Counted first, so each material's triangles land in one run however the reduction left them mixed.
        Sequence<UInt32> Cursors;
        Cursors.Fill(0, Surfaces.GetSize());

        for (UInt Corner = 0; Corner < Triangles.GetSize(); Corner += 3)
        {
            Cursors[Owners[Triangles[Corner]]] += 3;
        }

        Sequence<Scene::Surface> Runs;

        UInt32 Offset = static_cast<UInt32>(Indices.GetSize());

        for (UInt Index = 0; Index < Surfaces.GetSize(); ++Index)
        {
            const UInt32 Count = Exchange(Cursors[Index], Offset);

            if (Count > 0)
            {
                Runs.Append(Offset, Count, Surfaces[Index].Material);
                Offset += Count;
            }
        }

        Indices.Expand(Offset);

        for (UInt Corner = 0; Corner < Triangles.GetSize(); Corner += 3)
        {
            Ref<UInt32> Cursor = Cursors[Owners[Triangles[Corner]]];

            Copy(Indices.GetData() + Cursor, 3, Triangles.GetData() + Corner);
            Cursor += 3;
        }
        return Runs;
    }
}