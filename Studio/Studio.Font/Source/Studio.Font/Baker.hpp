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

#include "Studio.Font/Export/Exporter.hpp"
#include "Studio.Font/Import/Importer.hpp"
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

        /// \brief The key an importer is registered under: a lowercase extension without its dot.
        using Extension = Str16;

        /// \brief Maps every accepted extension to the importer that claims it.
        using Registry  = Table<Extension, Retainer<Importer>>;

        /// \brief Represents one typeface a bake draws from, and the codepoints it is asked for.
        struct Source final
        {
            /// The encoded source bytes.
            ConstSpan<Byte>    Data;

            /// The source extension, which selects the importer.
            Text               Type;

            /// The codepoints taken from it, or empty for the profile's.
            Sequence<Interval> Charset;
        };

        /// \brief Represents a fallback typeface on disk, and the codepoints it answers for.
        struct Fallback final
        {
            /// The typeface path, whose extension selects the importer.
            Text Path;

            /// The charset taken from it, or empty for the profile's.
            Text Charset;
        };

    public:

        /// \brief Constructs a baker with every importer the build enables already registered.
        ///
        /// \param Scheduler The pool the glyphs are generated on.
        explicit Baker(Ref<ZyJob::Service> Scheduler);

        /// \brief Registers an importer under each extension it accepts, taking over any already claimed.
        ///
        /// \param Codec The importer to register.
        void Register(ConstRetainer<Importer> Codec);

        /// \brief Unregisters whichever importer claims an extension.
        ///
        /// \param Type The source extension.
        /// \return `true` if an importer was unregistered, otherwise `false`.
        Bool Unregister(Text Type);

        /// \brief Finds the importer that claims a source extension.
        ///
        /// \param Type The source extension.
        /// \return The importer, or `nullptr` if none claims it.
        ConstPtr<Importer> Find(Text Type) const;

        /// \brief Gets every extension this baker accepts, keyed to the importer that claims it.
        ///
        /// \return The importer registry.
        ZY_INLINE ConstRef<Registry> GetRegistry() const
        {
            return mRegistry;
        }

        /// \brief Bakes several typefaces into one font, the first to carry a codepoint giving it.
        ///
        /// \param Sources The typefaces, in the order they are asked.
        /// \param Profile The settings to bake with.
        /// \return The font bytes, or an empty blob on failure.
        Blob Bake(ConstSpan<Source> Sources, ConstRef<Profile> Profile) const;

        /// \brief Bakes an encoded typeface held in memory.
        ///
        /// \param Source  The encoded typeface.
        /// \param Type    The source extension.
        /// \param Profile The settings to bake with.
        /// \return The font bytes, or an empty blob on failure.
        Blob Bake(ConstSpan<Byte> Source, Text Type, ConstRef<Profile> Profile) const;

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
        Registry            mRegistry;
    };
}