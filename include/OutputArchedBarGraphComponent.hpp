//================================================================================================
/// @file OutputArchedBarGraphComponent.hpp
///
/// @brief Defines a GUI component to draw an arched bar graph.
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef OUTPUT_ARCHED_BAR_GRAPH_COMPONENT_HPP
#define OUTPUT_ARCHED_BAR_GRAPH_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class OutputArchedBarGraphComponent : public isobus::OutputArchedBarGraph
  , public Component
{
public:
	OutputArchedBarGraphComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputArchedBarGraph sourceObject);

	void paint(Graphics &g) override;

private:
	/// @brief Returns the value of the referenced number variable, or the fallback if there is no such variable
	std::uint16_t get_referenced_value(std::uint16_t variableReference, std::uint16_t fallback) const;

	/// @brief Returns the point where a ray from the centre, at the given angle anticlockwise from the positive X axis, meets the ellipse
	static Point<float> point_on_ellipse(Point<float> centre, float radiusX, float radiusY, float angle);

	/// @brief Returns the part of the ring between the outer ellipse and the ellipse bandWidth pixels inside it,
	/// from one angle anticlockwise to the other (both in radians from the positive X axis)
	static Path make_band(Point<float> centre, float radiusX, float radiusY, float bandWidth, float fromAngle, float toAngle);

	/// @brief Returns how far value is from minimum to maximum, from 0 to 1. Values outside the range show as empty or full.
	static float get_fraction(std::uint16_t value, std::uint16_t minimum, std::uint16_t maximum);

	/// @brief Returns the angle, in radians, that a value is drawn at on the arc starting at startAngle
	float get_value_angle(std::uint16_t value, float startAngle, float sweep) const;

	/// @brief Draws a line across the bar at the given angle
	static void draw_radial_line(Graphics &g, Point<float> centre, float radiusX, float radiusY, float bandWidth, float angle, float thickness);

	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputArchedBarGraphComponent)
};

#endif // OUTPUT_ARCHED_BAR_GRAPH_COMPONENT_HPP
