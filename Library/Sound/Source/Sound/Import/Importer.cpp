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

    static Real64 Bessel(Real64 Value)
    {
        Real64 Sum  = 1.0;
        Real64 Term = 1.0;

        // The series of the zeroth modified Bessel function, summed until a term no longer moves the total.
        for (Real64 Order = 1.0; Term > Sum * kEpsilon<Real64>; Order += 1.0)
        {
            const Real64 Factor = Value / (2.0 * Order);

            Term *= Factor * Factor;
            Sum  += Term;
        }
        return Sum;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<auto Release>
    static Blob Adopt(Ptr<Real32> Samples, UInt64 Count)
    {
        // The decoder's buffer is adopted rather than copied, and the blob releases it through the matching free.
        const UInt32 Size = static_cast<UInt32>(Count * sizeof(Real32));

        return Blob(reinterpret_cast<Ptr<Byte>>(Samples), Size, Blob::Deleter([](Ptr<Byte> Address)
        {
            Release(Address, nullptr);
        }));
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

        const ConstPtr<Byte> Data = Source.GetData();
        const UInt           Size = Source.GetSize();

        UInt32 Stride    = 0;
        UInt32 Frequency = 0;
        Blob   Samples;

        // The three decoders count frames in one 64-bit type, which `UInt64` is not on every platform.
        drflac_uint64 Frames = 0;

        // FLAC and MP3 decode to the end of the stream, so a source that leaves out its length still reads.
        if (StrEqualCaseInsensitive(Type, "flac"))
        {
            const Ptr<Real32> Decoded = drflac_open_memory_and_read_pcm_frames_f32(
                Data, Size, AddressOf(Stride), AddressOf(Frequency), AddressOf(Frames), nullptr);
            Samples = Adopt<drflac_free>(Decoded, Frames * Stride);
        }
        else if (StrEqualCaseInsensitive(Type, "mp3"))
        {
            drmp3_config Config { };

            const Ptr<Real32> Decoded = drmp3_open_memory_and_read_pcm_frames_f32(
                Data, Size, AddressOf(Config), AddressOf(Frames), nullptr);
            Samples   = Adopt<drmp3_free>(Decoded, Frames * Config.channels);
            Stride    = Config.channels;
            Frequency = Config.sampleRate;
        }
        else if (StrEqualCaseInsensitive(Type, "wav"))
        {
            const Ptr<Real32> Decoded = drwav_open_memory_and_read_pcm_frames_f32(
                Data, Size, AddressOf(Stride), AddressOf(Frequency), AddressOf(Frames), nullptr);
            Samples = Adopt<drwav_free>(Decoded, Frames * Stride);
        }
        else
        {
            LOG_E("Sound: '{0}' is not a source format this baker understands", Type);
            return Sample();
        }

        if (Samples == nullptr)
        {
            LOG_E("Sound: the source is not a readable {0} stream", Type);

            return Sample();
        }
        return Resample(Move(Samples), Frequency, static_cast<UInt16>(Stride), Frames);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sample Importer::Resample(AnyRef<Blob> Samples, UInt32 Frequency, UInt16 Stride, UInt64 Frames)
    {
        // The mixer plays one or two channels, so a wider source is refused rather than folded down.
        if (Stride == 0 || Stride > ZyAudio::kMixerStride || Frequency == 0)
        {
            LOG_E("Sound: a source of {0} channels at {1} Hz is not one the mixer plays", Stride, Frequency);

            return Sample();
        }

        if (Frequency == ZyAudio::kMixerFrequency)
        {
            return Sample(Move(Samples), Frequency, Stride, Frames);
        }

        // The two rates reduce to a ratio of whole numbers, so every output frame falls on one of a set of phases.
        UInt32 Common = ZyAudio::kMixerFrequency;
        UInt32 Rest   = Frequency;

        while (Rest != 0)
        {
            Common = Exchange(Rest, Common % Rest);
        }

        const UInt64 Phases = ZyAudio::kMixerFrequency / Common;
        const UInt64 Step   = Frequency / Common;

        // The cutoff is in cycles per source frame, and the kernel reaches as far as its zero crossings span.
        const Real64 Cutoff = kBandwidth * 0.5 * Min(1.0, static_cast<Real64>(ZyAudio::kMixerFrequency) / Frequency);
        const Real64 Reach  = kCrossings / (2.0 * Cutoff);
        const UInt64 Half   = static_cast<UInt64>(Ceil(Reach));
        const UInt64 Taps   = 2 * Half + 1;

        Blob              Table   = Blob::Allocate<Real32>(Phases * Taps);
        const Ptr<Real32> Weights = Table.GetData<Real32>();

        for (UInt64 Phase = 0; Phase < Phases; ++Phase)
        {
            const Ptr<Real32> Row    = Weights + Phase * Taps;
            const Real64      Offset = static_cast<Real64>(Half) + static_cast<Real64>(Phase) / Phases;

            Real64 Sum = 0.0;

            for (UInt64 Tap = 0; Tap < Taps; ++Tap)
            {
                const Real64 Distance = static_cast<Real64>(Tap) - Offset;
                const Real64 Angle    = kPI<Real64> * 2.0 * Cutoff * Distance;
                const Real64 Square   = (Distance / Reach) * (Distance / Reach);
                const Real64 Sinc     = (Angle == 0.0 ? 1.0 : Sine(Angle) / Angle);

                Row[Tap] = static_cast<Real32>(Square < 1.0 ? Sinc * Bessel(kBeta * Sqrt(1.0 - Square)) : 0.0);
                Sum     += Row[Tap];
            }

            // Each phase sums to one, so a constant source keeps its level whatever the phase.
            for (UInt64 Tap = 0; Tap < Taps; ++Tap)
            {
                Row[Tap] = static_cast<Real32>(Row[Tap] / Sum);
            }
        }

        const UInt64 Produced = Frames * ZyAudio::kMixerFrequency / Frequency;

        Blob                   Result = Blob::Allocate<Real32>(Produced * Stride);
        const Ptr<Real32>      Output = Result.GetData<Real32>();
        const ConstPtr<Real32> Input  = Samples.GetData<Real32>();

        for (UInt64 Frame = 0; Frame < Produced; ++Frame)
        {
            const UInt64 Position = Frame * Step;
            const UInt64 Center   = Position / Phases;

            // The source reads as silence past both ends, so only the taps over its own frames are summed.
            const UInt64           Begin = (Center > Half ? Center - Half : 0);
            const UInt64           End   = Min(Center + Half + 1, Frames);
            const ConstPtr<Real32> Row   = Weights + (Position % Phases) * Taps + (Begin + Half - Center);

            for (UInt16 Channel = 0; Channel < Stride; ++Channel)
            {
                Real32 Sum = 0.0f;

                for (UInt64 Index = Begin; Index < End; ++Index)
                {
                    Sum += Row[Index - Begin] * Input[Index * Stride + Channel];
                }
                Output[Frame * Stride + Channel] = Sum;
            }
        }
        return Sample(Move(Result), ZyAudio::kMixerFrequency, Stride, Produced);
    }
}