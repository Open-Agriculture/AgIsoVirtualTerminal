/*******************************************************************************
** @file       OutputPolygonComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputPolygonComponent.hpp"
#include "LineArt.hpp"
#include "PictureGraphicComponent.hpp"

// An edge bounds the inside where exactly one of its sides is inside. A retraced edge has both or neither, and a
// crossing edge has neither at the crossing, so each edge is sampled at three points.
static bool bounds_inside(const Path &polygon, Point<float> from, Point<float> to)
{
	const auto side = Point<float>(from.y - to.y, to.x - from.x) * (0.5f / from.getDistanceFrom(to));
	bool bounds = false;

	for (int quarter = 1; (quarter <= 3) && !bounds; quarter++)
	{
		const auto sample = from + (to - from) * (quarter / 4.0f);
		bounds = (polygon.contains(sample + side) != polygon.contains(sample - side));
	}
	return bounds;
}

// The distance carries over between edges so the line art keeps its phase whichever way each edge grows
static void add_line_art(Path &pieces, Path &corners, Point<float> from, Point<float> to, double &distance, float spotLength, std::uint16_t lineArt)
{
	const double length = from.getDistanceFrom(to);
	const auto at = [&](double along) { return from + (to - from) * static_cast<float>(along / length); };
	// Nudged so a spot boundary that lands on the current position counts as already passed
	constexpr double BOUNDARY_TOLERANCE = 1e-6;
	double start = 0.0;

	while (start < length)
	{
		const double spot = std::floor((distance + start) / spotLength + BOUNDARY_TOLERANCE);
		const double end = (line_art::SOLID == lineArt) ? length : std::min(length, (spot + 1.0) * spotLength - distance);

		if ((line_art::SOLID == lineArt) || line_art::is_spot_drawn(lineArt, static_cast<int>(spot)))
		{
			pieces.startNewSubPath(at(start));
			pieces.lineTo(at(end));
			if (0.0 == start)
			{
				corners.startNewSubPath(from);
				corners.lineTo(from + Point<float>(0.01f, 0.0f));
			}
			if (end >= length)
			{
				corners.startNewSubPath(to);
				corners.lineTo(to + Point<float>(0.01f, 0.0f));
			}
		}
		start = end;
	}
	distance += length;
}

// A corner dot is a tiny stroked segment rather than an ellipse, so it winds the same way as the pieces it joins;
// opposite windings would cancel where they overlap
static Path stroke_pieces(const Path &pieces, const Path &corners, float width, PathStrokeType::EndCapStyle cornerStyle)
{
	Path area;
	Path cornerArea;
	PathStrokeType(width).createStrokedPath(area, pieces);
	PathStrokeType(width, PathStrokeType::mitered, cornerStyle).createStrokedPath(cornerArea, corners);
	area.addPath(cornerArea);
	return area;
}

OutputPolygonComponent::OutputPolygonComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputPolygon sourceObject) :
  isobus::OutputPolygon(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
}

void OutputPolygonComponent::paint(Graphics &g)
{
	if (get_number_of_points() >= 3)
	{
		Path polygonPath;
		// The standard allows only even-odd
		polygonPath.setUsingNonZeroWinding(false);
		float lineWidth = 0.0f;
		std::uint16_t lineArt = line_art::SOLID;
		auto lineColour = Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 1.0);

		if (isobus::NULL_OBJECT_ID != get_line_attributes())
		{
			auto child = get_object_by_id(get_line_attributes(), parentWorkingSet->get_object_tree());

			if ((nullptr != child) && (isobus::VirtualTerminalObjectType::LineAttributes == child->get_object_type()))
			{
				auto line = std::static_pointer_cast<isobus::LineAttributes>(child);
				lineWidth = line->get_width();
				lineArt = line->get_line_art_bit_pattern();
				auto lineAttributeColour = parentWorkingSet->get_colour(line->get_background_color());

				lineColour = Colour::fromFloatRGBA(lineAttributeColour.r, lineAttributeColour.g, lineAttributeColour.b, 1.0);
			}
		}

		for (std::uint16_t i = 0; i < get_number_of_points() - 1; i++)
		{
			const auto thisPoint = get_point(static_cast<std::uint8_t>(i));
			const auto nextPoint = get_point(static_cast<std::uint8_t>(i + 1));

			if (0 == i)
			{
				polygonPath.startNewSubPath(static_cast<float>(thisPoint.xValue), static_cast<float>(thisPoint.yValue));
			}
			polygonPath.lineTo(static_cast<float>(nextPoint.xValue), static_cast<float>(nextPoint.yValue));
		}

		if (PolygonType::Open != get_type())
		{
			polygonPath.closeSubPath();
		}

		// Closed polygons grow their lines inward from the outer edge of the pixels the points name, so lines on the
		// box edge aren't clipped. Each edge is stroked on its own, as one stroke wider than the shape folds back into a
		// hole. Open polygons and retraced edges have no inside to grow into, so they stay centred and clip on the box.
		const float brushWidth = 2.0f * lineWidth - 1.0f;
		const auto split = [&](std::uint16_t art, Path &inward, Path &inwardCorners, Path &centred, Path &centredCorners) {
			inward = inwardCorners = centred = centredCorners = Path();
			double distance = 0.0;
			for (std::uint16_t i = 0; i < get_number_of_points(); i++)
			{
				const auto thisPoint = get_point(static_cast<std::uint8_t>(i));
				const auto nextPoint = get_point(static_cast<std::uint8_t>((i + 1) % get_number_of_points()));
				const Point<float> from(thisPoint.xValue, thisPoint.yValue);
				const Point<float> to(nextPoint.xValue, nextPoint.yValue);

				if (from != to)
				{
					const bool grows = bounds_inside(polygonPath, from, to);
					add_line_art(grows ? inward : centred, grows ? inwardCorners : centredCorners, from, to, distance, lineWidth, art);
				}
			}
			inward.applyTransform(AffineTransform::translation(0.5f, 0.5f));
			inwardCorners.applyTransform(AffineTransform::translation(0.5f, 0.5f));
		};
		Path inwardLines, inwardCorners, centredLines, centredCorners;
		if ((PolygonType::Open != get_type()) && (lineWidth > 0.0f))
		{
			split(line_art::SOLID, inwardLines, inwardCorners, centredLines, centredCorners);
		}
		const bool inward = !inwardLines.isEmpty();
		Path pixelCentres(polygonPath);
		pixelCentres.applyTransform(AffineTransform::translation(0.5f, 0.5f));

		// Open polygons are never filled
		const auto fill = (PolygonType::Open != get_type()) ? PictureGraphicComponent::get_shape_fill(*this, parentWorkingSet, get_fill_attributes(), lineColour) : std::nullopt;
		if (fill.has_value())
		{
			Path border = line_art::outline(polygonPath, lineWidth, line_art::SOLID);
			if (inward)
			{
				border = stroke_pieces(inwardLines, inwardCorners, brushWidth, PathStrokeType::rounded);
				border.addPath(stroke_pieces(centredLines, centredCorners, lineWidth, PathStrokeType::rounded));
			}
			g.setFillType(fill.value());
			line_art::fill_inside_border(g, inward ? pixelCentres : polygonPath, border, lineArt);
		}

		g.resetToDefaultState();
		g.setColour(lineColour);
		if (inward)
		{
			if (line_art::SOLID != lineArt)
			{
				split(lineArt, inwardLines, inwardCorners, centredLines, centredCorners);
			}
			// The wide stroke clipped to the inside reaches half a pixel short of the outer edge, the 1 px stroke covers
			// that. Round corners fill the inner side of concave corners, square ones the outer pixel of convex ones.
			g.saveState();
			if (g.reduceClipRegion(pixelCentres))
			{
				g.fillPath(stroke_pieces(inwardLines, inwardCorners, brushWidth, PathStrokeType::rounded));
			}
			g.restoreState();
			g.fillPath(stroke_pieces(inwardLines, inwardCorners, 1.0f, PathStrokeType::square));
			g.fillPath(stroke_pieces(centredLines, centredCorners, lineWidth, PathStrokeType::rounded));
		}
		else
		{
			// Open, zero area or no line
			g.fillPath(line_art::outline(polygonPath, lineWidth, lineArt));
		}
	}
}