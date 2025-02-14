/*******************************************************************************
** @file       OutputEllipseComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "OutputEllipseComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

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

			if (get_ellipse_type() == isobus::OutputEllipse::EllipseType::Closed)
			{
				if (get_start_angle() == get_end_angle() && get_ellipse_type() != isobus::OutputEllipse::EllipseType::Closed)
				{
					/* B.10 / Table B.31 / Ellipse type / NOTE 2:
					 * If type = closed ellipse segment and start and end angle are the same, a
					 * single line with width = border width shall be drawn from the centre point to the
					 * point on the border defined by the start and end angles.*/
					auto angleRadians = degreesToRadians(-((get_start_angle() * 2.0f) - 90));
					if (angleRadians < 0)
					{
						angleRadians += juce::MathConstants<float>().twoPi;
					}

					auto pointX = (centerX + (centerX - (line->get_width() / 2.0f)) * std::cos(angleRadians));
					auto pointY = (centerY - (centerY - (line->get_height() / 2.0f)) * std::sin(angleRadians));

					g.setColour(Colour::fromFloatRGBA(lineColour.r, lineColour.g, lineColour.b, 1.0f));
					g.drawLine(centerX, centerY, pointX, pointY, line->get_width());
				}
				else
				{
					if (fillNeeded)
					{
						g.setColour(Colour::fromFloatRGBA(fillColour.r, fillColour.g, fillColour.b, 1.0f));
						g.fillEllipse(0, 0, get_width(), get_height());
					}
					/* If type > 0 (!= Closed) and start and end angles are the same, the ellipse is drawn closed. */
					g.setColour(Colour::fromFloatRGBA(lineColour.r, lineColour.g, lineColour.b, 1.0f));
					g.drawEllipse(line->get_width() / 2.0f, line->get_width() / 2.0f, get_width() - line->get_width(), get_height() - line->get_width(), line->get_width());
				}
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
					g.strokePath(arcPath, juce::PathStrokeType(line->get_width()));
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
					g.strokePath(arcPath, juce::PathStrokeType(line->get_width()));
				}
				else if (get_ellipse_type() == isobus::OutputEllipse::EllipseType::OpenDefinedByStartEndAngles)
				{
					g.setColour(Colour::fromFloatRGBA(lineColour.r, lineColour.g, lineColour.b, 1.0f));
					g.strokePath(arcPath, juce::PathStrokeType(line->get_width()));
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
	float halfWidth = w / 2;
	float halfHeight = h / 2;
	bool first = true;
	if (toRadians < fromRadians)
	{
		toRadians += juce::MathConstants<float>().twoPi;
	}

	for (double drawAngle = fromRadians; drawAngle < toRadians; drawAngle += 0.05)
	{
		double angle = std::fmod(drawAngle, juce::MathConstants<float>().twoPi);
		double tanAngle = std::tan(angle);
		double div = std::sqrt((halfHeight * halfHeight) + (halfWidth * halfWidth) * (tanAngle * tanAngle));
		double x_ = (halfWidth * halfHeight) / div;
		double y_ = -(halfWidth * halfHeight * tanAngle) / div;

		if (std::fabs(angle - juce::MathConstants<float>().halfPi) < 1e-6)
		{
			// handle 90°
			x_ = 0.0;
			y_ = -halfHeight;
		}
		else if (std::fabs(angle - 3 * juce::MathConstants<float>().halfPi) < 1e-6)
		{
			// handle 270°
			x_ = 0.0;
			y_ = halfHeight;
		}
		else if (!(angle < juce::MathConstants<float>().halfPi || angle > 3 * juce::MathConstants<float>().halfPi))
		{
			// handle left quadrants 1, 2
			x_ = -x_;
			y_ = -y_;
		}

		x_ += w / 2.0 + x;
		y_ += h / 2.0 + y;

		if (first && startAsNewSubPath)
		{
			first = false;
			path.startNewSubPath(x_, y_);
		}
		else
		{
			path.lineTo(x_, y_);
		}
	}
}
