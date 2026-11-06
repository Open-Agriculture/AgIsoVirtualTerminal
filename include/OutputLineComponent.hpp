//================================================================================================
/// @file OutputLineComponent.hpp
///
/// @brief Defines a GUI component to draw an output line.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef OUTPUT_LINE_COMPONENT_HPP
#define OUTPUT_LINE_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

#include <cstdint>
#include <vector>

class OutputLineComponent : public isobus::OutputLine
  , public Component
{
public:
	OutputLineComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputLine sourceObject);

	void paint(Graphics &g) override;

private:
	/// @brief Returns the points of a line from Bresenham's line algorithm, from its start to its end
	static std::vector<Point<int>> get_line_points(Point<int> from, Point<int> to);

	/// @brief Paints a dashed line: one brush per drawn spot of the line art.
	/// @details Spot k is the brush at the point k brush sizes along the line's longer axis (ISO 11783-6:2018,
	/// Figures B.14 and B.15). A spot cut short by the end of the line is the part of the last brush past its start.
	static void paint_line_art(Graphics &g, const std::vector<Point<int>> &points, Point<int> from, Point<int> to, int brushSize, std::uint16_t lineArt);

	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputLineComponent)
};

#endif // OUTPUT_LINE_COMPONENT_HPP
