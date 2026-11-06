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

#include <optional>

class PictureGraphicComponent : public isobus::PictureGraphic
  , public Component
  , private juce::Timer
{
public:
	PictureGraphicComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::PictureGraphic sourceObject);

	/// @brief Returns what an output shape is filled with, or nothing if it is not filled
	/// @param lineColour The colour of the shape's line, if it has Line Attributes
	static std::optional<FillType> get_shape_fill(const Component &shape, std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t fillAttributesId, std::optional<Colour> lineColour);

	void generate_and_store_image();

	void paint(Graphics &g) override;

	void visibilityChanged() override;

	void timerCallback() override;

	/// @brief Sets whether pictures are decoded into software images instead of the platform's native
	/// (possibly GPU backed) image type, which is the default. Only vt-render turns this on, so that its
	/// renders do not depend on the platform's graphics stack
	static void set_use_software_images(bool useSoftware);

	/// @brief Returns the image type to create images with, as set by set_use_software_images
	static const ImageType &get_image_type();

private:
	static bool useSoftwareImages;
	static FillType get_fill_pattern(const Component &shape, std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t pictureId);

	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	Image reconstructedImage;
	bool visible = false;
	bool showImage = true;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PictureGraphicComponent)
};

#endif // PICTURE_GRAPHIC_COMPONENT_HPP
