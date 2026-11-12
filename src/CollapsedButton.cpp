/*******************************************************************************
** @file       CollapsedButton.cpp
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "CollapsedButton.hpp"

#include "AppImages.h"
#include "ServerMainComponent.hpp"

CollapsedButton::CollapsedButton(ServerMainComponent &server) :
  parentServer(server),
  icon(juce::ImageCache::getFromMemory(AppImages::logosmall_png, AppImages::logosmall_pngSize))
{
	setSize(SIZE, SIZE);
	setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void CollapsedButton::paint(juce::Graphics &g)
{
	const auto bounds = getLocalBounds().toFloat();

	g.setColour(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
	g.fillRoundedRectangle(bounds, 8.0f);
	g.setColour(juce::Colours::grey);
	g.drawRoundedRectangle(bounds.reduced(1.0f), 8.0f, 2.0f);
	g.drawImage(icon, bounds.reduced(8.0f), juce::RectanglePlacement::centred);
}

void CollapsedButton::mouseDown(const juce::MouseEvent &event)
{
	dragged = false;
	dragger.startDraggingComponent(this, event);
}

void CollapsedButton::mouseDrag(const juce::MouseEvent &event)
{
	if (event.getDistanceFromDragStart() > DRAG_THRESHOLD)
	{
		dragged = true;
	}

	if (dragged)
	{
		dragger.dragComponent(this, event, nullptr);
	}
}

void CollapsedButton::mouseUp(const juce::MouseEvent &)
{
	if (dragged)
	{
		parentServer.on_collapsed_button_moved();
	}
	else
	{
		parentServer.expand_window();
	}
}
