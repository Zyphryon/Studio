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

#include "Studio.Texture/Export/Exporter.hpp"
#include "Studio.Texture/Import/Importer.hpp"
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

        /// \brief The key an importer is registered under: a lowercase extension without its dot.
        using Extension = Str16;

        /// \brief Maps every accepted extension to the importer that claims it.
        using Registry  = Table<Extension, Retainer<Importer>>;

    public:

        /// \brief Constructs a baker with every importer the build enables already registered.
        ///
        /// \param Scheduler The pool to bake on, which must outlive the baker.
        explicit Baker(Ref<ZyJob::Service> Scheduler);

        /// \brief Registers an importer under each extension it accepts.
        ///
        /// \param Codec The importer to register.
        void Register(ConstRetainer<Importer> Codec);

        /// \brief Unregisters whichever importer claims an extension.
        ///
        /// \param Type The source extension, with or without its leading dot.
        /// \return `true` if an importer was unregistered, otherwise `false`.
        Bool Unregister(Text Type);

        /// \brief Finds the importer that claims a source extension.
        ///
        /// \param Type The source extension, with or without its leading dot.
        /// \return The importer, or `nullptr` if none claims it.
        ConstPtr<Importer> Find(Text Type) const;

        /// \brief Gets every extension this baker accepts, keyed to the importer that claims it.
        ///
        /// \return The importer registry.
        ZY_INLINE ConstRef<Registry> GetRegistry() const
        {
            return mRegistry;
        }

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
        Registry            mRegistry;
    };
}