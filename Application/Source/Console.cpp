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
#include <Texture/Export/Picture.hpp>
#include <Texture/Process/Encoder.hpp>
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

    static Bool ReadFallbacks(ConstRef<Environment> Parsed, Ref<Sequence<Font::Baker::Fallback>> Output)
    {
        Bool Valid = true;

        // The fallbacks are separated by semicolons, each a path with an optional charset after an equals sign.
        StrSplit(Parsed.GetText("fallback", Text::Empty()), ';', [&](Text Token)
        {
            if (const Text Entry = StrTrim(Token); !Entry.IsEmpty())
            {
                const SInt Equals = StrFindLast(Entry, '=');

                Ref<Font::Baker::Fallback> Face = Output.Append();
                Face.Path    = Equals >= 0 ? Entry.Slice(0, Equals) : Entry;
                Face.Charset = Equals >= 0 ? Entry.Slice(Equals + 1) : Text::Empty();

                // The bake reads the charset again, but one it cannot read is the command line's fault.
                Sequence<Font::Interval> Ranges;
                Valid = Valid && (Face.Charset.IsEmpty() || Font::Profile::Parse(Face.Charset, Ranges));
            }
        });
        return Valid;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool ReadDestination(ConstSpan<Text> Operands, Text Suffix, ConstSpan<Text> Types, Ref<Str> Output)
    {
        if (Operands.GetSize() < 2 || Operands.GetSize() > 3)
        {
            LOG_E("Texture: '{0}' takes a source and, optionally, a destination", Operands[0]);

            return false;
        }

        // A destination left out goes beside the source, as the first of the files the command writes.
        if (Operands.GetSize() == 2)
        {
            Output = Path::Derive(Operands[1], Suffix, Types[0]);
            return true;
        }

        for (const Text Type : Types)
        {
            if (StrEqualCaseInsensitive(Path::GetExtension(Operands[2]), Type))
            {
                Output = Operands[2];
                return true;
            }
        }

        LOG_E("Texture: '{0}' is not a destination '{1}' writes", Operands[2], Operands[0]);
        return false;
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
        static constexpr Array<Text, 2> kTypes = { Texture::Picture::kOutput, Texture::Exporter::kOutput };

        Str Destination;

        if (!ReadDestination(Parsed.GetOperands(), Suffix, kTypes, Destination))
        {
            return Outcome::Misuse;
        }

        const Text             Source   = Parsed.GetOperands()[1];
        const Texture::Profile Settings = Texture::Profile::From(Parsed);
        const Texture::Surface Decoded  = Instance.Load(Source, Settings);

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

        // A block format is drawn into its plain source, which the baker then encodes.
        const ZyGraphic::TextureFormat Plain = Texture::Encoder::IsSupported(Output.Format)
            ? Texture::Encoder::GetSource(Output.Format)
            : Output.Format;

        for (ConstRef<Texture::Bitmap> Slice : Decoded.Slices)
        {
            Texture::Bitmap Map = Draw(Texture::Canvas::From(Slice)).To(Plain);

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

    Console::Console(Ref<ZyJob::Service> Scheduler)
        : mScheduler { Scheduler },
          mTextures  { Scheduler },
          mFonts     { Scheduler },
          mModels    { Scheduler }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::Run(ConstRef<Environment> Parsed, ConstRef<Environment> Typed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        if (Parsed.Contains("version"))
        {
            return Version(Parsed);
        }

        // With no command Studio prints the usage, until it has a GUI to open instead.
        if (Operands.IsEmpty() || Parsed.Contains("help"))
        {
            Usage::Print();
            return Parsed.Contains("help") ? Outcome::Success : Outcome::Misuse;
        }

        const ConstPtr<Entry> Found = Find(Operands[0]);

        if (Found == nullptr)
        {
            LOG_E("Studio: '{0}' is not a command; run 'Studio help' for the list", Operands[0]);

            return Outcome::Misuse;
        }

        if (!Usage::Check(Typed, Parsed, Found->Name, Found->Reader))
        {
            return Outcome::Misuse;
        }
        return (this->*Found->Handler)(Parsed);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Console::Entry> Console::Find(Text Name)
    {
        static constexpr Array<Entry, 9> kCommands =
        {
            Entry { "bake",    Usage::kBake,    &Console::Bake    },
            Entry { "pack",    Usage::kPack,    &Console::Pack    },
            Entry { "build",   Usage::kBuild,   &Console::Build   },
            Entry { "normal",  Usage::kNormal,  &Console::Normal  },
            Entry { "relief",  Usage::kRelief,  &Console::Relief  },
            Entry { "extract", Usage::kExtract, &Console::Extract },
            Entry { "formats", Usage::kFormats, &Console::Formats },
            Entry { "version", Usage::kVersion, &Console::Version },
            Entry { "help",    Usage::kHelp,    &Console::Help    },
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

    SInt Console::Pick(Text Type)
    {
        for (UInt Index = 0; Index < kKinds.GetSize(); ++Index)
        {
            if (kKinds[Index].Accepts(Type))
            {
                return static_cast<SInt>(Index);
            }
        }
        return -1;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Console::IsSource(Text Type)
    {
        const SInt Found = Pick(Type);

        return Found >= 0 && !StrEqualCaseInsensitive(Type, kKinds[Found].Output);
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
        Usage::Write("Studio 1.0.0");
        Usage::Write("  texture '.{0}' revision {1}", Texture::Exporter::kOutput, Texture::Exporter::kVersion);
        Usage::Write("  tracker '.json' revision {0}", Texture::Tracker::kVersion);
        Usage::Write("  font    '.{0}' revision {1}", Font::Exporter::kOutput, Font::Exporter::kVersion);
        Usage::Write("  mesh    '.{0}' revision {1}", Model::Exporter::kMeshOutput, Model::Exporter::kMeshVersion);
        Usage::Write("  rig     '.{0}' revision {1}",
            Model::Exporter::kSkeletonOutput, Model::Exporter::kSkeletonVersion);
        Usage::Write("  clip    '.{0}' revision {1}",
            Model::Exporter::kAnimationOutput, Model::Exporter::kAnimationVersion);
        Usage::Write("  sound   '.{0}' revision {1}", Sound::Exporter::kOutput, Sound::Exporter::kVersion);
        return Outcome::Success;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome Console::Bake(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();
        const Text            Folder   = StrTrimRight(StrTrimRight(Parsed.GetText("output", Text::Empty()), '/'), '\\');

        if (Operands.GetSize() < 2 || (Parsed.Contains("output") && Folder.IsEmpty()))
        {
            LOG_E("Studio: 'bake' takes one or more sources or folders, and '--output' takes a folder");

            return Outcome::Misuse;
        }

        // One source and a file of the kind it bakes into is a bake to that file, and any other operand is a source.
        const SInt First  = Pick(Path::GetExtension(Operands[1]));
        const Bool Single = Operands.GetSize() == 3 && Folder.IsEmpty() && First >= 0
            && StrEqualCaseInsensitive(Path::GetExtension(Operands[2]), kKinds[First].Output);

        Sequence<Str> Sources;

        for (UInt Index = 1; Index < (Single ? 2 : Operands.GetSize()); ++Index)
        {
            Collect(Operands[Index], IsSource, Text::Empty(), Sources);
        }

        if (Sources.IsEmpty())
        {
            LOG_E("Studio: nothing to bake; no folder holds a source any baker reads");

            return Outcome::Misuse;
        }

        Sequence<Str> Targets;

        for (ConstRef<Str> Source : Sources)
        {
            const SInt Found = Pick(Path::GetExtension(Source));

            if (Found < 0)
            {
                LOG_E("Studio: nothing bakes '{0}'; run 'Studio help' for the sources each one reads", Source);

                return Outcome::Misuse;
            }

            const Text Output = kKinds[Found].Output;

            if (Single)
            {
                Targets.Append(Operands[2]);
            }
            else if (Folder.IsEmpty())
            {
                Targets.Append(Path::Derive(Source, Text::Empty(), Output));
            }
            else
            {
                Targets.Append(Str::Print("{0}/{1}.{2}", Folder, Path::GetStem(Source), Output));
            }
        }

        // Two sources of one name would write the same file at once, so the run is refused before either starts.
        Sequence<Text> Sorted;

        for (ConstRef<Str> Target : Targets)
        {
            Sorted.Append(Target);
        }

        Sorted.Sort([](ConstRef<Text> Left, ConstRef<Text> Right)
        {
            return StrCompareCaseInsensitive(Left, Right) < 0;
        });

        for (UInt Index = 1; Index < Sorted.GetSize(); ++Index)
        {
            if (StrEqualCaseInsensitive(Sorted[Index - 1], Sorted[Index]))
            {
                LOG_E("Studio: two sources would both be baked into '{0}'", Sorted[Index]);

                return Outcome::Misuse;
            }
        }

        // Every baker's settings are read once, before any file, so a bad switch stops the run before it starts.
        const Texture::Profile Textures = Texture::Profile::From(Parsed);
        const Font::Profile    Fonts    = Font::Profile::From(Parsed);
        const Sound::Profile   Sounds   = Sound::Profile::From(Parsed);

        Model::Profile                  Models;
        Sequence<Font::Baker::Fallback> Faces;

        // A charset or a model switch that cannot be read has already said why.
        if (Fonts.Charset.IsEmpty() || !ReadFallbacks(Parsed, Faces) || !Model::Profile::From(Parsed, Models))
        {
            return Outcome::Misuse;
        }

        const UInt32   Count  = static_cast<UInt32>(Sources.GetSize());
        Atomic<UInt32> Failed { 0 };

        // Each file is a job of its own, cut one to a chunk, and each bake still spreads its own work on the pool.
        mScheduler.Parallel(ZyJob::Lane::Compute, Count, [&](UInt32 Start, UInt32 End)
        {
            for (UInt32 Index = Start; Index < End; ++Index)
            {
                const Text Source = Sources[Index];
                const Text Target = Targets[Index];
                Bool       Baked  = false;

                // The cases follow the order of the bakers in kKinds.
                switch (Pick(Path::GetExtension(Source)))
                {
                case 0:
                    Baked = mTextures.Bake(Source, Target, Textures);
                    break;
                case 1:
                    Baked = mFonts.Bake(Source, Target, Fonts, Faces);
                    break;
                case 2:
                    Baked = mModels.Bake(Source, Target, Models);
                    break;
                case 3:
                    Baked = mSounds.Bake(Source, Target, Sounds);
                    break;
                }

                if (!Baked)
                {
                    ++Failed;
                }
            }
        }, Count);

        const UInt32 Broken = Failed.load();

        LOG_I("Studio: {0} baked, {1} failed", Count - Broken, Broken);
        return Broken > 0 ? Outcome::Failure : Outcome::Success;
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

        const Texture::Profile Settings = Texture::Profile::From(Parsed);

        const Text Folder  = Path::GetFolder(Output);
        const Str  Derived = Path::Derive(Output, Text::Empty(), Texture::Exporter::kOutput);
        const Text Image   = Parsed.GetText("texture", Derived);

        // The texture's settings are kept with the tracker, so a later build writes it the same way.
        Texture::Tracker Atlas;
        Atlas.Mipmaps  = Settings.Mipmaps;
        Atlas.Linear   = Settings.Linear;
        Atlas.Compress = Settings.Compress;
        Atlas.Opacity  = Settings.Opacity;
        Atlas.Cutoff   = Settings.Cutoff;
        Atlas.Normal   = Settings.Normal;

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
            Collect(Operands[Index], Texture::Importer::Accepts, Image, Sources);
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
                    ? Str::Print("{0}_{1:03}", Path::GetStem(Source), Layer)
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

        if (Operands.GetSize() != 2)
        {
            LOG_E("Texture: 'build' takes one tracker");

            return Outcome::Misuse;
        }

        Texture::Profile Settings = Texture::Profile::From(Parsed);

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

        // So does any of the texture's settings, which otherwise carry over from the tracker, its layering included.
        Atlas.Mipmaps     = Parsed.Contains("mipmaps")     ? Settings.Mipmaps  : Atlas.Mipmaps;
        Atlas.Linear      = Parsed.Contains("linear")      ? Settings.Linear   : Atlas.Linear;
        Atlas.Compress    = Parsed.Contains("compressed")  ? Settings.Compress : Atlas.Compress;
        Atlas.Opacity     = Parsed.Contains("transparent") ? Settings.Opacity  : Atlas.Opacity;
        Atlas.Cutoff      = Parsed.Contains("cutoff")      ? Settings.Cutoff   : Atlas.Cutoff;
        Atlas.Normal      = Parsed.Contains("normal")      ? Settings.Normal   : Atlas.Normal;
        Settings.Mipmaps  = Atlas.Mipmaps;
        Settings.Linear   = Atlas.Linear;
        Settings.Compress = Atlas.Compress;
        Settings.Opacity  = Atlas.Opacity;
        Settings.Cutoff   = Atlas.Cutoff;
        Settings.Normal   = Atlas.Normal;
        Settings.Layered  = Parsed.Contains("layered")
            ? Settings.Layered
            : Atlas.Layout == ZyGraphic::TextureLayout::Texture2DArray;

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
            Texture::Packer::Settings Layout = Texture::Packer::Settings::From(Parsed);

            // The layout the tracker was packed with carries over unless the command line changes it or its mode.
            Layout.Layout = Parsed.Contains("mode") ? Layout.Layout : Atlas.Mode;

            if (Layout.Layout == Atlas.Mode)
            {
                const Bool Paged = Atlas.Mode == Texture::Packer::Mode::Atlas && Atlas.Slices > 1;
                const Bool Sized = Atlas.Mode == Texture::Packer::Mode::Array && !Parsed.Contains("extent");

                // A paged atlas keeps the size of its pages, and any other keeps room for the size it already has.
                const UInt16 Wide = Paged ? Atlas.Width  : Max(Layout.Width,  Atlas.Width);
                const UInt16 Tall = Paged ? Atlas.Height : Max(Layout.Height, Atlas.Height);

                Layout.Fit     = Parsed.Contains("fit")     ? Layout.Fit     : Atlas.Fit;
                Layout.Padding = Parsed.Contains("padding") ? Layout.Padding : Atlas.Padding;
                Layout.Extrude = Parsed.Contains("extrude") ? Layout.Extrude : Atlas.Extrude;
                Layout.Pages   = Parsed.Contains("pages")   ? Layout.Pages   : Paged;
                Layout.Width   = Parsed.Contains("width")   ? Layout.Width   : Wide;
                Layout.Height  = Parsed.Contains("height")  ? Layout.Height  : Tall;
                Layout.Extent  = Sized ? Texture::Profile::Extent(Atlas.Width, Atlas.Height) : Layout.Extent;
            }

            // Every place is cleared, so each region is measured and placed from scratch.
            for (Ref<Texture::Region> Entry : Atlas.Regions)
            {
                Entry.Slice = 0;
                Entry.Rect  = Texture::Area();
            }

            if (!Drawer.Measure(Atlas) || !Texture::Packer::Pack(Atlas, Layout))
            {
                return Outcome::Failure;
            }
        }

        // A hand-edited tracker may overlap two regions, and it still draws with the later region on top.
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
        static constexpr Array<Text, 1> kTypes = { Texture::Picture::kOutput };

        Str Destination;

        if (!ReadDestination(Parsed.GetOperands(), Text::Empty(), kTypes, Destination))
        {
            return Outcome::Misuse;
        }

        const Text             Source  = Parsed.GetOperands()[1];
        const Texture::Surface Decoded = mTextures.Load(Source, Texture::Profile::From(Parsed));

        if (!Decoded.IsValid())
        {
            return Outcome::Failure;
        }

        const UInt Count = Decoded.Slices.GetSize();

        for (UInt Index = 0; Index < Count; ++Index)
        {
            ConstRef<Texture::Bitmap> Slice = Decoded.Slices[Index];

            // sRGB colour goes back out as sRGB, and everything else, data included, as the values it holds.
            const Bool sRGB = ZyGraphic::GetTextureMetadata(Slice.GetFormat()).IsSRGB;

            const Str Target = (Count > 1)
                ? Path::Derive(Destination, Str::Print("_{0}", Index), Texture::Picture::kOutput)
                : Destination;

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
                Usage::Write(ZyEnum::GetName(Format));
            }
        }
        return Outcome::Success;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Console::Collect(Text Operand, Bool (* Accepts)(Text Type), Text Skip, Ref<Sequence<Str>> Output) const
    {
        const Text    Folder = StrTrimRight(StrTrimRight(Operand, '/'), '\\');
        Sequence<Str> Found;

        // Only a folder can be listed, so a failed listing means the operand is a file of its own.
        const Filesystem::Result Listed = Filesystem::Enumerate(Folder, [&](ConstRef<Filesystem::Record> Record)
        {
            if (Record.Type == Filesystem::Type::File && Accepts(Path::GetExtension(Record.Name)))
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