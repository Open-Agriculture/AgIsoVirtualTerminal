/*******************************************************************************
** @file       PictureGraphicComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "PictureGraphicComponent.hpp"

PictureGraphicComponent::PictureGraphicComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::PictureGraphic sourceObject) :
  isobus::PictureGraphic(sourceObject),
  parentWorkingSet(workingSet),
  reconstructedImage(Image::PixelFormat::ARGB, get_actual_width(), get_actual_height(), true)
{
	generate_and_store_image();
	setSize(PictureGraphic::get_width(), PictureGraphic::get_height());
}

void PictureGraphicComponent::generate_and_store_image()
{
	auto &rawPictureGraphicData = get_raw_data();
	std::size_t pixelIndex = 0;
	bool transparencyEnabled = get_option(Options::Transparent);

	for (std::uint_fast16_t i = 0; i < get_actual_height(); i++)
	{
		for (std::uint_fast16_t j = 0; j < get_actual_width(); j++)
		{
			auto vtColour = colourTable.get_colour(rawPictureGraphicData.at(pixelIndex));
			if (transparencyEnabled)
			{
				reconstructedImage.setPixelAt(j, i, Colour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, rawPictureGraphicData.at(pixelIndex) == this->get_transparency_colour() ? 0.0f : 1.0f)));
			}
			else
			{
				reconstructedImage.setPixelAt(j, i, Colour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f)));
			}
			pixelIndex++;
		}
	}

	if ((get_actual_height() != get_height()) || (get_actual_width() != get_width()))
	{
		reconstructedImage = reconstructedImage.rescaled(get_width(), get_height());
	}
}

void PictureGraphicComponent::paint(Graphics &g)
{
	g.drawImage(reconstructedImage, getLocalBounds().toFloat());
}
