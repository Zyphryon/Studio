// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Sound
{
    /// \brief Represents one decoded sound, held as interleaved floating-point frames.
    class Sample final
    {
    public:

        /// \brief Constructs an empty sample.
        ZY_INLINE Sample()
            : mFrequency { 0 },
              mStride    { 0 },
              mFrames    { 0 }
        {
        }

        /// \brief Constructs a sample over decoded frames.
        ///
        /// \param Samples   The interleaved frames, as 32-bit floating-point values.
        /// \param Frequency The rate the frames are clocked at, in hertz.
        /// \param Stride    The number of channels in a frame.
        /// \param Frames    The number of frames.
        ZY_INLINE Sample(AnyRef<Blob> Samples, UInt32 Frequency, UInt16 Stride, UInt64 Frames)
            : mSamples   { Move(Samples) },
              mFrequency { Frequency },
              mStride    { Stride },
              mFrames    { Frames }
        {
        }

        /// \brief Checks whether the sample holds no frames.
        ///
        /// \return `true` if the sample is empty, otherwise `false`.
        ZY_INLINE Bool IsEmpty() const
        {
            return mFrames == 0 || mStride == 0;
        }

        /// \brief Gets the interleaved frames.
        ///
        /// \return The frames, as 32-bit floating-point values.
        ZY_INLINE ConstSpan<Real32> GetSamples() const
        {
            return ConstSpan(mSamples.GetData<Real32>(), mFrames * mStride);
        }

        /// \brief Gets the rate the frames are clocked at.
        ///
        /// \return The frequency, in hertz.
        ZY_INLINE UInt32 GetFrequency() const
        {
            return mFrequency;
        }

        /// \brief Gets the number of channels in a frame.
        ///
        /// \return The channel count.
        ZY_INLINE UInt16 GetStride() const
        {
            return mStride;
        }

        /// \brief Gets the number of frames held.
        ///
        /// \return The frame count.
        ZY_INLINE UInt64 GetFrames() const
        {
            return mFrames;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Blob   mSamples;
        UInt32 mFrequency;
        UInt16 mStride;
        UInt64 mFrames;
    };
}