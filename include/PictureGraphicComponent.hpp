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
  , private juce::Timer
{
public:
	PictureGraphicComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::PictureGraphic sourceObject);

	void generate_and_store_image();

	void paint(Graphics &g) override;

	void visibilityChanged() override;

	void timerCallback() override;

	/// @brief Sets whether pictures are decoded into software images instead of the platform's native
	/// (possibly GPU backed) image type, which is the default
	static void set_use_software_images(bool useSoftware);

private:
	static bool useSoftwareImages;

	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	Image reconstructedImage;
	bool visible = false;
	bool showImage = true;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PictureGraphicComponent)
};

#endif // PICTURE_GRAPHIC_COMPONENT_HPP
