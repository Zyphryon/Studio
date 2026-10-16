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

#include "Exporter.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob Exporter::Export(
        Ref<ZyRender::Font::Metrics> Metrics,
        Ref<ZyRender::Font::Glyphs>  Glyphs,
        Ref<ZyRender::Font::Kerning> Kerning,
        ConstSpan<Blob>              Pages,
        UInt16                       Side)
    {
        if (Glyphs.IsEmpty() || Pages.IsEmpty())
        {
            LOG_E("Font: there is nothing to write");

            return Blob();
        }

        UInt Capacity = 1024;

        for (ConstRef<Blob> Page : Pages)
        {
            Capacity += Page.GetSize();
        }

        Writer Output(Capacity);
        Output.Write<UInt32>(kMagic);
        Output.Write<UInt16>(kVersion);

        Archive Serializer(Output);
        Serializer.Serialize(Metrics);
        Serializer.Serialize(Glyphs);

        if (!Kerning.IsEmpty())
        {
            Output.Write<UInt32>(kKerning);
            Output.WriteBlock<UInt32>([&](Ref<Writer> Body)
            {
                Archive(Body).Serialize(Kerning);
            });
        }

        // One chunk per page, in page order, so the loader finds a glyph's page by the index it carries.
        for (ConstRef<Blob> Page : Pages)
        {
            const ConstSpan Texels(Page.GetData(), Page.GetSize());

            Output.Write<UInt32>(kAtlas);
            Output.WriteBlock<UInt32>([&Texels, Side](Ref<Writer> Body)
            {
                Body.Write<UInt32>(kTexture);
                Body.Write<UInt16>(1);
                Body.Write(ZyGraphic::TextureLayout::Texture2D);
                Body.Write(ZyGraphic::TextureFormat::RGBA8UIntNorm);
                Body.Write<UInt16>(Side);
                Body.Write<UInt16>(Side);
                Body.Write<UInt16>(1);
                Body.Write<UInt8>(1);
                Body.WriteBlockCompressed<UInt32>(Texels, 3);
            });
        }
        return Output.Detach();
    }
}