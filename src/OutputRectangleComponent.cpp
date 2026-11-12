/*******************************************************************************
** @file       OutputRectangleComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputRectangleComponent.hpp"
#include "LineArt.hpp"
#include "PictureGraphicComponent.hpp"

OutputRectangleComponent::OutputRectangleComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputRectangle sourceObject) :
  isobus::OutputRectangle(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
}

void OutputRectangleComponent::paint(Graphics &g)
{
	std::shared_ptr<isobus::LineAttributes> line;
	std::optional<Colour> lineColour;
	auto lineObject = get_object_by_id(get_line_attributes(), parentWorkingSet->get_object_tree());

	if ((nullptr != lineObject) && (isobus::VirtualTerminalObjectType::LineAttributes == lineObject->get_object_type()))
	{
		line = std::static_pointer_cast<isobus::LineAttributes>(lineObject);
		const auto vtColour = parentWorkingSet->get_colour(line->get_background_color());
		lineColour = Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f);
	}

	const auto fill = PictureGraphicComponent::get_shape_fill(*this, parentWorkingSet, get_fill_attributes(), lineColour);
	if (fill.has_value())
	{
		// The fill stays inside the border, also where a side is suppressed or the line art skips a spot
		g.setFillType(fill.value());
		g.fillRect(Rectangle<int>(0, 0, get_width(), get_height()).reduced((nullptr != line) ? line->get_width() : 0));
	}

	if ((nullptr != line) && (0 != line->get_width()))
	{
		const int lineWidth = line->get_width();
		const int width = static_cast<int>(get_width());
		const int height = static_cast<int>(get_height());
		const auto isSuppressed = [this](LineSuppressionOption side) {
			return 0 != ((0x01 << static_cast<std::uint8_t>(side)) & get_line_suppression_bitfield());
		};
		g.setColour(lineColour.value());

		// A corner is only drawn when both of its sides are, see ISO 11783-6 Figure 21
		if (isSuppressed(LineSuppressionOption::SuppressTopLine))
		{
			g.excludeClipRegion({ 0, 0, width, lineWidth });
		}
		if (isSuppressed(LineSuppressionOption::SuppressBottomLine))
		{
			g.excludeClipRegion({ 0, height - lineWidth, width, lineWidth });
		}
		if (isSuppressed(LineSuppressionOption::SuppressLeftSideLine))
		{
			g.excludeClipRegion({ 0, 0, lineWidth, height });
		}
		if (isSuppressed(LineSuppressionOption::SuppressRightSideLine))
		{
			g.excludeClipRegion({ width - lineWidth, 0, lineWidth, height });
		}

		// The square brush stays inside the box, so each side is a strip of the line width along its edge
		const std::uint16_t lineArt = line->get_line_art_bit_pattern();
		if (line_art::SOLID == lineArt)
		{
			if (!isSuppressed(LineSuppressionOption::SuppressTopLine))
			{
				g.fillRect(0, 0, width, lineWidth);
			}
			if (!isSuppressed(LineSuppressionOption::SuppressBottomLine))
			{
				g.fillRect(0, height - lineWidth, width, lineWidth);
			}
			if (!isSuppressed(LineSuppressionOption::SuppressLeftSideLine))
			{
				g.fillRect(0, 0, lineWidth, height);
			}
			if (!isSuppressed(LineSuppressionOption::SuppressRightSideLine))
			{
				g.fillRect(width - lineWidth, 0, lineWidth, height);
			}
		}
		else
		{
			// The line art runs clockwise around the box from the top left corner, one bit per brush sized
			// spot. A suppressed side still takes its share of the pattern.
			int spot = 0;
			const auto paintSide = [&](LineSuppressionOption side, int length, const std::function<Point<int>(int)> &spotPosition) {
				const int spots = (length + lineWidth - 1) / lineWidth;
				for (int i = 0; i < spots; i++, spot++)
				{
					if (!isSuppressed(side) && line_art::is_spot_drawn(lineArt, spot))
					{
						const auto position = spotPosition(i);
						g.fillRect(position.x, position.y, lineWidth, lineWidth);
					}
				}
			};
			paintSide(LineSuppressionOption::SuppressTopLine, width, [&](int i) { return Point<int>(i * lineWidth, 0); });
			paintSide(LineSuppressionOption::SuppressRightSideLine, height, [&](int i) { return Point<int>(width - lineWidth, i * lineWidth); });
			paintSide(LineSuppressionOption::SuppressBottomLine, width, [&](int i) { return Point<int>(width - lineWidth - i * lineWidth, height - lineWidth); });
			paintSide(LineSuppressionOption::SuppressLeftSideLine, height, [&](int i) { return Point<int>(0, height - lineWidth - i * lineWidth); });
		}
	}
}
