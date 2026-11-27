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

#include "Texture/Image/Bitmap.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Rewrites a bitmap's texels into another interleaved format.
    class Transcoder final
    {
    public:

        /// \brief Transcodes every level of a bitmap into a target format.
        ///
        /// \param Source The bitmap to convert.
        /// \param Format The format to produce.
        /// \return The converted bitmap, or an invalid one on failure.
        static Bitmap Transcode(AnyRef<Bitmap> Source, ZyGraphic::TextureFormat Format);

        /// \brief Converts a run of texels from one interleaved format into another.
        ///
        /// \param Source The texels to read, one after another.
        /// \param From   The format the source texels are stored in.
        /// \param Target Receives the converted texels, one after another.
        /// \param To     The format to write.
        /// \param Texels The number of texels in the run.
        /// \return `true` if both formats are interleaved and the run was converted, otherwise `false`.
        static Bool Convert(
            ConstPtr<Byte>           Source,
            ZyGraphic::TextureFormat From,
            Ptr<Byte>                Target,
            ZyGraphic::TextureFormat To,
            UInt32                   Texels);
    };
}