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
#include <Model/Baker.hpp>
#include <Sound/Baker.hpp>
#include <Texture/Atlas/Composer.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    /// \brief Runs every command Studio is typed with, over textures, fonts, models and sounds.
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
        /// \param Typed  The switches typed on the command line, without a preset's.
        /// \return The exit code the command ends with.
        Outcome Run(ConstRef<Environment> Parsed, ConstRef<Environment> Typed) const;

    private:

        /// \brief Represents one command, as the name it is typed as and the member that runs it.
        struct Entry final
        {
            /// The name the command is typed as.
            Text   Name;

            /// The bit of the command among the commands that read a switch.
            UInt16 Reader;

            /// The member that runs the command.
            Outcome (Console::* Handler)(ConstRef<Environment>) const;
        };

        /// \brief Represents one baker, as the sources it reads and the file it writes.
        struct Kind final
        {
            /// The test a source's extension passes for the baker to read it.
            Bool (* Accepts)(Text Type);

            /// The extension of the file the baker writes, without its dot.
            Text    Output;
        };

        /// \brief The bakers, texture, font, model and sound, in the order a source's extension is matched.
        static constexpr Array<Kind, 4> kKinds =
        {
            Kind { Texture::Importer::Accepts, Texture::Exporter::kOutput },
            Kind { Font::Importer::Accepts,    Font::Exporter::kOutput    },
            Kind { Model::Importer::Accepts,   Model::Exporter::kOutput   },
            Kind { Sound::Importer::Accepts,   Sound::Exporter::kOutput   },
        };

        /// \brief Finds the command typed under a name.
        ///
        /// \param Name The name the command is typed as.
        /// \return The command, or `nullptr` if there is none.
        static ConstPtr<Entry> Find(Text Name);

        /// \brief Picks the baker that reads a source.
        ///
        /// \param Type The source extension, without its dot.
        /// \return The index of the baker in \ref kKinds, or -1 if none reads it.
        static SInt Pick(Text Type);

        /// \brief Checks whether a file of a folder is a source to bake, rather than one a bake wrote.
        ///
        /// \param Type The file extension, without its dot.
        /// \return `true` if a baker reads the file and none writes it, otherwise `false`.
        static Bool IsSource(Text Type);

        /// \brief Prints every command and switch.
        ///
        /// \param Parsed The parsed command line.
        /// \return The exit code the command ends with.
        Outcome Help(ConstRef<Environment> Parsed) const;

        /// \brief Prints the revision of every file Studio writes.
        ///
        /// \param Parsed The parsed command line.
        /// \return The exit code the command ends with.
        Outcome Version(ConstRef<Environment> Parsed) const;

        /// \brief Bakes sources and folders of sources into their native files, each picked by its extension.
        ///
        /// \param Parsed The parsed command line.
        /// \return The exit code the command ends with.
        Outcome Bake(ConstRef<Environment> Parsed) const;

        /// \brief Packs images and folders of images into an atlas or an array, and writes its tracker.
        ///
        /// \param Parsed The parsed command line.
        /// \return The exit code the command ends with.
        Outcome Pack(ConstRef<Environment> Parsed) const;

        /// \brief Draws the texture a tracker describes again, after its regions were edited.
        ///
        /// \param Parsed The parsed command line.
        /// \return The exit code the command ends with.
        Outcome Build(ConstRef<Environment> Parsed) const;

        /// \brief Draws a normal map out of an image read as a height.
        ///
        /// \param Parsed The parsed command line.
        /// \return The exit code the command ends with.
        Outcome Normal(ConstRef<Environment> Parsed) const;

        /// \brief Draws a relief map out of an image read as a height.
        ///
        /// \param Parsed The parsed command line.
        /// \return The exit code the command ends with.
        Outcome Relief(ConstRef<Environment> Parsed) const;

        /// \brief Writes every slice of a texture out as a picture.
        ///
        /// \param Parsed The parsed command line.
        /// \return The exit code the command ends with.
        Outcome Extract(ConstRef<Environment> Parsed) const;

        /// \brief Lists every texture format a bake can write.
        ///
        /// \param Parsed The parsed command line.
        /// \return The exit code the command ends with.
        Outcome Formats(ConstRef<Environment> Parsed) const;

        /// \brief Adds a file to the sources, or the files of a folder that pass a test, in name order.
        ///
        /// \param Operand The file or folder.
        /// \param Accepts The test a file's extension passes to be taken from a folder.
        /// \param Skip    The file to leave out.
        /// \param Output  Receives the sources.
        void Collect(Text Operand, Bool (* Accepts)(Text Type), Text Skip, Ref<Sequence<Str>> Output) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<ZyJob::Service> mScheduler;
        Texture::Baker      mTextures;
        Font::Baker         mFonts;
        Model::Baker        mModels;
        Sound::Baker        mSounds;
    };
}