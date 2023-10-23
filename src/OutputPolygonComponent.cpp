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

		for (std::uint16_t i = 0; i < get_number_children(); i++)
		{
			auto child = get_object_by_id(get_child_id(i));

			if ((nullptr != child) && (isobus::VirtualTerminalObjectType::LineAttributes == child->get_object_type()))
			{
				auto line = std::static_pointer_cast<isobus::LineAttributes>(child);
				lineWidth = line->get_width();
				auto lineAttributeColour = colourTable.get_colour(line->get_background_color());

				lineColour = Colour::fromFloatRGBA(lineAttributeColour.r, lineAttributeColour.g, lineAttributeColour.b, 1.0);
				break;
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

		for (std::uint16_t i = 0; i < get_number_children(); i++)
		{
			auto child = get_object_by_id(get_child_id(i));

			if ((nullptr != child) && (isobus::VirtualTerminalObjectType::FillAttributes == child->get_object_type()))
			{
				auto fill = std::static_pointer_cast<isobus::FillAttributes>(child);

				auto vtColour = colourTable.get_colour(fill->get_background_color());
				g.setFillType(FillType(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f)));
				break;
			}
		}

		g.fillPath(polygonPath);
		g.resetToDefaultState();
		g.setColour(lineColour);
		g.strokePath(polygonPath, PathStrokeType(lineWidth));
	}
}