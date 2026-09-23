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

#include "FontDomain.hpp"
#include "Studio.Application/Path.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    FontDomain::FontDomain(Ref<ZyJob::Service> Scheduler)
        : mBaker { Scheduler }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool FontDomain::Claims(Text Command) const
    {
        return Find(Command) != nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool FontDomain::Reads(Text Source) const
    {
        return mBaker.Find(Path::GetExtension(Source)) != nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome FontDomain::Run(Text Command, ConstRef<Environment> Parsed) const
    {
        const ConstPtr<Entry> Found = Find(Command);

        return Found ? (this->*Found->Handler)(Parsed) : Outcome::Misuse;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void FontDomain::Describe() const
    {
        ConstRef<Font::Baker::Registry> Registry = mBaker.GetRegistry();

        Str Types;

        for (UInt Index = 0; Index < Registry.GetSize(); ++Index)
        {
            if (!Types.IsEmpty())
            {
                Types.Append(", ");
            }
            Types.Append(Registry.GetData()[Index].First);
        }

        LOG_I("Font commands (sources: {0}):", Types);
        LOG_I("");
        LOG_I("  bake   <source> [destination]            Bakes one typeface into a '.{0}'", Font::Exporter::kOutput);
        LOG_I("");
        LOG_I("Font switches:");
        LOG_I("  --size <px>          Em size the glyph fields are generated at          (default: 40)");
        LOG_I("  --range <texels>     Width of the band the distance fades across        (default: 20)");
        LOG_I("  --angle <radians>    Join angle past which an edge counts as a corner   (default: 3)");
        LOG_I("  --charset <ranges>   Codepoints to bake, such as latin1,0x2010-0x205E   (default: ascii)");
        LOG_I("  --face <index>       Typeface to read out of a collection               (default: 0)");
        LOG_I("  --padding <texels>   Gap left between neighbouring glyphs               (default: 1)");
        LOG_I("  --limit <texels>     Largest atlas page before another one opens        (default: 8192)");
        LOG_I("  --underline <em>     Total vertical space an underline takes            (default: 1.2)");
        LOG_I("");
        LOG_I("The charset lists presets (ascii, latin1, punctuation), codepoints and 'first-last' ranges,");
        LOG_I("separated by commas; numbers are decimal unless they start with '0x'. The range is also how far");
        LOG_I("an outline or glow effect can reach, and it pads every glyph, so raise the size, not the range,");
        LOG_I("when glyphs look soft.");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<FontDomain::Entry> FontDomain::Find(Text Name)
    {
        static constexpr Entry kCommands[] =
        {
            { "bake", &FontDomain::Bake },
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

    Outcome FontDomain::Bake(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        if (Operands.GetSize() < 2)
        {
            LOG_E("Font: 'bake' takes a typeface and, optionally, a destination");

            return Outcome::Misuse;
        }

        // A charset that cannot be read has already said why, and leaves nothing to bake.
        const Font::Profile Settings = Font::Profile::From(Parsed);

        if (Settings.Charset.IsEmpty())
        {
            return Outcome::Misuse;
        }

        const Text Source      = Operands[1];
        const Str  Derived     = Path::Derive(Source, Text::Empty(), Font::Exporter::kOutput);
        const Text Destination = (Operands.GetSize() > 2) ? Operands[2] : Text(Derived);

        return mBaker.Bake(Source, Destination, Settings) ? Outcome::Success : Outcome::Failure;
    }
}