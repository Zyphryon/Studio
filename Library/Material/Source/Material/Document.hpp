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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Material
{
    /// \brief Represents the `.mtl` file of a material, with the textures it binds and the values it hands over.
    struct Document final
    {
        /// \brief The file extension a material is written with.
        static constexpr Text kOutput = "mtl";

        /// \brief Represents one texture a material binds, together with the sampler it may be read through.
        struct Binding final
        {
            /// The name the technique declares the texture under.
            Str                       Name;

            /// The texture, relative to the material or a mounted url, empty when the material sets a sampler alone.
            Str                       Path;

            /// `true` when the material sets its own sampler, `false` to keep the technique's and write nothing below.
            Bool                      Sampled    = false;

            /// The filter the sampler reads between texels with.
            ZyGraphic::TextureFilter  Filter     = ZyGraphic::TextureFilter::Point;

            /// The mode the sampler reads past the horizontal edges with.
            ZyGraphic::TextureAddress AddressU   = ZyGraphic::TextureAddress::Clamp;

            /// The mode the sampler reads past the vertical edges with.
            ZyGraphic::TextureAddress AddressV   = ZyGraphic::TextureAddress::Clamp;

            /// The mode the sampler reads past the depth edges with, which only a volume has.
            ZyGraphic::TextureAddress AddressW   = ZyGraphic::TextureAddress::Clamp;

            /// The test a comparison sampler runs, as a shadow map is read.
            ZyGraphic::TestCondition  Comparison = ZyGraphic::TestCondition::None;

            /// The colour read past the edges when an address mode is `Border`.
            ZyGraphic::TextureBorder  Border     = ZyGraphic::TextureBorder::OpaqueBlack;
        };

        /// \brief Represents one value a material hands its technique.
        struct Constant final
        {
            /// The name the technique declares the value under.
            Str                Name;

            /// The kind of value, which decides how many components are written and whether they are whole.
            ZyGraphic::Uniform Type  = ZyGraphic::Uniform::Float;

            /// The components, wide enough to hold every kind exactly, a boolean being true when the first is not zero.
            Array<Real64, 4>   Value = Array<Real64, 4>(0.0, 0.0, 0.0, 0.0);
        };

        /// The textures and samplers, in the order they were authored.
        Sequence<Binding>  Bindings;

        /// The values, in the order they were authored.
        Sequence<Constant> Constants;

        /// \brief Gets the texture bound under a name, adding one bound to nothing if there is none yet.
        ///
        /// \param Name The name the technique declares the texture under.
        /// \return The binding, which stays valid until another is added or removed.
        Ref<Binding> Bind(Text Name);

        /// \brief Gets the value handed over under a name, adding one of zero if there is none yet.
        ///
        /// \param Name The name the technique declares the value under.
        /// \return The value, which stays valid until another is added or removed.
        Ref<Constant> Set(Text Name);

        /// \brief Removes the texture, sampler and value a name stands for.
        ///
        /// \param Name The name the texture, sampler and value to remove stand under.
        /// \return `true` if anything was removed, `false` otherwise.
        Bool Remove(Text Name);

        /// \brief Reads a material from disk.
        ///
        /// \note A colour written as a hexadecimal string is read into its components, and written back as them.
        ///
        /// \param Path   The `.mtl` file to read.
        /// \param Output Receives the material, left untouched if the file cannot be read.
        /// \return `true` if the file was read, `false` otherwise.
        static Bool Read(Text Path, Ref<Document> Output);

        /// \brief Writes the material to disk, in the layout the engine's loader reads.
        ///
        /// \param Path The `.mtl` file to write, whose missing folders are created.
        /// \return `true` if the file was written, `false` otherwise.
        Bool Write(Text Path) const;
    };
}