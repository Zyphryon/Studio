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

        /// \brief The key an importer is registered under: a source extension, lowercased and without its dot.
        using Extension = Str16;

        /// \brief Maps every accepted extension to the importer that claims it.
        using Registry  = Table<Extension, Retainer<Importer>>;

        /// \brief Describes one typeface a bake draws from, and the codepoints it is asked for.
        struct Source final
        {
            /// The encoded source bytes.
            ConstSpan<Byte>    Data;

            /// The source extension, which selects the importer.
            Text               Type;

            /// The codepoints to take from this typeface, or empty for whatever the profile asks.
            Sequence<Interval> Charset;
        };

    public:

        /// \brief Constructs a baker with every importer the build enables already registered.
        ///
        /// \param Scheduler The pool the glyphs are generated on, which must outlive the baker.
        explicit Baker(Ref<ZyJob::Service> Scheduler);

        /// \brief Registers an importer under each extension it accepts.
        ///
        /// \note An extension another importer already claims is taken over, which is how a host replaces a
        ///       built-in decoder with its own.
        ///
        /// \param Codec The importer to register.
        void Register(ConstRetainer<Importer> Codec);

        /// \brief Unregisters whichever importer claims an extension.
        ///
        /// \param Type The source extension, with or without its leading dot.
        /// \return `true` if an importer was unregistered, otherwise `false`.
        Bool Unregister(Text Type);

        /// \brief Finds the importer that claims a source extension.
        ///
        /// \param Type The source extension, with or without its leading dot.
        /// \return The matching importer, or `nullptr` if no importer claims the extension.
        ConstPtr<Importer> Find(Text Type) const;

        /// \brief Gets every extension this baker accepts, keyed to the importer that claims it.
        ///
        /// \return The importer registry.
        ZY_INLINE ConstRef<Registry> GetRegistry() const
        {
            return mRegistry;
        }

        /// \brief Bakes several typefaces into one font.
        ///
        /// \note The sources are read in order, and the first to carry a codepoint is the one it comes from.
        ///
        /// \param Sources The typefaces to draw from, in the order they are asked.
        /// \param Profile The settings controlling the bake.
        /// \return A blob holding the native font bytes, or an empty blob on failure.
        Blob Bake(ConstSpan<Source> Sources, ConstRef<Profile> Profile) const;

        /// \brief Bakes an encoded typeface held in memory.
        ///
        /// \param Source  The encoded source bytes.
        /// \param Type    The source extension, which selects the importer.
        /// \param Profile The settings controlling the bake.
        /// \return A blob holding the native font bytes, or an empty blob on failure.
        Blob Bake(ConstSpan<Byte> Source, Text Type, ConstRef<Profile> Profile) const;

        /// \brief Bakes a typeface on disk and writes the native font to another path.
        ///
        /// \param Source      The typeface path, whose extension selects the importer.
        /// \param Destination The native output path; any folder in it that does not exist yet is created.
        /// \param Profile     The settings controlling the bake.
        /// \return `true` if the font was written, otherwise `false`.
        Bool Bake(Text Source, Text Destination, ConstRef<Profile> Profile) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<ZyJob::Service> mScheduler;
        Registry            mRegistry;
    };
}