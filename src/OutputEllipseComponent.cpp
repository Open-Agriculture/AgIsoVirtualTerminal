/*******************************************************************************
** @file       OutputEllipseComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputEllipseComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"
#include "LineArt.hpp"
#include "PictureGraphicComponent.hpp"

constexpr double ARC_STEP_RADIANS = 0.05;

// The angle is the real angle of the line from the centre, not the parametric angle Path::addArc uses
static Point<float> get_point_at_angle(float x, float y, float w, float h, double radians)
{
	const double halfWidth = w / 2.0;
	const double halfHeight = h / 2.0;
	const double angle = std::fmod(radians, juce::MathConstants<double>::twoPi);
	const double tanAngle = std::tan(angle);
	const double div = std::sqrt((halfHeight * halfHeight) + (halfWidth * halfWidth) * (tanAngle * tanAngle));
	double x_ = (halfWidth * halfHeight) / div;
	double y_ = -(halfWidth * halfHeight * tanAngle) / div;

	if (std::fabs(angle - juce::MathConstants<double>::halfPi) < 1e-6)
	{
		x_ = 0.0;
		y_ = -halfHeight;
	}
	else if (std::fabs(angle - 3 * juce::MathConstants<double>::halfPi) < 1e-6)
	{
		x_ = 0.0;
		y_ = halfHeight;
	}
	else if (!(angle < juce::MathConstants<double>::halfPi || angle > 3 * juce::MathConstants<double>::halfPi))
	{
		x_ = -x_;
		y_ = -y_;
	}

	return { static_cast<float>(x_ + halfWidth + x), static_cast<float>(y_ + halfHeight + y) };
}

OutputEllipseComponent::OutputEllipseComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputEllipse sourceObject) :
  isobus::OutputEllipse(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
}

void OutputEllipseComponent::paint(Graphics &g)
{
	auto child = get_object_by_id(get_line_attributes(), parentWorkingSet->get_object_tree());

	if ((nullptr != child) && (isobus::VirtualTerminalObjectType::LineAttributes == child->get_object_type()))
	{
		auto line = std::static_pointer_cast<isobus::LineAttributes>(child);
		const auto vtColour = parentWorkingSet->get_colour(line->get_background_color());
		const auto lineColour = Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f);
		const auto fill = PictureGraphicComponent::get_shape_fill(*this, parentWorkingSet, get_fill_attributes(), lineColour);

		const float lineWidth = line->get_width();
		const float wOffset = lineWidth / 2.0f;
		const float w = get_width() - lineWidth;
		const float h = get_height() - lineWidth;
		const Point<float> centre(get_width() / 2.0f, get_height() / 2.0f);
		const float startAngle = juce::degreesToRadians(get_start_angle() * 2.0f);
		const auto type = get_ellipse_type();
		// Angles are stored halved, so 0 and 180 are both the positive x-axis
		const bool sameAngles = ((get_start_angle() % 180) == (get_end_angle() % 180));
		g.setColour(lineColour);

		if (sameAngles && (EllipseType::ClosedEllipseSegment == type))
		{
			// A closed segment with equal angles is one line from the centre to the border
			g.drawLine(Line<float>(centre, get_point_at_angle(wOffset, wOffset, w, h, startAngle)), lineWidth);
		}
		else
		{
			Path path;
			if (sameAngles || (EllipseType::Closed == type))
			{
				// Any type with equal angles is drawn closed. The line art starts at the start angle.
				addArcToPath(path, wOffset, wOffset, w, h, startAngle, startAngle + juce::MathConstants<float>::twoPi, true);
			}
			else
			{
				if (EllipseType::ClosedEllipseSegment == type)
				{
					path.startNewSubPath(centre);
				}
				addArcToPath(path, wOffset, wOffset, w, h, startAngle, juce::degreesToRadians(get_end_angle() * 2.0f), EllipseType::ClosedEllipseSegment != type);
			}

			if (sameAngles || (EllipseType::OpenDefinedByStartEndAngles != type))
			{
				path.closeSubPath();
				if (fill.has_value())
				{
					g.setFillType(fill.value());
					line_art::fill_inside_border(g, path, line_art::outline(path, lineWidth, line_art::SOLID), line->get_line_art_bit_pattern(), PictureGraphicComponent::get_image_type());
					g.setColour(lineColour);
				}
			}
			g.fillPath(line_art::outline(path, lineWidth, line->get_line_art_bit_pattern()));
		}
	}
}

void OutputEllipseComponent::addArcToPath(Path &path, float x, float y, float w, float h, float fromRadians, float toRadians, bool startAsNewSubPath) const
{
	if (toRadians < fromRadians)
	{
		toRadians += juce::MathConstants<float>().twoPi;
	}

	// A fixed step count lands the last point exactly on toRadians
	const int steps = std::max(1, static_cast<int>(std::ceil((toRadians - fromRadians) / ARC_STEP_RADIANS)));

	for (int i = 0; i <= steps; i++)
	{
		const auto point = get_point_at_angle(x, y, w, h, fromRadians + (toRadians - fromRadians) * i / steps);

		if ((0 == i) && startAsNewSubPath)
		{
			path.startNewSubPath(point);
		}
		else
		{
			path.lineTo(point);
		}
	}
}
