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

#include "Exporter.hpp"
#include <Zyphryon.Graphic/Mesh.hpp>
#include <Zyphryon.Render/Model/Skeleton3D.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool Steady(ConstSpan<Vector3> Values, Real32 Threshold)
    {
        for (ConstRef<Vector3> Value : Values)
        {
            if ((Value - Values[0]).GetLength() > Threshold)
            {
                return false;
            }
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool Steady(ConstSpan<Quaternion> Values)
    {
        // Compared by dot, since a quaternion and its negation are the same rotation.
        for (ConstRef<Quaternion> Value : Values)
        {
            if (Abs(Quaternion::Dot(Value, Values[0])) < 1.0f - 1e-6f)
            {
                return false;
            }
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Type>
    static void Emit(Ref<Writer> Output, ConstSpan<Type> Values, ConstSpan<Type> Rates, Bool Fixed, Bool Shaped)
    {
        // A component held still is one value and describes no curve, so it goes out alone however the clip blends.
        const UInt32 Size = static_cast<UInt32>((Fixed ? 1 : Values.GetSize()) * sizeof(Type));
        Output.Write(Values.GetData(), Size);

        // The slope runs across the sample rather than up to it, so the arriving and leaving tangents are one block.
        if (Shaped && !Fixed)
        {
            Output.Write(Rates.GetData(), Size);
            Output.Write(Rates.GetData(), Size);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Exporter::ExportMesh(ConstRef<Layout::Result> Geometry, Box Extent, ConstSpan<Simplifier::Level> Levels)
    {
        UInt Primitives = 0;

        for (ConstRef<Simplifier::Level> Level : Levels)
        {
            Primitives += Level.Surfaces.GetSize();
        }

        if (Geometry.Bindings.GetSize() > 0xFF || Primitives > 0xFFFF || Levels.GetSize() > ZyGraphic::Mesh::kMaxDetail)
        {
            LOG_E("Model: the geometry has more bindings, primitives or levels than the format can name");

            return Blob();
        }

        Writer Output(1024 + Geometry.Vertices.GetSize() + Geometry.Indices.GetSize());
        Output.Write<UInt32>(kMeshMagic);
        Output.Write<UInt16>(kMeshVersion);

        Output.Write<UInt32>(kMeshChunk);
        Output.WriteBlock<UInt32>([&](Ref<Writer> Body)
        {
            Body.Write(Geometry.Extended ? ZyGraphic::Mesh::Property::Extended : ZyGraphic::Mesh::Property::None);
            Body.Write(Extent);
            Body.Write<UInt8>(static_cast<UInt8>(Geometry.Bindings.GetSize()));

            for (ConstRef<Layout::Binding> Entry : Geometry.Bindings)
            {
                Body.Write(Entry.Slot);
                Body.Write(Entry.Format);
                Body.Write<UInt16>(Entry.Stride);
                Body.Write<UInt32>(Entry.Offset);
            }

            Body.Write<UInt8>(static_cast<UInt8>(Levels.GetSize()));

            for (ConstRef<Simplifier::Level> Level : Levels)
            {
                Body.Write<Real32>(Level.Coverage);
            }

            // One flat run grouped by level, which is how the loader learns where each level begins.
            Body.Write<UInt16>(static_cast<UInt16>(Primitives));

            for (UInt Index = 0; Index < Levels.GetSize(); ++Index)
            {
                for (ConstRef<Scene::Surface> Entry : Levels[Index].Surfaces)
                {
                    ZyGraphic::Mesh::Primitive Primitive;
                    Zero(AddressOf(Primitive), 1);

                    Primitive.Range.Count     = Entry.Count;
                    Primitive.Range.Base      = 0;
                    Primitive.Range.Offset    = Entry.Offset;
                    Primitive.Range.Instances = 1;
                    Primitive.Material        = Entry.Material;
                    Primitive.Level           = static_cast<UInt8>(Index);
                    Body.Write(Primitive);
                }
            }
        });

        Output.Write<UInt32>(kVerticesChunk);
        Output.WriteBlock<UInt32>([&Geometry](Ref<Writer> Body)
        {
            const ConstSpan Packed(Geometry.Vertices.GetData<Byte>(), Geometry.Vertices.GetSize());
            Body.WriteBlockCompressed<UInt32>(Packed, 3);
        });

        Output.Write<UInt32>(kIndicesChunk);
        Output.WriteBlock<UInt32>([&Geometry](Ref<Writer> Body)
        {
            const ConstSpan Packed(Geometry.Indices.GetData<Byte>(), Geometry.Indices.GetSize());
            Body.WriteBlockCompressed<UInt32>(Packed, 3);
        });
        return Output.Detach();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Exporter::ExportSkeleton(ConstSpan<Scene::Joint> Joints)
    {
        if (Joints.IsEmpty() || Joints.GetSize() > Scene::kMaxJoints)
        {
            LOG_E("Model: the rig is empty or carries more bones than the format can name");

            return Blob();
        }

        Writer Output(16 + Joints.GetSize() * sizeof(ZyRender::Skeleton3D::Bone));
        Output.Write<UInt32>(kSkeletonMagic);
        Output.Write<UInt16>(kSkeletonVersion);
        Output.Write<SInt16>(static_cast<SInt16>(Joints.GetSize()));

        for (ConstRef<Scene::Joint> Entry : Joints)
        {
            ZyRender::Skeleton3D::Bone Bone;
            Zero(AddressOf(Bone), 1);

            Bone.Name     = Scene::Identify(Entry.Name);
            Bone.Parent   = Entry.Parent;
            Bone.Position = Entry.Position;
            Bone.Scale    = Entry.Scale;
            Bone.Rotation = Entry.Rotation;
            Bone.Inverse  = Entry.Inverse;
            Output.Write(Bone);
        }
        return Output.Detach();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Exporter::ExportAnimation(ConstRef<Scene::Clip> Clip, ConstSpan<Scene::Joint> Joints, Sphere Extent)
    {
        const Bool   Shaped = Clip.Blend == Interpolation::Hermite;
        const UInt32 Count  = Clip.Motions.IsEmpty() ? 0 : static_cast<UInt32>(Clip.Motions[0].Position.GetSize());

        // Measured up front, since a long clip over a dense rig runs to megabytes the writer would otherwise grow into.
        const UInt Sample = (2 * sizeof(Vector3) + sizeof(Quaternion)) * (Shaped ? 3 : 1);

        // Resampling inherits error from how far the model spans, so a still position is judged against its radius.
        const Real32 Threshold = Max(1e-6f, 1e-5f * Extent.GetRadius());

        Writer Output(static_cast<UInt32>(64 + Clip.Motions.GetSize() * (32 + Count * Sample)));
        Output.Write<UInt32>(kAnimationMagic);
        Output.Write<UInt16>(kAnimationVersion);
        Output.Write<Real64>(Clip.Duration);
        Output.Write<Sphere>(Extent);
        Output.Write<UInt32>(static_cast<UInt32>(Clip.Motions.GetSize()));

        for (ConstRef<Scene::Motion> Entry : Clip.Motions)
        {
            // Every lane drives all three components, and one that holds still says so once.
            const Bool Still    = Steady(Entry.Position, Threshold);
            const Bool Constant = Steady(Entry.Scale, 1e-5f);
            const Bool Fixed    = Steady(Entry.Rotation);

            Output.Write<UInt64>(Scene::Identify(Joints[Entry.Joint].Name));
            Output.Write<Real32>(Clip.Rate);
            Output.Write<UInt32>(Count);
            Output.Write<UInt8>(kPosition | kScale | kRotation);
            Output.Write<UInt8>((Still ? kPosition : 0) | (Constant ? kScale : 0) | (Fixed ? kRotation : 0));
            Output.Write(Clip.Blend);

            Emit<Vector3>(Output, Entry.Position, Entry.PositionRate, Still, Shaped);
            Emit<Vector3>(Output, Entry.Scale, Entry.ScaleRate, Constant, Shaped);
            Emit<Quaternion>(Output, Entry.Rotation, Entry.RotationRate, Fixed, Shaped);
        }
        return Output.Detach();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Exporter::ExportModel(Text Mesh, Text Skeleton, ConstSpan<Scene::Material> Materials)
    {
        JsonValue Root;
        Root.SetObject();

        JsonObject Top(Root);
        Top.SetString("Mesh", Mesh);

        // Named only when the geometry deforms, since the loader reads a missing skeleton as static geometry.
        if (!Skeleton.IsEmpty())
        {
            Top.SetString("Skeleton", Skeleton);
        }

        JsonArray Table = Top.SetArray("Materials");

        for (ConstRef<Scene::Material> Material : Materials)
        {
            JsonObject Entry  = Table.AddObject();
            JsonObject Images = Entry.SetObject("Images");

            for (const Scene::Slot Slot : ZyEnum::GetValues<Scene::Slot>())
            {
                if (ConstRef<Str> Path = Material.Images[ZyEnum::Cast(Slot)]; !Path.IsEmpty())
                {
                    Images.SetObject(ZyEnum::GetName(Slot)).SetString("Path", Path);
                }
            }

            // Only a material that names an image has anything to sample.
            if (!Images.IsEmpty())
            {
                JsonObject Sampler = Entry.SetObject("Samplers").SetObject(kSampler);
                Sampler.SetString("AddressModeU", "Repeat");
                Sampler.SetString("AddressModeV", "Repeat");
                Sampler.SetString("AddressModeW", "Repeat");
                Sampler.SetString("Filter",       "Linear");
            }

            // Constants take the very names their images do, so a shader reads `Albedo` either way.
            JsonObject Parameters;

            for (const Scene::Slot Slot : ZyEnum::GetValues<Scene::Slot>())
            {
                const Constant Kind = Classify(Slot);

                if (Kind == Constant::None || !Material.Declared[ZyEnum::Cast(Slot)])
                {
                    continue;
                }

                if (!Parameters.IsValid())
                {
                    Parameters = Entry.SetObject("Parameters");
                }

                ConstRef<Color> Factor    = Material.Factors[ZyEnum::Cast(Slot)];
                JsonObject      Parameter = Parameters.SetObject(ZyEnum::GetName(Slot));

                if (Kind == Constant::Tint)
                {
                    Parameter.SetEnum("Type", ZyGraphic::Uniform::Color);

                    JsonArray Value = Parameter.SetArray("Value");
                    Value.AddNumber(Factor.GetRed());
                    Value.AddNumber(Factor.GetGreen());
                    Value.AddNumber(Factor.GetBlue());
                    Value.AddNumber(Factor.GetAlpha());
                }
                else
                {
                    Parameter.SetEnum("Type", ZyGraphic::Uniform::Float);
                    Parameter.SetNumber("Value", Factor.GetRed());
                }
            }
        }
        return JsonDocument::Dump(Root, "    ");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Exporter::Constant Exporter::Classify(Scene::Slot Slot)
    {
        // A normal map's strength is left out, since a scene reports one a shader never meant to state.
        switch (Slot)
        {
        case Scene::Slot::Albedo:
        case Scene::Slot::Specular:
        case Scene::Slot::Emissive:
            return Constant::Tint;
        case Scene::Slot::Roughness:
        case Scene::Slot::Metallic:
        case Scene::Slot::Opacity:
            return Constant::Scalar;
        default:
            return Constant::None;
        }
    }
}