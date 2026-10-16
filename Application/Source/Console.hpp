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

#include "Outcome.hpp"
#include <Font/Baker.hpp>
#include <Sound/Baker.hpp>
#include <Texture/Atlas/Composer.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    /// \brief Runs every command Studio is typed with, over textures, fonts and sounds.
    class Console final
    {
    public:

        /// \brief Constructs the console with every importer the build enables.
        ///
        /// \param Scheduler The job pool, which must outlive the console.
        explicit Console(Ref<ZyJob::Service> Scheduler);

        /// \brief Runs the command a parsed command line names.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Run(ConstRef<Environment> Parsed) const;

    private:

        /// \brief Represents one command: the name it is typed as and the member that runs it.
        struct Entry final
        {
            /// The name the command is typed as.
            Text Name;

            /// The member that runs the command.
            Outcome (Console::* Handler)(ConstRef<Environment>) const;
        };

        /// \brief Finds the command typed under a name.
        ///
        /// \param Name The command name.
        /// \return The command, or `nullptr` if there is none.
        static ConstPtr<Entry> Find(Text Name);

        /// \brief Prints every command and switch.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Help(ConstRef<Environment> Parsed) const;

        /// \brief Prints the revision of every file Studio writes.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Version(ConstRef<Environment> Parsed) const;

        /// \brief Bakes one source into its native file, picked by the source's extension.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Bake(ConstRef<Environment> Parsed) const;

        /// \brief Bakes one source image into a native texture.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome BakeTexture(ConstRef<Environment> Parsed) const;

        /// \brief Bakes one typeface into a native font.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome BakeFont(ConstRef<Environment> Parsed) const;

        /// \brief Bakes one source sound into a native sound.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome BakeSound(ConstRef<Environment> Parsed) const;

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

        Texture::Baker mTextures;
        Font::Baker    mFonts;
        Sound::Baker   mSounds;
    };
}