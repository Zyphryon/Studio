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

#include "Studio.Application/Outcome.hpp"
#include <Studio.Texture/Atlas/Composer.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    /// \brief Provides the texture commands: bake, pack, build, normal, relief, extract and formats.
    class TextureDomain final
    {
    public:

        /// \brief Constructs the domain with every importer the build enables.
        ///
        /// \param Scheduler The job pool, which must outlive the domain.
        explicit TextureDomain(Ref<ZyJob::Service> Scheduler);

        /// \brief Checks whether a command belongs to this domain.
        ///
        /// \param Command The command name.
        /// \return `true` if this domain runs it, otherwise `false`.
        Bool Claims(Text Command) const;

        /// \brief Checks whether this domain bakes a source, which is how `bake` finds its domain.
        ///
        /// \param Source The source path.
        /// \return `true` if this domain reads it, otherwise `false`.
        Bool Reads(Text Source) const;

        /// \brief Runs one of this domain's commands.
        ///
        /// \param Command The command name.
        /// \param Parsed  The parsed command line.
        /// \return How the command ended.
        Outcome Run(Text Command, ConstRef<Environment> Parsed) const;

        /// \brief Prints the commands and switches of this domain.
        void Describe() const;

    private:

        /// \brief Represents one command: the name it is typed as and the member that runs it.
        struct Entry final
        {
            /// The name the command is typed as.
            Text Name;

            /// The member that runs the command.
            Outcome (TextureDomain::* Handler)(ConstRef<Environment>) const;
        };

        /// \brief Finds the command typed under a name.
        ///
        /// \param Name The command name.
        /// \return The command, or `nullptr` if there is none.
        static ConstPtr<Entry> Find(Text Name);

        /// \brief Bakes one source image into a native texture.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Bake(ConstRef<Environment> Parsed) const;

        /// \brief Packs images and folders of images into an atlas or an array, and writes its tracker.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Pack(ConstRef<Environment> Parsed) const;

        /// \brief Draws the texture a tracker describes again, after its regions were edited.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Build(ConstRef<Environment> Parsed) const;

        /// \brief Draws a normal map out of an image read as a height.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Normal(ConstRef<Environment> Parsed) const;

        /// \brief Draws a relief map out of an image read as a height.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Relief(ConstRef<Environment> Parsed) const;

        /// \brief Writes every slice of a texture out as a picture.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Extract(ConstRef<Environment> Parsed) const;

        /// \brief Lists every texture format a bake can write.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Formats(ConstRef<Environment> Parsed) const;

        /// \brief Adds a file to the sources, or the images of a folder in name order.
        ///
        /// \param Operand The file or folder.
        /// \param Skip    The file to leave out.
        /// \param Output  Receives the sources.
        void Collect(Text Operand, Text Skip, Ref<Sequence<Str>> Output) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Texture::Baker mBaker;
    };
}