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

#include "Console.hpp"
#include "Preset.hpp"
#include "Usage.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Outcome Run(UInt Count, ConstPtr<ConstPtr<Char>> Arguments)
    {
        Sequence<Str> Spelled;

        if (!Usage::Spell(Count, Arguments, Spelled))
        {
            return Outcome::Misuse;
        }

        Sequence<ConstPtr<Char>> Words;

        for (ConstRef<Str> Word : Spelled)
        {
            Words.Append(Word.GetData());
        }

        Environment Typed;
        Typed.Parse(Words.GetSize(), Words.GetData());

        Sequence<Str> Switches;

        if (!Preset::Expand(Typed, Switches))
        {
            return Outcome::Misuse;
        }

        // The preset's switches go before the typed ones, and the last of a name is the one read, so typing a
        // switch overrides the preset.
        for (UInt Index = 0; Index < Switches.GetSize(); ++Index)
        {
            Words.Insert(Index + 1, Switches[Index].GetData());
        }

        // A preset's switches are held to the same list as the typed ones, so a misspelt one is refused as well.
        if (Sequence<Str> Unused; !Usage::Spell(Switches.GetSize() + 1, Words.GetData(), Unused))
        {
            return Outcome::Misuse;
        }

        Environment Parsed;
        Parsed.Parse(Words.GetSize(), Words.GetData());

        ZyEngine::Subsystem::Host      Host;
        const Retainer<ZyJob::Service> Scheduler = Host.Register<ZyJob::Service>();

        const Outcome Result = Console(* Scheduler).Run(Parsed, Typed);

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