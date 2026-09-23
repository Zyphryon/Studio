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

#include "Studio.Sound/Export/Exporter.hpp"
#include "Studio.Sound/Import/Importer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Sound
{
    /// \brief Bakes source sounds into the engine's native sound format.
    class Baker final
    {
    public:

        /// \brief Bakes an encoded source sound held in memory.
        ///
        /// \param Source  The encoded source bytes.
        /// \param Type    The source extension, which selects the decoder.
        /// \param Profile The settings controlling encoding and compression.
        /// \return A blob holding the native sound bytes, or an empty blob on failure.
        Blob Bake(ConstSpan<Byte> Source, Text Type, ConstRef<Profile> Profile) const;

        /// \brief Bakes a source sound on disk and writes the native sound to another path.
        ///
        /// \param Source      The source sound path, whose extension selects the decoder.
        /// \param Destination The native output path; any folder in it that does not exist yet is created.
        /// \param Profile     The settings controlling encoding and compression.
        /// \return `true` if the sound was written, otherwise `false`.
        Bool Bake(Text Source, Text Destination, ConstRef<Profile> Profile) const;
    };
}