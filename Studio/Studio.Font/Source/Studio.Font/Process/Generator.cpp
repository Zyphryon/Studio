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

#include "Generator.hpp"
#include <msdfgen.h>
#include <core/ShapeDistanceFinder.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Studio::Font
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static msdfgen::Point2 Convert(Vector2 Value)
    {
        return msdfgen::Point2(Value.GetX(), Value.GetY());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Translate(ConstRef<Shape> Outline, Ref<msdfgen::Shape> Output)
    {
        // Both put the origin on the baseline with Y pointing up, so the points carry over as they are.
        Output.inverseYAxis = false;

        for (ConstRef<Contour> Loop : Outline.GetContours())
        {
            if (Loop.IsEmpty())
            {
                continue;
            }

            Ref<msdfgen::Contour> Target = Output.addContour();

            for (ConstRef<Edge> Segment : Loop)
            {
                switch (Segment.GetOrder())
                {
                case 1:
                    Target.addEdge(msdfgen::EdgeHolder(
                        Convert(Segment.GetPoint(0)),
                        Convert(Segment.GetPoint(1))));
                    break;
                case 2:
                    Target.addEdge(msdfgen::EdgeHolder(
                        Convert(Segment.GetPoint(0)),
                        Convert(Segment.GetPoint(1)),
                        Convert(Segment.GetPoint(2))));
                    break;
                case 3:
                    Target.addEdge(msdfgen::EdgeHolder(
                        Convert(Segment.GetPoint(0)),
                        Convert(Segment.GetPoint(1)),
                        Convert(Segment.GetPoint(2)),
                        Convert(Segment.GetPoint(3))));
                    break;
                default:
                    break;
                }
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Generator::Generate(ConstRef<Shape> Outline, Real32 Range, Real32 Angle, Ref<Field> Output)
    {
        Output.Width  = 0;
        Output.Height = 0;
        Output.Bounds = Rect();
        Output.Texels.Clear();

        if (Outline.IsEmpty())
        {
            return;
        }

        msdfgen::Shape Native;
        Translate(Outline, Native);

        Native.normalize();

        const msdfgen::Shape::Bounds Box = Native.getBounds();

        // An outline with no area draws nothing, and stb_truetype may even leave its lone point anywhere.
        if (Box.r <= Box.l || Box.t <= Box.b)
        {
            return;
        }

        // The shape is only ever reversed whole: reorienting contours one by one flips one stroke of an
        // overlapping pair, as variable fonts have, and punches a hole where they cross.
        const msdfgen::Point2 Outside(Box.l - (Box.r - Box.l) - 1.0, Box.b - (Box.t - Box.b) - 1.0);

        if (msdfgen::SimpleTrueShapeDistanceFinder::oneShotDistance(Native, Outside) > 0.0)
        {
            for (Ref<msdfgen::Contour> Loop : Native.contours)
            {
                Loop.reverse();
            }
        }

        msdfgen::edgeColoringSimple(Native, Angle);

        const msdfgen::Shape::Bounds Ink  = Native.getBounds();
        const Real32                 Half = Range * 0.5f;

        const Real32 SpanX  = static_cast<Real32>(Ink.r - Ink.l) + Range;
        const Real32 SpanY  = static_cast<Real32>(Ink.t - Ink.b) + Range;

        const UInt32 Width  = static_cast<UInt32>(Ceil(SpanX));
        const UInt32 Height = static_cast<UInt32>(Ceil(SpanY));

        if (Width == 0 || Height == 0)
        {
            return;
        }

        // Rounding the box out to whole pixels leaves a little slack, which is split evenly so the glyph stays
        // centred in its cell.
        const Real32 OriginX = (static_cast<Real32>(Ink.l) - Half) - (static_cast<Real32>(Width)  - SpanX) * 0.5f;
        const Real32 OriginY = (static_cast<Real32>(Ink.b) - Half) - (static_cast<Real32>(Height) - SpanY) * 0.5f;

        Output.Bounds = Rect(
            OriginX, OriginY, OriginX + static_cast<Real32>(Width), OriginY + static_cast<Real32>(Height));

        // One texel more than the box on each axis, so the first and last texel centres land exactly on the box's
        // edges. The atlas coordinates written later are inset by half a texel to match.
        Output.Width  = Width  + 1;
        Output.Height = Height + 1;

        // msdfgen samples texel centres, so shifting by half a texel puts texel zero exactly on the origin.
        const msdfgen::Projection Placement(
            msdfgen::Vector2(1.0, 1.0),
            msdfgen::Vector2(0.5 - OriginX, 0.5 - OriginY));

        const msdfgen::SDFTransformation Transformation(Placement, msdfgen::Range(Range));

        msdfgen::Bitmap<float, 4> Pixels(static_cast<int>(Output.Width), static_cast<int>(Output.Height));

        // Generate without error correction, fix each texel's sign with a non-zero scanline fill, and only then
        // correct the artifacts.
        const msdfgen::MSDFGeneratorConfig Generation(
            true, msdfgen::ErrorCorrectionConfig(msdfgen::ErrorCorrectionConfig::DISABLED));
        msdfgen::generateMTSDF(Pixels, Native, Transformation, Generation);

        msdfgen::distanceSignCorrection(Pixels, Native, Placement, msdfgen::FILL_NONZERO);

        msdfgen::MSDFGeneratorConfig Correction(true, msdfgen::ErrorCorrectionConfig(
            msdfgen::ErrorCorrectionConfig::EDGE_PRIORITY,
            msdfgen::ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE));
        msdfgen::msdfErrorCorrection(Pixels, Native, Transformation, Correction);

        Output.Texels.Advance(static_cast<UInt>(Output.Width) * Output.Height * 4);

        for (UInt32 Y = 0; Y < Output.Height; ++Y)
        {
            for (UInt32 X = 0; X < Output.Width; ++X)
            {
                const ConstPtr<float> Texel  = Pixels(static_cast<int>(X), static_cast<int>(Y));
                const UInt            Target = (static_cast<UInt>(Y) * Output.Width + X) * 4;

                Output.Texels[Target + 0] = EncodeNormalized<Byte>(Texel[0]);
                Output.Texels[Target + 1] = EncodeNormalized<Byte>(Texel[1]);
                Output.Texels[Target + 2] = EncodeNormalized<Byte>(Texel[2]);
                Output.Texels[Target + 3] = EncodeNormalized<Byte>(Texel[3]);
            }
        }
    }
}