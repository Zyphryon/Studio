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

        /// \brief Constructs a composer reading sources through a baker.
        ///
        /// \param Baker   The baker whose importers read the sources, which must outlive the composer.
        /// \param Folder  The folder a region's source is relative to, which is the tracker's own.
        /// \param Profile The settings sources are read with and slices are written in.
        Composer(ConstRef<Baker> Baker, Text Folder, ConstRef<Profile> Profile);

        /// \brief Reads every region's source and fills in whatever the region leaves to it.
        ///
        /// \note A region taking no part of its source takes all of it, and one not yet placed takes that size.
        ///
        /// \param Atlas The tracker whose regions are measured.
        /// \return `true` when every source was read and every part it names lies inside it, otherwise `false`.
        Bool Measure(Ref<Tracker> Atlas);

        /// \brief Draws every slice of a placed tracker.
        ///
        /// \note A region whose place differs in size from what it takes is filtered into its place.
        ///
        /// \param Atlas  The tracker to draw, whose format is settled here when it names none.
        /// \param Output Receives one bitmap per slice, in the tracker's format.
        /// \return `true` when every slice was drawn, otherwise `false`.
        Bool Compose(Ref<Tracker> Atlas, Ref<Sequence<Bitmap>> Output);

    private:

        /// \brief Gets a source, reading it the first time it is asked for.
        ///
        /// \param Source The source, relative to the composer's folder.
        /// \return The source's first slice, or `nullptr` when it cannot be read.
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
