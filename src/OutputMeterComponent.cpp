/*******************************************************************************
** @file       OutputMeterComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputMeterComponent.hpp"

#include <cmath>

// ISO angles are anticlockwise from the positive X axis, but screen Y grows downwards
static Point<float> point_on_circle(float centre, float radius, float angleDegrees)
{
	const float angle = degreesToRadians(angleDegrees);
	return { centre + radius * std::cos(angle), centre - radius * std::sin(angle) };
}

OutputMeterComponent::OutputMeterComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputMeter sourceObject) :
  isobus::OutputMeter(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
}

void OutputMeterComponent::paint(Graphics &g)
{
	const float centre = static_cast<float>(get_width()) / 2.0f;
	const float radius = centre - 0.5f;
	const bool clockwise = get_option(Options::DeflectionDirection);
	const float startAngle = get_start_angle() * 2.0f;

	// Equal angles, 0 and 360 included, mean a full circle.
	// A clockwise needle starts at the end angle, which is how real pools draw a 0-180 clockwise gauge over the top.
	int sweep = ((get_end_angle() - get_start_angle()) * 2) % 360;
	if (sweep <= 0)
	{
		sweep += 360;
	}
	const float minAngle = clockwise ? startAngle + sweep : startAngle;
	const float signedSweep = static_cast<float>(clockwise ? -sweep : sweep);

	if (get_option(Options::DrawBorder))
	{
		auto vtColour = parentWorkingSet->get_colour(get_border_colour());
		g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
		g.drawRect(0, 0, static_cast<int>(get_width()), static_cast<int>(get_height()), 1);
	}

	auto arcColour = parentWorkingSet->get_colour(get_arc_and_tick_colour());
	g.setColour(Colour::fromFloatRGBA(arcColour.r, arcColour.g, arcColour.b, 1.0f));

	if (get_option(Options::DrawArc))
	{
		// JUCE arc angles are clockwise from 12 o'clock
		Path p;
		p.addCentredArc(centre, centre, radius, radius, 0.0f, degreesToRadians(90.0f - minAngle), degreesToRadians(90.0f - (minAngle + signedSweep)), true);
		g.strokePath(p, PathStrokeType(1.0f));
	}

	if ((get_option(Options::DrawTicks)) && (get_number_of_ticks() > 0))
	{
		const float tickLength = std::max(1.0f, get_width() / 10.0f);

		for (std::uint8_t i = 0; i < get_number_of_ticks(); i++)
		{
			const float position = (1 == get_number_of_ticks()) ? 0.5f : static_cast<float>(i) / static_cast<float>(get_number_of_ticks() - 1);
			const float angle = minAngle + signedSweep * position;
			g.drawLine(Line<float>(point_on_circle(centre, radius, angle), point_on_circle(centre, radius - tickLength, angle)), 1.0f);
		}
	}

	std::uint32_t needleValue = get_value();
	if (isobus::NULL_OBJECT_ID != get_variable_reference())
	{
		auto varNum = get_object_by_id(get_variable_reference(), parentWorkingSet->get_object_tree());

		if ((nullptr != varNum) && (isobus::VirtualTerminalObjectType::NumberVariable == varNum->get_object_type()))
		{
			needleValue = std::static_pointer_cast<isobus::NumberVariable>(varNum)->get_value();
		}
	}

	float position = 0.0f;
	if (get_max_value() > get_min_value())
	{
		position = jlimit(0.0f, 1.0f, (static_cast<float>(needleValue) - get_min_value()) / static_cast<float>(get_max_value() - get_min_value()));
	}

	auto vtColour = parentWorkingSet->get_colour(get_needle_colour());
	g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
	g.drawLine(Line<float>({ centre, centre }, point_on_circle(centre, radius, minAngle + signedSweep * position)), 3.0f);
}
