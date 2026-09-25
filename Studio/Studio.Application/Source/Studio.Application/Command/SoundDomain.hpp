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
            Outcome (SoundDomain::* Handler)(ConstRef<Environment>) const;
        };

        /// \brief Finds the command typed under a name.
        ///
        /// \param Name The command name.
        /// \return The command, or `nullptr` if there is none.
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