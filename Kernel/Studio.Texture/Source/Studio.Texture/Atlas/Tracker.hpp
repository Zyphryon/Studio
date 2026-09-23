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

#include <Zyphryon.Graphic/Types.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Represents a rectangle of whole pixels, counted from the top-left.
    struct Area final
    {
        /// The left edge, in pixels.
        UInt16 X      = 0;

        /// The top edge, in pixels.
        UInt16 Y      = 0;

        /// The width, in pixels.
        UInt16 Width  = 0;

        /// The height, in pixels.
        UInt16 Height = 0;

        /// \brief Checks whether the rectangle covers anything.
        ///
        /// \return `true` when both sides are longer than zero, otherwise `false`.
        ZY_INLINE Bool IsValid() const
        {
            return Width > 0 && Height > 0;
        }
    };

    /// \brief Represents one image of an atlas: where it is taken from, and where it sits.
    struct Region final
    {
        /// The name a consumer looks the image up by.
        Str    Name;

        /// The source image, relative to the tracker's own folder.
        Str    Source;

        /// The part of the source taken, or an invalid area for the whole of it.
        Area   From;

        /// The slice of the texture the image sits on, which is zero for anything but an array.
        UInt16 Slice = 0;

        /// Where the image sits on its slice.
        Area   Rect;
    };

    /// \brief Represents an atlas or an array as a list of named regions, the file any tool can read it by.
    struct Tracker final
    {
        /// \brief The revision of the file layout this tracker reads and writes.
        static constexpr UInt32 kVersion = 1;

        /// The texture the regions are drawn into, relative to the tracker's own folder.
        Str                      Texture;

        /// The layout the texture is written as: a flat atlas, or an array of pages.
        ZyGraphic::TextureLayout Layout  = ZyGraphic::TextureLayout::Texture2D;

        /// The format the texture is written in, or unspecified to take the first source's.
        ZyGraphic::TextureFormat Format  = ZyGraphic::TextureFormat::Unspecified;

        /// The width of every slice, in pixels.
        UInt16                   Width   = 0;

        /// The height of every slice, in pixels.
        UInt16                   Height  = 0;

        /// The number of slices.
        UInt16                   Slices  = 1;

        /// The empty pixels kept between neighbouring regions.
        UInt16                   Padding = 0;

        /// The pixels each region's edge is repeated outward by, so filtering never reads a neighbour.
        UInt16                   Extrude = 0;

        /// The regions, in the order they were authored.
        Sequence<Region>         Regions;

        /// \brief Reads a tracker from disk.
        ///
        /// \param Path   The JSON file to read.
        /// \param Output Receives the tracker, left untouched when the file cannot be read.
        /// \return `true` when the file was read, otherwise `false`.
        static Bool Read(Text Path, Ref<Tracker> Output);

        /// \brief Writes the tracker to disk, with each region's crop worked out for consumers that sample by it.
        ///
        /// \param Path The JSON file to write; any folder in it that does not exist yet is created.
        /// \return `true` when the file was written, otherwise `false`.
        Bool Write(Text Path) const;
    };
}
