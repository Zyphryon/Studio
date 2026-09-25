# 🌌 Zyphryon Studio
*Every asset the engine loads, baked by one tool.*

**Studio** is the offline side of the [Zyphryon Engine](https://github.com/Zyphryon/Engine): one executable that
turns images, typefaces and sounds into the `.tex`, `.fnt` and `.snd` files the engine reads, and packs sprites into
atlases and texture arrays described by a plain JSON tracker. Its bakers are also static libraries, so an editor
can bake, pack and preview without shelling out.

---

## ✨ Features

- **Textures** — PNG, JPEG, TGA, BMP and HDR (or a baked `.tex`) into any engine texture format, with an optional
  mip chain, LZ4 compression, cube maps and arrays cut from a grid.
- **Atlases and arrays** — `pack` places images with MaxRects into an atlas or one slice each into an array
  (stretched to the slice, centred, or shrunk to fit with `--fit`), and writes a tracker beside the texture. Edit the tracker by hand, and `build` draws it again. Slots keep their slice,
  arrays grow with headroom, and channels can be packed from other images (a height into alpha, say).
- **Generated maps** — tangent-space normal maps (Sobel, Scharr, Prewitt or central differences, over several
  octaves) and greyscale relief maps (levels, contrast, gamma and a shaped bevel from the outline), both from one
  image read as a height.
- **Fonts** — TTF, OTF and TTC into multi-channel signed distance fields (MTSDF through msdfgen), with kerning,
  any set of codepoints, and fallback faces for the codepoints a face lacks.
- **Sounds** — FLAC, MP3 and WAV, resampled to the mixer's 48 kHz, stored as 16-bit PCM, IMA ADPCM or Opus.
- **Deterministic** — the same source and switches bake to the same bytes, every time.

---

## 🚀 Usage

```
Studio <command> <operands> [switches]
```

Operands come first: a switch takes the word after it as its value. `Studio help` prints every switch.

| Command | Does |
|---------|------|
| `bake <source> [dst]` | Bakes one image, typeface or sound; the source's extension picks the baker |
| `pack <image\|folder>... --output <t.json>` | Packs images into an atlas (`--mode Atlas`) or an array (`--mode Array`) |
| `build <t.json> [--repack]` | Draws a tracker's texture again after its regions were edited |
| `normal <src> [dst]` | Draws a normal map, as a `.png` or a `.tex` |
| `relief <src> [dst]` | Draws a relief map, as a `.png` or a `.tex` |
| `extract <tex> [dst]` | Writes each slice of a `.tex` as a `.png` |
| `formats` | Lists every texture format `--format` accepts |
| `version` | Prints the revision of every file Studio writes |

A few examples:

```sh
# A sprite: sRGB, mipmapped, as a one-slice array
Studio bake hero.png --preset sprite

# Every image in a folder packed into one atlas, then drawn again after hand edits
Studio pack Sprites/ --output Atlas/Sprites.json --padding 2 --extrude 1
Studio build Atlas/Sprites.json

# A normal map from a height, for a tiling texture
Studio normal rock_height.png rock_normal.tex --preset normal --wrap --strength 4

# A font with Latin-1 and punctuation, falling back to a second face
Studio bake Inter.ttf --charset latin1,punctuation --fallback "NotoSymbols.ttf=0x2190-0x21FF"

# A sound effect as ADPCM
Studio bake hit.wav --encoding Adaptive
```

### Presets

`--preset <name>` stands for a set of switches. The built-in ones are:

| Preset | Switches |
|--------|----------|
| `sprite` | `--layered --no-linear --mipmaps` |
| `normal` | `--layered --linear --mipmaps` |
| `relief` | `--layered --linear --mipmaps --format R8UIntNorm` |

A project can keep its own in a JSON file passed with `--presets <file>`, searched before the built-in ones:

```json
{
  "ui": { "layered": true, "linear": false, "mipmaps": false, "format": "RGBA8UIntNorm_sRGB" }
}
```

A switch typed on the command line always wins over the same switch in a preset.

### Exit codes

`0` when everything was done, `1` when some of the work failed, `2` when the command itself was wrong.

---

## 📦 Dependencies

Everything is fetched by CMake; nothing needs installing by hand.

| Library | Used by | For |
|---------|---------|-----|
| [Zyphryon Engine](https://github.com/Zyphryon/Engine) | everything | Containers, strings, formats and the file layouts Studio writes |
| [stb](https://github.com/nothings/stb) | textures, fonts | Image decoding and PNG writing; TrueType parsing |
| [msdfgen](https://github.com/Chlumsky/msdfgen) *(v1.12)* | fonts | Multi-channel signed distance fields |
| [dr_libs](https://github.com/mackron/dr_libs) | sounds | FLAC, MP3 and WAV decoding |
| [libopus](https://opus-codec.org/) *(v1.5.2)* | sounds | Opus encoding |

---

## 🛠️ Building

### Prerequisites
- **CMake** 3.26+
- **C++23** compliant compiler, as the engine requires
- **Ninja** recommended

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Studio_Application
```

The executable lands in `build/bin/Studio`. To build against a local engine checkout instead of fetching it,
configure with `-DFETCHCONTENT_SOURCE_DIR_EXTERNAL_ZYENGINE=<path>`.

### As a library

A host such as an editor fetches Studio and links the bakers it needs. `STUDIO_BUILD_APPLICATION` is on only when
Studio is the top-level project, so the executable stays out of the host's build.

```cmake
FetchContent_Declare(External_Studio
        GIT_REPOSITORY https://github.com/Zyphryon/Studio
        GIT_TAG        main)
FetchContent_MakeAvailable(External_Studio)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE "Studio_Font" "Studio_Sound" "Studio_Texture")
```

| Target | Namespace | Bakes |
|--------|-----------|-------|
| `Studio_Texture` | `Studio::Texture` | `.tex`, atlas and array trackers, normal and relief maps |
| `Studio_Font` | `Studio::Font` | `.fnt` |
| `Studio_Sound` | `Studio::Sound` | `.snd` |

Each domain depends on the engine alone, never on another domain.

---

## 🚧 Status

Known gaps, so you don't have to discover them:

- **No GUI yet.** With no command, the executable prints its usage.
- **`pack` relates source paths lexically**, so mixing a relative source with an absolute tracker path is refused.

---

## 📄 License

Zyphryon Studio is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

---

## 🤝 Contributing

Contributions are welcome. Expect:

- **Code reviews** focused on correctness of the output, since every game built on the engine reads it
- **Byte-identical bakes** — a change that alters what a bake writes has to say so, and why
- **New dependencies to need justification** — each one has to earn its place and gets listed in the table above
- **Documentation** for all public APIs, following the engine's conventions

Feel free to submit Pull Requests, open Issues, or discuss new features in Discussions.

---

*Built with ❤️ alongside the Zyphryon Engine.*
