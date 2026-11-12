/*******************************************************************************
** @file       PictureGraphicComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "PictureGraphicComponent.hpp"

bool PictureGraphicComponent::useSoftwareImages = false;

PictureGraphicComponent::PictureGraphicComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::PictureGraphic sourceObject) :
  isobus::PictureGraphic(sourceObject),
  parentWorkingSet(workingSet),
  reconstructedImage(Image::PixelFormat::ARGB,
                     get_actual_width(),
                     get_actual_height(),
                     true,
                     get_image_type())
{
	generate_and_store_image();
	setSize(PictureGraphic::get_width(), PictureGraphic::get_height());
}

std::optional<FillType> PictureGraphicComponent::get_shape_fill(const Component &shape, std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t fillAttributesId, std::optional<Colour> lineColour)
{
	std::optional<FillType> retVal;
	const auto object = workingSet->get_object_by_id(fillAttributesId);

	if ((nullptr != object) && (isobus::VirtualTerminalObjectType::FillAttributes == object->get_object_type()))
	{
		const auto fill = std::static_pointer_cast<isobus::FillAttributes>(object);

		switch (fill->get_type())
		{
			case isobus::FillAttributes::FillType::FillWithLineColor:
			{
				if (lineColour.has_value())
				{
					retVal = FillType(lineColour.value());
				}
			}
			break;

			case isobus::FillAttributes::FillType::FillWithSpecifiedColorInFillColorAttribute:
			{
				const auto vtColour = workingSet->get_colour(fill->get_background_color());
				retVal = FillType(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
			}
			break;

			case isobus::FillAttributes::FillType::FillWithPatternGivenByFillPatternAttribute:
			{
				retVal = get_fill_pattern(shape, workingSet, fill->get_fill_pattern());
			}
			break;

			case isobus::FillAttributes::FillType::NoFill:
			default:
				break;
		}
	}
	return retVal;
}

FillType PictureGraphicComponent::get_fill_pattern(const Component &shape, std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t pictureId)
{
	FillType retVal(Colours::transparentBlack);
	const auto object = get_object_by_id(pictureId, workingSet->get_object_tree());

	if ((nullptr != object) && (isobus::VirtualTerminalObjectType::PictureGraphic == object->get_object_type()))
	{
		const auto picture = std::static_pointer_cast<isobus::PictureGraphic>(object);
		const int width = picture->get_actual_width();
		const int height = picture->get_actual_height();
		const auto &rawData = picture->get_raw_data();

		if ((0 < width) && (0 < height) && (rawData.size() >= static_cast<std::size_t>(width) * height))
		{
			// Tiled at its actual size, ignoring transparency and flashing
			Image pattern(Image::PixelFormat::RGB, width, height, false, get_image_type());
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					const auto vtColour = workingSet->get_colour(rawData[static_cast<std::size_t>(y) * width + x]);
					pattern.setPixelAt(x, y, Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
				}
			}

			// Tiles start at the outermost pool object, so they line up across objects. Each soft key is its own
			// designator.
			const Component *anchor = &shape;
			for (auto parent = shape.getParentComponent(); nullptr != parent; parent = parent->getParentComponent())
			{
				const auto parentObject = dynamic_cast<const isobus::VTObject *>(parent);
				if (nullptr == parentObject)
				{
					break;
				}
				anchor = parent;
				if (isobus::VirtualTerminalObjectType::Key == parentObject->get_object_type())
				{
					break;
				}
			}
			retVal = FillType(pattern, AffineTransform::translation(shape.getLocalPoint(anchor, Point<float>())));
		}
	}
	return retVal;
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
			auto vtColour = parentWorkingSet->get_colour(rawPictureGraphicData.at(pixelIndex));
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
	if (!get_option(isobus::PictureGraphic::Options::Flashing))
	{
		stopTimer();
		showImage = true;
	}

	if (showImage)
	{
		// Smoothing blurs an enlarged bitmap, so its pixels are just repeated instead
		const auto pixelsPerImagePixel = g.getInternalContext().getPhysicalPixelScaleFactor() * static_cast<float>(getWidth()) / static_cast<float>(juce::jmax(1, reconstructedImage.getWidth()));

		if (pixelsPerImagePixel > 1.0f)
		{
			g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
		}
		g.drawImage(reconstructedImage, getLocalBounds().toFloat());
	}
}

void PictureGraphicComponent::visibilityChanged()
{
	if (visible != isVisible())
	{
		visible = isVisible();
		if (visible && get_option(isobus::PictureGraphic::Options::Flashing))
		{
			startTimer(500);
		}
		else
		{
			stopTimer();
		}
	}
}

void PictureGraphicComponent::set_use_software_images(bool useSoftware)
{
	useSoftwareImages = useSoftware;
}

const ImageType &PictureGraphicComponent::get_image_type()
{
	static const SoftwareImageType softwareImageType;
	static const NativeImageType nativeImageType;

	if (useSoftwareImages)
	{
		return softwareImageType;
	}
	return nativeImageType;
}

void PictureGraphicComponent::timerCallback()
{
	showImage = !showImage;
	repaint();
}
