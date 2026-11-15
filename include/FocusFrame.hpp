//================================================================================================
/// @file FocusFrame.hpp
///
/// @brief Declares the frame that shows which object has focus.
/// @author Gunics Balázs
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef FOCUS_FRAME_HPP
#define FOCUS_FRAME_HPP

#include "JuceHeader.h"

#include <cstdint>

/// @brief Draws the focus indicator that ISO 11783-6 requires for input objects, Buttons and Keys
class FocusFrame
{
public:
	/// @brief Draws a frame around each visible component of an object
	/// @param[in] g The graphics context of the area
	/// @param[in] area The component whose children are searched, in whose coordinates the frame is drawn
	/// @param[in] objectID The object that has focus, or NULL_OBJECT_ID for none
	static void paint(Graphics &g, Component &area, std::uint16_t objectID);

private:
	static void paint_children(Graphics &g, Component &area, Component &parent, std::uint16_t objectID);

	static constexpr float FRAME_WIDTH = 3.0f;
	static const Colour FRAME_COLOUR;
	static const Colour OUTLINE_COLOUR;
};

#endif // FOCUS_FRAME_HPP
