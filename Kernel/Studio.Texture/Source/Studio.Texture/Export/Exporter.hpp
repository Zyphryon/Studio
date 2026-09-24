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
#include "Studio.Texture/Profile.hpp"
#include <Zyphryon.Job/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Writes decoded bitmaps in the engine's native texture format.
    class Exporter final
    {
    public:

        /// \brief The file extension a baked texture is written with.
        static constexpr Text   kOutput      = "tex";

        /// \brief The four-character code every baked texture starts with, stored little-endian as `ZTEX`.
        static constexpr UInt32 kMagic       = 'Z' | ('T' << 8) | ('E' << 16) | ('X' << 24);

        /// \brief The layout revision of the header this exporter writes.
        static constexpr UInt16 kVersion     = 1;

        /// \brief The LZ4 search effort the payload is encoded at.
        static constexpr UInt32 kCompression = 9;

    public:

        /// \brief Checks whether this exporter can write the given format.
        ///
        /// \param Format The texture format to test.
        /// \return `true` if the format is supported, otherwise `false`.
        static Bool IsSupported(ZyGraphic::TextureFormat Format);

        /// \brief Serializes one or more decoded bitmaps into a native texture blob.
        ///
        /// \param Scheduler The pool the slices are processed on.
        /// \param Slices    The bitmaps to write, one per slice or face.
        /// \param Layout    The layout the slices compose.
        /// \param Profile   The settings to write with.
        /// \return The texture bytes, or an empty blob on failure.
        static Blob Export(
            Ref<ZyJob::Service>      Scheduler,
            AnyRef<Sequence<Bitmap>> Slices,
            ZyGraphic::TextureLayout Layout,
            ConstRef<Profile>        Profile);

        /// \brief Serializes a single decoded bitmap into a native texture blob.
        ///
        /// \param Scheduler The pool the mip chain is built on.
        /// \param Source    The decoded bitmap to write.
        /// \param Profile   The settings to write with.
        /// \return The texture bytes, or an empty blob on failure.
        static Blob Export(Ref<ZyJob::Service> Scheduler, AnyRef<Bitmap> Source, ConstRef<Profile> Profile);

        /// \brief Mips and transcodes one slice the way a bake does, for a live upload.
        ///
        /// \param Source The slice, holding its base level alone.
        /// \param Levels The number of levels, the base included.
        /// \param Format The format every level is written in.
        /// \return Every level in \p Format, or an empty bitmap on failure.
        static Bitmap Prepare(AnyRef<Bitmap> Source, UInt8 Levels, ZyGraphic::TextureFormat Format);
    };
}