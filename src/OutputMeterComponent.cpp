#include "OutputMeterComponent.hpp"

#include <cmath>

OutputMeterComponent::OutputMeterComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::OutputMeter sourceObject) :
  isobus::OutputMeter(sourceObject),
  parentWorkingSet(workingSet)
{
	setSize(get_width(), get_height());
	setOpaque(false);
}

void OutputMeterComponent::paint(Graphics &g)
{
	if (get_option(Options::DrawBorder))
	{
		auto vtColour = colourTable.get_colour(get_border_colour());
		g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
		g.drawRect(0, 0, static_cast<int>(get_width()), static_cast<int>(get_height()), 1);
	}
	if (get_option(Options::DrawArc))
	{
		Path p;
		PathStrokeType pathStroke(1.0f, PathStrokeType::JointStyle::curved);

		float startVtAngle = get_start_angle() * 2.0f * 0.0174533f;
		float endVtAngle = get_end_angle() * 2.0f * 0.0174533f;
		float ellipseRotation = 3.14159f / 2.0f;

		if (endVtAngle < startVtAngle)
		{
			endVtAngle += (2.0f * 3.14159f);
		}

		if (startVtAngle < endVtAngle)
		{
			ellipseRotation = -ellipseRotation;
		}

		p.addCentredArc(static_cast<float>(get_width()) / 2.0f, static_cast<float>(get_height()) / 2.0f, static_cast<float>(get_width()) / 2.0f, static_cast<float>(get_height()) / 2.0f, ellipseRotation, startVtAngle, endVtAngle, true);
		g.setColour(Colours::black);
		g.strokePath(p, pathStroke);
	}

	std::uint32_t needleValue = get_value();
	if ((this->get_number_children() > 0) && (0xFFFF != this->get_child_id(0)))
	{
		auto varNum = this->get_object_by_id(this->get_child_id(0));

		if ((nullptr != varNum) && (isobus::VirtualTerminalObjectType::NumberVariable == varNum->get_object_type()))
		{
			needleValue = std::static_pointer_cast<isobus::NumberVariable>(varNum)->get_value();
		}
	}
	auto vtColour = colourTable.get_colour(get_needle_colour());
	Path needlePath;
	float xCoord = 0.0f;
	float yCoord = 0.0f;
	float endVtAngleDeg = get_end_angle() * 2.0f;
	float startVtAngleDeg = get_start_angle() * 2.0f;

	if (endVtAngleDeg < startVtAngleDeg)
	{
		endVtAngleDeg += (360);
	}

	float theta = (static_cast<float>(needleValue) / get_max_value()) * (startVtAngleDeg - endVtAngleDeg);
	float needleEndAngle = 0.0f;

	if (true == get_option(Options::DeflectionDirection))
	{
		// clockwise
		needleEndAngle = (endVtAngleDeg + theta);
	}
	else
	{
		// counter clockwise
		needleEndAngle = (endVtAngleDeg - theta);
	}

	float xOffset = (get_width() / 2.0f) * std::cos(needleEndAngle * 3.14159265f / 180.0f);
	float yOffset = -(get_width() / 2.0f) * std::sin(needleEndAngle * 3.14159265f / 180.0f);

	xCoord = (get_width() / 2.0f) + xOffset;
	yCoord = (get_width() / 2.0f) + yOffset;

	g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
	g.drawLine((get_width() / 2.0f) + xOffset, (get_width() / 2.0f) + yOffset, get_width() / 2.0f, get_height() / 2.0f, 3.0f);

	g.setColour(Colours::black);
	if ((get_option(Options::DrawTicks)) && (get_number_of_ticks() > 0))
	{
		//float degreesPerTick = ((get_start_angle() * 2.0f) - (get_end_angle() * 2.0f)) / static_cast<float>(get_number_of_ticks());

		//for (std::uint8_t i = 0; i < get_number_of_ticks(); i++)
		//{
		//	if (true == get_option(Options::DeflectionDirection))
		//	{
		//		// clockwise
		//		needleEndAngle = (get_end_angle() * 2.0f) + (degreesPerTick * i);
		//	}
		//	else
		//	{
		//		// counter clockwise
		//		needleEndAngle = (get_end_angle() * 2.0f) - (degreesPerTick * i);
		//	}
		//	xCoord = (get_width() / 2.0f) * std::cos(needleEndAngle * 3.14159265f / 180.0f);
		//	yCoord = (get_width() / 2.0f) * std::sin(needleEndAngle * 3.14159265f / 180.0f);
		//	g.drawLine((get_width() / 2) + xCoord, (get_height() / 2) + yCoord, (get_width() / 2) * 0.9f + xCoord, (get_height() / 2) *0.9f + yCoord, 3.0f);
		//}
	}
}
