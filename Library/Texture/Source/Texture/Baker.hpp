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

#include "Texture/Export/Exporter.hpp"
#include "Texture/Import/Importer.hpp"
#include <Zyphryon.Job/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Bakes source images into the engine's native texture format.
    class Baker final
    {
    public:

        /// \brief Constructs a baker.
        ///
        /// \param Scheduler The pool to bake on, which must outlive the baker.
        explicit Baker(Ref<ZyJob::Service> Scheduler);

        /// \brief Reads and decodes a source image on disk.
        ///
        /// \param Path    The source path, whose extension selects the importer.
        /// \param Profile The settings the source is decoded with.
        /// \return The decoded surface, or an invalid one on failure.
        Surface Load(Text Path, ConstRef<Profile> Profile) const;

        /// \brief Encodes decoded bitmaps into the engine's native texture format.
        ///
        /// \param Slices  The bitmaps to write, one per slice or face.
        /// \param Layout  The layout the slices compose.
        /// \param Profile The settings to write with.
        /// \return The texture bytes, or an empty blob on failure.
        Blob Encode(AnyRef<Sequence<Bitmap>> Slices, ZyGraphic::TextureLayout Layout, ConstRef<Profile> Profile) const;

        /// \brief Bakes an encoded source image held in memory.
        ///
        /// \param Source  The encoded source bytes.
        /// \param Type    The source extension, which selects the importer.
        /// \param Profile The settings controlling the bake.
        /// \return The texture bytes, or an empty blob on failure.
        Blob Bake(ConstSpan<Byte> Source, Text Type, ConstRef<Profile> Profile) const;

        /// \brief Bakes a source image on disk and writes the texture, creating its folder.
        ///
        /// \param Source      The source path, whose extension selects the importer.
        /// \param Destination The texture to write.
        /// \param Profile     The settings controlling the bake.
        /// \return `true` if the texture was written, otherwise `false`.
        Bool Bake(Text Source, Text Destination, ConstRef<Profile> Profile) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<ZyJob::Service> mScheduler;
    };
}