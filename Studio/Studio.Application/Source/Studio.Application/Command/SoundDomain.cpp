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

#include "SoundDomain.hpp"
#include "Studio.Application/Path.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool ReadProfile(ConstRef<Environment> Parsed, Ref<Sound::Profile> Output)
    {
        Output = Sound::Profile::From(Parsed);

        // An encoding name that is not recognized reads as the default, so it is refused here instead of silently
        // baking linear samples.
        const Text Encoding = Parsed.GetText("encoding", Text::Empty());

        if (!Encoding.IsEmpty() && !StrEqualCaseInsensitive(ZyEnum::GetName(Output.Encoding), Encoding))
        {
            LOG_E("Sound: '{0}' is not an encoding; use Linear, Adaptive or Opus", Encoding);

            return false;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool SoundDomain::Claims(Text Command) const
    {
        return Find(Command) != nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool SoundDomain::Reads(Text Source) const
    {
        return Sound::Importer::Accepts(Path::GetExtension(Source));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome SoundDomain::Run(Text Command, ConstRef<Environment> Parsed) const
    {
        const ConstPtr<Entry> Found = Find(Command);

        return Found ? (this->*Found->Handler)(Parsed) : Outcome::Misuse;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void SoundDomain::Describe() const
    {
        Str Types;

        for (const Text Type : Sound::Importer::kTypes)
        {
            if (!Types.IsEmpty())
            {
                Types.Append(", ");
            }
            Types.Append(Type);
        }

        LOG_I("Sound commands (sources: {0}):", Types);
        LOG_I("");
        LOG_I("  bake   <source> [destination]            Bakes one sound into a '.{0}'", Sound::Exporter::kOutput);
        LOG_I("");
        LOG_I("Sound switches:");
        LOG_I("  --encoding <e>       Linear (16-bit), Adaptive (IMA ADPCM) or Opus      (default: Linear)");
        LOG_I("  --bitrate <n>        The rate Opus aims for, in bits per second         (default: 96000)");
        LOG_I("  --compressed         LZ4-compress the payload when it shrinks the output (default: on)");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<SoundDomain::Entry> SoundDomain::Find(Text Name)
    {
        static constexpr Entry kCommands[] =
        {
            { "bake", &SoundDomain::Bake },
        };

        for (ConstRef<Entry> Command : kCommands)
        {
            if (Command.Name == Name)
            {
                return AddressOf(Command);
            }
        }
        return nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome SoundDomain::Bake(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        if (Operands.GetSize() < 2)
        {
            LOG_E("Sound: 'bake' takes a source sound and, optionally, a destination");

            return Outcome::Misuse;
        }

        Sound::Profile Settings;

        if (!ReadProfile(Parsed, Settings))
        {
            return Outcome::Misuse;
        }

        const Text Source      = Operands[1];
        const Str  Derived     = Path::Derive(Source, Text::Empty(), Sound::Exporter::kOutput);
        const Text Destination = (Operands.GetSize() > 2) ? Operands[2] : Text(Derived);

        return mBaker.Bake(Source, Destination, Settings) ? Outcome::Success : Outcome::Failure;
    }
}