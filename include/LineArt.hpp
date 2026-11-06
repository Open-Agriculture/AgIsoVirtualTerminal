//================================================================================================
/// @file LineArt.hpp
///
/// @brief The line art of Line Attributes objects: which paintbrush spots of a line are drawn.
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef LINE_ART_HPP
#define LINE_ART_HPP

#include "JuceHeader.h"

#include <cstdint>
#include <vector>

namespace line_art
{
	constexpr std::uint16_t SOLID = 0xFFFF;

	/// @brief Returns whether the spot with the given index along a line is drawn.
	/// @details One bit per spot, most significant bit first, repeating every 16 spots.
	inline bool is_spot_drawn(std::uint16_t lineArt, int spotIndex)
	{
		return 0 != (lineArt & (0x8000u >> (static_cast<unsigned>(spotIndex) % 16u)));
	}

	/// @brief Returns the area covered by stroking a path, keeping only the line art's spots, each one line width long.
	inline Path outline(const Path &path, float lineWidth, std::uint16_t lineArt)
	{
		Path result;

		if (SOLID == lineArt)
		{
			PathStrokeType(lineWidth).createStrokedPath(result, path);
		}
		else if ((0 != lineArt) && (lineWidth > 0.0f))
		{
			// Dash and gap lengths alternate starting with a dash, so a leading gap needs a zero dash and the count
			// must be even
			std::vector<float> lengths;
			bool drawing = true;
			float run = 0.0f;
			for (int spot = 0; spot < 16; spot++)
			{
				const bool drawn = is_spot_drawn(lineArt, spot);
				if (drawn != drawing)
				{
					lengths.push_back(run);
					run = 0.0f;
					drawing = drawn;
				}
				run += lineWidth;
			}
			lengths.push_back(run);
			if (0 != (lengths.size() % 2))
			{
				lengths.push_back(0.0f);
			}
			PathStrokeType(lineWidth).createDashedStroke(result, path, lengths.data(), static_cast<int>(lengths.size()));
		}
		return result;
	}

	/// @brief Fills a closed path with the current fill, leaving every pixel of the given border unfilled.
	/// @details The background has to show through the gaps of the line art.
	inline void fill_inside_border(Graphics &g, const Path &path, const Path &border, std::uint16_t lineArt)
	{
		const auto area = path.getBounds().getSmallestIntegerContainer();

		if (SOLID == lineArt)
		{
			// A solid border is drawn over the fill and hides the pixels they share
			g.fillPath(path);
		}
		else if (!area.isEmpty())
		{
			const auto toArea = AffineTransform::translation(static_cast<float>(-area.getX()), static_cast<float>(-area.getY()));
			Image outside(Image::SingleChannel, area.getWidth(), area.getHeight(), true);
			{
				Graphics borderGraphics(outside);
				borderGraphics.setColour(Colours::white);
				borderGraphics.fillPath(border, toArea);
			}
			{
				const Image::BitmapData pixels(outside, Image::BitmapData::readWrite);
				for (int y = 0; y < area.getHeight(); y++)
				{
					for (int x = 0; x < area.getWidth(); x++)
					{
						std::uint8_t *const pixel = pixels.getPixelPointer(x, y);
						*pixel = static_cast<std::uint8_t>(255 - *pixel);
					}
				}
			}
			g.saveState();
			g.reduceClipRegion(outside, toArea.inverted());
			g.fillPath(path);
			g.restoreState();
		}
	}
}

#endif // LINE_ART_HPP
