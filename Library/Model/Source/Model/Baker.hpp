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

#include "Model/Export/Exporter.hpp"
#include "Model/Import/Importer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    /// \brief Bakes source scenes into the engine's native model, mesh, skeleton and animation formats.
    class Baker final
    {
    public:

        /// \brief Represents the native files one bake produces, before they are written to disk.
        struct Bundle final
        {
            /// \brief Represents one baked clip and the name its file is written under.
            struct Motion final
            {
                /// The clip's name, safe to put in a file name.
                Str  Name;

                /// The animation bytes.
                Blob Data;
            };

            /// The mesh bytes, or empty on failure.
            Blob             Mesh;

            /// The skeleton bytes, or empty for a model that never deforms.
            Blob             Skeleton;

            /// The animation of every clip.
            Sequence<Motion> Clips;

            /// The manifest the engine loads, as JSON text.
            Str              Manifest;
        };

    public:

        /// \brief Constructs a baker.
        ///
        /// \param Scheduler The pool clips are resampled and levels reduced on.
        explicit Baker(Ref<ZyJob::Service> Scheduler);

        /// \brief Bakes an encoded source scene held in memory.
        ///
        /// \param Source  The encoded source bytes.
        /// \param Type    The source extension.
        /// \param Name    The file name the mesh and skeleton share beside the manifest, without its extension.
        /// \param Profile The settings to bake with.
        /// \return The native files, whose mesh is empty on failure.
        Bundle Bake(ConstSpan<Byte> Source, Text Type, Text Name, ConstRef<Profile> Profile) const;

        /// \brief Bakes a source scene on disk and writes the model beside its files, creating any missing folder.
        ///
        /// \param Source      The source scene path.
        /// \param Destination The model manifest path, whose name the mesh, skeleton and clips are written under.
        /// \param Profile     The settings to bake with.
        /// \return `true` if every file was written, otherwise `false`.
        Bool Bake(Text Source, Text Destination, ConstRef<Profile> Profile) const;

    private:

        /// \brief Turns a name from the source into one safe to put in a file name.
        ///
        /// \param Value    The name the source gives.
        /// \param Fallback The name used when the value is empty.
        /// \return The name with every space and character a path reserves made an underscore.
        static Str Sanitize(Text Value, Text Fallback);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<ZyJob::Service> mScheduler;
    };
}