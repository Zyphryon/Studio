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
        if (Value.IsEmpty())
        {
            return false;
        }

        const Bool Hex   = Value.GetSize() > 2 && Value[0] == '0' && (Value[1] == 'x' || Value[1] == 'X');
        const UInt Start = Hex ? 2 : 0;

        UInt32 Result = 0;

        for (UInt Index = Start; Index < Value.GetSize(); ++Index)
        {
            const Char Digit = Value[Index];
            UInt32     Place;

            if (Digit >= '0' && Digit <= '9')
            {
                Place = static_cast<UInt32>(Digit - '0');
            }
            else if (Hex && Digit >= 'a' && Digit <= 'f')
            {
                Place = static_cast<UInt32>(Digit - 'a') + 10;
            }
            else if (Hex && Digit >= 'A' && Digit <= 'F')
            {
                Place = static_cast<UInt32>(Digit - 'A') + 10;
            }
            else
            {
                return false;
            }
            Result = Result * (Hex ? 16 : 10) + Place;
        }

        Output = Result;
        return true;
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

        for (UInt Cursor = 0; Cursor <= Description.GetSize(); )
        {
            // Everything up to the next comma is one entry; the last entry runs to the end.
            UInt Split = Cursor;

            while (Split < Description.GetSize() && Description[Split] != ',')
            {
                ++Split;
            }

            if (const Text Entry = StrTrim(Description.Slice(Cursor, Split - Cursor)); !Entry.IsEmpty())
            {
                if (!ReadEntry(Entry, Parsed))
                {
                    LOG_E("Font: '{0}' is not a codepoint, a range, or a known charset", Entry);

                    return false;
                }
            }
            Cursor = Split + 1;
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