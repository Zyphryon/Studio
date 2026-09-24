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

#include "Exporter.hpp"
#include <Zyphryon.Audio/Decoder/Adaptive.hpp>
#include <opus.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Sound
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static SInt16 Quantize(Real32 Value)
    {
        return static_cast<SInt16>(Clamp(Value, -1.0f, 1.0f) * 32767.0f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static UInt8 Compress(Ref<SInt32> Predictor, Ref<SInt32> Step, SInt16 Value)
    {
        using namespace ZyAudio::Codec;

        SInt32 Delta  = Value - Predictor;
        UInt8  Nibble = 0;

        if (Delta < 0)
        {
            Nibble = 0x8;
            Delta  = -Delta;
        }

        // Each of the three magnitude bits halves the step it compares against, as the decoder expands them.
        SInt32 Size  = Adaptive::kStepTable[Step];
        SInt32 Total = Size >> 3;

        if (Delta >= Size)
        {
            Nibble |= 0x4;
            Delta  -= Size;
            Total  += Size;
        }

        Size >>= 1;

        if (Delta >= Size)
        {
            Nibble |= 0x2;
            Delta  -= Size;
            Total  += Size;
        }

        Size >>= 1;

        if (Delta >= Size)
        {
            Nibble |= 0x1;
            Total  += Size;
        }

        Predictor = Clamp<SInt32>(Predictor + ((Nibble & 0x8) ? -Total : Total), -32768, 32767);
        Step      = Clamp<SInt32>(Step + Adaptive::kStepIndex[Nibble], 0, Adaptive::kStepLimit);
        return Nibble;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Exporter::Export(ConstRef<Sample> Source, ConstRef<Profile> Profile)
    {
        if (Source.IsEmpty())
        {
            LOG_E("Sound: refusing to write a sound with no frames");

            return Blob();
        }

        if (Source.GetStride() > ZyAudio::kMixerStride)
        {
            LOG_E("Sound: {0} channels is more than the mixer plays", Source.GetStride());

            return Blob();
        }

        Blob Payload;

        switch (Profile.Encoding)
        {
        case ZyAudio::Encoding::Adaptive:
            Payload = EncodeAdaptive(Source);
            break;
        case ZyAudio::Encoding::Opus:
            Payload = EncodeOpus(Source, Profile.Bitrate);
            break;
        case ZyAudio::Encoding::Linear:
            Payload = EncodeLinear(Source);
            break;
        }

        if (Payload == nullptr)
        {
            return Blob();
        }

        const ConstSpan<Byte> Bytes  = ConstSpan(Payload.GetData<Byte>(), Payload.GetSize());
        const UInt32          Length = static_cast<UInt32>(Bytes.GetSize());

        Writer Output(Length + 32);
        Output.Write<UInt32>(kMagic);
        Output.Write<UInt16>(kVersion);
        Output.Write<ZyAudio::Encoding>(Profile.Encoding);
        Output.Write<UInt16>(Source.GetStride());
        Output.Write<UInt32>(Source.GetFrequency());
        Output.Write<UInt64>(Source.GetFrames());
        Output.Write<UInt32>(Length);

        // The loader reads a payload the same size as the raw count as uncompressed, so only a payload that
        // actually shrank is worth keeping.
        if (Profile.Compress)
        {
            Blob         Scratch = Blob::Allocate<Byte>(LZ4Bound(Length));
            const UInt32 Size    = LZ4Encode(Bytes, Scratch.GetData<Byte>(), LZ4Bound(Length), kCompression);

            if (Size > 0 && Size < Length)
            {
                Output.WriteBlock<UInt32, Byte>(ConstSpan(Scratch.GetData<Byte>(), Size));
                return Output.Detach();
            }
        }

        Output.WriteBlock<UInt32, Byte>(Bytes);
        return Output.Detach();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Exporter::EncodeLinear(ConstRef<Sample> Source)
    {
        const ConstSpan<Real32> Samples = Source.GetSamples();

        Blob              Payload = Blob::Allocate<SInt16>(Samples.GetSize());
        const Ptr<SInt16> Output  = Payload.GetData<SInt16>();

        for (UInt Index = 0; Index < Samples.GetSize(); ++Index)
        {
            Output[Index] = Quantize(Samples[Index]);
        }
        return Payload;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Exporter::EncodeAdaptive(ConstRef<Sample> Source)
    {
        // The block size is read from the decoder, so the two can never disagree.
        constexpr UInt32 kBlockFrames = ZyAudio::Codec::Adaptive::kBlockFrames;
        constexpr UInt32 kBlockStride = ZyAudio::Codec::Adaptive::kBlockStride;

        const ConstSpan<Real32> Samples = Source.GetSamples();
        const UInt16            Stride  = Source.GetStride();
        const UInt64            Frames  = Source.GetFrames();
        const UInt64            Blocks  = (Frames + kBlockFrames - 1) / kBlockFrames;

        Blob            Payload = Blob::Allocate<Byte>(Blocks * Stride * kBlockStride);
        const Ptr<Byte> Output  = Payload.GetData<Byte>();

        Zero(Output, Blocks * Stride * kBlockStride);

        // The step index carries over from one block to the next, so a block boundary does not reset the quantizer
        // to its coarsest step. Each block's header records the step it starts at, so the decoder can pick it up.
        Array<SInt32, ZyAudio::kMixerStride> Steps { };

        for (UInt64 Block = 0; Block < Blocks; ++Block)
        {
            const UInt64 Origin = Block * kBlockFrames;

            for (UInt16 Channel = 0; Channel < Stride; ++Channel)
            {
                const Ptr<Byte> Cursor    = Output + (Block * Stride + Channel) * kBlockStride;
                SInt32          Predictor = Quantize(Samples[Origin * Stride + Channel]);

                Cursor[0] = static_cast<Byte>(Predictor & 0xFF);
                Cursor[1] = static_cast<Byte>((Predictor >> 8) & 0xFF);
                Cursor[2] = static_cast<Byte>(Steps[Channel]);
                Cursor[3] = 0;

                const UInt64 Length = Min<UInt64>(kBlockFrames, Frames - Origin);

                for (UInt64 Frame = 1; Frame < Length; ++Frame)
                {
                    const SInt16 Value  = Quantize(Samples[(Origin + Frame) * Stride + Channel]);
                    const UInt8  Nibble = Compress(Predictor, Steps[Channel], Value);
                    const UInt64 Slot   = 4 + ((Frame - 1) >> 1);

                    // Two samples share a byte, the earlier one in the low nibble.
                    Cursor[Slot] |= ((Frame - 1) & 1) ? (Nibble << 4) : Nibble;
                }
            }
        }
        return Payload;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Exporter::EncodeOpus(ConstRef<Sample> Source, SInt32 Bitrate)
    {
        constexpr UInt32 kPacketFrames = 960;
        constexpr UInt32 kPacketBytes  = 4000;

        const UInt16 Stride = Source.GetStride();

        SInt32                 Result  = 0;
        const Ptr<OpusEncoder> Encoder = ::opus_encoder_create(
            Source.GetFrequency(), Stride, OPUS_APPLICATION_AUDIO, AddressOf(Result));

        if (Result != OPUS_OK)
        {
            LOG_E("Sound: failed to create the Opus encoder ({0})", StrConvert(::opus_strerror(Result)));

            return Blob();
        }

        ::opus_encoder_ctl(Encoder, OPUS_SET_BITRATE(Bitrate));

        // The decoder has to discard exactly the encoder's lookahead, so it is stored as the pre-skip.
        opus_int32 Lookahead = 0;
        ::opus_encoder_ctl(Encoder, OPUS_GET_LOOKAHEAD(AddressOf(Lookahead)));

        const ConstSpan<Real32> Samples = Source.GetSamples();
        const UInt64            Frames  = Source.GetFrames();

        // Encoding one lookahead's worth of silence past the end flushes the tail, so the last frames survive.
        const UInt64 Total   = Frames + static_cast<UInt64>(Lookahead);
        const UInt64 Packets = (Total + kPacketFrames - 1) / kPacketFrames;

        Sequence<UInt16> Lengths(Packets);
        Writer           Encoded(static_cast<UInt32>(Packets * 256));

        Blob              Staging = Blob::Allocate<Real32>(kPacketFrames * Stride);
        Blob              Packet  = Blob::Allocate<Byte>(kPacketBytes);
        const Ptr<Real32> Input   = Staging.GetData<Real32>();

        for (UInt64 Index = 0; Index < Packets; ++Index)
        {
            const UInt64 Origin = Index * kPacketFrames;

            // Opus only takes whole frames, so the last packet is padded with silence rather than cut short.
            Zero(Input, kPacketFrames * Stride);

            if (Origin < Frames)
            {
                const UInt64 Length = Min<UInt64>(kPacketFrames, Frames - Origin);
                Copy(Input, Length * Stride, Samples.GetData() + Origin * Stride);
            }

            const SInt32 Written
                = ::opus_encode_float(Encoder, Input, kPacketFrames, Packet.GetData<Byte>(), kPacketBytes);

            if (Written < 0)
            {
                LOG_E("Sound: failed to encode an Opus packet ({0})", StrConvert(::opus_strerror(Written)));

                ::opus_encoder_destroy(Encoder);
                return Blob();
            }

            Lengths.Append(static_cast<UInt16>(Written));
            Encoded.Write<Byte>(Packet.GetData<Byte>(), static_cast<UInt32>(Written));
        }

        ::opus_encoder_destroy(Encoder);

        const Blob Body = Encoded.Detach();

        // The directory comes before the packets, so a seek can find its packet without walking the others.
        Writer Output(static_cast<UInt32>(Body.GetSize() + Lengths.GetSize() * sizeof(UInt16) + 16));
        Output.Write<UInt16>(static_cast<UInt16>(Lookahead));
        Output.Write<UInt16>(kPacketFrames);
        Output.Write<UInt32>(static_cast<UInt32>(Lengths.GetSize()));

        for (const UInt16 Length : Lengths)
        {
            Output.Write<UInt16>(Length);
        }

        // The two-argument overload appends raw bytes, where the one-argument form would serialize the value itself.
        Output.Write<Byte>(Body.GetData<Byte>(), static_cast<UInt32>(Body.GetSize()));
        return Output.Detach();
    }
}