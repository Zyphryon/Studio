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

#include "Document.hpp"
#include <Zyphryon.Graphic/Metadata.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Material
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void ReadValue(ConstRef<JsonObject> Source, Ref<Document::Constant> Output)
    {
        const ConstPtr<JsonValue> Value = Source.GetValue("Value");

        if (Value == nullptr)
        {
            return;
        }

        if (Value->IsBool())
        {
            Output.Value[0] = Value->GetBool() ? 1.0 : 0.0;
        }
        else if (Value->IsNumber())
        {
            Output.Value[0] = Value->GetNumber<Real64>();
        }
        else if (Value->IsString())
        {
            // The loader reads a colour's string as hexadecimal, in whole bytes for IntColor8 and fractions otherwise.
            const IntColor8 Colour = IntColor8::FromHexadecimal(Value->GetString());
            const Real64    Scale  = (Output.Type == ZyGraphic::Uniform::IntColor8) ? 1.0 : 1.0 / 255.0;

            Output.Value = Array(
                Colour.GetRed()   * Scale,
                Colour.GetGreen() * Scale,
                Colour.GetBlue()  * Scale,
                Colour.GetAlpha() * Scale);
        }
        else if (const JsonArray Values = Source.GetArray("Value"); !Values.IsNullOrEmpty())
        {
            for (UInt Index = 0; Index < 4 && Index < Values.GetSize(); ++Index)
            {
                Output.Value[Index] = Values.GetNumber<Real64>(Index);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void WriteValue(JsonObject Target, ConstRef<Document::Constant> Source)
    {
        const ZyGraphic::UniformMetadata Metadata = ZyGraphic::GetUniformMetadata(Source.Type);

        Target.SetString("Type", ZyEnum::GetName(Source.Type));

        if (Metadata.IsBoolean)
        {
            Target.SetBool("Value", Source.Value[0] != 0.0);
        }
        else if (Metadata.Components == 1)
        {
            if (Metadata.IsFloat)
            {
                Target.SetNumber<Real64>("Value", Source.Value[0]);
            }
            else
            {
                Target.SetNumber<SInt64>("Value", static_cast<SInt64>(Source.Value[0]));
            }
        }
        else
        {
            JsonArray Values = Target.SetArray("Value");

            for (UInt Index = 0; Index < Metadata.Components; ++Index)
            {
                if (Metadata.IsFloat)
                {
                    Values.AddNumber<Real64>(Source.Value[Index]);
                }
                else
                {
                    Values.AddNumber<SInt64>(static_cast<SInt64>(Source.Value[Index]));
                }
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ref<Document::Binding> Document::Bind(Text Name)
    {
        const auto Named = [Name](ConstRef<Binding> Entry) { return Entry.Name == Name; };

        if (const SInt Found = Bindings.Find(Named); Found >= 0)
        {
            return Bindings[Found];
        }

        Ref<Binding> Entry = Bindings.Append();
        Entry.Name = Str(Name);
        return Entry;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ref<Document::Constant> Document::Set(Text Name)
    {
        const auto Named = [Name](ConstRef<Constant> Entry) { return Entry.Name == Name; };

        if (const SInt Found = Constants.Find(Named); Found >= 0)
        {
            return Constants[Found];
        }

        Ref<Constant> Entry = Constants.Append();
        Entry.Name = Str(Name);
        return Entry;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Document::Remove(Text Name)
    {
        const UInt Textures = Bindings.RemoveSomeIf([Name](ConstRef<Binding> Entry)
        {
            return Entry.Name == Name;
        });
        const UInt Values   = Constants.RemoveSomeIf([Name](ConstRef<Constant> Entry)
        {
            return Entry.Name == Name;
        });
        return Textures + Values > 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Document::Read(Text Path, Ref<Document> Output)
    {
        Blob Input;

        if (Filesystem::Read(Path, Input) != Filesystem::Result::Success || Input == nullptr)
        {
            LOG_E("Material: failed to read '{0}'", Path);

            return false;
        }

        JsonValue Root = JsonDocument::Parse(Text(Input.GetData<Char>(), Input.GetSize()));

        // A `JsonObject` assumes it wraps an object, so the document is checked before it is wrapped.
        if (!Root.IsObject())
        {
            LOG_E("Material: '{0}' is not a JSON material", Path);

            return false;
        }

        const JsonObject Top(Root);

        Document Result;

        if (const JsonObject Images = Top.GetObject("Images"); Images.IsValid())
        {
            Images.ForEach([&](ConstRef<Str> Key, Ref<JsonValue> Node)
            {
                if (Node.IsObject())
                {
                    Result.Bind(Key).Path = Str(JsonObject(Node).GetString("Path"));
                }
            });
        }

        // A sampler is keyed by the same name as its texture, and one without a texture is kept as well.
        if (const JsonObject Samplers = Top.GetObject("Samplers"); Samplers.IsValid())
        {
            Samplers.ForEach([&](ConstRef<Str> Key, Ref<JsonValue> Node)
            {
                if (!Node.IsObject())
                {
                    return;
                }

                const JsonObject Sampler(Node);

                Ref<Binding> Target = Result.Bind(Key);
                Target.Sampled    = true;
                Target.Filter     = Sampler.GetEnum("Filter",       ZyGraphic::TextureFilter::Point);
                Target.AddressU   = Sampler.GetEnum("AddressModeU", ZyGraphic::TextureAddress::Clamp);
                Target.AddressV   = Sampler.GetEnum("AddressModeV", ZyGraphic::TextureAddress::Clamp);
                Target.AddressW   = Sampler.GetEnum("AddressModeW", ZyGraphic::TextureAddress::Clamp);
                Target.Comparison = Sampler.GetEnum("Comparison",   ZyGraphic::TestCondition::None);
                Target.Border     = Sampler.GetEnum("Border",       ZyGraphic::TextureBorder::OpaqueBlack);
            });
        }

        if (const JsonObject Parameters = Top.GetObject("Parameters"); Parameters.IsValid())
        {
            Parameters.ForEach([&](ConstRef<Str> Key, Ref<JsonValue> Node)
            {
                if (!Node.IsObject())
                {
                    return;
                }

                const JsonObject Source(Node);

                Ref<Constant> Target = Result.Set(Key);
                Target.Type = Source.GetEnum("Type", ZyGraphic::Uniform::Float);

                ReadValue(Source, Target);
            });
        }

        Output = Move(Result);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Document::Write(Text Path) const
    {
        JsonValue Root;
        Root.SetObject();

        JsonObject Top(Root);

        // Each section is only written when something goes in it, as the loader treats a missing one as empty.
        JsonObject Images;
        JsonObject Samplers;

        for (ConstRef<Binding> Entry : Bindings)
        {
            if (Entry.Path.IsEmpty())
            {
                continue;
            }

            if (!Images.IsValid())
            {
                Images = Top.SetObject("Images");
            }
            Images.SetObject(Entry.Name).SetString("Path", Entry.Path);
        }

        for (ConstRef<Binding> Entry : Bindings)
        {
            if (!Entry.Sampled)
            {
                continue;
            }

            if (!Samplers.IsValid())
            {
                Samplers = Top.SetObject("Samplers");
            }

            JsonObject Sampler = Samplers.SetObject(Entry.Name);
            Sampler.SetEnum("Filter",       Entry.Filter);
            Sampler.SetEnum("AddressModeU", Entry.AddressU);
            Sampler.SetEnum("AddressModeV", Entry.AddressV);
            Sampler.SetEnum("AddressModeW", Entry.AddressW);

            // Most samplers never compare or read a border, so those two are only written when they differ.
            if (Entry.Comparison != ZyGraphic::TestCondition::None)
            {
                Sampler.SetEnum("Comparison", Entry.Comparison);
            }
            if (Entry.Border != ZyGraphic::TextureBorder::OpaqueBlack)
            {
                Sampler.SetEnum("Border", Entry.Border);
            }
        }

        if (!Constants.IsEmpty())
        {
            JsonObject Parameters = Top.SetObject("Parameters");

            for (ConstRef<Constant> Entry : Constants)
            {
                WriteValue(Parameters.SetObject(Entry.Name), Entry);
            }
        }

        const Str Content = JsonDocument::Dump(Root, "    ");

        Filesystem::Ensure(Path);

        if (Filesystem::Write(Path, Text(Content)) != Filesystem::Result::Success)
        {
            LOG_E("Material: failed to write '{0}'", Path);

            return false;
        }
        return true;
    }
}