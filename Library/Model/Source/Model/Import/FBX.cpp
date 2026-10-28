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

#include "FBX.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene FBX::Read(Ref<ZyJob::Service> Scheduler, ConstSpan<Byte> Source, ConstRef<Profile> Profile)
    {
        // Read right-handed and Y-up, then mirrored across Z by hand, so the mirror is the one place axes change.
        ufbx_load_opts Options { };
        Options.target_axes              = ufbx_axes_right_handed_y_up;
        Options.space_conversion         = UFBX_SPACE_CONVERSION_TRANSFORM_ROOT;
        Options.generate_missing_normals = Profile.Normals;
        Options.target_unit_meters       = Profile.Units;

        ufbx_error Error;

        const Ptr<ufbx_scene> Native
            = ufbx_load_memory(Source.GetData(), Source.GetSize(), AddressOf(Options), AddressOf(Error));

        if (Native == nullptr)
        {
            LOG_E("Model: the source is not a readable FBX scene ({0})", Wrap(Error.description));

            return Scene();
        }

        Scene Result = Convert(Scheduler, * Native, Profile);
        ufbx_free_scene(Native);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene FBX::Convert(Ref<ZyJob::Service> Scheduler, ConstRef<ufbx_scene> Native, ConstRef<Profile> Profile)
    {
        Scene Result;

        Sequence<SInt32> Lookup;
        Result.Joints = Build(Native, Profile, Lookup);

        for (UInt Index = 0; !Result.Joints.IsEmpty() && Index < Native.anim_stacks.count; ++Index)
        {
            if (ConstRef<ufbx_anim_stack> Stack = * Native.anim_stacks.data[Index]; Stack.time_end > Stack.time_begin)
            {
                Result.Clips.Append(Resample(Scheduler, Native, Result.Joints, Lookup, Profile, Stack));
            }
        }

        // An attribute is read only when it is asked for and at least one mesh carries it.
        Result.Attributes = Scene::Attribute::Position;

        for (UInt Index = 0; Index < Native.meshes.count; ++Index)
        {
            ConstRef<ufbx_mesh> Mesh = * Native.meshes.data[Index];

            const auto Mark = [&](Bool Carried, ZyGraphic::VertexSlot Slot, Scene::Attribute Value)
            {
                Result.Attributes |= Carried && Profile.Wants(Slot) ? Value : Scene::Attribute::None;
            };

            const Bool Tangents = Mesh.vertex_tangent.exists && Mesh.vertex_uv.exists;
            Mark(Mesh.vertex_normal.exists, ZyGraphic::VertexSlot::Normal,    Scene::Attribute::Normal);
            Mark(Mesh.vertex_uv.exists,     ZyGraphic::VertexSlot::TexCoord0, Scene::Attribute::TexCoord0);
            Mark(Mesh.uv_sets.count > 1,    ZyGraphic::VertexSlot::TexCoord1, Scene::Attribute::TexCoord1);
            Mark(Mesh.vertex_color.exists,  ZyGraphic::VertexSlot::Color0,    Scene::Attribute::Color);
            Mark(Tangents,                  ZyGraphic::VertexSlot::Tangent,   Scene::Attribute::Tangent);
        }

        if (!Result.Joints.IsEmpty())
        {
            Result.Attributes |= Scene::Attribute::Skin;
        }

        // Every shading model the scene may carry arrives already translated into the same physically based maps.
        if (Native.materials.count > Scene::kMaxMaterials)
        {
            LOG_W("Model: the scene carries {0} materials and only the first {1} can be addressed",
                Native.materials.count, Scene::kMaxMaterials);
        }

        for (UInt Index = 0; Index < Min<UInt>(Native.materials.count, Scene::kMaxMaterials); ++Index)
        {
            ConstRef<ufbx_material> Source = * Native.materials.data[Index];

            Ref<Scene::Material> Entry = Result.Materials.Append();
            Entry.Name = Wrap(Source.name);
            Attach(Entry, Scene::Slot::Albedo,       Source.pbr.base_color);
            Attach(Entry, Scene::Slot::Normal,       Source.pbr.normal_map);
            Attach(Entry, Scene::Slot::Roughness,    Source.pbr.roughness);
            Attach(Entry, Scene::Slot::Metallic,     Source.pbr.metalness);
            Attach(Entry, Scene::Slot::Specular,     Source.pbr.specular_color);
            Attach(Entry, Scene::Slot::Emissive,     Source.pbr.emission_color);
            Attach(Entry, Scene::Slot::Displacement, Source.pbr.displacement_map);
            Attach(Entry, Scene::Slot::Occlusion,    Source.pbr.ambient_occlusion);
            Attach(Entry, Scene::Slot::Opacity,      Source.pbr.opacity);
        }

        UInt Widest = 3;

        for (UInt Index = 0; Index < Native.meshes.count; ++Index)
        {
            Widest = Max<UInt>(Widest, Native.meshes.data[Index]->max_face_triangles * 3);
        }

        Sequence<UInt32> Fan;
        Fan.Resize(Widest);

        // The triangles of every material, gathered in one walk and laid end to end, one run per primitive.
        Sequence<Sequence<UInt32>> Runs;
        Runs.Resize(Max<UInt>(Result.Materials.GetSize(), 1));

        for (UInt Index = 0; Index < Native.nodes.count; ++Index)
        {
            ConstPtr<ufbx_node> Node = Native.nodes.data[Index];

            if (Node->mesh == nullptr || Node->is_root)
            {
                continue;
            }

            // Instances are flattened into world space, each copy placed where the scene puts it.
            ConstRef<ufbx_mesh> Mesh     = * Node->mesh;
            const ufbx_matrix   Geometry = Node->geometry_to_world;
            const ufbx_matrix   Rotation = ufbx_matrix_for_normals(AddressOf(Geometry));

            // Only the first skin binding is honoured, since a vertex carries one set of bones.
            ConstPtr<ufbx_skin_deformer> Skin
                = Result.Joints.IsEmpty() || Mesh.skin_deformers.count == 0 ? nullptr : Mesh.skin_deformers.data[0];

            for (UInt Face = 0; Face < Mesh.faces.count; ++Face)
            {
                const UInt Owner = Result.Materials.IsEmpty() || Mesh.face_material.count <= Face
                    ? 0
                    : Mesh.face_material.data[Face];

                // A face past the materials that can be addressed has no primitive to draw it.
                if (Owner >= Runs.GetSize())
                {
                    continue;
                }

                // The source winds its fronts counter-clockwise, which the mirror turns into the engine's.
                const UInt32 Count
                    = ufbx_triangulate_face(Fan.GetData(), Fan.GetSize(), AddressOf(Mesh), Mesh.faces.data[Face]);

                for (UInt Corner = 0; Corner < Count * 3; ++Corner)
                {
                    Runs[Owner].Append(static_cast<UInt32>(Result.Vertices.GetSize()));

                    Ref<Scene::Vertex> Vertex = Result.Vertices.Append(
                        Resolve(Mesh, Fan[Corner], Geometry, Rotation, Profile, Result.Attributes));

                    if (!Result.Joints.IsEmpty())
                    {
                        Bind(Vertex, Skin, Mesh.vertex_indices.data[Fan[Corner]], Lookup);
                    }
                }
            }
        }

        for (UInt Slot = 0; Slot < Runs.GetSize(); ++Slot)
        {
            if (Runs[Slot].IsEmpty())
            {
                continue;
            }

            Ref<Scene::Surface> Entry = Result.Surfaces.Append();
            Entry.Offset   = static_cast<UInt32>(Result.Indices.GetSize());
            Entry.Count    = static_cast<UInt32>(Runs[Slot].GetSize());
            Entry.Material = static_cast<UInt8>(Slot);

            Result.Indices.Append(Runs[Slot]);
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sequence<Scene::Joint> FBX::Build(
        ConstRef<ufbx_scene> Native, ConstRef<Profile> Profile, Ref<Sequence<SInt32>> Lookup)
    {
        Sequence<Scene::Joint> Joints;
        Sequence<ufbx_matrix>  Worlds;

        Lookup.Fill(-1, Native.nodes.count);

        // Bones that carry weight, not every bone a cluster names, so controls and helpers stay out of the rig. Each is
        // measured from where it was bound rather than where the scene parks it, since rigs are saved in control poses.
        Sequence<ConstPtr<ufbx_matrix>> Binding;
        Binding.Fill(nullptr, Native.nodes.count);

        for (UInt Index = 0; Profile.Rig && Index < Native.skin_deformers.count; ++Index)
        {
            ConstRef<ufbx_skin_deformer> Skin = * Native.skin_deformers.data[Index];

            for (UInt Vertex = 0; Vertex < Skin.vertices.count; ++Vertex)
            {
                const ufbx_skin_vertex Entry = Skin.vertices.data[Vertex];

                for (UInt Slot = 0; Slot < Min<UInt>(Entry.num_weights, Scene::kInfluences); ++Slot)
                {
                    const ufbx_skin_weight      Weight  = Skin.weights.data[Entry.weight_begin + Slot];
                    ConstPtr<ufbx_skin_cluster> Cluster = Skin.clusters.data[Weight.cluster_index];

                    if (Weight.weight > Scene::kMinWeight && Cluster->bone_node && !Cluster->bone_node->is_root)
                    {
                        Binding[Cluster->bone_node->typed_id] = AddressOf(Cluster->bind_to_world);
                    }
                }
            }
        }

        // ufbx lists every parent ahead of its children, which is the order the engine composes in.
        for (UInt Index = 0; Index < Native.nodes.count; ++Index)
        {
            ConstRef<ufbx_node> Node = * Native.nodes.data[Index];

            if (Binding[Node.typed_id] == nullptr)
            {
                continue;
            }

            if (Joints.GetSize() >= Scene::kMaxJoints)
            {
                LOG_W("Model: the rig carries more than {0} bones and the rest are dropped", Scene::kMaxJoints);
                break;
            }

            const ufbx_matrix World = Place(* Binding[Node.typed_id], Profile.Scale);

            Ref<Scene::Joint> Joint = Joints.Append();
            Joint.Name = Wrap(Node.name);

            // The nearest ancestor that is itself a bone, which skips the helper nodes a rig hangs its bones off.
            for (ConstPtr<ufbx_node> Walk = Node.parent; Walk && Joint.Parent < 0; Walk = Walk->parent)
            {
                Joint.Parent = static_cast<SInt16>(Lookup[Walk->typed_id]);
            }

            if (Joint.Parent >= 0)
            {
                const ufbx_matrix Invert = ufbx_matrix_invert(AddressOf(Worlds[Joint.Parent]));
                Decompose(ufbx_matrix_mul(AddressOf(Invert), AddressOf(World)), Joint);
            }
            else
            {
                Decompose(World, Joint);
            }

            // The bind pose undone, so composing the rest pose and folding this in lands exactly on identity.
            const ufbx_matrix Inverse = ufbx_matrix_invert(AddressOf(World));
            Joint.Inverse = Adopt(Inverse);

            Worlds.Append(World);
            Lookup[Node.typed_id] = static_cast<SInt32>(Joints.GetSize() - 1);
        }
        return Joints;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void FBX::Bind(Ref<Scene::Vertex> Vertex, ConstPtr<ufbx_skin_deformer> Skin, UInt32 Index, ConstSpan<SInt32> Lookup)
    {
        Real32 Total = 0.0f;
        UInt   Slot  = 0;

        // ufbx lists a vertex's weights heaviest first, so the ones that matter are the first few.
        if (Skin != nullptr && Index < Skin->vertices.count)
        {
            const ufbx_skin_vertex Entry = Skin->vertices.data[Index];

            for (UInt Taken = 0; Taken < Min<UInt>(Entry.num_weights, Scene::kInfluences); ++Taken)
            {
                const ufbx_skin_weight    Weight = Skin->weights.data[Entry.weight_begin + Taken];
                const ConstPtr<ufbx_node> Bone   = Skin->clusters.data[Weight.cluster_index]->bone_node;

                if (Bone && Lookup[Bone->typed_id] >= 0)
                {
                    Vertex.Joints[Slot]  = static_cast<UInt16>(Lookup[Bone->typed_id]);
                    Vertex.Weights[Slot] = static_cast<Real32>(Weight.weight);
                    Total += Vertex.Weights[Slot];
                    ++Slot;
                }
            }
        }

        // Dropping the lighter influences leaves the rest short of one, which would shrink the vertex to the origin.
        if (Total > 0.0f)
        {
            for (Ref<Real32> Weight : Vertex.Weights)
            {
                Weight /= Total;
            }
        }
        else
        {
            // A vertex no skin reaches follows the root, the one bone every rig has.
            Vertex.Joints[0]  = 0;
            Vertex.Weights[0] = 1.0f;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Clip FBX::Resample(
        Ref<ZyJob::Service>       Scheduler,
        ConstRef<ufbx_scene>      Native,
        ConstSpan<Scene::Joint>   Joints,
        ConstSpan<SInt32>         Lookup,
        ConstRef<Profile>         Profile,
        ConstRef<ufbx_anim_stack> Stack)
    {
        const Real64 Begin  = Stack.time_begin;
        const Real64 Length = Stack.time_end - Stack.time_begin;
        const Bool   Shaped = Profile.Blend == Interpolation::Hermite;
        const UInt32 Count  = static_cast<UInt32>(Length * Profile.Rate) + 1;

        // Only the bones and their ancestors are evaluated, never the meshes, controls and helpers around them.
        Sequence<UInt32>              Owners;
        Sequence<Bool>                Reached;
        Sequence<ConstPtr<ufbx_node>> Chain;
        Owners.Fill(0, Joints.GetSize());
        Reached.Fill(false, Native.nodes.count);

        for (UInt Index = 0; Index < Native.nodes.count; ++Index)
        {
            if (Lookup[Index] < 0)
            {
                continue;
            }

            Owners[Lookup[Index]] = static_cast<UInt32>(Index);

            ConstPtr<ufbx_node> Walk = Native.nodes.data[Index];

            for (; Walk && !Reached[Walk->typed_id]; Walk = Walk->parent)
            {
                Reached[Walk->typed_id] = true;
            }
        }

        for (UInt Index = 0; Index < Native.nodes.count; ++Index)
        {
            if (Reached[Index])
            {
                Chain.Append(Native.nodes.data[Index]);
            }
        }

        Scene::Clip Clip;
        Clip.Name     = Wrap(Stack.name);
        Clip.Duration = Length;
        Clip.Rate     = Profile.Rate;
        Clip.Blend    = Profile.Blend;

        for (UInt Joint = 0; Joint < Joints.GetSize(); ++Joint)
        {
            Ref<Scene::Motion> Motion = Clip.Motions.Append();
            Motion.Joint = static_cast<UInt16>(Joint);
            Motion.Position.Resize(Count);
            Motion.Scale.Resize(Count);
            Motion.Rotation.Resize(Count);

            if (Shaped)
            {
                Motion.PositionRate.Resize(Count);
                Motion.ScaleRate.Resize(Count);
                Motion.RotationRate.Resize(Count);
            }
        }

        // A tick reads the source and nothing its neighbours produced, so the whole take is one range to spread.
        Scheduler.Parallel(ZyJob::Lane::Compute, Count, [&](UInt32 Start, UInt32 End)
        {
            Sequence<ufbx_matrix>  World;
            Sequence<ufbx_matrix>  Bones;
            Sequence<ufbx_matrix>  Inverse;
            Sequence<Scene::Joint> Current;
            Sequence<Scene::Joint> Before;
            Sequence<Scene::Joint> After;
            World.Resize(Native.nodes.count);
            Bones.Resize(Joints.GetSize());
            Inverse.Resize(Joints.GetSize());
            Current.Resize(Joints.GetSize());
            Before.Resize(Shaped ? Joints.GetSize() : 0);
            After.Resize(Shaped ? Joints.GetSize() : 0);

            // A bone is measured against its nearest kept ancestor, so the whole chain is composed at every moment.
            const auto Evaluate = [&](Real64 Time, Ref<Sequence<Scene::Joint>> Pose)
            {
                for (ConstPtr<ufbx_node> Node : Chain)
                {
                    const ufbx_transform Local = ufbx_evaluate_transform(Stack.anim, Node, Time);
                    const ufbx_matrix    Delta = ufbx_transform_to_matrix(AddressOf(Local));

                    World[Node->typed_id] = Node->parent
                        ? ufbx_matrix_mul(AddressOf(World[Node->parent->typed_id]), AddressOf(Delta))
                        : Delta;
                }

                for (UInt Joint = 0; Joint < Joints.GetSize(); ++Joint)
                {
                    Bones[Joint]   = Place(World[Owners[Joint]], Profile.Scale);
                    Inverse[Joint] = ufbx_matrix_invert(AddressOf(Bones[Joint]));
                }

                for (UInt Joint = 0; Joint < Joints.GetSize(); ++Joint)
                {
                    if (const SInt16 Parent = Joints[Joint].Parent; Parent >= 0)
                    {
                        Decompose(ufbx_matrix_mul(AddressOf(Inverse[Parent]), AddressOf(Bones[Joint])), Pose[Joint]);
                    }
                    else
                    {
                        Decompose(Bones[Joint], Pose[Joint]);
                    }
                }
            };

            for (UInt32 Tick = Start; Tick < End; ++Tick)
            {
                const Real64 Time = Begin + Min(static_cast<Real64>(Tick) / Profile.Rate, Length);

                Evaluate(Time, Current);

                for (UInt Joint = 0; Joint < Joints.GetSize(); ++Joint)
                {
                    Ref<Scene::Motion> Motion = Clip.Motions[Joint];
                    Motion.Position[Tick] = Current[Joint].Position;
                    Motion.Scale[Tick]    = Current[Joint].Scale;
                    Motion.Rotation[Tick] = Current[Joint].Rotation;
                }

                if (!Shaped)
                {
                    continue;
                }

                // Straddling the tick, so a key held exactly on it gives the curve's slope and not one arm of a corner.
                const Real64 Step  = kDerivative / Profile.Rate;
                const Real64 Lower = Max(Begin, Time - Step);
                const Real64 Upper = Min(Begin + Length, Time + Step);

                Evaluate(Lower, Before);
                Evaluate(Upper, After);

                const Real32 Divisor = Upper > Lower ? static_cast<Real32>(1.0 / (Upper - Lower)) : 0.0f;

                for (UInt Joint = 0; Joint < Joints.GetSize(); ++Joint)
                {
                    Ref<Scene::Motion> Motion = Clip.Motions[Joint];
                    Motion.PositionRate[Tick] = (After[Joint].Position - Before[Joint].Position) * Divisor;
                    Motion.ScaleRate[Tick]    = (After[Joint].Scale - Before[Joint].Scale) * Divisor;

                    // Both ends taken on the sample's hemisphere, or a slope across a flip reads twice as steep.
                    ConstRef<Quaternion> Anchor = Current[Joint].Rotation;
                    const Quaternion     From   = Quaternion::Dot(Anchor, Before[Joint].Rotation) < 0.0f
                        ? -Before[Joint].Rotation
                        :  Before[Joint].Rotation;
                    const Quaternion     To     = Quaternion::Dot(Anchor, After[Joint].Rotation) < 0.0f
                        ? -After[Joint].Rotation
                        :  After[Joint].Rotation;
                    Motion.RotationRate[Tick] = (To - From) * Divisor;
                }
            }
        });

        // A Hermite segment blends its ends as plain components, so consecutive samples have to share a hemisphere.
        if (Shaped)
        {
            for (Ref<Scene::Motion> Motion : Clip.Motions)
            {
                for (UInt32 Tick = 1; Tick < Count; ++Tick)
                {
                    if (Quaternion::Dot(Motion.Rotation[Tick - 1], Motion.Rotation[Tick]) < 0.0f)
                    {
                        Motion.Rotation[Tick]     = -Motion.Rotation[Tick];
                        Motion.RotationRate[Tick] = -Motion.RotationRate[Tick];
                    }
                }
            }
        }
        return Clip;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Vertex FBX::Resolve(
        ConstRef<ufbx_mesh>   Mesh,
        UInt32                Corner,
        ConstRef<ufbx_matrix> Geometry,
        ConstRef<ufbx_matrix> Rotation,
        ConstRef<Profile>     Profile,
        Scene::Attribute      Attributes)
    {
        // Value-initialized, so an attribute this mesh leaves undefined reads alike in every vertex and still welds.
        Scene::Vertex Result { };

        const ufbx_vec3 Position = ufbx_transform_position(
            AddressOf(Geometry), ufbx_get_vertex_vec3(AddressOf(Mesh.vertex_position), Corner));
        Result.Position = Vector3(
            static_cast<Real32>(Position.x), static_cast<Real32>(Position.y), static_cast<Real32>(-Position.z))
            * Profile.Scale;

        if (HasBit(Attributes, Scene::Attribute::Normal) && Mesh.vertex_normal.exists)
        {
            Result.Normal = Orient(Rotation, ufbx_get_vertex_vec3(AddressOf(Mesh.vertex_normal), Corner));
        }

        if (HasBit(Attributes, Scene::Attribute::Tangent) && Mesh.vertex_tangent.exists)
        {
            Result.Tangent = Orient(Rotation, ufbx_get_vertex_vec3(AddressOf(Mesh.vertex_tangent), Corner));

            // Only the side the bitangent falls on is kept, turned over since V runs the other way once flipped.
            if (Mesh.vertex_bitangent.exists)
            {
                const Vector3 Bitangent
                    = Orient(Rotation, ufbx_get_vertex_vec3(AddressOf(Mesh.vertex_bitangent), Corner));
                const Vector3 Expected = Vector3::Cross(Result.Normal, Result.Tangent);
                Result.Handedness = Vector3::Dot(Expected, Bitangent) < 0.0f ? 1.0f : -1.0f;
            }
        }

        // FBX runs V up the image, and the engine runs it down.
        if (HasBit(Attributes, Scene::Attribute::TexCoord0) && Mesh.vertex_uv.exists)
        {
            const ufbx_vec2 Coordinate = ufbx_get_vertex_vec2(AddressOf(Mesh.vertex_uv), Corner);
            Result.TexCoord0 = Vector2(static_cast<Real32>(Coordinate.x), static_cast<Real32>(1.0 - Coordinate.y));
        }

        if (HasBit(Attributes, Scene::Attribute::TexCoord1) && Mesh.uv_sets.count > 1)
        {
            const ufbx_vec2 Coordinate = ufbx_get_vertex_vec2(AddressOf(Mesh.uv_sets.data[1].vertex_uv), Corner);
            Result.TexCoord1 = Vector2(static_cast<Real32>(Coordinate.x), static_cast<Real32>(1.0 - Coordinate.y));
        }

        if (HasBit(Attributes, Scene::Attribute::Color) && Mesh.vertex_color.exists)
        {
            const ufbx_vec4 Tint = ufbx_get_vertex_vec4(AddressOf(Mesh.vertex_color), Corner);
            Result.Color = Color(
                static_cast<Real32>(Tint.x), static_cast<Real32>(Tint.y),
                static_cast<Real32>(Tint.z), static_cast<Real32>(Tint.w)).ToColor8();
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void FBX::Attach(Ref<Scene::Material> Output, Scene::Slot Slot, ConstRef<ufbx_material_map> Map)
    {
        const UInt Index = ZyEnum::Cast(Slot);

        // ufbx fills all four components, widening a scalar and giving a colour without alpha an opaque one.
        if (Map.has_value)
        {
            const ufbx_vec4 Value = Map.value_vec4;
            Output.Factors[Index]  = Color(Value.x, Value.y, Value.z, Value.w);
            Output.Declared[Index] = true;
        }

        if (Map.texture == nullptr || !Map.texture_enabled)
        {
            return;
        }

        // The relative path is preferred, since a scene may or may not have been saved beside its textures.
        const Text Path = Map.texture->relative_filename.length > 0
            ? Wrap(Map.texture->relative_filename)
            : Wrap(Map.texture->filename);

        if (!Path.IsEmpty())
        {
            Output.Images[Index] = Scene::Retarget(Path);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Vector3 FBX::Orient(ConstRef<ufbx_matrix> Basis, ConstRef<ufbx_vec3> Direction)
    {
        const ufbx_vec3 Rotated = ufbx_transform_direction(AddressOf(Basis), Direction);
        const Vector3   Result(
            static_cast<Real32>(Rotated.x),
            static_cast<Real32>(Rotated.y),
            static_cast<Real32>(-Rotated.z));

        // A degenerate face leaves a zero-length direction, which normalizing would turn into NaNs.
        const Real32 Length = Result.GetLength();
        return IsAlmostZero(Length) ? Vector3(0.0f, 0.0f, 0.0f) : Result * (1.0f / Length);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ufbx_matrix FBX::Place(ConstRef<ufbx_matrix> World, Real32 Scale)
    {
        ufbx_matrix Result = World;

        // Mirroring both sides flips every element in the Z row or the Z column, but not in both.
        for (UInt Column = 0; Column < 4; ++Column)
        {
            for (UInt Row = 0; Row < 3; ++Row)
            {
                const Real64 Sign = (Row == 2) != (Column == 2) ? -1.0 : 1.0;
                Result.cols[Column].v[Row] *= Sign * Scale;
            }
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void FBX::Decompose(ConstRef<ufbx_matrix> Matrix, Ref<Scene::Joint> Joint)
    {
        const ufbx_transform Local = ufbx_matrix_to_transform(AddressOf(Matrix));

        Joint.Position = Vector3(
            static_cast<Real32>(Local.translation.x),
            static_cast<Real32>(Local.translation.y),
            static_cast<Real32>(Local.translation.z));
        Joint.Scale    = Vector3(
            static_cast<Real32>(Local.scale.x),
            static_cast<Real32>(Local.scale.y),
            static_cast<Real32>(Local.scale.z));
        Joint.Rotation = Quaternion(
            static_cast<Real32>(Local.rotation.x),
            static_cast<Real32>(Local.rotation.y),
            static_cast<Real32>(Local.rotation.z),
            static_cast<Real32>(Local.rotation.w));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Matrix4x3 FBX::Adopt(ConstRef<ufbx_matrix> Matrix)
    {
        const auto Row = [&Matrix](UInt Axis)
        {
            return Vector4(
                static_cast<Real32>(Matrix.cols[0].v[Axis]),
                static_cast<Real32>(Matrix.cols[1].v[Axis]),
                static_cast<Real32>(Matrix.cols[2].v[Axis]),
                static_cast<Real32>(Matrix.cols[3].v[Axis]));
        };
        return Matrix4x3(Row(0), Row(1), Row(2));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text FBX::Wrap(ConstRef<ufbx_string> Value)
    {
        return Text(Value.data, Value.length);
    }
}