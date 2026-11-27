// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include <Model/Profile.hpp>
#include <Sound/Profile.hpp>
#include <Texture/Atlas/Packer.hpp>
#include <Texture/Generate/Normal.hpp>
#include <Texture/Generate/Relief.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    /// \brief Describes every switch Studio reads, checks a command line against it and prints the help.
    class Usage final
    {
    public:

        /// \brief The bit of `bake` among the commands that read a switch.
        static constexpr UInt16 kBake    = 1 << 0;

        /// \brief The bit of `pack` among the commands that read a switch.
        static constexpr UInt16 kPack    = 1 << 1;

        /// \brief The bit of `build` among the commands that read a switch.
        static constexpr UInt16 kBuild   = 1 << 2;

        /// \brief The bit of `normal` among the commands that read a switch.
        static constexpr UInt16 kNormal  = 1 << 3;

        /// \brief The bit of `relief` among the commands that read a switch.
        static constexpr UInt16 kRelief  = 1 << 4;

        /// \brief The bit of `extract` among the commands that read a switch.
        static constexpr UInt16 kExtract = 1 << 5;

        /// \brief The bit of `formats` among the commands that read a switch.
        static constexpr UInt16 kFormats = 1 << 6;

        /// \brief The bit of `version` among the commands that read a switch.
        static constexpr UInt16 kVersion = 1 << 7;

        /// \brief The bit of `help` among the commands that read a switch.
        static constexpr UInt16 kHelp    = 1 << 8;

    public:

        /// \brief Prints every command and switch.
        static void Print();

        /// \brief Prints one line on standard output, without the logger's prefix and colour.
        ///
        /// \param Line The text of the line, without its break.
        static void Write(Text Line);

        /// \brief Prints one line on standard output, with each parameter put in its place.
        ///
        /// \param Pattern    The text of the line, without its break, with a `{0}` style place for each parameter.
        /// \param Parameters The values put in the places.
        template<typename... Arguments>
        ZY_INLINE static void Write(Text Pattern, AnyRef<Arguments>... Parameters)
        {
            Write(Str::Print(Pattern, Parameters...));
        }

        /// \brief Copies a command line, spelling each bare toggle as `--name=true` so it never takes the next word.
        ///
        /// \param Count     The number of entries in \p Arguments.
        /// \param Arguments The argument vector, as handed to `main`.
        /// \param Output    Receives every word, the program's name first.
        /// \return `true` if every switch is one Studio reads, spelled the way it takes its value, otherwise `false`.
        static Bool Spell(UInt Count, ConstPtr<ConstPtr<Char>> Arguments, Ref<Sequence<Str>> Output);

        /// \brief Checks that a command reads every switch typed for it, and that every value it reads is one it takes.
        ///
        /// \param Typed   The switches typed on the command line, without a preset's.
        /// \param Parsed  The switches the command reads, a preset's included.
        /// \param Command The name the command is typed as.
        /// \param Reader  The bit of the command among the commands that read a switch.
        /// \return `true` if every typed switch is read and every value taken, otherwise `false`.
        static Bool Check(ConstRef<Environment> Typed, ConstRef<Environment> Parsed, Text Command, UInt16 Reader);

    private:

        /// \brief Specifies the section of the help a switch is listed in.
        enum class Section : UInt8
        {
            Every,      ///< Lists the switches every command reads.
            Bake,       ///< Lists the switches every bake reads.
            Texture,    ///< Lists the switches of every command that writes a texture.
            Pack,       ///< Lists the switches `pack` reads, and `build` with `--repack`.
            Normal,     ///< Lists the switches `normal` reads, some of them shared with `relief`.
            Relief,     ///< Lists the switches `relief` reads.
            Font,       ///< Lists the switches a font bake reads.
            Model,      ///< Lists the switches a model bake reads.
            Sound,      ///< Lists the switches a sound bake reads.
        };

        /// \brief Represents one switch, as the help lists it and the check reads it.
        struct Option final
        {
            /// The name typed after the two dashes.
            Text    Name;

            /// The value the help shows after the name, or empty for a toggle, which takes none.
            Text    Hint;

            /// The section of the help the switch is listed in.
            Section Group;

            /// The commands that read the switch, one bit each.
            UInt16  Readers;

            /// The test a value has to pass, or `nullptr` for any text.
            Bool (* Test)(Text Value);

            /// The text the help prints for the switch.
            Text    Help;

            /// The value the switch has when left out, or empty for none to show.
            Text    Default;
        };

        /// \brief The bits of every command that writes a texture.
        static constexpr UInt16 kTextured = kBake | kPack | kBuild | kNormal | kRelief;

        /// \brief The bits of every command that decodes a source image.
        static constexpr UInt16 kDecoded  = kTextured | kExtract;

        /// \brief The bits of the commands that lay an atlas out.
        static constexpr UInt16 kPacked   = kPack | kBuild;

        /// \brief The bits of the commands that read an image as a height.
        static constexpr UInt16 kHeights  = kNormal | kRelief;

        /// \brief The bits of every command.
        static constexpr UInt16 kEvery    = 0xFFFF;

        /// \brief Finds the first switch listed under a name.
        ///
        /// \param Name The name typed after the two dashes, in any case.
        /// \return The switch, or `nullptr` if Studio reads none by that name.
        static ConstPtr<Option> Find(Text Name);

        /// \brief Checks whether a command reads a switch, under any of the sections it is listed in.
        ///
        /// \param Name   The name typed after the two dashes.
        /// \param Reader The bit of the command among the commands that read a switch.
        /// \return `true` if the command reads the switch, otherwise `false`.
        static Bool Reads(Text Name, UInt16 Reader);

        /// \brief Prints every switch of one section of the help.
        ///
        /// \param Group The section to print.
        static void Show(Section Group);

        /// \brief Checks whether a value is one a toggle takes.
        ///
        /// \param Value The value typed, empty for a bare toggle.
        /// \return `true` if empty, `0`, `1`, `true` or `false`, otherwise `false`.
        static Bool IsToggle(Text Value);

        /// \brief Checks whether a value is a number written out, with nothing after it.
        ///
        /// \param Value The value typed.
        /// \return `true` if the value is a number, otherwise `false`.
        static Bool IsNumber(Text Value);

        /// \brief Checks whether a value is a width and a height written as `<width>x<height>`.
        ///
        /// \param Value The value typed.
        /// \return `true` if both sides are whole numbers from 1 to 65535, otherwise `false`.
        static Bool IsExtent(Text Value);

        /// \brief Checks whether a value names one of the choices of an enumeration.
        ///
        /// \param Value The value typed, in any case.
        /// \return `true` if the value names a choice, otherwise `false`.
        template<typename Type>
        ZY_INLINE static Bool IsChoice(Text Value)
        {
            Type Choice { };
            return ZyEnum::Find(Value, Choice);
        }

    private:

        /// \brief Every switch Studio reads, in the order the help lists them.
        static constexpr Array<Option, 75> kOptions =
        {
            Option { "preset",        "<name>",     Section::Every,   kEvery,    nullptr,
                     "Apply a preset: sprite, normal, relief, or one of your own", "" },
            Option { "presets",       "<file>",     Section::Every,   kEvery,    nullptr,
                     "JSON file of your own presets, searched before the built-in ones", "" },
            Option { "help",          "",           Section::Every,   kEvery,    IsToggle,
                     "Print this text, whatever the command", "" },
            Option { "version",       "",           Section::Every,   kEvery,    IsToggle,
                     "Print the revision of every file Studio writes, whatever the command", "" },

            Option { "output",        "<folder>",   Section::Bake,    kBake,     nullptr,
                     "Write every result there, named after its source", "beside its source" },

            Option { "format",        "<name>",     Section::Texture, kTextured, IsChoice<ZyGraphic::TextureFormat>,
                     "Format, such as R8UIntNorm or RGBA16Float", "inferred" },
            Option { "mipmaps",       "",           Section::Texture, kTextured, IsToggle,
                     "Generate a full mip chain down to 1x1", "off" },
            Option { "compressed",    "",           Section::Texture, kTextured, IsToggle,
                     "LZ4-compress the payload when it shrinks the output", "on" },
            Option { "linear",        "",           Section::Texture, kDecoded,  IsToggle,
                     "Read the source as linear; '--no-linear' for sRGB art", "on" },
            Option { "layered",       "",           Section::Texture, kTextured, IsToggle,
                     "Write a lone image as a one-slice array", "off" },
            Option { "transparent",   "",           Section::Texture, kTextured, IsToggle,
                     "Weigh mips by alpha, bleed colour into clear texels", "off" },
            Option { "cutoff",        "<alpha>",    Section::Texture, kTextured, IsNumber,
                     "Alpha test value every mip keeps coverage at, 0 for off", "0" },
            Option { "normal",        "",           Section::Texture, kTextured, IsToggle,
                     "Renormalise every mip level of a normal map", "off" },
            Option { "cube",          "<c>x<r>",    Section::Texture, kBake,     IsExtent,
                     "bake: cut the source into cube faces, a grid counted in faces", "" },
            Option { "array",         "<w>x<h>",    Section::Texture, kBake,     IsExtent,
                     "bake: cut the source into array slices of that size in texels", "" },

            Option { "output",        "<path>",     Section::Pack,    kPack,     nullptr,
                     "Tracker to write; the texture goes beside it", "pack: required" },
            Option { "texture",       "<path>",     Section::Pack,    kPack,     nullptr,
                     "Write the texture here instead", "tracker's name" },
            Option { "mode",          "<m>",        Section::Pack,    kPacked,   IsChoice<Texture::Packer::Mode>,
                     "Atlas shares slices; Array gives each region its own", "Atlas" },
            Option { "width",         "<n>",        Section::Pack,    kPacked,   IsNumber,
                     "Widest a slice may grow, in pixels", "2048" },
            Option { "height",        "<n>",        Section::Pack,    kPacked,   IsNumber,
                     "Tallest a slice may grow, in pixels", "2048" },
            Option { "padding",       "<n>",        Section::Pack,    kPacked,   IsNumber,
                     "Empty pixels between neighbouring regions", "1" },
            Option { "extrude",       "<n>",        Section::Pack,    kPacked,   IsNumber,
                     "Pixels each region's edge is repeated outward", "0" },
            Option { "extent",        "<w>x<h>",    Section::Pack,    kPacked,   IsExtent,
                     "Array: size of every slice", "largest region" },
            Option { "fit",           "<f>",        Section::Pack,    kPacked,   IsChoice<Texture::Packer::Fitting>,
                     "Array: Stretch, Center or Contain each region", "Stretch" },
            Option { "headroom",      "<n>",        Section::Pack,    kPacked,   IsNumber,
                     "Array: round the slice count up to a multiple of n", "1" },
            Option { "fill",          "<RRGGBBAA>", Section::Pack,    kPacked,   nullptr,
                     "Colour of the pixels no region covers, as stored", "00000000" },
            Option { "pot",           "",           Section::Pack,    kPacked,   IsToggle,
                     "Round each side of a slice up to a power of two", "on" },
            Option { "square",        "",           Section::Pack,    kPacked,   IsToggle,
                     "Keep each slice square", "off" },
            Option { "pages",         "",           Section::Pack,    kPacked,   IsToggle,
                     "Spill what does not fit onto further slices", "off" },
            Option { "repack",        "",           Section::Pack,    kBuild,    IsToggle,
                     "build: place the regions again instead of keeping their places", "" },

            Option { "channel",       "<c>",        Section::Normal,  kHeights,  IsChoice<Texture::Channel>,
                     "Height channel: Luminance, Red, Green, Blue or Alpha", "Luminance" },
            Option { "filter",        "<k>",        Section::Normal,  kNormal,   IsChoice<Texture::Normal::Kernel>,
                     "Slope filter: Central, Sobel, Scharr or Prewitt", "Sobel" },
            Option { "strength",      "<x>",        Section::Normal,  kNormal,   IsNumber,
                     "How steep one unit of height reads", "2" },
            Option { "blur",          "<px>",       Section::Normal,  kHeights,  IsNumber,
                     "Smooth the height before measuring it", "0" },
            Option { "octaves",       "<n>",        Section::Normal,  kNormal,   IsNumber,
                     "Scales added, each twice as coarse, 1-8", "1" },
            Option { "falloff",       "<x>",        Section::Normal,  kNormal,   IsNumber,
                     "Weight of each coarser scale against the one before", "0.5" },
            Option { "invert",        "",           Section::Normal,  kHeights,  IsToggle,
                     "Read dark as raised", "off" },
            Option { "flip-x",        "",           Section::Normal,  kNormal,   IsToggle,
                     "Mirror the horizontal axis", "off" },
            Option { "flip-y",        "",           Section::Normal,  kNormal,   IsToggle,
                     "Mirror the vertical axis, for green-down programs", "off" },
            Option { "wrap",          "",           Section::Normal,  kHeights,  IsToggle,
                     "Wrap around the edges, for tiling textures", "off" },
            Option { "masked",        "",           Section::Normal,  kHeights,  IsToggle,
                     "Transparent pixels are ground, so the outline stands up", "on" },

            Option { "low",           "<x>",        Section::Relief,  kRelief,   IsNumber,
                     "Height read as the bottom", "0" },
            Option { "high",          "<x>",        Section::Relief,  kRelief,   IsNumber,
                     "Height read as the top", "1" },
            Option { "contrast",      "<x>",        Section::Relief,  kRelief,   IsNumber,
                     "Spread around the middle; above 1 hardens", "1" },
            Option { "brightness",    "<x>",        Section::Relief,  kRelief,   IsNumber,
                     "Lift every height, before the contrast", "0" },
            Option { "gamma",         "<x>",        Section::Relief,  kRelief,   IsNumber,
                     "Bend the heights; above 1 sinks the middle tones", "1" },
            Option { "bevel",         "<px>",       Section::Relief,  kRelief,   IsNumber,
                     "Distance from the outline to full height; 0 for none", "0" },
            Option { "shape",         "<s>",        Section::Relief,  kRelief,   IsChoice<Texture::Relief::Shape>,
                     "Bevel profile: Linear, Round, Sharp or Plateau", "Round" },
            Option { "detail",        "<x>",        Section::Relief,  kRelief,   IsNumber,
                     "How much of the image's own height rides on the bevel", "1" },

            Option { "size",          "<px>",       Section::Font,    kBake,     IsNumber,
                     "Em size the glyphs are generated at", "40" },
            Option { "range",         "<texels>",   Section::Font,    kBake,     IsNumber,
                     "Width of the band the distance fades across", "20" },
            Option { "angle",         "<radians>",  Section::Font,    kBake,     IsNumber,
                     "Join angle past which an edge counts as a corner", "3" },
            Option { "charset",       "<list>",     Section::Font,    kBake,     nullptr,
                     "Codepoints to bake, such as latin1,0x2010-0x205E", "ascii" },
            Option { "face",          "<index>",    Section::Font,    kBake,     IsNumber,
                     "Typeface to read out of a collection", "0" },
            Option { "padding",       "<texels>",   Section::Font,    kBake,     IsNumber,
                     "Gap between neighbouring glyphs", "1" },
            Option { "limit",         "<texels>",   Section::Font,    kBake,     IsNumber,
                     "Largest atlas page before another one opens", "8192" },
            Option { "underline",     "<em>",       Section::Font,    kBake,     IsNumber,
                     "Vertical space an underline takes", "1.2" },
            Option { "fallback",      "<list>",     Section::Font,    kBake,     nullptr,
                     "More typefaces, as 'path[=charset];...'; the first carrying a codepoint keeps it", "" },

            Option { "scale",         "<factor>",   Section::Model,   kBake,     IsNumber,
                     "Uniform factor every position is scaled by", "1" },
            Option { "units",         "<metres>",   Section::Model,   kBake,     IsNumber,
                     "Metres one baked unit stands for, 0 keeps the source's", "0" },
            Option { "rate",          "<hertz>",    Section::Model,   kBake,     IsNumber,
                     "Clip samples per second, bent to end on the last pose", "30" },
            Option { "interpolation", "<i>",        Section::Model,   kBake,     IsChoice<Interpolation>,
                     "Step, Linear, Cubic or Hermite", "Linear" },
            Option { "attributes",    "<list>",     Section::Model,   kBake,     nullptr,
                     "Attributes to write and their streams, see below", "all but TexCoord1, one stream" },
            Option { "normals",       "",           Section::Model,   kBake,     IsToggle,
                     "Generate normals where the scene carries none", "on" },
            Option { "tangents",      "",           Section::Model,   kBake,     IsToggle,
                     "Generate the tangents normal maps are read through", "on" },
            Option { "rig",           "",           Section::Model,   kBake,     IsToggle,
                     "Carry the skin and its clips across", "on" },
            Option { "lod",           "<n>",        Section::Model,   kBake,     IsNumber,
                     "Reduced levels of detail after the full mesh, 0 to 3", "3" },
            Option { "coverage",      "<list>",     Section::Model,   kBake,     nullptr,
                     "View height share each level is kept down to, by commas", "0.03,0.015,0.0075" },
            Option { "drift",         "<pixels>",   Section::Model,   kBake,     IsNumber,
                     "Pixels a level may stray at 1080p, 0 for --coverage", "1" },
            Option { "density",       "<n>",        Section::Model,   kBake,     IsNumber,
                     "Pixels a unit covers up close, 0 for no density warning", "64" },
            Option { "once",          "<names>",    Section::Model,   kBake,     nullptr,
                     "Clips that play once and hold, by baked name, by commas", "none" },

            Option { "encoding",      "<e>",        Section::Sound,   kBake,     IsChoice<ZyAudio::Encoding>,
                     "Linear (16-bit), Adaptive (IMA ADPCM) or Opus", "Linear" },
            Option { "bitrate",       "<n>",        Section::Sound,   kBake,     IsNumber,
                     "Rate Opus aims for, in bits per second", "96000" },
            Option { "compressed",    "",           Section::Sound,   kBake,     IsToggle,
                     "LZ4-compress the payload when it shrinks the output", "on" },
            Option { "loop",          "",           Section::Sound,   kBake,     IsToggle,
                     "Loop an Opus sound without a click", "off" },
        };
    };
}