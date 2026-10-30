/*******************************************************************************
** @file       OutputLinearBarGraphComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputLinearBarGraphComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

static std::uint32_t get_value_or_variable(std::uint32_t value, std::uint16_t variableReference, std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	if (isobus::NULL_OBJECT_ID != variableReference)
	{
		auto child = isobus::VTObject::get_object_by_id(variableReference, workingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type()))
		{
			value = std::static_pointer_cast<isobus::NumberVariable>(child)->get_value();
		}
	}
	return value;
}

static float get_position(std::uint32_t value, std::uint16_t minValue, std::uint16_t maxValue)
{
	float position = 0.0f;
	if (maxValue > minValue)
	{
		position = jlimit(0.0f, 1.0f, (static_cast<float>(value) - minValue) / static_cast<float>(maxValue - minValue));
	}
	return position;
}

// distance is measured from the minimum end of the bar
static Rectangle<float> get_band(Rectangle<float> bounds, bool horizontal, bool growsPositive, float distance, float size)
{
	if (horizontal)
	{
		return { growsPositive ? distance : bounds.getWidth() - distance - size, 0.0f, size, bounds.getHeight() };
	}
	return { 0.0f, growsPositive ? bounds.getHeight() - distance - size : distance, bounds.getWidth(), size };
}

OutputLinearBarGraphComponent::OutputLinearBarGraphComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputLinearBarGraph sourceObject) :
  isobus::OutputLinearBarGraph(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
}

void OutputLinearBarGraphComponent::paint(Graphics &g)
{
	const auto bounds = getLocalBounds().toFloat();
	const bool horizontal = get_option(Options::AxisOrientation);
	const bool growsPositive = get_option(Options::Direction);
	const float length = horizontal ? bounds.getWidth() : bounds.getHeight();
	const float valuePosition = get_position(get_value_or_variable(get_value(), get_variable_reference(), parentWorkingSet), get_min_value(), get_max_value());
	const float targetPosition = get_position(get_value_or_variable(get_target_value(), get_target_value_reference(), parentWorkingSet), get_min_value(), get_max_value());

	auto vtColour = parentWorkingSet->get_colour(get_colour());
	g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0));

	if (get_option(Options::DrawBorder))
	{
		g.drawRect(0, 0, getWidth(), getHeight(), 3);
	}

	if (get_option(Options::BarGraphType))
	{
		const float lineWidth = std::min(3.0f, length);
		g.fillRect(get_band(bounds, horizontal, growsPositive, valuePosition * (length - lineWidth), lineWidth));
	}
	else
	{
		g.fillRect(get_band(bounds, horizontal, growsPositive, 0.0f, valuePosition * length));
	}

	if (get_option(Options::DrawTargetLine))
	{
		auto vtTargetLineColour = parentWorkingSet->get_colour(get_target_line_colour());
		g.setColour(Colour::fromFloatRGBA(vtTargetLineColour.r, vtTargetLineColour.g, vtTargetLineColour.b, 1.0));
		g.fillRect(get_band(bounds, horizontal, growsPositive, targetPosition * (length - 1.0f), 1.0f));
	}
}
