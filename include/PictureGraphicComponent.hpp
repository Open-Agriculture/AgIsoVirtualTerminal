//================================================================================================
/// @file PictureGraphicComponent.hpp
///
/// @brief Defines a GUI component to draw a picture graphic.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef PICTURE_GRAPHIC_COMPONENT_HPP
#define PICTURE_GRAPHIC_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class PictureGraphicComponent : public isobus::PictureGraphic
  , public Component
{
public:
	PictureGraphicComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::PictureGraphic sourceObject);

	void generate_and_store_image();

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	Image reconstructedImage;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PictureGraphicComponent);
};

#endif // PICTURE_GRAPHIC_COMPONENT_HPP
