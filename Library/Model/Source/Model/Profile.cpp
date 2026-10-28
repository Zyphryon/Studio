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

#include "Profile.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Model
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Profile::Wants(ZyGraphic::VertexSlot Slot) const
    {
        return Attributes.Contains([Slot](ConstRef<Channel> Entry)
        {
            return Entry.Slot == Slot;
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sequence<Profile::Channel> Profile::Standard()
    {
        // One stream holding everything, since a mesh drawn whole reads all of a vertex at once.
        Sequence<Channel> Result;
        Result.Append(ZyGraphic::VertexSlot::Position,     0);
        Result.Append(ZyGraphic::VertexSlot::Normal,       0);
        Result.Append(ZyGraphic::VertexSlot::Tangent,      0);
        Result.Append(ZyGraphic::VertexSlot::TexCoord0,    0);
        Result.Append(ZyGraphic::VertexSlot::BlendIndices, 0);
        Result.Append(ZyGraphic::VertexSlot::BlendWeights, 0);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sequence<Profile::Detail> Profile::StandardDetail()
    {
        // Triangles follow area and area the square of the coverage, so a quarter of them per halving keeps pace.
        Sequence<Detail> Result;
        Result.Append(1.00f,  0.25f, 0.02f);
        Result.Append(0.25f,  0.12f, 0.03f);
        Result.Append(0.06f,  0.05f, 0.05f);
        Result.Append(0.015f, 0.00f, 0.10f);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Profile::Parse(Text Description, Ref<Sequence<Channel>> Output)
    {
        Sequence<Channel> Parsed;

        UInt8 Group  = 0;
        Bool  Shared = false;
        UInt  Cursor = 0;

        while (Cursor < Description.GetSize())
        {
            const Char Symbol = Description[Cursor];

            if (Symbol == ' ' || Symbol == ',')
            {
                ++Cursor;
                continue;
            }

            if (Symbol == '[' || Symbol == ']')
            {
                if (Shared == (Symbol == '['))
                {
                    LOG_E("Model: the attribute list opens or closes a stream out of turn");

                    return false;
                }

                // A bracketed run takes one number between its names, and closing it moves the counter on.
                Shared = Symbol == '[';
                Group += Shared ? 0 : 1;
                ++Cursor;
                continue;
            }

            const UInt Start = Cursor;

            while (Cursor < Description.GetSize() && StrFind(Text(" ,[]"), Description[Cursor]) < 0)
            {
                ++Cursor;
            }

            const Text                  Name = Description.Slice(Start, Cursor - Start);
            const ZyGraphic::VertexSlot Slot = ZyEnum::Cast(Name, ZyGraphic::VertexSlot::None);

            if (Slot == ZyGraphic::VertexSlot::None || Scene::Describe(Slot) == nullptr)
            {
                LOG_E("Model: '{0}' is not an attribute this baker can write", Name);

                return false;
            }

            const Bool Repeated = Parsed.Contains([Slot](ConstRef<Channel> Entry)
            {
                return Entry.Slot == Slot;
            });

            if (Repeated)
            {
                LOG_E("Model: '{0}' is named more than once", Name);

                return false;
            }

            Parsed.Append(Slot, Group);
            Group += Shared ? 0 : 1;
        }

        if (Shared)
        {
            LOG_E("Model: the attribute list opens a stream it never closes");

            return false;
        }

        const Bool Placed = Parsed.Contains([](ConstRef<Channel> Entry)
        {
            return Entry.Slot == ZyGraphic::VertexSlot::Position;
        });

        if (!Placed)
        {
            LOG_E("Model: the attribute list has to name 'Position', since nothing draws without it");

            return false;
        }

        Output = Move(Parsed);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Profile::From(ConstRef<Environment> Environment, Ref<Profile> Output)
    {
        Profile Result;
        Result.Scale    = Environment.GetNumber<Real32>("scale", Result.Scale);
        Result.Units    = Environment.GetNumber<Real32>("units", Result.Units);
        Result.Rate     = Environment.GetNumber<Real32>("rate",  Result.Rate);
        Result.Normals  = Environment.GetBool("normals",          Result.Normals);
        Result.Tangents = Environment.GetBool("tangents",         Result.Tangents);
        Result.Rig      = Environment.GetBool("rig",              Result.Rig);

        // An unknown name reads as the fallback, so reading against two fallbacks is what tells a typo from a match.
        if (const Text Name = Environment.GetText("interpolation", Text::Empty()); !Name.IsEmpty())
        {
            Result.Blend = ZyEnum::Cast(Name, Interpolation::Step);

            if (Result.Blend != ZyEnum::Cast(Name, Interpolation::Linear))
            {
                LOG_E("Model: '{0}' is not an interpolation; use Step, Linear, Cubic or Hermite", Name);

                return false;
            }
        }

        if (const Text Description = Environment.GetText("attributes", Text::Empty()); !Description.IsEmpty())
        {
            if (!Parse(Description, Result.Attributes))
            {
                return false;
            }
        }

        Output = Move(Result);
        return true;
    }
}