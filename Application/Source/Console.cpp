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
#include "Path.hpp"
#include "Usage.hpp"
#include <Texture/Atlas/Packer.hpp>
#include <Texture/Export/Picture.hpp>
#include <Texture/Generate/Normal.hpp>
#include <Texture/Generate/Relief.hpp>
#include <Zyphryon.Graphic/Metadata.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool Store(Text Target, ConstSpan<Byte> Bytes)
    {
        Filesystem::Ensure(Target);

        if (Filesystem::Write(Target, Bytes) != Filesystem::Result::Success)
        {
            LOG_E("Texture: failed to write '{0}'", Target);

            return false;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool ReadProfile(ConstRef<Environment> Parsed, Ref<Texture::Profile> Output)
    {
        Output = Texture::Profile::From(Parsed);

        // A format name that is not recognized reads as `Unspecified`, the same as leaving the switch out, so it is
        // refused here instead of silently falling back to the source's format.
        if (Parsed.Contains("format") && Output.Format == ZyGraphic::TextureFormat::Unspecified)
        {
            LOG_E("Texture: '{0}' is not a recognized format name", Parsed.GetText("format", Text::Empty()));

            return false;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool ReadTracker(ConstRef<Environment> Parsed, Ref<Texture::Tracker> Output)
    {
        Output.Headroom = Parsed.GetNumber<UInt16>("headroom", Output.Headroom);

        if (!Parsed.Contains("fill"))
        {
            return true;
        }

        const Text Value  = Parsed.GetText("fill", Text::Empty());
        const Text Digits = StrStartsWith(Value, "#") ? Value.Slice(1) : Value;

        // FromHexadecimal reads anything as a colour, so the digits are checked first to refuse a typo.
        Bool Valid = Digits.GetSize() == 6 || Digits.GetSize() == 8;

        for (UInt Index = 0; Valid && Index < Digits.GetSize(); ++Index)
        {
            const Char Digit = Digits[Index];

            Valid = (Digit >= '0' && Digit <= '9') || (Digit >= 'a' && Digit <= 'f') || (Digit >= 'A' && Digit <= 'F');
        }

        if (!Valid)
        {
            LOG_E("Texture: '--fill' expects 'RRGGBB' or 'RRGGBBAA' in hexadecimal, which '{0}' is not", Value);

            return false;
        }

        Output.Fill = IntColor8::FromHexadecimal(Value);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Callable>
    static Outcome Generate(
        ConstRef<Texture::Baker> Instance,
        ConstRef<Environment>    Parsed,
        Text                     Suffix,
        AnyRef<Callable>         Draw)
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        if (Operands.GetSize() < 2)
        {
            LOG_E("Texture: '{0}' takes a source image and, optionally, a destination", Operands[0]);

            return Outcome::Misuse;
        }

        Texture::Profile Settings;

        if (!ReadProfile(Parsed, Settings))
        {
            return Outcome::Misuse;
        }

        const Text Source      = Operands[1];
        const Str  Derived     = Path::Derive(Source, Suffix, Texture::Picture::kOutput);
        const Text Destination = (Operands.GetSize() > 2) ? Operands[2] : Text(Derived);

        const Texture::Surface Decoded = Instance.Load(Source, Settings);

        if (!Decoded.IsValid())
        {
            return Outcome::Failure;
        }

        // A native texture keeps every slice, while a picture holds only one.
        if (!StrEqualCaseInsensitive(Path::GetExtension(Destination), Texture::Exporter::kOutput))
        {
            if (Decoded.Slices.GetSize() > 1)
            {
                LOG_E("Texture: '{0}' holds {1} slices, which only a '.{2}' destination keeps",
                    Source, Decoded.Slices.GetSize(), Texture::Exporter::kOutput);

                return Outcome::Failure;
            }

            const Texture::Canvas Map = Draw(Texture::Canvas::From(Decoded.Slices.GetFront()));

            // A map holds data rather than colour, so its values are stored as they are.
            if (!Texture::Picture::Write(Destination, Map, false))
            {
                return Outcome::Failure;
            }

            LOG_I("Texture: '{0}' -> '{1}'", Source, Destination);
            return Outcome::Success;
        }

        // A map holds data rather than colour, so it is written linear unless a format is asked for.
        Texture::Profile Output = Settings;

        if (Output.Format == ZyGraphic::TextureFormat::Unspecified)
        {
            Output.Format = ZyGraphic::TextureFormat::RGBA8UIntNorm;
        }

        Sequence<Texture::Bitmap> Slices;

        for (ConstRef<Texture::Bitmap> Slice : Decoded.Slices)
        {
            Texture::Bitmap Map = Draw(Texture::Canvas::From(Slice)).To(Output.Format);

            if (Map.GetPixels().IsEmpty())
            {
                return Outcome::Failure;
            }
            Slices.Append(Move(Map));
        }

        const Blob Bytes = Instance.Encode(Move(Slices), Output.GetLayout(Decoded.Layout), Output);

        if (Bytes == nullptr || !Store(Destination, Bytes))
        {
            return Outcome::Failure;
        }

        LOG_I("Texture: '{0}' -> '{1}' ({2} bytes)", Source, Destination, Bytes.GetSize());
        return Outcome::Success;
    }

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

    Console::Console(Ref<ZyJob::Service> Scheduler)
        : mTextures { Scheduler },
          mFonts    { Scheduler }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::Run(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        if (Parsed.Contains("version"))
        {
            return Version(Parsed);
        }

        // With no command Studio will open its GUI; until that exists, it prints the usage instead.
        if (Operands.IsEmpty() || Parsed.Contains("help"))
        {
            Usage::Print();
            return Operands.IsEmpty() ? Outcome::Misuse : Outcome::Success;
        }

        const ConstPtr<Entry> Found = Find(Operands[0]);

        if (Found == nullptr)
        {
            LOG_E("Studio: '{0}' is not a command; run 'Studio help' for the list", Operands[0]);

            return Outcome::Misuse;
        }
        return (this->*Found->Handler)(Parsed);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Console::Entry> Console::Find(Text Name)
    {
        static constexpr Entry kCommands[] =
        {
            { "bake",    &Console::Bake    },
            { "pack",    &Console::Pack    },
            { "build",   &Console::Build   },
            { "normal",  &Console::Normal  },
            { "relief",  &Console::Relief  },
            { "extract", &Console::Extract },
            { "formats", &Console::Formats },
            { "version", &Console::Version },
            { "help",    &Console::Help    },
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

    Outcome Console::Help(ConstRef<Environment> Parsed) const
    {
        Usage::Print();
        return Outcome::Success;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::Version(ConstRef<Environment> Parsed) const
    {
        LOG_I("Studio 1.0.0");
        LOG_I("  texture '.{0}' revision {1}", Texture::Exporter::kOutput, Texture::Exporter::kVersion);
        LOG_I("  tracker '.json' revision {0}", Texture::Tracker::kVersion);
        LOG_I("  font    '.{0}' revision {1}", Font::Exporter::kOutput, Font::Exporter::kVersion);
        LOG_I("  sound   '.{0}' revision {1}", Sound::Exporter::kOutput, Sound::Exporter::kVersion);
        return Outcome::Success;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::Bake(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        if (Operands.GetSize() < 2)
        {
            LOG_E("Studio: 'bake' takes a source and, optionally, a destination");

            return Outcome::Misuse;
        }

        const Text Extension = Path::GetExtension(Operands[1]);

        if (Texture::Importer::Accepts(Extension))
        {
            return BakeTexture(Parsed);
        }
        if (Font::Importer::Accepts(Extension))
        {
            return BakeFont(Parsed);
        }
        if (Sound::Importer::Accepts(Extension))
        {
            return BakeSound(Parsed);
        }

        LOG_E("Studio: nothing bakes '{0}'; run 'Studio help' for the sources each one reads", Operands[1]);
        return Outcome::Misuse;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::BakeTexture(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        Texture::Profile Settings;

        if (!ReadProfile(Parsed, Settings))
        {
            return Outcome::Misuse;
        }

        const Text Source      = Operands[1];
        const Str  Derived     = Path::Derive(Source, Text::Empty(), Texture::Exporter::kOutput);
        const Text Destination = (Operands.GetSize() > 2) ? Operands[2] : Text(Derived);

        return mTextures.Bake(Source, Destination, Settings) ? Outcome::Success : Outcome::Failure;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::BakeFont(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        // A charset that cannot be read has already said why, and leaves nothing to bake.
        const Font::Profile Settings = Font::Profile::From(Parsed);

        if (Settings.Charset.IsEmpty())
        {
            return Outcome::Misuse;
        }

        const Text Source      = Operands[1];
        const Str  Derived     = Path::Derive(Source, Text::Empty(), Font::Exporter::kOutput);
        const Text Destination = (Operands.GetSize() > 2) ? Operands[2] : Text(Derived);
        const Text Fallbacks   = Parsed.GetText("fallback", Text::Empty());

        // The fallbacks are separated by semicolons, each a path with an optional charset after an equals sign.
        Sequence<Font::Baker::Fallback> Faces;

        StrSplit(Fallbacks, ';', [&](Text Token)
        {
            if (const Text Entry = StrTrim(Token); !Entry.IsEmpty())
            {
                const SInt Equals = StrFindLast(Entry, '=');

                Ref<Font::Baker::Fallback> Face = Faces.Append();
                Face.Path    = Equals >= 0 ? Entry.Slice(0, Equals) : Entry;
                Face.Charset = Equals >= 0 ? Entry.Slice(Equals + 1) : Text::Empty();
            }
        });

        return mFonts.Bake(Source, Destination, Settings, Faces) ? Outcome::Success : Outcome::Failure;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::BakeSound(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        Sound::Profile Settings;

        if (!ReadProfile(Parsed, Settings))
        {
            return Outcome::Misuse;
        }

        const Text Source      = Operands[1];
        const Str  Derived     = Path::Derive(Source, Text::Empty(), Sound::Exporter::kOutput);
        const Text Destination = (Operands.GetSize() > 2) ? Operands[2] : Text(Derived);

        return mSounds.Bake(Source, Destination, Settings) ? Outcome::Success : Outcome::Failure;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::Pack(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();
        const Text            Output   = Parsed.GetText("output", Text::Empty());

        if (Operands.GetSize() < 2 || Output.IsEmpty())
        {
            LOG_E("Texture: 'pack' takes one or more images or folders, and an '--output <tracker>.json'");

            return Outcome::Misuse;
        }

        Texture::Profile Settings;

        if (!ReadProfile(Parsed, Settings))
        {
            return Outcome::Misuse;
        }

        const Text Folder  = Path::GetFolder(Output);
        const Str  Derived = Path::Derive(Output, Text::Empty(), Texture::Exporter::kOutput);
        const Text Image   = Parsed.GetText("texture", Derived);

        Texture::Tracker Atlas;

        if (!Texture::Tracker::Relate(Folder, Image, Atlas.Texture))
        {
            return Outcome::Misuse;
        }

        if (!ReadTracker(Parsed, Atlas))
        {
            return Outcome::Misuse;
        }

        Sequence<Str> Sources;

        for (UInt Index = 1; Index < Operands.GetSize(); ++Index)
        {
            Collect(Operands[Index], Image, Sources);
        }

        if (Sources.IsEmpty())
        {
            LOG_E("Texture: nothing to pack; no operand names an image this baker understands");

            return Outcome::Misuse;
        }

        Texture::Composer Drawer(mTextures, Folder, Settings);

        for (ConstRef<Str> Source : Sources)
        {
            Str Relative;

            if (!Texture::Tracker::Relate(Folder, Source, Relative))
            {
                return Outcome::Misuse;
            }

            const UInt16 Layers = Drawer.Count(Relative);

            if (Layers == 0)
            {
                return Outcome::Failure;
            }

            // An array brings every slice along, each a region of its own, numbered so the names sort in slice order.
            for (UInt16 Layer = 0; Layer < Layers; ++Layer)
            {
                const Str Name = (Layers > 1)
                    ? Str::Print<"{0}_{1:03}">(Path::GetStem(Source), Layer)
                    : Str(Path::GetStem(Source));

                // A consumer looks a region up by its name, so no two regions may share one.
                for (ConstRef<Texture::Region> Other : Atlas.Regions)
                {
                    if (StrEqualCaseInsensitive(Other.Name, Name))
                    {
                        LOG_E("Texture: '{0}' and '{1}' would both be named '{2}'", Other.Source, Source, Name);

                        return Outcome::Failure;
                    }
                }

                Ref<Texture::Region> Entry = Atlas.Regions.Append();
                Entry.Name   = Name;
                Entry.Source = Relative;
                Entry.Layer  = Layer;
            }
        }

        if (!Drawer.Measure(Atlas) || !Texture::Packer::Pack(Atlas, Texture::Packer::Settings::From(Parsed)))
        {
            return Outcome::Failure;
        }
        return Drawer.Build(Atlas) && Atlas.Write(Output) ? Outcome::Success : Outcome::Failure;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::Build(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        if (Operands.GetSize() < 2)
        {
            LOG_E("Texture: 'build' takes a tracker");

            return Outcome::Misuse;
        }

        Texture::Profile Settings;

        if (!ReadProfile(Parsed, Settings))
        {
            return Outcome::Misuse;
        }

        const Text Output = Operands[1];
        const Text Folder = Path::GetFolder(Output);

        Texture::Tracker Atlas;

        if (!Texture::Tracker::Read(Output, Atlas))
        {
            return Outcome::Failure;
        }

        // A format, fill or headroom typed on the command line wins over the tracker's own.
        if (Settings.Format != ZyGraphic::TextureFormat::Unspecified)
        {
            Atlas.Format = Settings.Format;
        }

        if (!ReadTracker(Parsed, Atlas))
        {
            return Outcome::Misuse;
        }

        if (Atlas.Texture.IsEmpty())
        {
            Atlas.Texture = Path::Derive(Path::GetStem(Output), Text::Empty(), Texture::Exporter::kOutput);
        }

        Texture::Composer Drawer(mTextures, Folder, Settings);

        if (Parsed.GetBool("repack", false))
        {
            // Every place is cleared, so each region is measured and placed from scratch.
            for (Ref<Texture::Region> Entry : Atlas.Regions)
            {
                Entry.Slice = 0;
                Entry.Rect  = Texture::Area();
            }

            Texture::Packer::Settings Layout = Texture::Packer::Settings::From(Parsed);

            // The spacing the tracker was packed with carries over unless the command line changes it.
            if (!Parsed.Contains("padding"))
            {
                Layout.Padding = Atlas.Padding;
            }
            if (!Parsed.Contains("extrude"))
            {
                Layout.Extrude = Atlas.Extrude;
            }

            if (!Drawer.Measure(Atlas) || !Texture::Packer::Pack(Atlas, Layout))
            {
                return Outcome::Failure;
            }
        }

        // A hand-edited tracker may overlap two regions; it still draws, with the later region on top.
        for (UInt Index = 0; Index < Atlas.Regions.GetSize(); ++Index)
        {
            for (UInt Other = Index + 1; Other < Atlas.Regions.GetSize(); ++Other)
            {
                ConstRef<Texture::Region> First  = Atlas.Regions[Index];
                ConstRef<Texture::Region> Second = Atlas.Regions[Other];

                if (First.Slice == Second.Slice && First.Rect.Overlaps(Second.Rect))
                {
                    LOG_W("Texture: '{0}' and '{1}' overlap on slice {2}; the later one is drawn on top",
                        First.Name, Second.Name, First.Slice);
                }
            }
        }

        return Drawer.Build(Atlas) && Atlas.Write(Output) ? Outcome::Success : Outcome::Failure;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::Normal(ConstRef<Environment> Parsed) const
    {
        const Texture::Normal::Settings Knobs = Texture::Normal::Settings::From(Parsed);

        return Generate(mTextures, Parsed, "_normal", [&](ConstRef<Texture::Canvas> Source)
        {
            return Texture::Normal::Generate(Source, Knobs);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::Relief(ConstRef<Environment> Parsed) const
    {
        const Texture::Relief::Settings Knobs = Texture::Relief::Settings::From(Parsed);

        return Generate(mTextures, Parsed, "_relief", [&](ConstRef<Texture::Canvas> Source)
        {
            return Texture::Relief::Generate(Source, Knobs);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::Extract(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        if (Operands.GetSize() < 2)
        {
            LOG_E("Texture: 'extract' takes a texture and, optionally, a destination");

            return Outcome::Misuse;
        }

        Texture::Profile Settings;

        if (!ReadProfile(Parsed, Settings))
        {
            return Outcome::Misuse;
        }

        const Text Source      = Operands[1];
        const Str  Derived     = Path::Derive(Source, Text::Empty(), Texture::Picture::kOutput);
        const Text Destination = (Operands.GetSize() > 2) ? Operands[2] : Text(Derived);

        const Texture::Surface Decoded = mTextures.Load(Source, Settings);

        if (!Decoded.IsValid())
        {
            return Outcome::Failure;
        }

        const UInt Count = Decoded.Slices.GetSize();

        for (UInt Index = 0; Index < Count; ++Index)
        {
            ConstRef<Texture::Bitmap> Slice = Decoded.Slices[Index];

            // sRGB colour goes back out as sRGB; everything else, data included, is written as the values it holds.
            const Bool sRGB = ZyGraphic::GetTextureMetadata(Slice.GetFormat()).IsSRGB;

            const Str Target = (Count > 1)
                ? Path::Derive(Destination, Str::Print<"_{0}">(Index), Texture::Picture::kOutput)
                : Str(Destination);

            if (!Texture::Picture::Write(Target, Texture::Canvas::From(Slice), sRGB))
            {
                return Outcome::Failure;
            }
        }

        LOG_I("Texture: '{0}' -> {1} picture(s) at '{2}'", Source, Count, Destination);
        return Outcome::Success;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::Formats(ConstRef<Environment> Parsed) const
    {
        for (const ZyGraphic::TextureFormat Format : ZyEnum::GetValues<ZyGraphic::TextureFormat>())
        {
            if (Texture::Exporter::IsSupported(Format))
            {
                LOG_I("{0}", ZyEnum::GetName(Format));
            }
        }
        return Outcome::Success;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Console::Collect(Text Operand, Text Skip, Ref<Sequence<Str>> Output) const
    {
        const Text    Folder = StrTrimRight(StrTrimRight(Operand, '/'), '\\');
        Sequence<Str> Found;

        // Only a folder can be listed, so a failed listing means the operand is a file of its own.
        const Filesystem::Result Listed = Filesystem::Enumerate(Folder, [&](ConstRef<Filesystem::Record> Record)
        {
            if (Record.Type == Filesystem::Type::File && Texture::Importer::Accepts(Path::GetExtension(Record.Name)))
            {
                Str File(Folder);
                File.Append('/');
                File.Append(Record.Name);

                if (!StrEqualCaseInsensitive(File, Skip))
                {
                    Found.Append(Move(File));
                }
            }
            return true;
        });

        if (Listed != Filesystem::Result::Success)
        {
            Output.Append(Str(Operand));
            return;
        }

        // A folder lists in whatever order the platform keeps, so it is sorted by name to pack the same every time.
        Found.Sort([](ConstRef<Str> Left, ConstRef<Str> Right)
        {
            return StrCompareCaseInsensitive(Left, Right) < 0;
        });

        for (Ref<Str> File : Found)
        {
            Output.Append(Move(File));
        }
    }
}