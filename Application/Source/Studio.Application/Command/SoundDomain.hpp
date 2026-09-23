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
#include <Studio.Sound/Baker.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    /// \brief Provides the sound commands: bake.
    class SoundDomain final
    {
    public:

        /// \brief Checks whether a command belongs to this domain.
        ///
        /// \param Command The command name, as typed.
        /// \return `true` if this domain runs the command, otherwise `false`.
        Bool Claims(Text Command) const;

        /// \brief Checks whether this domain bakes a source, which is how `bake` finds its domain.
        ///
        /// \param Source The source path, whose extension decides.
        /// \return `true` if the importer of this domain reads the source, otherwise `false`.
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
            Outcome (SoundDomain::* Handler)(ConstRef<Environment>) const;
        };

        /// \brief Finds the command typed under a name.
        ///
        /// \param Name The command name, as typed.
        /// \return The command, or `nullptr` if this domain has none by that name.
        static ConstPtr<Entry> Find(Text Name);

        /// \brief Bakes one source sound into a native sound.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Bake(ConstRef<Environment> Parsed) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Sound::Baker mBaker;
    };
}