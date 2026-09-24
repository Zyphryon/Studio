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

#include "Baker.hpp"
#include "Studio.Texture/Import/STBImporter.hpp"
#include "Studio.Texture/Import/TEXImporter.hpp"
#include "Studio.Texture/Process/Slicer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Baker::Extension Normalize(Text Type)
    {
        const Text Trimmed = (!Type.IsEmpty() && Type[0] == '.') ? Type.Slice(1) : Type;

        Baker::Extension Result(Trimmed.GetSize());

        for (UInt Index = 0; Index < Trimmed.GetSize(); ++Index)
        {
            Result.Append(StrLowercase(Trimmed[Index]));
        }
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Baker::Baker(Ref<ZyJob::Service> Scheduler)
        : mScheduler { Scheduler }
    {
        Register(Retainer<STBImporter>::Create());
        Register(Retainer<TEXImporter>::Create());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Baker::Register(ConstRetainer<Importer> Codec)
    {
        ZY_ASSERT(Codec != nullptr, "Cannot register a null importer");

        for (const Text Type : Codec->GetTypes())
        {
            mRegistry.Assign(Normalize(Type), Codec);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Baker::Unregister(Text Type)
    {
        return mRegistry.Erase(Normalize(Type));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Importer> Baker::Find(Text Type) const
    {
        if (const ConstPtr<Retainer<Importer>> Found = mRegistry.Find(Normalize(Type)))
        {
            return static_cast<Ptr<Importer>>(* Found);
        }
        return nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Baker::Bake(ConstSpan<Byte> Source, Text Type, ConstRef<Profile> Profile) const
    {
        const ConstPtr<Importer> Codec = Find(Type);

        if (Codec == nullptr)
        {
            LOG_E("Texture: '{0}' is not a source format this baker understands", Type);

            return Blob();
        }

        // A cube and an array are different layouts, so one source cannot be cut into both.
        if (Profile.Cube.IsValid() && Profile.Slice.IsValid())
        {
            LOG_E("Texture: a source is cut either into cube faces or into array slices, not both");

            return Blob();
        }

        Surface Decoded = Codec->Import(Source, Profile);

        if (!Decoded.IsValid())
        {
            return Blob();
        }

        // A source with several slices (a baked array or cube) is already cut, so it cannot be cut again.
        if (Decoded.Slices.GetSize() > 1)
        {
            if (Profile.Cube.IsValid() || Profile.Slice.IsValid())
            {
                LOG_E("Texture: the source already holds {0} slices, so it cannot also be cut along a grid",
                    Decoded.Slices.GetSize());

                return Blob();
            }
            return Exporter::Export(mScheduler, Move(Decoded.Slices), Decoded.Layout, Profile);
        }

        ConstRef<Bitmap> Atlas = Decoded.Slices.GetFront();

        if (Profile.Cube.IsValid())
        {
            Sequence<Bitmap> Faces = Slicer::Slice(Atlas, Slicer::Describe(Profile.Cube.Width, Profile.Cube.Height));

            if (Faces.IsEmpty())
            {
                return Blob();
            }
            return Exporter::Export(mScheduler, Move(Faces), ZyGraphic::TextureLayout::TextureCube, Profile);
        }

        if (Profile.Slice.IsValid())
        {
            Sequence<Bitmap> Slices = Slicer::Slice(Atlas, Slicer::Divide(Atlas, Profile.Slice.Width, Profile.Slice.Height));

            if (Slices.IsEmpty())
            {
                return Blob();
            }
            return Exporter::Export(mScheduler, Move(Slices), ZyGraphic::TextureLayout::Texture2DArray, Profile);
        }

        // A lone slice keeps the layout it was decoded as, so a one-slice array stays an array.
        return Exporter::Export(mScheduler, Move(Decoded.Slices), Profile.GetLayout(Decoded.Layout), Profile);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Surface Baker::Load(Text Path, ConstRef<Profile> Profile) const
    {
        const ConstPtr<Importer> Codec = Find(StrAfterLast(Path, '.'));

        if (Codec == nullptr)
        {
            LOG_E("Texture: '{0}' is not a source format this baker understands", Path);

            return Surface();
        }

        Blob Input;

        if (Filesystem::Read(Path, Input) != Filesystem::Result::Success || Input == nullptr)
        {
            LOG_E("Texture: failed to read '{0}'", Path);

            return Surface();
        }
        return Codec->Import(Input, Profile);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Baker::Encode(AnyRef<Sequence<Bitmap>> Slices, ZyGraphic::TextureLayout Layout, ConstRef<Profile> Profile) const
    {
        return Exporter::Export(mScheduler, Move(Slices), Layout, Profile);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Baker::Bake(Text Source, Text Destination, ConstRef<Profile> Profile) const
    {
        // A baked texture can be a source too, so nothing else stops a bake from writing over its own input.
        if (StrEqualCase(Source, Destination))
        {
            LOG_E("Texture: '{0}' is both the source and the destination", Source);

            return false;
        }

        Blob Input;

        if (Filesystem::Read(Source, Input) != Filesystem::Result::Success || Input == nullptr)
        {
            LOG_E("Texture: failed to read '{0}'", Source);

            return false;
        }

        const Blob Output = Bake(Input, StrAfterLast(Source, '.'), Profile);

        if (Output == nullptr)
        {
            return false;
        }

        Filesystem::Ensure(Destination);

        if (Filesystem::Write(Destination, Output) != Filesystem::Result::Success)
        {
            LOG_E("Texture: failed to write '{0}'", Destination);

            return false;
        }

        LOG_I("Texture: '{0}' -> '{1}' ({2} bytes)", Source, Destination, Output.GetSize());
        return true;
    }
}