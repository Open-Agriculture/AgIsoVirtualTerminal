/*******************************************************************************
** @file       OutputEllipseComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputEllipseComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"
#include "LineArt.hpp"

constexpr double ARC_STEP_RADIANS = 0.05;

// Figure B.8: the angle is the real angle of the line from the centre, not the parametric angle of the ellipse
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
	bool fillNeeded = false;
	bool useLineColourForFill = false;
	isobus::VTColourVector fillColour;
	// Ensure we fill first, then draw the outline if needed
	if (isobus::NULL_OBJECT_ID != get_fill_attributes())
	{
		auto child = get_object_by_id(get_fill_attributes(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::FillAttributes == child->get_object_type()))
		{
			auto fill = std::static_pointer_cast<isobus::FillAttributes>(child);
			if (fill->get_type() != isobus::FillAttributes::FillType::NoFill)
			{
				if (fill->get_type() == isobus::FillAttributes::FillType::FillWithSpecifiedColorInFillColorAttribute)
				{
					fillColour = parentWorkingSet->get_colour(fill->get_background_color());
				}
				else if (fill->get_type() == isobus::FillAttributes::FillType::FillWithLineColor)
				{
					useLineColourForFill = true;
				}
				else if (fill->get_type() == isobus::FillAttributes::FillType::FillWithPatternGivenByFillPatternAttribute)
				{
					// @todo
				}
				fillNeeded = true;
			}
		}
	}

	if (isobus::NULL_OBJECT_ID != get_line_attributes())
	{
		auto child = get_object_by_id(get_line_attributes(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) && (isobus::VirtualTerminalObjectType::LineAttributes == child->get_object_type()))
		{
			auto line = std::static_pointer_cast<isobus::LineAttributes>(child);

			auto lineColour = parentWorkingSet->get_colour(line->get_background_color());
			if (useLineColourForFill)
			{
				fillColour = lineColour;
			}

			float centerX = get_width() / 2.0f;
			float centerY = get_height() / 2.0f;

			// Angles are stored halved, so 0 and 180 are both the positive x-axis
			const bool sameAngles = ((get_start_angle() % 180) == (get_end_angle() % 180));

			if (sameAngles && (isobus::OutputEllipse::EllipseType::ClosedEllipseSegment == get_ellipse_type()))
			{
				// Table B.31 NOTE 2: a closed segment with equal angles is one line from the centre to the border
				const float wOffset = line->get_width() / 2.0f;
				const auto borderPoint = get_point_at_angle(wOffset, wOffset, get_width() - line->get_width(), get_height() - line->get_width(), juce::degreesToRadians(get_start_angle() * 2.0f));

				g.setColour(Colour::fromFloatRGBA(lineColour.r, lineColour.g, lineColour.b, 1.0f));
				g.drawLine(centerX, centerY, borderPoint.x, borderPoint.y, line->get_width());
			}
			else if (sameAngles || (isobus::OutputEllipse::EllipseType::Closed == get_ellipse_type()))
			{
				// Table B.31 NOTE 1: any type with equal angles is drawn closed
				if (fillNeeded)
				{
					g.setColour(Colour::fromFloatRGBA(fillColour.r, fillColour.g, fillColour.b, 1.0f));
					g.fillEllipse(0, 0, get_width(), get_height());
				}
				g.setColour(Colour::fromFloatRGBA(lineColour.r, lineColour.g, lineColour.b, 1.0f));
				Path ellipse;
				ellipse.addEllipse(line->get_width() / 2.0f, line->get_width() / 2.0f, get_width() - line->get_width(), get_height() - line->get_width());
				line_art::stroke_path(g, ellipse, line->get_width(), line->get_line_art_bit_pattern());
			}
			else
			{
				// Juce coordinate system 0° is at the Y axis positive, calculating clockwise
				// IsoBus coordinate system 0° is at the X axis positive, calculating counter-clockwise
				float startAngle = juce::degreesToRadians(((get_start_angle() * 2.0f)));
				float endAngle = juce::degreesToRadians(((get_end_angle() * 2.0f)));

				juce::Path arcPath;
				if (get_ellipse_type() == isobus::OutputEllipse::EllipseType::ClosedEllipseSegment)
				{
					// segment: the ellipse section endpoints connected to the center with two lines
					arcPath.startNewSubPath(centerX, centerY);
				}

				float wOffset = line->get_width() / 2.0f;

				addArcToPath(arcPath, wOffset, wOffset, get_width() - line->get_width(), get_height() - line->get_width(), startAngle, endAngle, get_ellipse_type() != isobus::OutputEllipse::EllipseType::ClosedEllipseSegment);

				if (get_ellipse_type() == isobus::OutputEllipse::EllipseType::ClosedEllipseSegment)
				{
					// segment: the ellipse section endpoints connected to the center with two lines
					arcPath.lineTo(centerX, centerY);
					if (fillNeeded)
					{
						g.setColour(Colour::fromFloatRGBA(fillColour.r, fillColour.g, fillColour.b, 1.0f));
						g.fillPath(arcPath);
					}

					g.setColour(Colour::fromFloatRGBA(lineColour.r, lineColour.g, lineColour.b, 1.0f));
					line_art::stroke_path(g, arcPath, line->get_width(), line->get_line_art_bit_pattern());
				}
				else if (get_ellipse_type() == isobus::OutputEllipse::EllipseType::ClosedEllipseSection)
				{
					// section: the ellipse section endpoints connected with a straight line
					arcPath.closeSubPath();
					if (fillNeeded)
					{
						g.setColour(Colour::fromFloatRGBA(fillColour.r, fillColour.g, fillColour.b, 1.0f));
						g.fillPath(arcPath);
					}

					g.setColour(Colour::fromFloatRGBA(lineColour.r, lineColour.g, lineColour.b, 1.0f));
					line_art::stroke_path(g, arcPath, line->get_width(), line->get_line_art_bit_pattern());
				}
				else if (get_ellipse_type() == isobus::OutputEllipse::EllipseType::OpenDefinedByStartEndAngles)
				{
					g.setColour(Colour::fromFloatRGBA(lineColour.r, lineColour.g, lineColour.b, 1.0f));
					line_art::stroke_path(g, arcPath, line->get_width(), line->get_line_art_bit_pattern());
				}
			}
		}
	}
}

/**
 * @brief OutputEllipseComponent::addArcToPath
 * Method to draw ellipse segments to keep the angle between the start and end accurate.
 * Juce Path::addArc does not keep the angle when drawing.
 */
void OutputEllipseComponent::addArcToPath(Path &path, float x, float y, float w, float h, float fromRadians, float toRadians, bool startAsNewSubPath) const
{
	if (toRadians < fromRadians)
	{
		toRadians += juce::MathConstants<float>().twoPi;
	}

	// A fixed step count lands the last point on toRadians; stepping by ARC_STEP_RADIANS stopped up to one step short
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
