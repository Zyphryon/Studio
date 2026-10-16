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
#include <Sound/Baker.hpp>
#include <Texture/Baker.hpp>

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

    static void PrintTextures()
    {
        LOG_I("Textures ({0}):", List(Texture::Importer::kTypes));
        LOG_I("  bake <source> [destination]     Bake an image into a '.{0}'", Texture::Exporter::kOutput);
        LOG_I("  pack <image|folder>...          Pack images into an atlas or array; '--output' names its tracker");
        LOG_I("  build <tracker.json>            Draw a tracker's texture again, after its regions were edited");
        LOG_I("  normal <source> [destination]   Draw a normal map, as a '.png' or a '.{0}'", Texture::Exporter::kOutput);
        LOG_I("  relief <source> [destination]   Draw a relief map, as a '.png' or a '.{0}'", Texture::Exporter::kOutput);
        LOG_I("  extract <texture> [destination] Write each slice as a '.png', numbered '_0', '_1'... if several");
        LOG_I("  formats                         List every name '--format' accepts");
        LOG_I("");
        LOG_I("Texture switches (bake, pack, build; normal and relief into a '.{0}'):", Texture::Exporter::kOutput);
        LOG_I("  --format <name>       Format, such as R8UIntNorm or RGBA16Float               [inferred]");
        LOG_I("  --mipmaps             Generate a full mip chain down to 1x1                   [off]");
        LOG_I("  --compressed          LZ4-compress the payload when it shrinks the output     [on]");
        LOG_I("  --linear              Read the source as linear; '--no-linear' for sRGB art   [on]");
        LOG_I("  --layered             Write a lone image as a one-slice array                 [off]");
        LOG_I("  --cube <c>x<r>        bake: cut the source into cube faces, a grid counted in faces");
        LOG_I("  --array <w>x<h>       bake: cut the source into array slices of that size in texels");
        LOG_I("");
        LOG_I("Pack switches (pack, and build with '--repack'; build always reads '--fill' and '--headroom'):");
        LOG_I("  --output <path>       Tracker to write; the texture goes beside it            [pack: required]");
        LOG_I("  --texture <path>      Write the texture here instead                          [tracker's name]");
        LOG_I("  --mode <m>            Atlas shares slices; Array gives each region its own    [Atlas]");
        LOG_I("  --width <n>           Widest a slice may grow, in pixels                      [2048]");
        LOG_I("  --height <n>          Tallest a slice may grow, in pixels                     [2048]");
        LOG_I("  --padding <n>         Empty pixels between neighbouring regions               [1]");
        LOG_I("  --extrude <n>         Pixels each region's edge is repeated outward           [0]");
        LOG_I("  --extent <w>x<h>      Array: size of every slice                              [largest region]");
        LOG_I("  --fit <f>             Array: Stretch, Center or Contain each region           [Stretch]");
        LOG_I("  --headroom <n>        Array: round the slice count up to a multiple of n      [1]");
        LOG_I("  --fill <RRGGBBAA>     Colour of the pixels no region covers, as stored        [00000000]");
        LOG_I("  --pot                 Round each side of a slice up to a power of two         [on]");
        LOG_I("  --square              Keep each slice square                                  [off]");
        LOG_I("  --pages               Spill what does not fit onto further slices             [off]");
        LOG_I("  --repack              build: place the regions again instead of keeping their places");
        LOG_I("");
        LOG_I("Normal switches:");
        LOG_I("  --channel <c>         Height channel: Luminance, Red, Green, Blue or Alpha    [Luminance]");
        LOG_I("  --filter <k>          Slope filter: Central, Sobel, Scharr or Prewitt         [Sobel]");
        LOG_I("  --strength <x>        How steep one unit of height reads                      [2]");
        LOG_I("  --blur <px>           Smooth the height before measuring it                   [0]");
        LOG_I("  --octaves <n>         Scales added, each twice as coarse, 1-8                 [1]");
        LOG_I("  --falloff <x>         Weight of each coarser scale against the one before     [0.5]");
        LOG_I("  --invert              Read dark as raised                                     [off]");
        LOG_I("  --flip-x, --flip-y    Mirror an axis; '--flip-y' for green-down programs      [off]");
        LOG_I("  --wrap                Wrap around the edges, for tiling textures              [off]");
        LOG_I("  --masked              Transparent pixels are ground, so the outline stands up [on]");
        LOG_I("");
        LOG_I("Relief switches (and '--channel', '--invert', '--blur', '--wrap', '--masked' as for normal maps):");
        LOG_I("  --low <x>, --high <x> Heights read as the bottom and the top                  [0 and 1]");
        LOG_I("  --contrast <x>        Spread around the middle; above 1 hardens               [1]");
        LOG_I("  --brightness <x>      Lift every height, before the contrast                  [0]");
        LOG_I("  --gamma <x>           Bend the heights; above 1 sinks the middle tones        [1]");
        LOG_I("  --bevel <px>          Distance from the outline to full height; 0 for none    [0]");
        LOG_I("  --shape <s>           Bevel profile: Linear, Round, Sharp or Plateau          [Round]");
        LOG_I("  --detail <x>          How much of the image's own height rides on the bevel   [1]");
        LOG_I("  Masked pixels sink to the bottom of a relief.");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void PrintFonts()
    {
        LOG_I("Fonts ({0}):", List(Font::Importer::kTypes));
        LOG_I("  bake <source> [destination]     Bake a typeface into a '.{0}'", Font::Exporter::kOutput);
        LOG_I("");
        LOG_I("Font switches:");
        LOG_I("  --size <px>           Em size the glyphs are generated at                     [40]");
        LOG_I("  --range <texels>      Width of the band the distance fades across             [20]");
        LOG_I("  --angle <radians>     Join angle past which an edge counts as a corner        [3]");
        LOG_I("  --charset <list>      Codepoints to bake, such as latin1,0x2010-0x205E        [ascii]");
        LOG_I("  --face <index>        Typeface to read out of a collection                    [0]");
        LOG_I("  --padding <texels>    Gap between neighbouring glyphs                         [1]");
        LOG_I("  --limit <texels>      Largest atlas page before another one opens             [8192]");
        LOG_I("  --underline <em>      Vertical space an underline takes                       [1.2]");
        LOG_I("  --fallback <list>     More typefaces, as 'path[=charset];...'; the first carrying a codepoint keeps it");
        LOG_I("  A charset lists presets (ascii, latin1, punctuation), codepoints and 'first-last' ranges, split by");
        LOG_I("  commas; numbers are decimal unless they start with '0x'. The range also pads every glyph and is how");
        LOG_I("  far an outline or glow can reach, so raise the size, not the range, when glyphs look soft.");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void PrintSounds()
    {
        LOG_I("Sounds ({0}):", List(Sound::Importer::kTypes));
        LOG_I("  bake <source> [destination]     Bake a sound into a '.{0}'", Sound::Exporter::kOutput);
        LOG_I("");
        LOG_I("Sound switches:");
        LOG_I("  --encoding <e>        Linear (16-bit), Adaptive (IMA ADPCM) or Opus           [Linear]");
        LOG_I("  --bitrate <n>         Rate Opus aims for, in bits per second                  [96000]");
        LOG_I("  --compressed          LZ4-compress the payload when it shrinks the output     [on]");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Usage::Print()
    {
        LOG_I("Usage: Studio <command> <operands> [switches]");
        LOG_I("");
        LOG_I("Operands go before switches, since a switch takes the word after it as its value.");
        LOG_I("A toggle is turned off with its '--no-' spelling, as in '--no-mipmaps'. Defaults are in brackets.");
        LOG_I("Exit codes: 0 when everything was done, 1 when some of it failed, 2 when the command was wrong.");
        LOG_I("");
        LOG_I("Commands:");
        LOG_I("  help                            Print this text");
        LOG_I("  version                         Print the revision of every file Studio writes");
        LOG_I("  bake <source> [destination]     Bake a texture, font or sound; the source's extension picks which");
        LOG_I("");
        LOG_I("Every command:");
        LOG_I("  --preset <name>       Apply a preset: sprite, normal, relief, or one of your own");
        LOG_I("  --presets <file>      JSON file of your own presets, searched before the built-in ones");
        LOG_I("  A switch typed on the command line wins over the same switch in a preset.");
        LOG_I("");
        PrintTextures();
        LOG_I("");
        PrintFonts();
        LOG_I("");
        PrintSounds();
    }
}