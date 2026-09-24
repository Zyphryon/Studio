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
    /// \brief Provides the path handling every command shares, taking either slash as a separator.
    class Path final
    {
    public:

        /// \brief Gets the folder a path sits in.
        ///
        /// \param Value The path to split.
        /// \return The folder, or empty if there is none.
        ZY_INLINE static Text GetFolder(Text Value)
        {
            const SInt Separator = FindSeparator(Value);

            return Separator >= 0 ? Value.Slice(0, Separator) : Text::Empty();
        }

        /// \brief Gets the file name of a path without its extension.
        ///
        /// \param Value The path to split.
        /// \return The file name, up to its last dot.
        ZY_INLINE static Text GetStem(Text Value)
        {
            const Text Name = Value.Slice(FindSeparator(Value) + 1);
            const SInt Dot  = StrFindLast(Name, '.');

            return Dot > 0 ? Name.Slice(0, Dot) : Name;
        }

        /// \brief Gets the extension of a path.
        ///
        /// \param Value The path to split.
        /// \return The extension without its dot, or empty if there is none.
        ZY_INLINE static Text GetExtension(Text Value)
        {
            const Text Name = Value.Slice(FindSeparator(Value) + 1);
            const SInt Dot  = StrFindLast(Name, '.');

            return Dot > 0 ? Name.Slice(Dot + 1) : Text::Empty();
        }

        /// \brief Derives a path beside a source, with a suffix added to its name and another extension.
        ///
        /// \param Source    The path to derive from.
        /// \param Suffix    The text appended to the name.
        /// \param Extension The extension, without its dot.
        /// \return The derived path.
        ZY_INLINE static Str Derive(Text Source, Text Suffix, Text Extension)
        {
            Str Result(Source.Slice(0, FindSeparator(Source) + 1));
            Result.Append(GetStem(Source));
            Result.Append(Suffix);
            Result.Append('.');
            Result.Append(Extension);
            return Result;
        }

    private:

        /// \brief Finds the last separator in a path, whichever slash it is.
        ///
        /// \param Value The path to search.
        /// \return The index of the separator, or -1 if there is none.
        ZY_INLINE static SInt FindSeparator(Text Value)
        {
            return Max(StrFindLast(Value, '/'), StrFindLast(Value, '\\'));
        }
    };
}