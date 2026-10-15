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

#include "Studio.Texture/Image/Bitmap.hpp"
#include <Zyphryon.Job/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Packs a bitmap's texels into any block-compressed format that has an encoder, each format read from its own plain source.
    class Encoder final
    {
    public:

        /// \brief Gets the interleaved format a block-compressed format is packed from.
        ///
        /// \param Format The block-compressed format to write.
        /// \return The format whose texels the encoder reads, or `Unspecified` if no encoder writes \p Format.
        static ZyGraphic::TextureFormat GetSource(ZyGraphic::TextureFormat Format);

        /// \brief Checks whether a block-compressed format has an encoder.
        ///
        /// \param Format The block-compressed format to write.
        /// \return `true` if \ref GetSource names a plain format for \p Format, otherwise `false`.
        static Bool IsSupported(ZyGraphic::TextureFormat Format);

        /// \brief Packs every level of a bitmap into a block-compressed format, four by four texels per block.
        ///
        /// \param Scheduler The scheduler each level's rows of blocks are spread over.
        /// \param Source    The bitmap to pack, in the format \ref GetSource names for \p Format.
        /// \param Format    The block-compressed format to produce.
        /// \return The bitmap in \p Format, or an empty one on failure.
        static Bitmap Encode(Ref<ZyJob::Service> Scheduler, ConstRef<Bitmap> Source, ZyGraphic::TextureFormat Format);

    private:

        /// \brief Packs one block of texels, gathered from the source in raster order.
        ///
        /// \param Format The block-compressed format to produce.
        /// \param Texels The sixteen texels, in the format \ref GetSource names for \p Format.
        /// \param Block  The block to write, eight or sixteen bytes long.
        static void Pack(ZyGraphic::TextureFormat Format, ConstPtr<Byte> Texels, Ptr<Byte> Block);
    };
}