/*******************************************************************************
** @file       OutputPolygonComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputPolygonComponent.hpp"

OutputPolygonComponent::OutputPolygonComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputPolygon sourceObject) :
  isobus::OutputPolygon(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
}

void OutputPolygonComponent::paint(Graphics &g)
{
	// 3 Points MUST exist or the object cannot be drawn
	if (get_number_of_points() >= 3)
	{
		Path polygonPath;
		float lineWidth = 0.0f;
		auto lineColour = Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, 1.0);

		if (isobus::NULL_OBJECT_ID != get_line_attributes())
		{
			auto child = get_object_by_id(get_line_attributes(), parentWorkingSet->get_object_tree());

			if ((nullptr != child) && (isobus::VirtualTerminalObjectType::LineAttributes == child->get_object_type()))
			{
				auto line = std::static_pointer_cast<isobus::LineAttributes>(child);
				lineWidth = line->get_width();
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

		// If the polygon type is not open, it must be closed by us
		if (PolygonType::Open != get_type())
		{
			const auto thisPoint = get_point(static_cast<std::uint8_t>(get_number_of_points() - 1));
			const auto nextPoint = get_point(0U);

			polygonPath.closeSubPath();
		}

		if (isobus::NULL_OBJECT_ID != get_fill_attributes())
		{
			auto child = get_object_by_id(get_fill_attributes(), parentWorkingSet->get_object_tree());

			if ((nullptr != child) && (isobus::VirtualTerminalObjectType::FillAttributes == child->get_object_type()))
			{
				auto fill = std::static_pointer_cast<isobus::FillAttributes>(child);

				if (fill->get_type() == isobus::FillAttributes::FillType::FillWithLineColor)
				{
					g.setFillType(FillType(lineColour));
				}
				else if (fill->get_type() == isobus::FillAttributes::FillType::FillWithSpecifiedColorInFillColorAttribute)
				{
					auto vtColour = parentWorkingSet->get_colour(fill->get_background_color());
					g.setFillType(FillType(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f)));
				}
				else
				{
					// @ TODO support gradient fill
				}

				if (fill->get_type() != isobus::FillAttributes::FillType::NoFill)
					g.fillPath(polygonPath);
			}
		}

		g.resetToDefaultState();
		g.setColour(lineColour);
		g.strokePath(polygonPath, PathStrokeType(lineWidth));
	}
}