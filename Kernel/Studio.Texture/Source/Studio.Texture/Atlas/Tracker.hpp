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
        /// \return `true` if both sides are longer than zero, otherwise `false`.
        ZY_INLINE Bool IsValid() const
        {
            return Width > 0 && Height > 0;
        }
    };

    /// \brief Represents one channel of a region read from another image, such as a height packed into alpha.
    struct Splice final
    {
        /// The channel of the region written, from 0 (red) to 3 (alpha).
        UInt8 Target = 3;

        /// The image the channel is read from, as linear data.
        Str   Source;

        /// The channel of the source read, from 0 (red) to 3 (alpha).
        UInt8 Origin = 0;
    };

    /// \brief Represents one image of an atlas: where it is taken from, and where it sits.
    struct Region final
    {
        /// The name a consumer looks the image up by.
        Str              Name;

        /// The source image, relative to the tracker's own folder.
        Str              Source;

        /// The slice of the source taken.
        UInt16           Layer   = 0;

        /// The part of the source taken, or an invalid area for the whole of it.
        Area             From;

        /// The slice of the texture the image sits on.
        UInt16           Slice   = 0;

        /// The place the image takes on its slice.
        Area             Rect;

        /// The channels taken from other images, after the image is fitted.
        Sequence<Splice> Channels;

        /// Whether the region is retired: still drawn, but no longer offered.
        Bool             Retired = false;
    };

    /// \brief Represents an atlas or an array as a list of named regions, saved as JSON.
    struct Tracker final
    {
        /// \brief The revision of the file layout this tracker reads and writes.
        static constexpr UInt32 kVersion = 1;

        /// The texture the regions are drawn into, relative to the tracker's own folder.
        Str                      Texture;

        /// The layout the texture is written as: a flat atlas, or an array of pages.
        ZyGraphic::TextureLayout Layout   = ZyGraphic::TextureLayout::Texture2D;

        /// The format the texture is written in, or `Unspecified` to take the first source's.
        ZyGraphic::TextureFormat Format   = ZyGraphic::TextureFormat::Unspecified;

        /// The width of every slice, in pixels.
        UInt16                   Width    = 0;

        /// The height of every slice, in pixels.
        UInt16                   Height   = 0;

        /// The number of slices.
        UInt16                   Slices   = 1;

        /// The empty pixels kept between neighbouring regions.
        UInt16                   Padding  = 0;

        /// The pixels each region's edge is repeated outward by.
        UInt16                   Extrude  = 0;

        /// The multiple the slice count of an array is rounded up to, leaving blank slices for later.
        UInt16                   Headroom = 1;

        /// The colour of every pixel no region covers, as the texture stores it.
        IntColor8                Fill     = IntColor8::Transparent();

        /// The regions, in the order they were authored.
        Sequence<Region>         Regions;

        /// \brief Gets how many slices every region needs, rounded up to the headroom.
        ///
        /// \return The slice count, never below \ref Slices.
        UInt16 GetDepth() const;

        /// \brief Reads a tracker from disk.
        ///
        /// \param Path   The JSON file to read.
        /// \param Output Receives the tracker.
        /// \return `true` if the file was read, otherwise `false`.
        static Bool Read(Text Path, Ref<Tracker> Output);

        /// \brief Writes the tracker to disk, with each region's crop.
        ///
        /// \param Path The JSON file to write.
        /// \return `true` if the file was written, otherwise `false`.
        Bool Write(Text Path) const;

        /// \brief Turns a path as a tracker holds it into one the process can open.
        ///
        /// \param Folder The folder the tracker sits in.
        /// \param Path   The path as the tracker holds it.
        /// \return The path, joined to \p Folder unless absolute.
        static Str Resolve(Text Folder, Text Path);

        /// \brief Turns a path the process can open into one relative to the tracker, comparing them as text.
        ///
        /// \param Folder The folder the tracker sits in.
        /// \param Path   The path to relate.
        /// \param Output Receives the relative path.
        /// \return `true` if the path was related, otherwise `false`.
        static Bool Relate(Text Folder, Text Path, Ref<Str> Output);
    };
}