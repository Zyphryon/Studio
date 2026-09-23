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

#include "TextureDomain.hpp"
#include "Studio.Application/Path.hpp"
#include <Studio.Texture/Atlas/Packer.hpp>
#include <Studio.Texture/Export/Picture.hpp>
#include <Studio.Texture/Generate/Normal.hpp>
#include <Studio.Texture/Generate/Relief.hpp>
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

    static Bool Overlaps(ConstRef<Texture::Region> First, ConstRef<Texture::Region> Second)
    {
        ConstRef<Texture::Area> A = First.Rect;
        ConstRef<Texture::Area> B = Second.Rect;

        return First.Slice == Second.Slice
            && A.X < B.X + B.Width  && B.X < A.X + A.Width
            && A.Y < B.Y + B.Height && B.Y < A.Y + A.Height;
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

    template<typename Callable>
    static Outcome Generate(ConstRef<Texture::Baker> Instance, ConstRef<Environment> Parsed, Text Suffix, AnyRef<Callable> Draw)
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

    TextureDomain::TextureDomain(Ref<ZyJob::Service> Scheduler)
        : mBaker { Scheduler }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool TextureDomain::Claims(Text Command) const
    {
        return Find(Command) != nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool TextureDomain::Reads(Text Source) const
    {
        return mBaker.Find(Path::GetExtension(Source)) != nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome TextureDomain::Run(Text Command, ConstRef<Environment> Parsed) const
    {
        const ConstPtr<Entry> Found = Find(Command);

        return Found ? (this->*Found->Handler)(Parsed) : Outcome::Misuse;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void TextureDomain::Describe() const
    {
        ConstRef<Texture::Baker::Registry> Registry = mBaker.GetRegistry();

        Str Types;

        for (UInt Index = 0; Index < Registry.GetSize(); ++Index)
        {
            if (!Types.IsEmpty())
            {
                Types.Append(", ");
            }
            Types.Append(Registry.GetData()[Index].First);
        }

        LOG_I("Texture commands (sources: {0}):", Types);
        LOG_I("");
        LOG_I("  bake   <source> [destination]            Bakes one image into a '.{0}'", Texture::Exporter::kOutput);
        LOG_I("  pack   <image|folder>... --output <.json> Packs images into an atlas or array, and writes its tracker");
        LOG_I("  build  <tracker.json>                    Draws a tracker's texture again, after its regions were edited");
        LOG_I("  normal <source> [destination]            Draws a normal map, as a '.png' or a '.{0}'", Texture::Exporter::kOutput);
        LOG_I("  relief <source> [destination]            Draws a relief map, as a '.png' or a '.{0}'", Texture::Exporter::kOutput);
        LOG_I("  extract <texture> [destination]          Writes each slice as a '.png'; several are numbered '_0', '_1'...");
        LOG_I("");
        LOG_I("Texture switches (bake, pack, build, and normal or relief into a '.{0}'):", Texture::Exporter::kOutput);
        LOG_I("  --format <name>      Any texture format name, such as R8UIntNorm or RGBA16Float (default: inferred)");
        LOG_I("  --mipmaps            Generate a full mip chain down to 1x1                      (default: off)");
        LOG_I("  --compressed         LZ4-compress the payload when it shrinks the output        (default: on)");
        LOG_I("  --linear             Treat the source as linear-encoded colour; '--no-linear' for sRGB art (default: on)");
        LOG_I("  --layered            Write a lone image as a one-slice array                    (default: off)");
        LOG_I("  --cube <c>x<r>       bake: cut the source into cube faces, a grid counted in faces");
        LOG_I("  --array <w>x<h>      bake: cut the source into array slices of that size in texels");
        LOG_I("");
        LOG_I("Pack switches (pack, and build with --repack):");
        LOG_I("  --output <path>      The tracker to write; the texture sits beside it       (pack: required)");
        LOG_I("  --texture <path>     The texture to write instead                           (default: the tracker's name)");
        LOG_I("  --mode <m>           Atlas packs regions together; Array gives each a slice (default: Atlas)");
        LOG_I("  --width <n>          The widest a slice may grow, in pixels                 (default: 2048)");
        LOG_I("  --height <n>         The tallest a slice may grow, in pixels                (default: 2048)");
        LOG_I("  --padding <n>        Empty pixels between neighbouring regions              (default: 1)");
        LOG_I("  --extrude <n>        Pixels each region's edge is repeated outward by       (default: 0)");
        LOG_I("  --pot                Round each side of a slice up to a power of two        (default: on)");
        LOG_I("  --square             Keep each slice square                                 (default: off)");
        LOG_I("  --pages              Spill what does not fit onto further slices            (default: off)");
        LOG_I("  --repack             build: place the regions again instead of keeping their places");
        LOG_I("");
        LOG_I("Normal switches:");
        LOG_I("  --channel <c>        Luminance, Red, Green, Blue or Alpha read as the height (default: Luminance)");
        LOG_I("  --filter <k>         Central, Sobel, Scharr or Prewitt slope measure       (default: Sobel)");
        LOG_I("  --strength <x>       How steep a unit of height reads                       (default: 2)");
        LOG_I("  --blur <px>          Smooth the height before measuring it                  (default: 0)");
        LOG_I("  --octaves <n>        Scales measured and added, each twice as coarse, 1-8   (default: 1)");
        LOG_I("  --falloff <x>        Weight of each coarser scale against the one before    (default: 0.5)");
        LOG_I("  --invert             Read dark as raised                                    (default: off)");
        LOG_I("  --flip-x, --flip-y   Mirror an axis; '--flip-y' for green-down programs     (default: off)");
        LOG_I("  --wrap               Wrap around the edges, for tiling textures             (default: off)");
        LOG_I("  --masked             Transparent pixels are ground, so the outline stands up (default: on)");
        LOG_I("");
        LOG_I("Relief switches:");
        LOG_I("  --channel, --invert, --blur, --wrap, --masked  As for normal maps; masked pixels sink to the bottom");
        LOG_I("  --low <x>, --high <x> The heights read as the bottom and the top            (default: 0 and 1)");
        LOG_I("  --contrast <x>       Spread about the middle; above 1 hardens               (default: 1)");
        LOG_I("  --brightness <x>     Lift every height, before the contrast                 (default: 0)");
        LOG_I("  --gamma <x>          Bend the heights; above 1 sinks the middle tones       (default: 1)");
        LOG_I("  --bevel <px>         Distance from the outline to full height; 0 for none   (default: 0)");
        LOG_I("  --shape <s>          Linear, Round, Sharp or Plateau bevel                  (default: Round)");
        LOG_I("  --detail <x>         How much of the image's own height rides on the bevel  (default: 1)");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<TextureDomain::Entry> TextureDomain::Find(Text Name)
    {
        static constexpr Entry kCommands[] =
        {
            { "bake",    &TextureDomain::Bake    },
            { "pack",    &TextureDomain::Pack    },
            { "build",   &TextureDomain::Build   },
            { "normal",  &TextureDomain::Normal  },
            { "relief",  &TextureDomain::Relief  },
            { "extract", &TextureDomain::Extract },
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

    Outcome TextureDomain::Bake(ConstRef<Environment> Parsed) const
    {
        const ConstSpan<Text> Operands = Parsed.GetOperands();

        if (Operands.GetSize() < 2)
        {
            LOG_E("Texture: 'bake' takes a source image and, optionally, a destination");

            return Outcome::Misuse;
        }

        Texture::Profile Settings;

        if (!ReadProfile(Parsed, Settings))
        {
            return Outcome::Misuse;
        }

        const Text Source      = Operands[1];
        const Str  Derived     = Path::Derive(Source, Text::Empty(), Texture::Exporter::kOutput);
        const Text Destination = (Operands.GetSize() > 2) ? Operands[2] : Text(Derived);

        return mBaker.Bake(Source, Destination, Settings) ? Outcome::Success : Outcome::Failure;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome TextureDomain::Pack(ConstRef<Environment> Parsed) const
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

        for (ConstRef<Str> Source : Sources)
        {
            const Text Name = Path::GetStem(Source);

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
            Entry.Name = Str(Name);

            if (!Texture::Tracker::Relate(Folder, Source, Entry.Source))
            {
                return Outcome::Misuse;
            }
        }

        Texture::Composer Drawer(mBaker, Folder, Settings);

        if (!Drawer.Measure(Atlas) || !Texture::Packer::Pack(Atlas, Texture::Packer::Settings::From(Parsed)))
        {
            return Outcome::Failure;
        }
        return Publish(Drawer, Atlas, Output, Image, Settings);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome TextureDomain::Build(ConstRef<Environment> Parsed) const
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

        // A format asked for on the command line wins over the one the tracker was last written in.
        if (Settings.Format != ZyGraphic::TextureFormat::Unspecified)
        {
            Atlas.Format = Settings.Format;
        }

        if (Atlas.Texture.IsEmpty())
        {
            Atlas.Texture = Path::Derive(Path::GetStem(Output), Text::Empty(), Texture::Exporter::kOutput);
        }

        Texture::Composer Drawer(mBaker, Folder, Settings);

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

        // A hand-edited tracker may overlap two regions. It still draws, but the later region hides part of the other.
        for (UInt Index = 0; Index < Atlas.Regions.GetSize(); ++Index)
        {
            for (UInt Other = Index + 1; Other < Atlas.Regions.GetSize(); ++Other)
            {
                ConstRef<Texture::Region> First  = Atlas.Regions[Index];
                ConstRef<Texture::Region> Second = Atlas.Regions[Other];

                if (Overlaps(First, Second))
                {
                    LOG_W("Texture: '{0}' and '{1}' overlap on slice {2}; the later one is drawn on top",
                        First.Name, Second.Name, First.Slice);
                }
            }
        }

        const Str Image = Texture::Tracker::Resolve(Folder, Atlas.Texture);

        return Publish(Drawer, Atlas, Output, Image, Settings);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome TextureDomain::Normal(ConstRef<Environment> Parsed) const
    {
        const Texture::Normal::Settings Knobs = Texture::Normal::Settings::From(Parsed);

        return Generate(mBaker, Parsed, "_normal", [&](ConstRef<Texture::Canvas> Source)
        {
            return Texture::Normal::Generate(Source, Knobs);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome TextureDomain::Relief(ConstRef<Environment> Parsed) const
    {
        const Texture::Relief::Settings Knobs = Texture::Relief::Settings::From(Parsed);

        return Generate(mBaker, Parsed, "_relief", [&](ConstRef<Texture::Canvas> Source)
        {
            return Texture::Relief::Generate(Source, Knobs);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Outcome TextureDomain::Extract(ConstRef<Environment> Parsed) const
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

        const Texture::Surface Decoded = mBaker.Load(Source, Settings);

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

    Outcome TextureDomain::Publish(
        Ref<Texture::Composer>     Drawer,
        Ref<Texture::Tracker>      Atlas,
        Text                       Output,
        Text                       Image,
        ConstRef<Texture::Profile> Settings) const
    {
        Sequence<Texture::Bitmap> Slices;

        if (!Drawer.Compose(Atlas, Slices))
        {
            return Outcome::Failure;
        }

        Atlas.Layout = Settings.GetLayout(Atlas.Layout);

        // The slices are already in the tracker's format, so the texture is written in it rather than inferred again.
        Texture::Profile Written = Settings;
        Written.Format = Atlas.Format;

        const Blob Bytes = mBaker.Encode(Move(Slices), Atlas.Layout, Written);

        if (Bytes == nullptr || !Store(Image, Bytes) || !Atlas.Write(Output))
        {
            return Outcome::Failure;
        }

        LOG_I("Texture: {0} regions on {1} slice(s) of {2}x{3} -> '{4}' ({5} bytes) and '{6}'",
            Atlas.Regions.GetSize(), Atlas.Slices, Atlas.Width, Atlas.Height, Image, Bytes.GetSize(), Output);
        return Outcome::Success;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void TextureDomain::Collect(Text Operand, Text Skip, Ref<Sequence<Str>> Output) const
    {
        const Text    Folder = StrTrimRight(StrTrimRight(Operand, '/'), '\\');
        Sequence<Str> Found;

        // Only a folder can be listed, so a failed listing means the operand is a file of its own.
        const Filesystem::Result Listed = Filesystem::Enumerate(Folder, [&](ConstRef<Filesystem::Record> Record)
        {
            if (Record.Type == Filesystem::Type::File && mBaker.Find(Path::GetExtension(Record.Name)) != nullptr)
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