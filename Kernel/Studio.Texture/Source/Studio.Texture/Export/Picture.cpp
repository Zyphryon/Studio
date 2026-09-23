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

#include "Picture.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Texture
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Collect(Ptr<void> Context, Ptr<void> Data, SInt32 Size)
    {
        Ref<Sequence<Byte>> Output = * static_cast<Ptr<Sequence<Byte>>>(Context);

        const ConstPtr<Byte> Bytes = static_cast<ConstPtr<Byte>>(Data);

        for (SInt32 Index = 0; Index < Size; ++Index)
        {
            Output.Append(Bytes[Index]);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Picture::Encode(ConstRef<Canvas> Source, Bool sRGB)
    {
        const Bitmap Pixels = Source.To(sRGB
            ? ZyGraphic::TextureFormat::RGBA8UIntNorm_sRGB
            : ZyGraphic::TextureFormat::RGBA8UIntNorm);

        if (Pixels.GetPixels().IsEmpty())
        {
            return Blob();
        }

        Sequence<Byte> Output;

        const SInt32 Width  = Pixels.GetWidth();
        const SInt32 Height = Pixels.GetHeight();

        if (!stbi_write_png_to_func(Collect, AddressOf(Output), Width, Height, 4, Pixels.GetPixels().GetData(), Width * 4))
        {
            LOG_E("Texture: failed to encode a {0}x{1} picture", Width, Height);

            return Blob();
        }
        return Blob::Copy(ConstSpan<Byte>(Output.GetData(), Output.GetSize()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Picture::Write(Text Path, ConstRef<Canvas> Source, Bool sRGB)
    {
        const Blob Output = Encode(Source, sRGB);

        if (Output == nullptr)
        {
            return false;
        }

        Filesystem::Ensure(Path);

        if (Filesystem::Write(Path, ConstSpan<Byte>(Output.GetData(), Output.GetSize())) != Filesystem::Result::Success)
        {
            LOG_E("Texture: failed to write '{0}'", Path);

            return false;
        }
        return true;
    }
}
