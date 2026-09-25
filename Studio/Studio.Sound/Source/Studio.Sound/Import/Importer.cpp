// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Importer.hpp"
#include <Zyphryon.Audio/Resampler.hpp>
#include <Zyphryon.Audio/Types.hpp>

#define DR_FLAC_IMPLEMENTATION
#define DR_MP3_IMPLEMENTATION
#define DR_WAV_IMPLEMENTATION
#include <dr_flac.h>
#include <dr_mp3.h>
#include <dr_wav.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Sound
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Sample Resample(AnyRef<Blob> Samples, UInt32 Frequency, UInt16 Stride, UInt64 Frames)
    {
        if (Frequency == ZyAudio::kMixerFrequency)
        {
            return Sample(Move(Samples), Frequency, Stride, Frames);
        }

        const ConstSpan Source(Samples.GetData<Real32>(), Frames * Stride);

        UInt64 Produced = 0;
        Blob   Data
            = ZyAudio::Resampler::Convert(Source, Stride, Frames, Frequency, ZyAudio::kMixerFrequency, Produced);
        return Sample(Move(Data), ZyAudio::kMixerFrequency, Stride, Produced);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Sample ImportFLAC(ConstSpan<Byte> Source)
    {
        const Ptr<drflac> Description = drflac_open_memory(Source.GetData(), Source.GetSize(), nullptr);

        if (Description == nullptr)
        {
            LOG_E("Sound: the source is not a readable FLAC stream");

            return Sample();
        }

        const UInt32 Frequency = Description->sampleRate;
        const UInt16 Stride    = static_cast<UInt16>(Description->channels);

        Blob Samples = Blob::Allocate<Real32>(Description->totalPCMFrameCount * Stride);

        // A truncated stream decodes fewer frames than its header promises, so the count read is the one kept.
        const UInt64 Frames
            = drflac_read_pcm_frames_f32(Description, Description->totalPCMFrameCount, Samples.GetData<Real32>());
        drflac_close(Description);

        return Resample(Move(Samples), Frequency, Stride, Frames);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Sample ImportMP3(ConstSpan<Byte> Source)
    {
        drmp3 Description;

        if (!drmp3_init_memory(AddressOf(Description), Source.GetData(), Source.GetSize(), nullptr))
        {
            LOG_E("Sound: the source is not a readable MP3 stream");

            return Sample();
        }

        const UInt64 Total     = drmp3_get_pcm_frame_count(AddressOf(Description));
        const UInt32 Frequency = Description.sampleRate;
        const UInt16 Stride    = static_cast<UInt16>(Description.channels);

        Blob Samples = Blob::Allocate<Real32>(Total * Stride);

        const UInt64 Frames = drmp3_read_pcm_frames_f32(AddressOf(Description), Total, Samples.GetData<Real32>());
        drmp3_uninit(AddressOf(Description));

        return Resample(Move(Samples), Frequency, Stride, Frames);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Sample ImportWAV(ConstSpan<Byte> Source)
    {
        drwav Description;

        if (!drwav_init_memory(AddressOf(Description), Source.GetData(), Source.GetSize(), nullptr))
        {
            LOG_E("Sound: the source is not a readable WAV stream");

            return Sample();
        }

        const UInt32 Frequency = Description.sampleRate;
        const UInt16 Stride    = Description.channels;

        Blob Samples = Blob::Allocate<Real32>(Description.totalPCMFrameCount * Stride);

        const UInt64 Frames = drwav_read_pcm_frames_f32(
            AddressOf(Description), Description.totalPCMFrameCount, Samples.GetData<Real32>());
        drwav_uninit(AddressOf(Description));

        return Resample(Move(Samples), Frequency, Stride, Frames);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Importer::Accepts(Text Type)
    {
        for (const Text Entry : kTypes)
        {
            if (StrEqualCaseInsensitive(Entry, Type))
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sample Importer::Import(ConstSpan<Byte> Source, Text Type)
    {
        if (Source.IsEmpty())
        {
            LOG_E("Sound: source sound is empty");

            return Sample();
        }

        if (StrEqualCaseInsensitive(Type, "flac"))
        {
            return ImportFLAC(Source);
        }
        if (StrEqualCaseInsensitive(Type, "mp3"))
        {
            return ImportMP3(Source);
        }
        if (StrEqualCaseInsensitive(Type, "wav"))
        {
            return ImportWAV(Source);
        }

        LOG_E("Sound: '{0}' is not a source format this baker understands", Type);
        return Sample();
    }
}