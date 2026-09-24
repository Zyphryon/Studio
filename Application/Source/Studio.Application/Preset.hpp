// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Application
{
    /// \brief Provides the named sets of switches `--preset` picks from.
    class Preset final
    {
    public:

        /// \brief The built-in presets, in the layout a project's own file uses.
        static constexpr Text kBuiltin = R"({
            "sprite": { "layered": true, "linear": false, "mipmaps": true },
            "normal": { "layered": true, "linear": true,  "mipmaps": true },
            "relief": { "layered": true, "linear": true,  "mipmaps": true, "format": "R8UIntNorm" }
        })";

    public:

        /// \brief Gets the switches the preset named by `--preset` stands for.
        ///
        /// \param Parsed The parsed command line.
        /// \param Output Receives one `--name=value` per switch.
        /// \return `true` if no preset or a known one was named, otherwise `false`.
        static Bool Expand(ConstRef<Environment> Parsed, Ref<Sequence<Str>> Output);
    };
}