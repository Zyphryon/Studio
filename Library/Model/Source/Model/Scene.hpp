// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include <Zyphryon.Graphic/Types.hpp>
#include <Zyphryon.Math/Geometry/Box.hpp>
#include <Zyphryon.Math/Matrix4x3.hpp>
#include <Zyphryon.Math/Motion/Track.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    /// \brief Represents one decoded scene, triangulated and sharing one index space, as an importer hands it over.
    struct Scene final
    {
        /// \brief Enumerates the vertex attributes a scene can carry.
        enum class Attribute : UInt8
        {
            None      = 0,      ///< Carries no attribute.
            Position  = 1 << 0, ///< Carries the position.
            Normal    = 1 << 1, ///< Carries the normal.
            Tangent   = 1 << 2, ///< Carries the tangent frame.
            TexCoord0 = 1 << 3, ///< Carries the first set of texture coordinates.
            TexCoord1 = 1 << 4, ///< Carries the second set of texture coordinates.
            Color     = 1 << 5, ///< Carries the vertex colour.
            Skin      = 1 << 6, ///< Carries the bones and weights the vertex is deformed by.
        };
        ZY_DEFINE_BITWISE_FRIEND_ENUM(Attribute)

        /// \brief Enumerates the textures a material binds, spelled as the technique declares them.
        enum class Slot : UInt8
        {
            Albedo,       ///< Binds the base colour.
            Normal,       ///< Binds the tangent-space normal map.
            Roughness,    ///< Binds how roughly the surface scatters light.
            Metallic,     ///< Binds how much the surface behaves as a conductor.
            Specular,     ///< Binds the tint a non-metal reflects.
            Emissive,     ///< Binds the light the surface gives off.
            Displacement, ///< Binds the height the surface stands off its geometry by.
            Occlusion,    ///< Binds the ambient light the surface's shape keeps out.
            Opacity,      ///< Binds how much of what is behind the surface comes through.
        };

        /// \brief Represents how one vertex attribute is fed and stored.
        struct Encoding final
        {
            /// The decoded attribute that feeds it.
            Attribute               Source;

            /// The element type it is written as.
            ZyGraphic::VertexFormat Format;

            /// The bytes one element takes.
            UInt16                  Size;
        };

        /// \brief Represents one fully resolved vertex.
        struct Vertex final
        {
            /// The position, in the scene's units.
            Vector3                      Position;

            /// The unit normal.
            Vector3                      Normal;

            /// The unit tangent, pointing along growing `TexCoord0.X`.
            Vector3                      Tangent;

            /// The sign the bitangent is rebuilt with, either -1 or +1.
            Real32                       Handedness = 1.0f;

            /// The first set of texture coordinates.
            Vector2                      TexCoord0;

            /// The second set of texture coordinates.
            Vector2                      TexCoord1;

            /// The vertex colour, white where the source gives none.
            IntColor8                    Color      = IntColor8(255, 255, 255, 255);

            /// The bones deforming the vertex, as indices into the joints.
            Array<UInt16, 4>             Joints     = { };

            /// The share each bone deforms the vertex by, summing to one.
            Array<Real32, 4>             Weights    = { };
        };

        /// \brief Represents one bone of the rig, which always follows the bone it hangs from.
        struct Joint final
        {
            /// The name the source gives the bone.
            Str        Name;

            /// The index of the bone this one hangs from, or -1 for one that hangs from nothing.
            SInt16     Parent   = -1;

            /// The resting position, relative to the parent.
            Vector3    Position;

            /// The resting scale, relative to the parent.
            Vector3    Scale    = Vector3(1.0f, 1.0f, 1.0f);

            /// The resting rotation, relative to the parent.
            Quaternion Rotation;

            /// The matrix taking a vertex from the bind pose into the bone's space.
            Matrix4x3  Inverse  = Matrix4x3::Identity();
        };

        /// \brief Represents one bone's motion within a clip, one sample per tick of the clip's cadence.
        struct Motion final
        {
            /// The index of the bone driven.
            UInt16               Joint = 0;

            /// The position at each tick, relative to the parent.
            Sequence<Vector3>    Position;

            /// The scale at each tick, relative to the parent.
            Sequence<Vector3>    Scale;

            /// The rotation at each tick, relative to the parent.
            Sequence<Quaternion> Rotation;

            /// The slope of the source curve at each position sample per second, filled only for a Hermite clip.
            Sequence<Vector3>    PositionRate;

            /// The slope of the source curve at each scale sample per second, filled only for a Hermite clip.
            Sequence<Vector3>    ScaleRate;

            /// The slope of the source curve at each rotation sample per second, filled only for a Hermite clip.
            Sequence<Quaternion> RotationRate;
        };

        /// \brief Represents one named clip of motion over the rig.
        struct Clip final
        {
            /// The name the source gives the clip.
            Str              Name;

            /// The length, in seconds.
            Real64           Duration = 0.0;

            /// The samples per second every motion was resampled onto.
            Real32           Rate     = 0.0f;

            /// The way the runtime reads between samples, which decides whether the rates are written.
            Interpolation    Blend    = Interpolation::Linear;

            /// The motion of every bone.
            Sequence<Motion> Motions;
        };

        /// \brief Represents one run of the index buffer drawn with a single material.
        struct Surface final
        {
            /// The index of the first entry the run draws.
            UInt32 Offset   = 0;

            /// The number of indices the run draws.
            UInt32 Count    = 0;

            /// The slot in the material table the run draws with.
            UInt8  Material = 0;
        };

        /// \brief Represents one decoded material and the images it references, every table keyed by \ref Slot.
        struct Material final
        {
            /// The name the source gives the material.
            Str                                 Name;

            /// The image of each slot, empty for a slot left unbound.
            Array<Str, ZyEnum::Count<Slot>()>   Images;

            /// The constant each slot is modulated by, a scalar one kept in the red channel.
            Array<Color, ZyEnum::Count<Slot>()> Factors;

            /// The slots whose constant the source states, `true` for each one stated.
            Array<Bool, ZyEnum::Count<Slot>()>  Declared = { };
        };

        /// The attributes the vertices carry.
        Attribute          Attributes = Attribute::None;

        /// The vertices, sharing one index space.
        Sequence<Vertex>   Vertices;

        /// The triangle indices into the vertices.
        Sequence<UInt32>   Indices;

        /// The runs the indices split into, one per material used.
        Sequence<Surface>  Surfaces;

        /// The material table the surfaces index into.
        Sequence<Material> Materials;

        /// The bones with parents first, or empty for geometry that never deforms.
        Sequence<Joint>    Joints;

        /// The clips played over the bones.
        Sequence<Clip>     Clips;

        /// \brief Checks whether the vertices carry an attribute.
        ///
        /// \param Value The attribute to look for.
        /// \return `true` when the scene carries it, otherwise `false`.
        ZY_INLINE Bool Has(Attribute Value) const
        {
            return HasBit(Attributes, Value);
        }

        /// \brief Computes the axis-aligned bounds of every vertex position.
        ///
        /// \return The bounds, or an invalid box for a scene without vertices.
        Box GetBounds() const;

    public:

        /// \brief The most bones that deform one vertex.
        static constexpr UInt   kInfluences   = 4;

        /// \brief The most bones a rig may carry, as many as a bone's parent link can name.
        static constexpr UInt   kMaxJoints    = 32767;

        /// \brief The most materials a mesh can address, since a primitive names its slot in a byte.
        static constexpr UInt   kMaxMaterials = 255;

        /// \brief The smallest weight that binds a vertex to a bone, below what the vertex format can express.
        static constexpr Real32 kMinWeight    = 1e-4f;

        /// \brief The extension every referenced image is rewritten to, the one the texture baker writes.
        static constexpr Text   kTexture      = "tex";

        /// \brief Hashes a bone's name into the value rigs and clips are matched by.
        ///
        /// \param Name The bone's name, as the source spells it.
        /// \return The hash written into both the rig and the clips.
        static UInt64 Identify(Text Name);

        /// \brief Rewrites the path a source records for an image into the baked texture beside the manifest.
        ///
        /// \param Path The path the source records, absolute or relative.
        /// \return The image's file name with the baked texture's extension.
        static Str Retarget(Text Path);

        /// \brief Looks up how a vertex attribute is fed and stored.
        ///
        /// \param Slot The semantic to look up.
        /// \return The encoding, or `nullptr` when the baker cannot produce the attribute.
        static ConstPtr<Encoding> Describe(ZyGraphic::VertexSlot Slot);
    };
}