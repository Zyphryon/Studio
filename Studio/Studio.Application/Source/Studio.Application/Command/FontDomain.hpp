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
#include <Studio.Font/Baker.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    /// \brief Provides the font commands: bake.
    class FontDomain final
    {
    public:

        /// \brief Constructs the domain with every importer the build enables.
        ///
        /// \param Scheduler The job pool, which must outlive the domain.
        explicit FontDomain(Ref<ZyJob::Service> Scheduler);

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
            Outcome (FontDomain::* Handler)(ConstRef<Environment>) const;
        };

        /// \brief Finds the command typed under a name.
        ///
        /// \param Name The command name.
        /// \return The command, or `nullptr` if there is none.
        static ConstPtr<Entry> Find(Text Name);

        /// \brief Bakes one typeface into a native font.
        ///
        /// \param Parsed The parsed command line.
        /// \return How the command ended.
        Outcome Bake(ConstRef<Environment> Parsed) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Font::Baker mBaker;
    };
}