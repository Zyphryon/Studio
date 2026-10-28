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

#include "Layout.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Layout::Result Layout::Pack(ConstRef<Scene> Source, ConstSpan<Profile::Channel> Attributes)
    {
        const UInt Count = Source.Vertices.GetSize();

        // An attribute the scene never decoded is left out rather than written as a block of zeroes.
        Sequence<Profile::Channel> Selected;

        for (ConstRef<Profile::Channel> Entry : Attributes)
        {
            const ConstPtr<Scene::Encoding> Storage = Scene::Describe(Entry.Slot);

            if (Storage && Source.Has(Storage->Source))
            {
                Selected.Append(Entry);
            }
        }

        Result Output;
        Output.Extended = Count > 0xFFFF;

        // Attributes sharing a stream stride over the record they form, and each stream lands after the last.
        UInt32 Cursor = 0;

        for (UInt Group = 0; Group <= 0xFF; ++Group)
        {
            UInt32 Record = 0;

            for (ConstRef<Profile::Channel> Entry : Selected)
            {
                Record += Entry.Group == Group ? Scene::Describe(Entry.Slot)->Size : 0;
            }

            if (Record == 0)
            {
                continue;
            }

            UInt32 Field = 0;

            for (ConstRef<Profile::Channel> Entry : Selected)
            {
                if (Entry.Group != Group)
                {
                    continue;
                }

                const ConstPtr<Scene::Encoding> Storage = Scene::Describe(Entry.Slot);

                Ref<Binding> Placed = Output.Bindings.Append();
                Placed.Slot   = Entry.Slot;
                Placed.Format = Storage->Format;
                Placed.Stride = static_cast<UInt16>(Record);
                Placed.Offset = Cursor + Field;

                Field += Storage->Size;
            }

            Cursor += Record * static_cast<UInt32>(Count);
            ++Output.Streams;
        }

        if (Output.Bindings.IsEmpty())
        {
            LOG_E("Model: none of the requested attributes survived the scene");

            return Result();
        }

        Output.Vertices = Blob::Allocate<Byte>(Cursor);

        for (ConstRef<Binding> Entry : Output.Bindings)
        {
            const Ptr<Byte> Block = Output.Vertices.GetData<Byte>() + Entry.Offset;

            for (UInt Index = 0; Index < Count; ++Index)
            {
                Write(Source.Vertices[Index], Entry.Slot, Block + Index * Entry.Stride);
            }
        }

        if (Output.Extended)
        {
            Output.Indices = Blob::Allocate<UInt32>(Source.Indices.GetSize());
            Output.Indices.Copy<UInt32>(Source.Indices.GetData(), Source.Indices.GetSize());
        }
        else
        {
            Output.Indices = Blob::Allocate<UInt16>(Source.Indices.GetSize());

            for (UInt Index = 0; Index < Source.Indices.GetSize(); ++Index)
            {
                Output.Indices.GetData<UInt16>()[Index] = static_cast<UInt16>(Source.Indices[Index]);
            }
        }
        return Output;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Layout::Write(ConstRef<Scene::Vertex> Vertex, ZyGraphic::VertexSlot Slot, Ptr<Byte> Target)
    {
        const Ptr<Real32> Real = reinterpret_cast<Ptr<Real32>>(Target);
        const Ptr<UInt16> Half = reinterpret_cast<Ptr<UInt16>>(Target);

        switch (Slot)
        {
        case ZyGraphic::VertexSlot::Position:
            Real[0] = Vertex.Position.GetX();
            Real[1] = Vertex.Position.GetY();
            Real[2] = Vertex.Position.GetZ();
            break;
        case ZyGraphic::VertexSlot::Normal:
            Real[0] = Vertex.Normal.GetX();
            Real[1] = Vertex.Normal.GetY();
            Real[2] = Vertex.Normal.GetZ();
            break;
        case ZyGraphic::VertexSlot::Tangent:
            Real[0] = Vertex.Tangent.GetX();
            Real[1] = Vertex.Tangent.GetY();
            Real[2] = Vertex.Tangent.GetZ();
            Real[3] = Vertex.Handedness;
            break;
        case ZyGraphic::VertexSlot::TexCoord0:
            Real[0] = Vertex.TexCoord0.GetX();
            Real[1] = Vertex.TexCoord0.GetY();
            break;
        case ZyGraphic::VertexSlot::TexCoord1:
            Real[0] = Vertex.TexCoord1.GetX();
            Real[1] = Vertex.TexCoord1.GetY();
            break;
        case ZyGraphic::VertexSlot::Color0:
            Target[0] = Vertex.Color.GetRed();
            Target[1] = Vertex.Color.GetGreen();
            Target[2] = Vertex.Color.GetBlue();
            Target[3] = Vertex.Color.GetAlpha();
            break;
        case ZyGraphic::VertexSlot::BlendIndices:
            for (UInt Influence = 0; Influence < Scene::kInfluences; ++Influence)
            {
                Half[Influence] = Vertex.Joints[Influence];
            }
            break;
        case ZyGraphic::VertexSlot::BlendWeights:
        {
            // Rounding each weight on its own can leave the four a step off one, which the heaviest absorbs.
            UInt32 Total    = 0;
            UInt   Heaviest = 0;

            for (UInt Influence = 0; Influence < Scene::kInfluences; ++Influence)
            {
                Half[Influence] = static_cast<UInt16>(Clamp(Vertex.Weights[Influence], 0.0f, 1.0f) * 65535.0f + 0.5f);
                Total          += Half[Influence];
                Heaviest        = Vertex.Weights[Influence] > Vertex.Weights[Heaviest] ? Influence : Heaviest;
            }

            if (Total > 0)
            {
                Half[Heaviest] = static_cast<UInt16>(Clamp<SInt32>(
                    static_cast<SInt32>(Half[Heaviest]) + 65535 - static_cast<SInt32>(Total), 0, 65535));
            }
            break;
        }
        default:
            break;
        }
    }
}