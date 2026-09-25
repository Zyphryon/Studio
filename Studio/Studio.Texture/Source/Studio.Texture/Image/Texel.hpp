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

#include <Zyphryon.Graphic/Metadata.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    /// \brief The most channels a texel carries, which is also the size of the scratch it unpacks into.
    static constexpr UInt32 kMaxComponents = 4;

    /// \brief Specifies how one channel of a texel is stored.
    enum class Component : UInt8
    {
        UInt8,      ///< 8-bit unsigned normalized.
        SInt8,      ///< 8-bit signed normalized.
        UInt16,     ///< 16-bit unsigned normalized.
        SInt16,     ///< 16-bit signed normalized.
        Half,       ///< 16-bit floating-point.
        Real32,     ///< 32-bit floating-point.
    };

    /// \brief Specifies the colour-space conversion a transcode applies between its two ends.
    enum class Gamma : UInt8
    {
        None,       ///< Both ends share a colour space, so values pass as they are.
        Linear,     ///< The source is sRGB-encoded and the target is linear.
        sRGB,       ///< The source is linear and the target is sRGB-encoded.
    };

    /// \brief Gets how the channels of a format are stored.
    ///
    /// \param Format The format to classify.
    /// \return The storage of each channel.
    ZY_INLINE Component GetComponent(ConstRef<ZyGraphic::TextureMetadata> Format)
    {
        const UInt32 Bits = Format.BitsPerComponent();

        if (Format.IsFloat)
        {
            return (Bits == 16) ? Component::Half : Component::Real32;
        }
        if (Format.IsSigned)
        {
            return (Bits == 16) ? Component::SInt16 : Component::SInt8;
        }
        return (Bits == 16) ? Component::UInt16 : Component::UInt8;
    }

    /// \brief Calls a function templated on a channel's storage type, picked from a storage known only at runtime.
    ///
    /// \param Storage The storage of each channel.
    /// \param Select  The function, called as `Select.template operator()<Type>(Index)`.
    /// \param Index   The index handed to \p Select.
    /// \return What \p Select returns.
    template<typename Selector>
    ZY_INLINE auto Dispatch(Component Storage, ConstRef<Selector> Select, UInt32 Index)
    {
        switch (Storage)
        {
        case Component::SInt8:
            return Select.template operator()<SInt8>(Index);
        case Component::UInt16:
            return Select.template operator()<UInt16>(Index);
        case Component::SInt16:
            return Select.template operator()<SInt16>(Index);
        case Component::Half:
            return Select.template operator()<Half>(Index);
        case Component::Real32:
            return Select.template operator()<Real32>(Index);
        default:
            return Select.template operator()<UInt8>(Index);
        }
    }

    /// \brief Gets the colour-space conversion between two ends of a transcode.
    ///
    /// \param Source `true` if the source is sRGB-encoded.
    /// \param Target `true` if the target is sRGB-encoded.
    /// \return The conversion to apply.
    ZY_INLINE Gamma GetGamma(Bool Source, Bool Target)
    {
        if (Source == Target)
        {
            return Gamma::None;
        }
        return Source ? Gamma::Linear : Gamma::sRGB;
    }

    /// \brief Represents the sRGB transfer as lookup tables, so eight-bit channels never evaluate the curve.
    struct Curve final
    {
        /// \brief The number of values an eight-bit channel can hold.
        static constexpr UInt32 kSteps = 256;

        /// \brief The number of entries in the encode hint, which only has to land close to the right byte.
        static constexpr UInt32 kGrid  = 4096;

        /// The linear value of each encoded byte.
        Array<Real32, kSteps>     Linear;

        /// The approximate encoded byte of a linear value, indexed by the value's square root.
        Array<Byte, kGrid>        Hint;

        /// The lowest linear value that encodes to each byte above zero.
        Array<Real32, kSteps - 1> Boundary;

        /// \brief Constructs every table by sampling the engine's own transfer.
        Curve()
        {
            for (UInt32 Step = 0; Step < kSteps; ++Step)
            {
                const Real32 Value = DecodeNormalized(static_cast<UInt8>(Step));

                Linear[Step] = Color(Value, Value, Value, 1.0f).ToLinear().GetRed();
            }

            for (UInt32 Step = 0; Step < kGrid; ++Step)
            {
                const Real32 Root = (static_cast<Real32>(Step) + 0.5f) / static_cast<Real32>(kGrid - 1);

                Hint[Step] = static_cast<Byte>(Sample(Min(Root * Root, 1.0f)));
            }

            for (UInt32 Step = 1; Step < kSteps; ++Step)
            {
                Boundary[Step - 1] = Threshold(Step);
            }
        }

    private:

        /// \brief Encodes a linear value through the engine's transfer and rounds it to the nearest byte.
        ///
        /// \param Value The linear value.
        /// \return The encoded step, from 0 to 255.
        static UInt32 Sample(Real32 Value)
        {
            return EncodeNormalized<UInt8>(Color(Value, Value, Value, 1.0f).ToSRGB().GetRed());
        }

        /// \brief Finds the lowest linear value that encodes to a given step.
        ///
        /// \param Level The encoded step, from 1 to 255.
        /// \return The lowest linear value \ref Sample maps to that step.
        static Real32 Threshold(UInt32 Level)
        {
            // Non-negative floats sort like their bit patterns, so bisecting the bits lands exactly on the boundary.
            UInt32 Low  = 0x00000000;   // 0.0f
            UInt32 High = 0x3F800000;   // 1.0f

            while (Low < High)
            {
                const UInt32 Middle = Low + (High - Low) / 2;

                if (Sample(CastBit<Real32>(Middle)) >= Level)
                {
                    High = Middle;
                }
                else
                {
                    Low = Middle + 1;
                }
            }

            return CastBit<Real32>(Low);
        }
    };

    /// \brief The sRGB tables, built once when the program starts.
    inline const Curve kCurve;

    /// \brief Encodes a linear value as an sRGB byte.
    ///
    /// \param Value The linear value, clamped into unit range.
    /// \return The encoded byte.
    ZY_INLINE Byte EncodeGamma(Real32 Value)
    {
        const Real32 Unit = Clamp(Value, 0.0f, 1.0f);

        UInt32 Step = kCurve.Hint[static_cast<UInt32>(Sqrt(Unit) * (Curve::kGrid - 1))];

        // The hint lands a step or so away, so a short walk against the boundaries finds the exact byte.
        while (Step < Curve::kSteps - 1 && Unit >= kCurve.Boundary[Step])
        {
            ++Step;
        }
        while (Step > 0 && Unit < kCurve.Boundary[Step - 1])
        {
            --Step;
        }
        return static_cast<Byte>(Step);
    }

    /// \brief Decodes one channel as a normalized or floating-point value.
    ///
    /// \tparam Type   The type the channel is stored as.
    /// \param  Source The address of the channel.
    /// \return The decoded value.
    template<typename Type>
    ZY_INLINE Real32 Decode(ConstPtr<Byte> Source)
    {
        if constexpr (IsAnyOf<Type, Half>)
        {
            UInt16 Bits;
            Blit(AddressOf(Bits), sizeof(Bits), Source);

            return Half::FromBits(Bits);
        }
        else if constexpr (IsAnyOf<Type, Real32>)
        {
            Real32 Result;
            Blit(AddressOf(Result), sizeof(Result), Source);

            return Result;
        }
        else
        {
            Type Bits;
            Blit(AddressOf(Bits), sizeof(Bits), Source);

            return DecodeNormalized(Bits);
        }
    }

    /// \brief Encodes one channel from a normalized or floating-point value.
    ///
    /// \tparam Type   The type the channel is stored as.
    /// \param  Target The address of the channel.
    /// \param  Value  The value to write.
    template<typename Type>
    ZY_INLINE void Encode(Ptr<Byte> Target, Real32 Value)
    {
        if constexpr (IsAnyOf<Type, Half>)
        {
            const UInt16 Bits = Half(Value).GetBits();

            Blit(Target, sizeof(Bits), AddressOf(Bits));
        }
        else if constexpr (IsAnyOf<Type, Real32>)
        {
            Blit(Target, sizeof(Value), AddressOf(Value));
        }
        else
        {
            const Type Bits = EncodeNormalized<Type>(Value);

            Blit(Target, sizeof(Bits), AddressOf(Bits));
        }
    }

    /// \brief Reads one texel as a linear-space colour.
    ///
    /// \tparam Type     The type each channel is stored as.
    /// \tparam Channels The number of channels a texel carries.
    /// \tparam sRGB     `true` if the colour channels are sRGB-encoded.
    /// \param  Source   The address of the level.
    /// \param  Index    The texel index within the level.
    /// \return The texel, in linear space.
    template<typename Type, UInt32 Channels, Bool sRGB>
    ZY_INLINE Color Load(ConstPtr<Byte> Source, UInt32 Index)
    {
        const ConstPtr<Byte> Address = Source + static_cast<UInt64>(Index) * Channels * sizeof(Type);

        Array Components(0.0f, 0.0f, 0.0f, 1.0f);

        for (UInt32 Channel = 0; Channel < Channels; ++Channel)
        {
            if constexpr (sRGB && IsAnyOf<Type, UInt8>)
            {
                Components[Channel] = (Channel < 3) ? kCurve.Linear[Address[Channel]] : Decode<Type>(Address + Channel);
            }
            else
            {
                Components[Channel] = Decode<Type>(Address + Channel * sizeof(Type));
            }
        }

        const Color Result(Components[0], Components[1], Components[2], Components[3]);

        if constexpr (sRGB && !IsAnyOf<Type, UInt8>)
        {
            return Result.ToLinear();
        }
        return Result;
    }

    /// \brief Writes one texel from a linear-space colour.
    ///
    /// \tparam Type     The type each channel is stored as.
    /// \tparam Channels The number of channels a texel carries.
    /// \tparam sRGB     `true` if the colour channels are sRGB-encoded.
    /// \param  Target   The address of the level.
    /// \param  Index    The texel index within the level.
    /// \param  Value    The texel to write, in linear space.
    template<typename Type, UInt32 Channels, Bool sRGB>
    ZY_INLINE void Store(Ptr<Byte> Target, UInt32 Index, ConstRef<Color> Value)
    {
        const Ptr<Byte> Address = Target + static_cast<UInt64>(Index) * Channels * sizeof(Type);

        Color Result = Value;

        // Only eight-bit channels have a table, so any other width evaluates the curve itself.
        if constexpr (sRGB && !IsAnyOf<Type, UInt8>)
        {
            Result = Result.ToSRGB();
        }

        Array Components(Result.GetRed(), Result.GetGreen(), Result.GetBlue(), Result.GetAlpha());

        for (UInt32 Channel = 0; Channel < Channels; ++Channel)
        {
            // Alpha is never gamma-encoded, so it keeps the plain path.
            if constexpr (sRGB && IsAnyOf<Type, UInt8>)
            {
                if (Channel < 3)
                {
                    Address[Channel] = EncodeGamma(Components[Channel]);
                }
                else
                {
                    Encode<Type>(Address + Channel, Components[Channel]);
                }
            }
            else
            {
                Encode<Type>(Address + Channel * sizeof(Type), Components[Channel]);
            }
        }
    }
}