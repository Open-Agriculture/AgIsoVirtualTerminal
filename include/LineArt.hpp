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
	/// All spots drawn: a solid line
	constexpr std::uint16_t SOLID = 0xFFFF;

	/// @brief Returns whether the spot with the given index along a line is drawn.
	/// @details ISO 11783-6 line art has one bit per paintbrush spot, read from the most significant bit and repeated
	/// every 16 spots. 1 bits are drawn in the line colour, 0 bits are skipped so the background shows.
	inline bool is_spot_drawn(std::uint16_t lineArt, int spotIndex)
	{
		return 0 != (lineArt & (0x8000u >> (static_cast<unsigned>(spotIndex) % 16u)));
	}

	/// @brief Strokes a path, drawing only the line art's spots as dashes one line width long each.
	/// @details For outlines drawn as strokes (ellipses, polygons) rather than with the paintbrush.
	inline void stroke_path(Graphics &g, const Path &path, float lineWidth, std::uint16_t lineArt)
	{
		const PathStrokeType stroke(lineWidth);

		if (SOLID == lineArt)
		{
			g.strokePath(path, stroke);
			return;
		}
		if ((0 == lineArt) || (lineWidth <= 0.0f))
		{
			return;
		}

		// Runs of equal bits become alternating dash and gap lengths, starting with a dash (of length 0 if the
		// pattern starts with a gap). The list must have an even length to keep dashes and gaps apart.
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

		Path dashed;
		stroke.createDashedStroke(dashed, path, lengths.data(), static_cast<int>(lengths.size()));
		g.fillPath(dashed);
	}
}

#endif // LINE_ART_HPP
