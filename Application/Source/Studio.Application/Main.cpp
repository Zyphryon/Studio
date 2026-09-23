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

#include "Studio.Application/Command/FontDomain.hpp"
#include "Studio.Application/Command/SoundDomain.hpp"
#include "Studio.Application/Command/TextureDomain.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static constexpr Text kVersion = "1.0.0";

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Usage(ConstRef<TextureDomain> Textures, ConstRef<FontDomain> Fonts, ConstRef<SoundDomain> Sounds)
    {
        LOG_I("Usage: Studio <command> <operands> [switches]");
        LOG_I("");
        LOG_I("'bake' is shared by every domain: the source's extension picks the one that bakes it.");
        LOG_I("");
        Textures.Describe();
        LOG_I("");
        Fonts.Describe();
        LOG_I("");
        Sounds.Describe();
        LOG_I("");
        LOG_I("Other commands:");
        LOG_I("  version                                  Prints the revision of every file Studio writes");
        LOG_I("  help                                     Prints this text");
        LOG_I("");
        LOG_I("Operands go before switches, since a switch takes the word after it as its value.");
        LOG_I("Every toggle may be negated with its '--no-' spelling, as in '--no-mipmaps'.");
        LOG_I("Exit codes: 0 when everything was done, 1 when some of it failed, 2 when the command was wrong.");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Version()
    {
        LOG_I("Studio {0}", kVersion);
        LOG_I("  texture '.{0}' revision {1}", Texture::Exporter::kOutput, Texture::Exporter::kVersion);
        LOG_I("  tracker '.json' revision {0}", Texture::Tracker::kVersion);
        LOG_I("  font    '.{0}' revision {1}", Font::Exporter::kOutput, Font::Exporter::kVersion);
        LOG_I("  sound   '.{0}' revision {1}", Sound::Exporter::kOutput, Sound::Exporter::kVersion);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Outcome Dispatch(
        ConstRef<TextureDomain> Textures,
        ConstRef<FontDomain>    Fonts,
        ConstRef<SoundDomain>   Sounds,
        ConstRef<Environment>   Parsed)
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        if (Parsed.Contains("version") || (!Operands.IsEmpty() && Operands[0] == "version"))
        {
            Version();
            return Outcome::Success;
        }

        // With no command Studio will open its GUI; until that exists, it prints the usage instead.
        if (Operands.IsEmpty() || Operands[0] == "help" || Parsed.Contains("help"))
        {
            Usage(Textures, Fonts, Sounds);
            return Operands.IsEmpty() ? Outcome::Misuse : Outcome::Success;
        }

        const Text Command = Operands[0];

        // Every domain bakes, so `bake` goes to whichever one reads the source.
        if (Command == "bake")
        {
            if (Operands.GetSize() < 2)
            {
                LOG_E("Studio: 'bake' takes a source and, optionally, a destination");

                return Outcome::Misuse;
            }

            if (Textures.Reads(Operands[1]))
            {
                return Textures.Run(Command, Parsed);
            }
            if (Fonts.Reads(Operands[1]))
            {
                return Fonts.Run(Command, Parsed);
            }
            if (Sounds.Reads(Operands[1]))
            {
                return Sounds.Run(Command, Parsed);
            }

            LOG_E("Studio: no domain bakes '{0}'; run 'Studio help' for the sources each one reads", Operands[1]);
            return Outcome::Misuse;
        }

        if (Textures.Claims(Command))
        {
            return Textures.Run(Command, Parsed);
        }
        if (Fonts.Claims(Command))
        {
            return Fonts.Run(Command, Parsed);
        }
        if (Sounds.Claims(Command))
        {
            return Sounds.Run(Command, Parsed);
        }

        LOG_E("Studio: '{0}' is not a command; run 'Studio help' for the list", Command);
        return Outcome::Misuse;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Outcome Run(UInt Count, ConstPtr<ConstPtr<Char>> Arguments)
    {
        ZyEngine::Subsystem::Host      Host;
        const Retainer<ZyJob::Service> Scheduler = Host.Register<ZyJob::Service>();

        Environment Parsed;
        Parsed.Parse(Count, Arguments);

        const Outcome Result = Dispatch(TextureDomain(* Scheduler), FontDomain(* Scheduler), SoundDomain(), Parsed);

        Host.Teardown();
        return Result;
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

int main(int Count, char * Arguments[])
{
    const Studio::Application::Outcome Result = Studio::Application::Run(static_cast<UInt>(Count), Arguments);

    ZyLog::Flush();
    return static_cast<int>(Result);
}