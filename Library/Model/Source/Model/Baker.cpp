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

#include "Baker.hpp"
#include "Model/Process/Indexer.hpp"
#include "Model/Process/Tangent.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Baker::Baker(Ref<ZyJob::Service> Scheduler)
        : mScheduler { Scheduler }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Baker::Bundle Baker::Bake(ConstSpan<Byte> Source, Text Type, Text Name, ConstRef<Profile> Profile) const
    {
        if (Profile.Scale <= 0.0f || Profile.Units < 0.0f || Profile.Rate <= 0.0f)
        {
            LOG_E("Model: the scale and the rate have to be positive and the unit target cannot be negative");

            return Bundle();
        }

        Scene Decoded = Importer::Import(mScheduler, Source, Type, Profile);

        if (Decoded.Vertices.IsEmpty() || Decoded.Indices.IsEmpty())
        {
            LOG_E("Model: the scene carries no drawable geometry");

            return Bundle();
        }

        // An infinity or a NaN would poison the bounds the engine culls against, and either minus itself is a NaN.
        for (ConstRef<Scene::Vertex> Vertex : Decoded.Vertices)
        {
            const Vector3 Spread = Vertex.Position - Vertex.Position;

            if (IsNaN(Spread.GetX()) || IsNaN(Spread.GetY()) || IsNaN(Spread.GetZ()))
            {
                LOG_E("Model: the scene carries a position that is not a finite number");

                return Bundle();
            }
        }

        const UInt Corners   = Decoded.Vertices.GetSize();
        const UInt Triangles = Decoded.Indices.GetSize() / 3;

        Indexer::Unify(Decoded);

        // Built after the weld, so a vertex shared by several triangles takes all of them and the frame stays smooth.
        const Bool Bare = !Decoded.Has(Scene::Attribute::Tangent);

        if (Profile.Tangents && Profile.Wants(ZyGraphic::VertexSlot::Tangent) && Bare)
        {
            Tangent::Generate(Decoded);
        }

        const Sequence<Simplifier::Level> Levels = Simplifier::Generate(mScheduler, Decoded, Profile.Details);

        Indexer::Optimize(Decoded, Levels);

        const Layout::Result Packed = Layout::Pack(Decoded, Profile.Attributes);

        if (Packed.Vertices == nullptr)
        {
            return Bundle();
        }

        LOG_I("Model: {0} vertices from {1} corners, {2} triangles, {3} materials, {4} attributes in {5} streams",
            Decoded.Vertices.GetSize(),
            Corners, Triangles,
            Decoded.Materials.GetSize(),
            Packed.Bindings.GetSize(),
            Packed.Streams);

        for (UInt Index = 0; Index < Levels.GetSize(); ++Index)
        {
            UInt Count = 0;

            for (ConstRef<Scene::Surface> Surface : Levels[Index].Surfaces)
            {
                Count += Surface.Count / 3;
            }

            LOG_I("Model:     level {0} keeps {1} triangles ({2}%) above {3}% coverage",
                Index,
                Count,
                Count * 100 / Triangles,
                static_cast<UInt>(Levels[Index].Coverage * 100.0f));
        }

        const Box Extent = Decoded.GetBounds();

        Bundle Result;
        Result.Mesh = Exporter::ExportMesh(Packed, Extent, Levels);

        if (Result.Mesh == nullptr)
        {
            return Bundle();
        }

        Str Skeleton;

        if (!Decoded.Joints.IsEmpty())
        {
            Result.Skeleton = Exporter::ExportSkeleton(Decoded.Joints);

            if (Result.Skeleton == nullptr)
            {
                return Bundle();
            }

            Skeleton = Str::Print<"{0}.{1}">(Name, Exporter::kSkeletonOutput);

            // A clip is bounded by the sphere around the resting mesh, which holds while no bone swings outside it.
            for (ConstRef<Scene::Clip> Clip : Decoded.Clips)
            {
                Ref<Bundle::Motion> Motion = Result.Clips.Append();
                Motion.Name = Sanitize(Clip.Name, "Clip");
                Motion.Data = Exporter::ExportAnimation(Clip, Decoded.Joints, Sphere::Enclose(Extent));
            }

            LOG_I("Model: {0} bones, {1} clips at {2} Hz, blended {3}",
                Decoded.Joints.GetSize(),
                Decoded.Clips.GetSize(),
                Profile.Rate,
                ZyEnum::GetName(Profile.Blend));
        }

        // The manifest names everything by file name alone, since the engine resolves it next to the manifest.
        Result.Manifest = Exporter::ExportModel(
            Str::Print<"{0}.{1}">(Name, Exporter::kMeshOutput), Skeleton, Decoded.Materials);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Baker::Bake(Text Source, Text Destination, ConstRef<Profile> Profile) const
    {
        Blob Input;

        if (Filesystem::Read(Source, Input) != Filesystem::Result::Success || Input == nullptr)
        {
            LOG_E("Model: failed to read '{0}'", Source);

            return false;
        }

        // Every file lands beside the manifest under its name, which is how the manifest refers to them.
        const Text Stem  = StrBeforeLast(Destination, '.');
        const SInt Slash = Max(StrFindLast(Stem, '/'), StrFindLast(Stem, '\\'));
        const Text Name  = Stem.Slice(static_cast<UInt>(Slash + 1));

        const Bundle Output = Bake(Input, StrAfterLast(Source, '.'), Name, Profile);

        if (Output.Mesh == nullptr)
        {
            return false;
        }

        const auto Store = [Source](Text Path, ConstSpan<Byte> Bytes)
        {
            Filesystem::Ensure(Path);

            if (Filesystem::Write(Path, Bytes) != Filesystem::Result::Success)
            {
                LOG_E("Model: failed to write '{0}'", Path);

                return false;
            }

            LOG_I("Model: '{0}' -> '{1}' ({2} bytes)", Source, Path, Bytes.GetSize());
            return true;
        };

        if (!Store(Str::Print<"{0}.{1}">(Stem, Exporter::kMeshOutput), Output.Mesh))
        {
            return false;
        }

        const Str Skeleton = Str::Print<"{0}.{1}">(Stem, Exporter::kSkeletonOutput);

        if (Output.Skeleton != nullptr && !Store(Skeleton, Output.Skeleton))
        {
            return false;
        }

        // Named after the clip as well as the model, so several clips do not write over one another.
        for (ConstRef<Bundle::Motion> Clip : Output.Clips)
        {
            if (!Store(Str::Print<"{0}.{1}.{2}">(Stem, Clip.Name, Exporter::kAnimationOutput), Clip.Data))
            {
                return false;
            }
        }

        // Written last, since it is the file the engine loads and the one that names all the others.
        const Text Manifest = Output.Manifest;
        return Store(Destination, ConstSpan(reinterpret_cast<ConstPtr<Byte>>(Manifest.GetData()), Manifest.GetSize()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Baker::Sanitize(Text Value, Text Fallback)
    {
        // FBX qualifies a take as 'Rig|Action', and these are the characters a path forbids or reads as its own.
        Str Result;

        for (UInt Index = 0; Index < Value.GetSize(); ++Index)
        {
            const Char Letter = Value[Index];
            Result.Append(Letter == ' ' || StrFind(Text("/\\:|<>\"?*"), Letter) >= 0 ? '_' : Letter);
        }
        return Result.IsEmpty() ? Str(Fallback) : Result;
    }
}