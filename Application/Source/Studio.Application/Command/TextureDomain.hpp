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
    /// \brief Provides the texture commands: bake, pack, build, normal, relief and extract.
    class TextureDomain final
    {
    public:

        /// \brief Constructs the domain with every importer the build enables.
        ///
        /// \param Scheduler The pool textures are baked on, which must outlive the domain.
        explicit TextureDomain(Ref<ZyJob::Service> Scheduler);

        /// \brief Checks whether a command belongs to this domain.
        ///
        /// \param Command The command name, as typed.
        /// \return `true` if this domain runs the command, otherwise `false`.
        Bool Claims(Text Command) const;

        /// \brief Checks whether this domain bakes a source, which is how `bake` finds its domain.
        ///
        /// \param Source The source path, whose extension decides.
        /// \return `true` if an importer of this domain reads the source, otherwise `false`.
        Bool Reads(Text Source) const;

        /// \brief Runs one of this domain's commands.
        ///
        /// \param Command The command name, which must be one this domain claims.
        /// \param Parsed  The parsed command line, whose first operand is the command itself.
        /// \return How the command ended.
        Outcome Run(Text Command, ConstRef<Environment> Parsed) const;

        /// \brief Prints the commands and switches of this domain.
        void Describe() const;

    private:

        /// \brief Describes one command: the name it is typed as and the member that runs it.
        struct Entry final
        {
            /// The name the command is typed as.
            Text Name;

            /// The member that runs the command.
            Outcome (TextureDomain::* Handler)(ConstRef<Environment>) const;
        };

        /// \brief Finds the command typed under a name.
        ///
        /// \param Name The command name, as typed.
        /// \return The command, or `nullptr` if this domain has none by that name.
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

        /// \brief Writes every slice of a texture out as a picture, for art that goes back to an image editor.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Extract(ConstRef<Environment> Parsed) const;

        /// \brief Draws the tracker's slices and writes both the texture and the tracker.
        ///
        /// \param Drawer   The composer the tracker's sources are read through.
        /// \param Atlas    The placed tracker, whose format and layout are settled here.
        /// \param Output   The tracker file to write.
        /// \param Image    The texture file to write, as a path the process can open.
        /// \param Settings The settings the texture is written with.
        /// \return How the writing ended.
        Outcome Publish(
            Ref<Texture::Composer>     Drawer,
            Ref<Texture::Tracker>      Atlas,
            Text                       Output,
            Text                       Image,
            ConstRef<Texture::Profile> Settings) const;

        /// \brief Adds a file to the sources, or every image directly inside it if it is a folder.
        ///
        /// \param Operand The file or folder, as typed.
        /// \param Skip    The file never to add, which is the texture about to be written.
        /// \param Output  Receives the sources, a folder's in name order.
        void Collect(Text Operand, Text Skip, Ref<Sequence<Str>> Output) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Texture::Baker mBaker;
    };
}