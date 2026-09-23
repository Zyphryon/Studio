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
        /// \param Baker   The baker whose importers read the sources, which must outlive the composer.
        /// \param Folder  The folder the tracker sits in, which every region's source is relative to.
        /// \param Profile The settings sources are read with and slices are written in.
        Composer(ConstRef<Baker> Baker, Text Folder, ConstRef<Profile> Profile);

        /// \brief Reads every region's source and fills in the sizes the region leaves out.
        ///
        /// \note A region with no `From` takes the whole source, and one with no `Rect` takes the size of its `From`.
        ///
        /// \param Atlas The tracker whose regions are measured.
        /// \return `true` if every source was read and every `From` lies inside its source, otherwise `false`.
        Bool Measure(Ref<Tracker> Atlas);

        /// \brief Draws every slice of a tracker whose regions are already placed.
        ///
        /// \note A region whose `Rect` differs in size from its `From` is resized to fit.
        ///
        /// \param Atlas  The tracker to draw, whose format is settled here if it names none.
        /// \param Output Receives one bitmap per slice, in the tracker's format.
        /// \return `true` if every slice was drawn, otherwise `false`.
        Bool Compose(Ref<Tracker> Atlas, Ref<Sequence<Bitmap>> Output);

    private:

        /// \brief Gets a source, reading it the first time it is asked for.
        ///
        /// \param Source The source, relative to the composer's folder.
        /// \return The source's first slice, or `nullptr` if it cannot be read.
        ConstPtr<Canvas> Fetch(Text Source);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        ConstRef<Baker>          mBaker;
        Str                      mFolder;
        ConstRef<Profile>        mProfile;
        Table<Str, Canvas>       mSources;
        ZyGraphic::TextureFormat mFirst;
    };
}