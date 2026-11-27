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

#include "Font/Profile.hpp"
#include "Font/Typeface.hpp"
#include <Zyphryon.Job/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    /// \brief Decodes a TrueType or OpenType typeface into outlines and metrics, through `stb_truetype`.
    class Importer final
    {
    public:

        /// \brief The source file extensions this importer accepts.
        static constexpr Text kTypes[] =
        {
            "ttf", "otf", "ttc"
        };

    public:

        /// \brief Checks whether a source extension is one this importer reads.
        ///
        /// \param Type The source extension, without its dot.
        /// \return `true` if the extension is accepted, otherwise `false`.
        static Bool Accepts(Text Type);

        /// \brief Decodes the codepoints of a typeface that no earlier face gave, and the kerning between them.
        ///
        /// \param Scheduler The pool the kerning is measured on.
        /// \param Source    The encoded typeface.
        /// \param Profile   The settings naming the codepoints, the size and the face read out of a collection.
        /// \param Claimed   The codepoints earlier faces gave, which receives the ones this face gives.
        /// \param Output    Receives the glyphs and kerning, and the metrics if it held no glyph yet.
        static void Import(
            Ref<ZyJob::Service>      Scheduler,
            ConstSpan<Byte>          Source,
            ConstRef<Profile>        Profile,
            Ref<Table<UInt32, Bool>> Claimed,
            Ref<Typeface>            Output);
    };
}