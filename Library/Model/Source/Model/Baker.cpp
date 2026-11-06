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
            UInt Cast  = 0;

            for (ConstRef<Scene::Surface> Surface : Levels[Index].Surfaces)
            {
                Count += Surface.Count / 3;
            }

            for (ConstRef<Scene::Surface> Surface : Levels[Index].Casters)
            {
                Cast += Surface.Count / 3;
            }

            LOG_I("Model:     level {0} keeps {1} triangles ({2}%) above {3}% coverage, and casts {4}",
                Index,
                Count,
                Count * 100 / Triangles,
                static_cast<UInt>(Levels[Index].Coverage * 100.0f),
                Cast > 0 ? Cast : Count);
        }

        const Box Extent = Decoded.GetBounds();

        Bundle Result;
        Result.Mesh = Exporter::ExportMesh(Packed, Extent, Levels);

        if (Result.Mesh == nullptr)
        {
            return Bundle();
        }

        Str           Skeleton;
        Sequence<Str> Clips;

        if (!Decoded.Joints.IsEmpty())
        {
            Result.Skeleton = Exporter::ExportSkeleton(Decoded.Joints);

            if (Result.Skeleton == nullptr)
            {
                return Bundle();
            }

            Skeleton = Str::Print<"{0}.{1}">(Name, Exporter::kSkeletonOutput);

            // A clip is bounded by every pose it takes, so a limb swung past the resting mesh is never culled early.
            const Sequence<Real32> Reaches = Measure(Decoded);

            for (ConstRef<Scene::Clip> Clip : Decoded.Clips)
            {
                Ref<Bundle::Motion> Motion = Result.Clips.Append();
                Motion.Name = Sanitize(Clip.Name, "Clip");

                // Takes the cleaning brings to one name would write over each other's file, so a later one is numbered.
                if (Clips.Contains(Motion.Name))
                {
                    Motion.Name = Str::Print<"{0}_{1}">(Motion.Name, Clips.GetSize());
                }
                Motion.Data = Exporter::ExportAnimation(Clip, Decoded.Joints, Enclose(Clip, Decoded.Joints, Reaches));
                Clips.Append(Motion.Name);
            }

            for (ConstRef<Str> Once : Profile.Once)
            {
                if (!Clips.Contains(Once))
                {
                    LOG_W("Model: '--once' names '{0}', which no clip of the scene is called", Once);
                }
            }

            LOG_I("Model: {0} bones, {1} clips at {2} Hz, blended {3}",
                Decoded.Joints.GetSize(),
                Decoded.Clips.GetSize(),
                Profile.Rate,
                ZyEnum::GetName(Profile.Blend));
        }

        // The manifest names everything by file name alone, since the engine resolves it next to the manifest.
        Result.Manifest = Exporter::ExportModel(
            Str::Print<"{0}.{1}">(Name, Exporter::kMeshOutput), Skeleton, Decoded.Materials, Clips, Profile.Once);
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
        const SInt Slash = Max(StrFindLast(Destination, '/'), StrFindLast(Destination, '\\'));
        const SInt Dot   = StrFindLast(Destination, '.');
        const Text Stem  = Dot > Slash ? Destination.Slice(0, static_cast<UInt>(Dot)) : Destination;
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

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sequence<Real32> Baker::Measure(ConstRef<Scene> Decoded)
    {
        Sequence<Real32> Reaches;
        Reaches.Fill(0.0f, Decoded.Joints.GetSize());

        // Every bone a vertex leans on is grown to it, so a vertex blended between bones lies within all of them.
        for (ConstRef<Scene::Vertex> Vertex : Decoded.Vertices)
        {
            for (UInt Influence = 0; Influence < Scene::kInfluences; ++Influence)
            {
                if (Vertex.Weights[Influence] < Scene::kMinWeight)
                {
                    continue;
                }

                const UInt16 Joint = Vertex.Joints[Influence];
                const Real32 Reach = Matrix4x3::Project(Decoded.Joints[Joint].Inverse, Vertex.Position).GetLength();

                Reaches[Joint] = Max(Reaches[Joint], Reach);
            }
        }
        return Reaches;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sphere Baker::Enclose(ConstRef<Scene::Clip> Clip, ConstSpan<Scene::Joint> Joints, ConstSpan<Real32> Reaches)
    {
        const UInt Ticks = Clip.Motions.IsEmpty() ? 1 : Clip.Motions[0].Position.GetSize();

        Sequence<Matrix4x3> Locals;
        Sequence<Matrix4x3> Worlds;
        Sequence<Sphere>    Grown;

        Worlds.Fill(Matrix4x3::Identity(), Joints.GetSize());

        for (UInt Tick = 0; Tick < Ticks; ++Tick)
        {
            // A bone the clip leaves still holds its resting pose, and a driven one takes the tick's sample.
            Locals.Clear();

            for (ConstRef<Scene::Joint> Joint : Joints)
            {
                Locals.Append(Matrix4x3::FromTransform(Joint.Position, Joint.Scale, Joint.Rotation));
            }

            for (ConstRef<Scene::Motion> Motion : Clip.Motions)
            {
                if (Tick < Motion.Position.GetSize())
                {
                    Locals[Motion.Joint] = Matrix4x3::FromTransform(
                        Motion.Position[Tick], Motion.Scale[Tick], Motion.Rotation[Tick]);
                }
            }

            // Composed parents first, as the engine's skeleton does, so each bone stands where it is drawn.
            for (UInt Index = 0; Index < Joints.GetSize(); ++Index)
            {
                const SInt16 Parent = Joints[Index].Parent;

                Worlds[Index] = (Parent < 0 ? Locals[Index] : Worlds[Parent] * Locals[Index]);

                if (Reaches[Index] > 0.0f)
                {
                    const Vector3 Scale = Worlds[Index].GetScale();
                    const Real32  Most  = Max(Scale.GetX(), Scale.GetY(), Scale.GetZ());

                    Grown.Append(Sphere(Worlds[Index].GetTranslation(), Reaches[Index] * Most));
                }
            }
        }

        if (Grown.IsEmpty())
        {
            return Sphere(Vector3::Zero(), 0.0f);
        }

        // Centred on the box around every grown bone, and wide enough to hold the farthest of them whole.
        Vector3 Lower = Grown[0].GetCenter();
        Vector3 Upper = Grown[0].GetCenter();

        for (ConstRef<Sphere> Bone : Grown)
        {
            Lower = Vector3::Min(Lower, Bone.GetCenter() - Vector3(Bone.GetRadius()));
            Upper = Vector3::Max(Upper, Bone.GetCenter() + Vector3(Bone.GetRadius()));
        }

        const Vector3 Center = (Lower + Upper) * 0.5f;

        Real32 Radius = 0.0f;

        for (ConstRef<Sphere> Bone : Grown)
        {
            Radius = Max(Radius, Center.GetDistance(Bone.GetCenter()) + Bone.GetRadius());
        }
        return Sphere(Center, Radius);
    }
}