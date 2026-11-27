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

#include "Usage.hpp"
#include <Font/Baker.hpp>
#include <Model/Baker.hpp>
#include <Sound/Baker.hpp>
#include <Texture/Baker.hpp>
#include <stdio.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Str List(ConstSpan<Text> Types)
    {
        Str Output;

        for (const Text Type : Types)
        {
            if (!Output.IsEmpty())
            {
                Output.Append(", ");
            }
            Output.Append(Type);
        }
        return Output;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Usage::Print()
    {
        Write("Usage: Studio <command> <operands> [switches]");
        Write("");
        Write("A switch with a value takes the word after it, or the text after '=', and a toggle takes none.");
        Write("A toggle is turned off with its '--no-' spelling, as in '--no-mipmaps'. Defaults are in brackets.");
        Write("Exit codes: 0 when everything was done, 1 when some of it failed, 2 when the command was wrong.");
        Write("");
        Write("Commands:");
        Write("  help                            Print this text");
        Write("  version                         Print the revision of every file Studio writes");
        Write("  bake <source|folder>...         Bake each texture, font, model or sound, picked by its extension");
        Write("");
        Write("Every command:");
        Show(Section::Every);
        Write("  A switch typed on the command line wins over the same switch in a preset.");
        Write("");
        Write("Bake switches:");
        Show(Section::Bake);
        Write("  A folder bakes each file in it a baker reads, but not its subfolders or the files bakes write.");
        Write("  The files bake in parallel, and the run ends with how many were baked and how many failed.");
        Write("");
        Write("Textures ({0}):", List(Texture::Importer::kTypes));
        Write("  bake <source> [destination]     Bake an image into a '.{0}'", Texture::Exporter::kOutput);
        Write("  pack <image|folder>...          Pack images into an atlas or array; '--output' names its tracker");
        Write("  build <tracker.json>            Draw a tracker's texture again, after its regions were edited");
        Write("  normal <source> [destination]   Draw a normal map, as a '.png' or a '.{0}'",
            Texture::Exporter::kOutput);
        Write("  relief <source> [destination]   Draw a relief map, as a '.png' or a '.{0}'",
            Texture::Exporter::kOutput);
        Write("  extract <texture> [destination] Write each slice as a '.png', numbered '_0', '_1'... if several");
        Write("  formats                         List every name '--format' accepts");
        Write("");
        Write("Texture switches (bake, pack, build; normal and relief into a '.{0}'):", Texture::Exporter::kOutput);
        Show(Section::Texture);
        Write("");
        Write("Pack switches (pack, and build with '--repack'; build always reads '--fill' and '--headroom'):");
        Show(Section::Pack);
        Write("  A repack keeps the tracker's mode, fit, spacing, pages and slice size, unless they are typed.");
        Write("");
        Write("Normal switches:");
        Show(Section::Normal);
        Write("");
        Write("Relief switches (and '--channel', '--invert', '--blur', '--wrap', '--masked' as for normal maps):");
        Show(Section::Relief);
        Write("  Masked pixels sink to the bottom of a relief.");
        Write("");
        Write("Fonts ({0}):", List(Font::Importer::kTypes));
        Write("  bake <source> [destination]     Bake a typeface into a '.{0}'", Font::Exporter::kOutput);
        Write("");
        Write("Font switches:");
        Show(Section::Font);
        Write("  A charset lists presets (ascii, latin1, punctuation), codepoints and 'first-last' ranges, split by");
        Write("  commas; numbers are decimal unless they start with '0x'. The range also pads every glyph and is how");
        Write("  far an outline or glow can reach, so raise the size, not the range, when glyphs look soft.");
        Write("");
        Write("Models ({0}):", List(Model::Importer::kTypes));
        Write("  bake <source> [destination]     Bake a scene into a '.{0}', '.{1}', '.{2}' and a '.{3}' per clip",
            Model::Exporter::kOutput, Model::Exporter::kMeshOutput, Model::Exporter::kSkeletonOutput,
            Model::Exporter::kAnimationOutput);
        Write("");
        Write("Model switches:");
        Show(Section::Model);
        Write("  The attribute list names Position, Normal, Tangent, TexCoord0, TexCoord1, Color0, BlendIndices and");
        Write("  BlendWeights; a bracketed run shares one interleaved stream and a bare name is a stream of its own,");
        Write("  as in '[Position],[Normal,Tangent,TexCoord0]'. It has to name Position, and an attribute the scene");
        Write("  lacks is left out. Cubic fits a spline through the samples for free; Hermite stores the source's");
        Write("  slope at each one and writes every sample three times over.");
        Write("  Every node is flattened into one mesh with its levels of detail, one primitive per material. The");
        Write("  manifest carries every material inline, its images renamed to the '.tex' the texture bake writes.");
        Write("");
        Write("Sounds ({0}):", List(Sound::Importer::kTypes));
        Write("  bake <source> [destination]     Bake a sound into a '.{0}'", Sound::Exporter::kOutput);
        Write("");
        Write("Sound switches:");
        Show(Section::Sound);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Usage::Write(Text Line)
    {
        fwrite(Line.GetData(), 1, Line.GetSize(), stdout);
        fputc('\n', stdout);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Usage::Spell(UInt Count, ConstPtr<ConstPtr<Char>> Arguments, Ref<Sequence<Str>> Output)
    {
        for (UInt Index = 0; Index < Count; ++Index)
        {
            const Text Word = StrConvert(Arguments[Index]);
            Ref<Str>   Copy = Output.Append(Word);

            if (Index == 0 || !StrStartsWith(Word, "--"))
            {
                continue;
            }

            // Split as the engine reads it, a value after an equals sign and a toggle turned off by `--no-`.
            const Text Switch  = Word.Slice(2);
            const SInt Equals  = StrFind(Switch, '=');
            const Text Spelled = (Equals >= 0) ? Switch.Slice(0, Equals) : Switch;
            const Bool Negated = Equals < 0 && StrStartsWith(Spelled, "no-");
            const Text Name    = Negated ? Spelled.Slice(3) : Spelled;

            const ConstPtr<Option> Found = Find(Name);

            if (Found == nullptr)
            {
                LOG_E("Studio: '--{0}' is not a switch; run 'Studio help' for the list", Name);

                return false;
            }

            if (!Found->Hint.IsEmpty() && Negated)
            {
                LOG_E("Studio: '--{0}' takes a value, so it has no '--no-' spelling", Name);

                return false;
            }

            if (Found->Hint.IsEmpty() && Equals < 0 && !Negated)
            {
                Copy.Append("=true");
            }
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Usage::Check(ConstRef<Environment> Typed, ConstRef<Environment> Parsed, Text Command, UInt16 Reader)
    {
        for (ConstRef<Option> Entry : kOptions)
        {
            if (!HasAnyBit(Entry.Readers, Reader))
            {
                // A preset may carry switches for other commands, so only a typed one is refused.
                if (Typed.Contains(Entry.Name) && !Reads(Entry.Name, Reader))
                {
                    LOG_E("Studio: '{0}' does not read '--{1}'", Command, Entry.Name);

                    return false;
                }
                continue;
            }

            const Text Value = Parsed.GetText(Entry.Name, Text::Empty());

            // A value that is not a number or a choice reads as something else, so it is refused here instead.
            if (Entry.Test != nullptr && Parsed.Contains(Entry.Name) && !Entry.Test(Value))
            {
                LOG_E("Studio: '{0}' is not a value '--{1}' takes", Value, Entry.Name);

                return false;
            }
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Usage::Option> Usage::Find(Text Name)
    {
        for (ConstRef<Option> Entry : kOptions)
        {
            if (StrEqualCaseInsensitive(Entry.Name, Name))
            {
                return AddressOf(Entry);
            }
        }
        return nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Usage::Reads(Text Name, UInt16 Reader)
    {
        for (ConstRef<Option> Entry : kOptions)
        {
            if (HasAnyBit(Entry.Readers, Reader) && StrEqualCaseInsensitive(Entry.Name, Name))
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Usage::Show(Section Group)
    {
        for (ConstRef<Option> Entry : kOptions)
        {
            if (Entry.Group != Group)
            {
                continue;
            }

            const Str Spelling = Entry.Hint.IsEmpty()
                ? Str::Print("--{0}", Entry.Name)
                : Str::Print("--{0} {1}", Entry.Name, Entry.Hint);

            if (Entry.Default.IsEmpty())
            {
                Write("  {0:21} {1}", Spelling, Entry.Help);
            }
            else
            {
                Write("  {0:21} {1:55} [{2}]", Spelling, Entry.Help, Entry.Default);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Usage::IsToggle(Text Value)
    {
        UInt Cursor = 0;
        StrExtractBool(Value, Cursor);

        return Cursor == Value.GetSize();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Usage::IsNumber(Text Value)
    {
        UInt Cursor = 0;
        StrExtractNumber<Real64>(Value, Cursor);

        return Cursor > 0 && Cursor == Value.GetSize();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Usage::IsExtent(Text Value)
    {
        UInt         Cursor = 0;
        const UInt32 Width  = StrExtractNumber<10, UInt32>(Value, Cursor);

        if (Cursor >= Value.GetSize() || (Value[Cursor] != 'x' && Value[Cursor] != 'X'))
        {
            return false;
        }
        ++Cursor;

        const UInt32 Height = StrExtractNumber<10, UInt32>(Value, Cursor);

        return Cursor == Value.GetSize() && Width > 0 && Height > 0 && Width <= 0xFFFF && Height <= 0xFFFF;
    }
}