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

#include "Tracker.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Area ReadArea(ConstRef<JsonArray> Values)
    {
        if (!Values.IsNullOrEmpty() && Values.GetSize() == 4)
        {
            return Area(Values.GetNumber<UInt16>(0), Values.GetNumber<UInt16>(1),
                        Values.GetNumber<UInt16>(2), Values.GetNumber<UInt16>(3));
        }
        return Area();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void WriteArea(JsonArray Values, ConstRef<Area> Value)
    {
        Values.AddNumber<UInt16>(Value.X);
        Values.AddNumber<UInt16>(Value.Y);
        Values.AddNumber<UInt16>(Value.Width);
        Values.AddNumber<UInt16>(Value.Height);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool IsAbsolute(Text Path)
    {
        return (Path.GetSize() > 1 && Path[1] == ':') || (!Path.IsEmpty() && (Path[0] == '/' || Path[0] == '\\'));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Sequence<Text> Split(Text Path)
    {
        Sequence<Text> Parts;

        UInt Start = 0;

        for (UInt Index = 0; Index <= Path.GetSize(); ++Index)
        {
            if (Index < Path.GetSize() && Path[Index] != '/' && Path[Index] != '\\')
            {
                continue;
            }

            const Text Part = Path.Slice(Start, Index - Start);
            Start = Index + 1;

            // A '..' cancels the folder before it, unless that one is a '..' too.
            if (Part == "..")
            {
                if (!Parts.IsEmpty() && !(Parts.GetBack() == ".."))
                {
                    Parts.RemoveLast();
                    continue;
                }
                Parts.Append(Part);
            }
            else if (!Part.IsEmpty() && !(Part == "."))
            {
                Parts.Append(Part);
            }
        }
        return Parts;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Tracker::Read(Text Path, Ref<Tracker> Output)
    {
        Blob Input;

        if (Filesystem::Read(Path, Input) != Filesystem::Result::Success || Input == nullptr)
        {
            LOG_E("Texture: failed to read the tracker '{0}'", Path);

            return false;
        }

        JsonValue Document = JsonDocument::Parse(Text(Input.GetData<Char>(), Input.GetSize()));

        // A `JsonObject` assumes it wraps an object, so the document is checked before it is wrapped.
        if (!Document.IsObject())
        {
            LOG_E("Texture: '{0}' is not a JSON tracker", Path);

            return false;
        }

        const JsonObject Root(Document);

        if (const UInt32 Version = Root.GetNumber<UInt32>("Version", 0); Version != kVersion)
        {
            LOG_E("Texture: '{0}' is tracker version {1}, and this one reads {2}", Path, Version, kVersion);

            return false;
        }

        Tracker Result;
        Result.Texture = Str(Root.GetString("Texture"));
        Result.Layout  = Root.GetEnum("Layout",  Result.Layout);
        Result.Format  = Root.GetEnum("Format",  Result.Format);
        Result.Width   = Root.GetNumber<UInt16>("Width",   0);
        Result.Height  = Root.GetNumber<UInt16>("Height",  0);
        Result.Slices  = Max<UInt16>(Root.GetNumber<UInt16>("Slices", 1), 1);
        Result.Padding = Root.GetNumber<UInt16>("Padding", 0);
        Result.Extrude = Root.GetNumber<UInt16>("Extrude", 0);

        const JsonArray Regions = Root.GetArray("Regions");

        for (UInt Index = 0; !Regions.IsNullOrEmpty() && Index < Regions.GetSize(); ++Index)
        {
            const JsonObject Entry = Regions.GetObject(Index);

            Ref<Region> Value = Result.Regions.Append();
            Value.Name   = Str(Entry.GetString("Name"));
            Value.Source = Str(Entry.GetString("Source"));
            Value.From   = ReadArea(Entry.GetArray("From"));
            Value.Slice  = Entry.GetNumber<UInt16>("Slice", 0);
            Value.Rect   = ReadArea(Entry.GetArray("Rect"));

            if (Value.Source.IsEmpty())
            {
                LOG_E("Texture: region {0} of '{1}' names no source", Index, Path);

                return false;
            }
        }

        Output = Move(Result);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Tracker::Write(Text Path) const
    {
        JsonValue Document;
        Document.SetObject();

        JsonObject Root(Document);

        Root.SetNumber<UInt32>("Version", kVersion);
        Root.SetString("Texture", Texture);
        Root.SetEnum("Layout",  Layout);
        Root.SetEnum("Format",  Format);
        Root.SetNumber<UInt16>("Width",   Width);
        Root.SetNumber<UInt16>("Height",  Height);
        Root.SetNumber<UInt16>("Slices",  Slices);
        Root.SetNumber<UInt16>("Padding", Padding);
        Root.SetNumber<UInt16>("Extrude", Extrude);

        JsonArray List = Root.SetArray("Regions");

        const Real32 Across = 1.0f / Max<Real32>(Width,  1.0f);
        const Real32 Down   = 1.0f / Max<Real32>(Height, 1.0f);

        for (ConstRef<Region> Value : Regions)
        {
            JsonObject Entry = List.AddObject();
            Entry.SetString("Name",   Value.Name);
            Entry.SetString("Source", Value.Source);

            if (Value.From.IsValid())
            {
                WriteArea(Entry.SetArray("From"), Value.From);
            }

            Entry.SetNumber<UInt16>("Slice", Value.Slice);
            WriteArea(Entry.SetArray("Rect"), Value.Rect);

            // The crop is the rectangle a sprite or a sheet samples by, as fractions of its slice.
            JsonArray Crop = Entry.SetArray("Crop");
            Crop.AddNumber<Real32>(Value.Rect.X * Across);
            Crop.AddNumber<Real32>(Value.Rect.Y * Down);
            Crop.AddNumber<Real32>(static_cast<Real32>(Value.Rect.X + Value.Rect.Width)  * Across);
            Crop.AddNumber<Real32>(static_cast<Real32>(Value.Rect.Y + Value.Rect.Height) * Down);
        }

        const Str Content = JsonDocument::Dump(Document, "    ");

        Filesystem::Ensure(Path);

        if (Filesystem::Write(Path, Text(Content)) != Filesystem::Result::Success)
        {
            LOG_E("Texture: failed to write the tracker '{0}'", Path);

            return false;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Tracker::Resolve(Text Folder, Text Path)
    {
        if (IsAbsolute(Path) || Folder.IsEmpty())
        {
            return Str(Path);
        }

        Str Result(Folder);
        Result.Append('/');
        Result.Append(Path);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Tracker::Relate(Text Folder, Text Path, Ref<Str> Output)
    {
        const Bool Absolute = IsAbsolute(Path);

        if (!Absolute && IsAbsolute(Folder))
        {
            LOG_E("Texture: '{0}' is relative and '{1}' is absolute, so one cannot be written against the other",
                Path, Folder);

            return false;
        }

        const Sequence<Text> From = Split(Folder);
        const Sequence<Text> To   = Split(Path);

        UInt Common = 0;

        while (Common < From.GetSize() && Common + 1 < To.GetSize() && StrEqualCaseInsensitive(From[Common], To[Common]))
        {
            ++Common;
        }

        // Two absolute paths with nothing in common sit on different drives, so the path is kept as it is.
        if (Absolute && Common == 0)
        {
            Output = Str(Path);
            return true;
        }

        Str Result;

        for (UInt Index = Common; Index < From.GetSize(); ++Index)
        {
            // Stepping out of a folder that is itself a '..' would need the name of the folder above it.
            if (From[Index] == "..")
            {
                LOG_E("Texture: '{0}' cannot be written against '{1}'", Path, Folder);

                return false;
            }
            Result.Append("../");
        }

        for (UInt Index = Common; Index < To.GetSize(); ++Index)
        {
            if (Index > Common)
            {
                Result.Append('/');
            }
            Result.Append(To[Index]);
        }

        Output = Move(Result);
        return true;
    }
}