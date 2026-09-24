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

#include "TEXImporter.hpp"
#include "Studio.Texture/Export/Exporter.hpp"
#include <Zyphryon.Graphic/Metadata.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Surface TEXImporter::Import(ConstSpan<Byte> Source, ConstRef<Profile> Profile) const
    {
        if (Source.IsEmpty())
        {
            LOG_E("Texture: source image is empty");

            return Surface();
        }

        Reader Input(Source.GetData(), static_cast<UInt32>(Source.GetSize()));

        if (Input.Read<UInt32>() != Exporter::kMagic)
        {
            LOG_E("Texture: source is not a ZTEX file (bad magic)");

            return Surface();
        }

        if (const UInt16 Version = Input.Read<UInt16>(); Version != Exporter::kVersion)
        {
            LOG_E("Texture: ZTEX version {0} is not one this importer reads", Version);

            return Surface();
        }

        const ZyGraphic::TextureLayout Layout  = Input.Read<ZyGraphic::TextureLayout>();
        const ZyGraphic::TextureFormat Format  = Input.Read<ZyGraphic::TextureFormat>();
        const UInt16                   Width   = Input.Read<UInt16>();
        const UInt16                   Height  = Input.Read<UInt16>();
        const UInt16                   Layers  = Input.Read<UInt16>();
        const UInt8                    Levels  = Input.Read<UInt8>();
        const UInt32                   Size    = Input.Read<UInt32>();
        const ConstSpan<Byte>          Payload = Input.ReadBlock<UInt32, Byte>();

        if (Width == 0 || Height == 0 || Layers == 0 || Levels == 0 || Size == 0 || Payload.IsEmpty())
        {
            LOG_E("Texture: ZTEX header describes no surface to read back");

            return Surface();
        }

        // Later stages read one texel at a time, which a compressed or packed payload cannot give.
        const ZyGraphic::TextureMetadata Description = ZyGraphic::GetTextureMetadata(Format);

        if (Description.IsCompressed() || Description.IsPacked || !Description.IsSampler())
        {
            LOG_E("Texture: '{0}' stores no plain interleaved texels, so it cannot be read back", ZyEnum::GetName(Format));

            return Surface();
        }

        // Slices follow one another with their whole chain, so the header fixes the payload's length.
        const UInt32 Stride = ZyGraphic::GetLevelOffset(Format, Width, Height, Levels);

        if (static_cast<UInt64>(Stride) * Layers != Size)
        {
            LOG_E("Texture: ZTEX payload is {0} bytes, but {1}x{2} over {3} slice(s) needs {4}",
                Size, Width, Height, Layers, static_cast<UInt64>(Stride) * Layers);

            return Surface();
        }

        // A payload shorter than `Size` is one LZ4 block over every slice, so it is decoded whole.
        Blob Scratch;

        if (Size != Payload.GetSize())
        {
            Scratch = Blob::Allocate<Byte>(Size);

            if (LZ4Decode(Payload, Scratch.GetData<Byte>(), Size) != Size)
            {
                LOG_E("Texture: ZTEX payload failed to decompress ({0} != {1})", Payload.GetSize(), Size);

                return Surface();
            }
        }

        const ConstPtr<Byte> Pixels = (Scratch == nullptr ? Payload.GetData() : Scratch.GetData<Byte>());

        // Later stages take a single level, so each slice keeps its base level and the chain is built again.
        const UInt32 Length = ZyGraphic::GetLevelSize(Format, Width, Height, 0);

        Surface Result;
        Result.Layout = Layout;
        Result.Slices.Reserve(Layers);

        for (UInt32 Slice = 0; Slice < Layers; ++Slice)
        {
            Blob Data = Blob::Copy(ConstSpan(Pixels + static_cast<UInt>(Slice) * Stride, Length));

            Result.Slices.Append(Bitmap(Format, Width, Height, 1, Move(Data)));
        }
        return Result;
    }
}