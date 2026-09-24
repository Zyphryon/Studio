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

#include "Tracker.hpp"
#include "Studio.Texture/Baker.hpp"
#include "Studio.Texture/Image/Canvas.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief Draws the slices of a tracker out of the sources its regions name, each source read once.
    class Composer final
    {
    public:

        /// \brief Constructs a composer that reads sources through a baker.
        ///
        /// \param Baker   The baker that reads the sources.
        /// \param Folder  The folder the sources are relative to.
        /// \param Profile The settings to read and write with.
        Composer(ConstRef<Baker> Baker, Text Folder, ConstRef<Profile> Profile);

        /// \brief Reads every region's sources and fills in the sizes it leaves out.
        ///
        /// \param Atlas The tracker to measure.
        /// \return `true` if every region is valid, otherwise `false`.
        Bool Measure(Ref<Tracker> Atlas);

        /// \brief Draws every slice of a tracker whose regions are already placed.
        ///
        /// \param Atlas  The tracker to draw.
        /// \param Output Receives one bitmap per slice.
        /// \return `true` if every slice was drawn, otherwise `false`.
        Bool Compose(Ref<Tracker> Atlas, Ref<Sequence<Bitmap>> Output);

        /// \brief Draws every slice of a placed tracker and encodes them as a texture.
        ///
        /// \param Atlas The tracker to draw.
        /// \return The texture bytes, or an empty blob on failure.
        Blob Bake(Ref<Tracker> Atlas);

        /// \brief Counts the slices a source holds.
        ///
        /// \param Source The source, relative to the composer's folder.
        /// \return The slice count, or zero if it cannot be read.
        UInt16 Count(Text Source);

    private:

        /// \brief Gets a source, reading it the first time it is asked for.
        ///
        /// \param Source The source, relative to the composer's folder.
        /// \param Data   `true` to read it as linear data.
        /// \return The decoded source, or `nullptr` on failure.
        ConstPtr<Surface> Fetch(Text Source, Bool Data);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        ConstRef<Baker>          mBaker;
        Str                      mFolder;
        ConstRef<Profile>        mProfile;
        Table<Str, Surface>      mSources;
        Table<Str, Surface>      mData;
        ZyGraphic::TextureFormat mFirst;
    };
}