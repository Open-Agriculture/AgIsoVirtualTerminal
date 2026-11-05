//================================================================================================
/// @file DesignatorFit.hpp
///
/// @brief Declares a helper to scale a designator's artwork into the area it is drawn in.
/// @author Sujan Dumaru
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef DESIGNATOR_FIT_HPP
#define DESIGNATOR_FIT_HPP

#include "JuceHeader.h"

/// @brief Scales a designator's artwork into the area it is drawn in
/// @param[in] designator The component whose children carry the artwork
/// @param[in] button The area to fit the artwork into
/// @param[in] placement Where the fitted artwork sits in the button, and whether it may be enlarged.
/// A placement carrying onlyReduceInSize leaves artwork that already fits untouched
void fit_designator_to_button(juce::Component &designator, juce::Rectangle<int> button, juce::RectanglePlacement placement);

#endif // DESIGNATOR_FIT_HPP
