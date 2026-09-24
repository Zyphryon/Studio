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

#include "Preset.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool Find(Text Content, Text Name, Ref<Sequence<Str>> Output)
    {
        const JsonValue Document = JsonDocument::Parse(Content);

        if (!Document.IsObject())
        {
            return false;
        }

        ConstRef<JsonValue::Object> Presets = Document.GetObject();

        for (UInt Index = 0; Index < Presets.GetSize(); ++Index)
        {
            ConstRef<JsonValue::Object::Pair> Entry = Presets.GetData()[Index];

            if (!StrEqualCaseInsensitive(Entry.First, Name) || !Entry.Second.IsObject())
            {
                continue;
            }

            ConstRef<JsonValue::Object> Switches = Entry.Second.GetObject();

            for (UInt Switch = 0; Switch < Switches.GetSize(); ++Switch)
            {
                ConstRef<JsonValue::Object::Pair> Value = Switches.GetData()[Switch];

                // Spelled with an equals sign, so a switch never takes the word after it as its value.
                Str Word("--");
                Word.Append(Value.First);
                Word.Append('=');

                if (Value.Second.IsBool())
                {
                    Word.Append(Value.Second.GetBool() ? "true"_Text : "false"_Text);
                }
                else if (Value.Second.IsNumber())
                {
                    Word.Append(Str::Print<"{0}">(Value.Second.GetNumber<Real64>()));
                }
                else
                {
                    Word.Append(Value.Second.GetString());
                }
                Output.Append(Move(Word));
            }
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Preset::Expand(ConstRef<Environment> Parsed, Ref<Sequence<Str>> Output)
    {
        const Text Name = Parsed.GetText("preset", Text::Empty());

        if (Name.IsEmpty())
        {
            return !Parsed.Contains("preset");
        }

        if (const Text File = Parsed.GetText("presets", Text::Empty()); !File.IsEmpty())
        {
            Blob Input;

            if (Filesystem::Read(File, Input) != Filesystem::Result::Success || Input == nullptr)
            {
                LOG_E("Studio: failed to read the presets '{0}'", File);

                return false;
            }

            if (Find(Text(Input.GetData<Char>(), Input.GetSize()), Name, Output))
            {
                return true;
            }
        }

        if (Find(kBuiltin, Name, Output))
        {
            return true;
        }

        LOG_E("Studio: '{0}' is not a preset; the built-in ones are sprite, normal and relief", Name);
        return false;
    }
}