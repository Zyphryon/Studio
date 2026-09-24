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

#include "Profile.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool ReadCodepoint(Text Value, Ref<UInt32> Output)
    {
        const Bool Hex    = Value.GetSize() > 2 && Value[0] == '0' && (Value[1] == 'x' || Value[1] == 'X');
        UInt       Cursor = Hex ? 2 : 0;

        Output = Hex ? StrExtractNumber<16, UInt32>(Value, Cursor) : StrExtractNumber<10, UInt32>(Value, Cursor);

        // Anything left unread, such as a stray letter, makes the whole value unreadable.
        return !Value.IsEmpty() && Cursor == Value.GetSize();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool ReadEntry(Text Entry, Ref<Sequence<Interval>> Output)
    {
        // The presets name the ranges a Latin bake almost always wants, which are tedious to spell out.
        if (StrEqualCaseInsensitive(Entry, "ascii"))
        {
            Output.Append(Interval(0x20, 0x7E));
            return true;
        }
        if (StrEqualCaseInsensitive(Entry, "latin1"))
        {
            Output.Append(Interval(0x20, 0x7E));
            Output.Append(Interval(0xA0, 0xFF));
            return true;
        }
        if (StrEqualCaseInsensitive(Entry, "punctuation"))
        {
            Output.Append(Interval(0x2010, 0x205E));
            return true;
        }

        // No codepoint is negative, so a dash anywhere past the first character can only split a range.
        UInt Split = 1;

        while (Split < Entry.GetSize() && Entry[Split] != '-')
        {
            ++Split;
        }

        UInt32 Minimum = 0;
        UInt32 Maximum = 0;

        if (!ReadCodepoint(StrTrim(Entry.Slice(0, Split)), Minimum))
        {
            return false;
        }

        if (Split >= Entry.GetSize())
        {
            Maximum = Minimum;
        }
        else if (!ReadCodepoint(StrTrim(Entry.Slice(Split + 1)), Maximum) || Maximum < Minimum)
        {
            return false;
        }

        Output.Append(Interval(Minimum, Maximum));
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Profile Profile::From(ConstRef<Environment> Environment)
    {
        Profile Result;
        Result.Size      = Environment.GetNumber<Real32>("size",      Result.Size);
        Result.Range     = Environment.GetNumber<Real32>("range",     Result.Range);
        Result.Angle     = Environment.GetNumber<Real32>("angle",     Result.Angle);
        Result.Underline = Environment.GetNumber<Real32>("underline", Result.Underline);
        Result.Face      = Environment.GetNumber<UInt32>("face",      Result.Face);
        Result.Padding   = Environment.GetNumber<UInt32>("padding",   Result.Padding);
        Result.Limit     = Environment.GetNumber<UInt32>("limit",     Result.Limit);

        Parse(Environment.GetText("charset", "ascii"), Result.Charset);

        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Profile::Parse(Text Description, Ref<Sequence<Interval>> Output)
    {
        Sequence<Interval> Parsed;
        Bool               Valid = true;

        StrSplit(Description, ',', [&](Text Token)
        {
            if (const Text Entry = StrTrim(Token); !Entry.IsEmpty() && !ReadEntry(Entry, Parsed))
            {
                LOG_E("Font: '{0}' is not a codepoint, a range, or a known charset", Entry);

                Valid = false;
            }
            return Valid;
        });

        if (!Valid)
        {
            return false;
        }

        if (Parsed.IsEmpty())
        {
            LOG_E("Font: the charset selects no codepoint at all");

            return false;
        }

        Output = Move(Parsed);
        return true;
    }
}