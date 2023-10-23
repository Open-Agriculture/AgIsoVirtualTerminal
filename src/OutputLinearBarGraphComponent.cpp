#include "OutputLinearBarGraphComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

OutputLinearBarGraphComponent::OutputLinearBarGraphComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputLinearBarGraph sourceObject) :
  isobus::OutputLinearBarGraph(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
}

void OutputLinearBarGraphComponent::paint(Graphics &g)
{
	float valueRatioToMax = static_cast<float>(get_value()) / static_cast<float>(get_max_value());
	auto vtBackgroundColour = colourTable.get_colour(get_colour());
	auto vtTargetLineColour = colourTable.get_colour(get_target_line_colour());
	g.setColour(Colour::fromFloatRGBA(vtBackgroundColour.r, vtBackgroundColour.g, vtBackgroundColour.b, 1.0));

	for (std::uint16_t i = 0; i < get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i));

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type()))
		{
			valueRatioToMax = static_cast<float>(std::static_pointer_cast<isobus::NumberVariable>(child)->get_value()) / static_cast<float>(get_max_value());
		}
	}

	// Figure out what kind of bar graph we are
	if (get_option(Options::BarGraphType))
	{
		// Not filled, but has value line

		if (get_option(Options::AxisOrientation))
		{
			// X Axis
			if (get_option(Options::Direction))
			{
				// From left
			}
			else
			{
				// From right
			}
		}
		else
		{
			// Y Axis
			if (get_option(Options::Direction))
			{
				// From left
			}
			else
			{
				// From right
			}
		}
	}
	else
	{
		// Filled

		if (get_option(Options::AxisOrientation))
		{
			// X Axis
			if (get_option(Options::Direction))
			{
				// From left
				g.fillRect(0.0f, 0.0f, static_cast<float>(get_width()) * valueRatioToMax, static_cast<float>(get_height()));
			}
			else
			{
				// From right
				g.fillRect(static_cast<float>(get_width()) * (1 - valueRatioToMax), 0.0f, static_cast<float>(get_width()) * valueRatioToMax, static_cast<float>(get_height()));
			}
		}
		else
		{
			// Y Axis
			if (get_option(Options::Direction))
			{
				// From bottom
				g.fillRect(0.0f, static_cast<float>(get_height() * (1 - valueRatioToMax)), static_cast<float>(get_width()), static_cast<float>(get_height()) * valueRatioToMax);
			}
			else
			{
				// From top
				g.fillRect(0.0f, 0.0f, static_cast<float>(get_width()), static_cast<float>(get_height()) * valueRatioToMax);
			}
		}
	}
}
