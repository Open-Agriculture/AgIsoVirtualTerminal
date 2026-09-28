/*******************************************************************************
** @file       OutputArchedBarGraphComponent.cpp
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputArchedBarGraphComponent.hpp"

#include <algorithm>
#include <cmath>

/// The start and end angle attributes count in steps of 2 degrees
static constexpr float DEGREES_PER_ANGLE_STEP = 2.0f;
/// The arcs are drawn as polygons with a corner at least every degree
static constexpr float RADIANS_PER_ARC_SEGMENT = MathConstants<float>::pi / 180.0f;

OutputArchedBarGraphComponent::OutputArchedBarGraphComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputArchedBarGraph sourceObject) :
  isobus::OutputArchedBarGraph(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
}

std::uint16_t OutputArchedBarGraphComponent::get_referenced_value(std::uint16_t variableReference, std::uint16_t fallback) const
{
	if (isobus::NULL_OBJECT_ID != variableReference)
	{
		auto child = get_object_by_id(variableReference, parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type()))
		{
			return static_cast<std::uint16_t>(std::min<std::uint32_t>(std::static_pointer_cast<isobus::NumberVariable>(child)->get_value(), 0xFFFF));
		}
	}
	return fallback;
}

Point<float> OutputArchedBarGraphComponent::point_on_ellipse(Point<float> centre, float radiusX, float radiusY, float angle)
{
	// On an ellipse that is not a circle, the point on the ray is not the point at the same parametric angle
	const float parametricAngle = std::atan2(radiusX * std::sin(angle), radiusY * std::cos(angle));
	return { centre.x + radiusX * std::cos(parametricAngle), centre.y - radiusY * std::sin(parametricAngle) };
}

Path OutputArchedBarGraphComponent::make_band(Point<float> centre, float radiusX, float radiusY, float bandWidth, float fromAngle, float toAngle)
{
	const int segments = std::max(2, static_cast<int>(std::ceil((toAngle - fromAngle) / RADIANS_PER_ARC_SEGMENT)));
	const float innerRadiusX = std::max(0.0f, radiusX - bandWidth);
	const float innerRadiusY = std::max(0.0f, radiusY - bandWidth);
	Path band;

	for (int i = 0; i <= segments; i++)
	{
		const auto point = point_on_ellipse(centre, radiusX, radiusY, fromAngle + (toAngle - fromAngle) * static_cast<float>(i) / static_cast<float>(segments));
		if (0 == i)
		{
			band.startNewSubPath(point);
		}
		else
		{
			band.lineTo(point);
		}
	}
	for (int i = segments; i >= 0; i--)
	{
		band.lineTo(point_on_ellipse(centre, innerRadiusX, innerRadiusY, fromAngle + (toAngle - fromAngle) * static_cast<float>(i) / static_cast<float>(segments)));
	}
	band.closeSubPath();
	return band;
}

float OutputArchedBarGraphComponent::get_fraction(std::uint16_t value, std::uint16_t minimum, std::uint16_t maximum)
{
	if (maximum <= minimum)
	{
		return (value >= maximum) ? 1.0f : 0.0f;
	}
	return jlimit(0.0f, 1.0f, (static_cast<float>(value) - static_cast<float>(minimum)) / (static_cast<float>(maximum) - static_cast<float>(minimum)));
}

float OutputArchedBarGraphComponent::get_value_angle(std::uint16_t value, float startAngle, float sweep) const
{
	// The value grows from the start angle anticlockwise, or from the end angle clockwise
	const float fraction = get_fraction(value, get_min_value(), get_max_value());
	return get_option(Options::Deflection) ? (startAngle + sweep - fraction * sweep) : (startAngle + fraction * sweep);
}

void OutputArchedBarGraphComponent::draw_radial_line(Graphics &g, Point<float> centre, float radiusX, float radiusY, float bandWidth, float angle, float thickness)
{
	const auto inner = point_on_ellipse(centre, std::max(0.0f, radiusX - bandWidth), std::max(0.0f, radiusY - bandWidth), angle);
	const auto outer = point_on_ellipse(centre, radiusX, radiusY, angle);
	g.drawLine({ inner, outer }, thickness);
}

void OutputArchedBarGraphComponent::paint(Graphics &g)
{
	if ((get_width() < 2) || (get_height() < 2))
	{
		return;
	}

	// The graph is drawn on the ellipse that fills the object's rectangle, half a pixel in so the border stays inside it
	const Point<float> centre(static_cast<float>(get_width()) / 2.0f, static_cast<float>(get_height()) / 2.0f);
	const float radiusX = centre.x - 0.5f;
	const float radiusY = centre.y - 0.5f;

	// The VT may draw a thinner bar than requested when it would not fit, but must not store the reduced value
	const float bandWidth = jlimit(1.0f, std::min(radiusX, radiusY), static_cast<float>(get_bar_graph_width()));

	// Angles run anticlockwise from the positive X axis; equal start and end angles mean a closed ring
	const float startAngle = degreesToRadians(DEGREES_PER_ANGLE_STEP * static_cast<float>(get_start_angle()));
	float sweep = degreesToRadians(DEGREES_PER_ANGLE_STEP * (static_cast<float>(get_end_angle()) - static_cast<float>(get_start_angle())));
	if (sweep <= 0.0f)
	{
		sweep += MathConstants<float>::twoPi;
	}
	const float endAngle = startAngle + sweep;
	const bool clockwise = get_option(Options::Deflection);

	const auto vtColour = parentWorkingSet->get_colour(get_colour());
	const auto colour = Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f);
	const float valueAngle = get_value_angle(get_referenced_value(get_variable_reference(), get_value()), startAngle, sweep);
	g.setColour(colour);

	if (get_option(Options::BarGraphType))
	{
		// Not filled: the value is a line across the bar
		draw_radial_line(g, centre, radiusX, radiusY, bandWidth, valueAngle, 3.0f);
	}
	else if (clockwise && (valueAngle < endAngle))
	{
		g.fillPath(make_band(centre, radiusX, radiusY, bandWidth, valueAngle, endAngle));
	}
	else if (!clockwise && (valueAngle > startAngle))
	{
		g.fillPath(make_band(centre, radiusX, radiusY, bandWidth, startAngle, valueAngle));
	}

	if (get_option(Options::DrawBorder))
	{
		if (sweep >= MathConstants<float>::twoPi)
		{
			// A closed ring has no ends to draw
			g.drawEllipse(centre.x - radiusX, centre.y - radiusY, 2.0f * radiusX, 2.0f * radiusY, 1.0f);
			const float innerRadiusX = std::max(0.0f, radiusX - bandWidth);
			const float innerRadiusY = std::max(0.0f, radiusY - bandWidth);
			g.drawEllipse(centre.x - innerRadiusX, centre.y - innerRadiusY, 2.0f * innerRadiusX, 2.0f * innerRadiusY, 1.0f);
		}
		else
		{
			g.strokePath(make_band(centre, radiusX, radiusY, bandWidth, startAngle, endAngle), PathStrokeType(1.0f));
		}
	}

	if (get_option(Options::DrawTargetLine))
	{
		const auto vtTargetLineColour = parentWorkingSet->get_colour(get_target_line_colour());
		g.setColour(Colour::fromFloatRGBA(vtTargetLineColour.r, vtTargetLineColour.g, vtTargetLineColour.b, 1.0f));
		draw_radial_line(g, centre, radiusX, radiusY, bandWidth, get_value_angle(get_referenced_value(get_target_value_reference(), get_target_value()), startAngle, sweep), 1.0f);
	}
}
