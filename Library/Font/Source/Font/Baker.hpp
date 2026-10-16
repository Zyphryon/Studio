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

#include "Font/Export/Exporter.hpp"
#include "Font/Import/Importer.hpp"
#include <Zyphryon.Job/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    /// \brief Bakes typefaces into the engine's native font format.
    class Baker final
    {
    public:

        /// \brief Represents one typeface a bake draws from, and the codepoints it is asked for.
        struct Source final
        {
            /// The encoded source bytes.
            ConstSpan<Byte>    Data;

            /// The source extension.
            Text               Type;

            /// The codepoints taken from it, or empty for the profile's.
            Sequence<Interval> Charset;
        };

        /// \brief Represents a fallback typeface on disk, and the codepoints it answers for.
        struct Fallback final
        {
            /// The typeface path.
            Text Path;

            /// The charset taken from it, or empty for the profile's.
            Text Charset;
        };

    public:

        /// \brief Constructs a baker.
        ///
        /// \param Scheduler The pool the glyphs are generated on.
        explicit Baker(Ref<ZyJob::Service> Scheduler);

        /// \brief Bakes several typefaces into one font, the first to carry a codepoint giving it.
        ///
        /// \param Sources The typefaces, in the order they are asked.
        /// \param Profile The settings to bake with.
        /// \return The font bytes, or an empty blob on failure.
        Blob Bake(ConstSpan<Source> Sources, ConstRef<Profile> Profile) const;

        /// \brief Bakes a typeface on disk, and any fallbacks, into a font file.
        ///
        /// \param Source      The typeface path.
        /// \param Destination The font path to write.
        /// \param Profile     The settings to bake with.
        /// \param Fallbacks   The typefaces folded in after the source.
        /// \return `true` if the font was written, otherwise `false`.
        Bool Bake(Text Source, Text Destination, ConstRef<Profile> Profile, ConstSpan<Fallback> Fallbacks = { }) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<ZyJob::Service> mScheduler;
    };
}