/*******************************************************************************
** @file       OutputLinearBarGraphComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
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
	float targetRatioToMax = static_cast<float>(get_target_value()) / static_cast<float>(get_max_value());
	auto vtBackgroundColour = parentWorkingSet->get_colour(get_colour());
	auto vtTargetLineColour = parentWorkingSet->get_colour(get_target_line_colour());
	g.setColour(Colour::fromFloatRGBA(vtBackgroundColour.r, vtBackgroundColour.g, vtBackgroundColour.b, 1.0));

	if (get_option(Options::DrawBorder))
	{
		g.drawRect(0, 0, getWidth(), getHeight(), 3);
	}

	if (isobus::NULL_OBJECT_ID != get_variable_reference())
	{
		auto child = get_object_by_id(get_variable_reference(), parentWorkingSet->get_object_tree());

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
				g.drawLine(static_cast<float>(get_width()) * valueRatioToMax, 0.0f, static_cast<float>(get_width()) * valueRatioToMax, static_cast<float>(get_height()), 3);

				if (get_option(Options::DrawTargetLine))
				{
					g.setColour(Colour::fromFloatRGBA(vtTargetLineColour.r, vtTargetLineColour.g, vtTargetLineColour.b, 1.0));
					g.drawVerticalLine(static_cast<float>(get_width()) * targetRatioToMax, 0.0f, static_cast<float>(get_height()));
				}
			}
			else
			{
				// From right
				g.drawLine(static_cast<float>(get_width()) * (1.0f - valueRatioToMax), 0.0f, static_cast<float>(get_width()) * (1.0f - valueRatioToMax), static_cast<float>(get_height()), 3);

				if (get_option(Options::DrawTargetLine))
				{
					g.setColour(Colour::fromFloatRGBA(vtTargetLineColour.r, vtTargetLineColour.g, vtTargetLineColour.b, 1.0));
					g.drawVerticalLine(static_cast<float>(get_width()) * (1.0f - targetRatioToMax), 0.0f, static_cast<float>(get_height()));
				}
			}
		}
		else
		{
			// Y Axis
			if (get_option(Options::Direction))
			{
				// From bottom
				g.drawLine(0, (1.0f - valueRatioToMax) * getHeight(), getWidth(), (1.0f - valueRatioToMax) * getHeight(), 3);

				if (get_option(Options::DrawTargetLine))
				{
					g.setColour(Colour::fromFloatRGBA(vtTargetLineColour.r, vtTargetLineColour.g, vtTargetLineColour.b, 1.0));
					g.drawHorizontalLine(static_cast<float>(get_height() * (1.0f - targetRatioToMax)), 0.0f, static_cast<float>(get_width()));
				}
			}
			else
			{
				// From top
				g.drawLine(0, valueRatioToMax * getHeight(), getWidth(), valueRatioToMax * getHeight(), 3);

				if (get_option(Options::DrawTargetLine))
				{
					g.setColour(Colour::fromFloatRGBA(vtTargetLineColour.r, vtTargetLineColour.g, vtTargetLineColour.b, 1.0));
					g.drawHorizontalLine(static_cast<float>(get_height() * targetRatioToMax), 0.0f, static_cast<float>(get_width()));
				}
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

				if (get_option(Options::DrawTargetLine))
				{
					g.setColour(Colour::fromFloatRGBA(vtTargetLineColour.r, vtTargetLineColour.g, vtTargetLineColour.b, 1.0));
					g.drawVerticalLine(static_cast<float>(get_width()) * targetRatioToMax, 0.0f, static_cast<float>(get_height()));
				}
			}
			else
			{
				// From right
				g.fillRect(static_cast<float>(get_width()) * (1 - valueRatioToMax), 0.0f, static_cast<float>(get_width()) * valueRatioToMax, static_cast<float>(get_height()));

				if (get_option(Options::DrawTargetLine))
				{
					g.setColour(Colour::fromFloatRGBA(vtTargetLineColour.r, vtTargetLineColour.g, vtTargetLineColour.b, 1.0));
					g.drawVerticalLine(static_cast<float>(get_width()) * (1 - targetRatioToMax), 0.0f, static_cast<float>(get_height()));
				}
			}
		}
		else
		{
			// Y Axis
			if (get_option(Options::Direction))
			{
				// From bottom
				g.fillRect(0.0f, static_cast<float>(get_height() * (1 - valueRatioToMax)), static_cast<float>(get_width()), static_cast<float>(get_height()) * valueRatioToMax);

				if (get_option(Options::DrawTargetLine))
				{
					g.setColour(Colour::fromFloatRGBA(vtTargetLineColour.r, vtTargetLineColour.g, vtTargetLineColour.b, 1.0));
					g.drawHorizontalLine(static_cast<float>(get_height() * (1 - targetRatioToMax)), 0.0f, static_cast<float>(get_width()));
				}
			}
			else
			{
				// From top
				g.fillRect(0.0f, 0.0f, static_cast<float>(get_width()), static_cast<float>(get_height()) * valueRatioToMax);

				if (get_option(Options::DrawTargetLine))
				{
					g.setColour(Colour::fromFloatRGBA(vtTargetLineColour.r, vtTargetLineColour.g, vtTargetLineColour.b, 1.0));
					g.drawHorizontalLine(static_cast<float>(get_height() * targetRatioToMax), 0.0f, static_cast<float>(get_width()));
				}
			}
		}
	}
}
