/*******************************************************************************
** @file       OutputLineComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputLineComponent.hpp"
#include "LineArt.hpp"

OutputLineComponent::OutputLineComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputLine sourceObject) :
  isobus::OutputLine(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
}

void OutputLineComponent::paint(Graphics &g)
{
	if (isobus::NULL_OBJECT_ID != get_line_attributes())
	{
		auto child = get_object_by_id(get_line_attributes(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::LineAttributes == child->get_object_type()))
		{
			auto line = std::static_pointer_cast<isobus::LineAttributes>(child);
			const int brushSize = line->get_width();
			const std::uint16_t lineArt = line->get_line_art_bit_pattern();

			if ((0 != get_width()) && (0 != get_height()) && (0 != brushSize))
			{
				auto vtColour = parentWorkingSet->get_colour(line->get_background_color());
				g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));

				// ISO 11783-6 draws lines with a square paintbrush the size of the line width, each point of the line
				// being the brush's upper left corner. The brush stays inside the object's box, so the line runs
				// between the corners that leave room for it; where the box is smaller than the brush, the brush is
				// clipped to the box (Figure B.4).
				const int endX = std::max(0, static_cast<int>(get_width()) - brushSize);
				const int endY = std::max(0, static_cast<int>(get_height()) - brushSize);
				const bool bottomLeftToTopRight = (LineDirection::BottomLeftToTopRight == get_line_direction());
				const Point<int> from(0, bottomLeftToTopRight ? endY : 0);
				const Point<int> to(endX, bottomLeftToTopRight ? 0 : endY);

				// Bresenham's line algorithm, painting the brush at every point. The line art has one bit per spot
				// the size of the brush, so the spot of a point is its step along the line divided by the brush size.
				const int deltaX = std::abs(to.x - from.x);
				const int deltaY = -std::abs(to.y - from.y);
				const int stepX = (from.x < to.x) ? 1 : -1;
				const int stepY = (from.y < to.y) ? 1 : -1;
				int error = deltaX + deltaY;
				Point<int> point = from;
				int step = 0;

				while (true)
				{
					if (line_art::is_spot_drawn(lineArt, step / brushSize))
					{
						g.fillRect(point.x, point.y, brushSize, brushSize);
					}
					step++;
					if (point == to)
					{
						break;
					}
					const int doubledError = 2 * error;
					if (doubledError >= deltaY)
					{
						error += deltaY;
						point.x += stepX;
					}
					if (doubledError <= deltaX)
					{
						error += deltaX;
						point.y += stepY;
					}
				}
			}
		}
	}
}
