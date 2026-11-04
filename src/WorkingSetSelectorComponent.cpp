/*******************************************************************************
** @file       WorkingSetSelectorComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "WorkingSetSelectorComponent.hpp"
#include "AuxiliaryAssignmentComponent.hpp"
#include "DesignatorFit.hpp"
#include "JuceManagedWorkingSetCache.hpp"
#include "ServerMainComponent.hpp"
#include "WorkingSetLoadingIndicatorComponent.hpp"

#include <algorithm>

WorkingSetSelectorComponent::ColumnButton::ColumnButton(const juce::String &label) :
  juce::TextButton(label)
{
}

void WorkingSetSelectorComponent::ColumnButton::paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	auto backgroundColour = findColour(getToggleState() ? juce::TextButton::buttonOnColourId : juce::TextButton::buttonColourId);
	getLookAndFeel().drawButtonBackground(g, *this, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
	g.setColour(juce::Colours::deepskyblue.withAlpha(0.75f));
	g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 4.0f, 4.0f);

	auto textBounds = getLocalBounds().withSizeKeepingCentre(juce::roundToInt(static_cast<float>(getWidth()) * LABEL_WIDTH_RATIO), getHeight());
	juce::Font font(juce::FontOptions(1.0f));
	juce::GlyphArrangement glyphs;
	glyphs.addLineOfText(font, getButtonText(), 0.0f, 0.0f);
	const auto textWidthAtUnitHeight = glyphs.getBoundingBox(0, -1, true).getWidth();
	if (textWidthAtUnitHeight > 0.0f)
	{
		font.setHeight(static_cast<float>(textBounds.getWidth()) / textWidthAtUnitHeight);
	}
	font.setHeight(std::min(font.getHeight(), static_cast<float>(textBounds.getHeight()) * LABEL_WIDTH_RATIO));

	g.setColour(findColour(getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId));
	g.setFont(font);
	g.drawText(getButtonText(), textBounds, juce::Justification::centred, false);
}

WorkingSetSelectorComponent::WorkingSetSelectorComponent(ServerMainComponent &server) :
  parentServer(server),
  ackButton("ACK"),
  auxButton("AUX")
{
	setOpaque(false);

	// The server is not fully constructed yet, so it sets the real bounds when it lays itself out
	setSize(WIDTH, 0);
	ackButton.onStateChange = [this]() {
		const auto isPressed = ackButton.isDown();
		if (ackButtonPressed != isPressed)
		{
			ackButtonPressed = isPressed;
			parentServer.send_alarm_ack_command(isPressed ? isobus::VirtualTerminalBase::KeyActivationCode::ButtonPressedOrLatched : isobus::VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased);
		}
	};
	addAndMakeVisible(ackButton);
	ackButton.setVisible(false);
	auxButton.onClick = [this]() { parentServer.show_auxiliary_page(); };
	addChildComponent(auxButton);
}

void WorkingSetSelectorComponent::update_drawn_working_sets(std::vector<std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet>> &managedWorkingSetList)
{
	children.clear();

	for (std::size_t i = 0; i < managedWorkingSetList.size(); i++)
	{
		children.push_back({ managedWorkingSetList.at(i) });

		if ((isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == managedWorkingSetList.at(i)->get_object_pool_processing_state()) ||
		    managedWorkingSetList.at(i)->is_object_pool_transfer_in_progress())
		{
			children.back().childComponents.push_back(getWorkingSetChildComponent(managedWorkingSetList.at(i), i));
		}
	}

	update_auxiliary_button();
	ackButton.toFront(false);
	repaint();
}

void WorkingSetSelectorComponent::paint(Graphics &g)
{
	g.setColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
	g.fillAll();

	// a designator rarely matches the button's aspect ratio, so fill the letterboxing with its own
	// background colour instead of leaving the app's default panel colour showing through
	for (std::size_t i = 0; i < children.size(); i++)
	{
		const auto &workingSet = children.at(i).workingSet;
		auto workingSetObject = workingSet->get_working_set_object();

		if (nullptr != workingSetObject)
		{
			auto vtColour = workingSet->get_colour(workingSetObject->get_background_color());
			g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
			g.fillRect(button_bounds(static_cast<int>(i)));
		}
	}
}

// the active highlight straddles the button edge, and a designator scaled to fill the button is a child
// component, so JUCE paints it after paint() and would cover the highlight's inner half
void WorkingSetSelectorComponent::paintOverChildren(Graphics &g)
{
	auto activeWorkingSet = parentServer.get_active_working_set();

	if (auxButton.isVisible())
	{
		const auto auxBounds = auxButton.getBounds().toFloat();

		// Highlighted like an active working set, as its screen takes the place of one
		if (parentServer.get_is_auxiliary_page_shown())
		{
			g.setColour(juce::Colours::yellow.withAlpha(0.4f));
			g.drawRoundedRectangle(auxBounds.expanded(2.0f), 4.0f, 4.0f);
		}
		if (hasAuxiliaryNotice)
		{
			g.setColour(juce::Colours::red);
			g.fillEllipse(juce::Rectangle<float>(12.0f, 12.0f).withCentre(auxBounds.getTopRight().translated(-10.0f, 10.0f)));
		}
	}

	if ((nullptr == activeWorkingSet) || (nullptr == activeWorkingSet->get_control_function()))
	{
		return;
	}
	const auto activeName = activeWorkingSet->get_control_function()->get_NAME().get_full_name();

	g.setColour(juce::Colours::yellow.withAlpha(0.4f));

	for (std::size_t i = 0; i < children.size(); i++)
	{
		auto controlFunction = children.at(i).workingSet->get_control_function();

		if ((nullptr != controlFunction) && (activeName == controlFunction->get_NAME().get_full_name()))
		{
			g.drawRoundedRectangle(button_bounds(static_cast<int>(i)).toFloat().expanded(2.0f), 4.0f, 4.0f);
		}
	}
}

void WorkingSetSelectorComponent::resized()
{
	// The owner positions this column; setting bounds here used to move it over the menu bar
	update_column_button_bounds();
}

void WorkingSetSelectorComponent::redraw()
{
	int workingSetIndex = 0;
	for (auto &workingSet : children)
	{
		// A working set without a pool, such as after Delete Object Pool, keeps an empty slot
		if (!workingSet.childComponents.empty())
		{
			workingSet.childComponents.clear();
			workingSet.childComponents.push_back(getWorkingSetChildComponent(workingSet.workingSet, workingSetIndex));
		}
		workingSetIndex++;
	}
	repaint();
	auxButton.toFront(false);
	ackButton.toFront(false);
}

void WorkingSetSelectorComponent::update_iop_load_indicators()
{
	for (auto &child : children)
	{
		if (child.workingSet->is_object_pool_transfer_in_progress() &&
		    child.workingSet->get_object_pool_processing_state() == isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::None)
		{
			for (auto &subChild : child.childComponents)
			{
				subChild->repaint();
			}
		}
	}
}

void WorkingSetSelectorComponent::set_ack_button_visible(bool shouldBeVisible)
{
	if (!shouldBeVisible && ackButtonPressed)
	{
		ackButtonPressed = false;
		parentServer.send_alarm_ack_command(isobus::VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased);
	}
	ackButton.setVisible(shouldBeVisible);
	update_column_button_bounds();
	ackButton.toFront(false);
	repaint();
}

void WorkingSetSelectorComponent::update_auxiliary_button()
{
	const bool hasAuxiliaryObjects = std::any_of(children.begin(), children.end(), [](const SELECTOR_CHILD_OBJECTS_STRUCT &child) { return AuxiliaryControlSnapshot::has_auxiliary_objects(child.workingSet); });

	auxButton.setVisible(hasAuxiliaryObjects || parentServer.get_is_auxiliary_page_shown());
	update_column_button_bounds();
	auxButton.toFront(false);
	repaint();
}

void WorkingSetSelectorComponent::set_auxiliary_notice(bool hasNotice)
{
	hasAuxiliaryNotice = hasNotice;
	repaint();
}

constexpr int WorkingSetSelectorComponent::button_padding()
{
	return (WIDTH - BUTTON_WIDTH) / 2;
}

juce::Rectangle<int> WorkingSetSelectorComponent::button_bounds(int index)
{
	return { button_padding(), button_padding() + index * (BUTTON_HEIGHT + button_padding()), BUTTON_WIDTH, BUTTON_HEIGHT };
}

std::shared_ptr<Component> WorkingSetSelectorComponent::getWorkingSetChildComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, int workingSetIndex)
{
	auto workingSetObject = workingSet->get_working_set_object();
	std::shared_ptr<Component> workingSetComponent;
	if (nullptr != workingSetObject)
	{
		workingSetComponent = JuceManagedWorkingSetCache::create_component(workingSet, workingSetObject);
	}
	else
	{
		workingSetComponent = std::make_shared<WorkingSetLoadingIndicatorComponent>(workingSet, BUTTON_HEIGHT, BUTTON_WIDTH);
	}
	const auto bounds = button_bounds(workingSetIndex);

	workingSetComponent->setTopLeftPosition(bounds.getPosition());
	// Unlike a soft key, a working set designator is the only thing in its button, so artwork
	// smaller than the button is enlarged to fill it.
	fit_designator_to_button(*workingSetComponent, bounds, juce::RectanglePlacement::centred);
	addAndMakeVisible(*workingSetComponent);
	return workingSetComponent;
}

void WorkingSetSelectorComponent::update_column_button_bounds()
{
	const auto buttonSize = BUTTON_WIDTH;
	auto buttonY = std::max(0, getHeight() - buttonSize - button_padding());

	// ACK keeps the bottom, AUX goes above it while both are shown
	if (ackButton.isVisible())
	{
		ackButton.setBounds(button_padding(), buttonY, buttonSize, buttonSize);
		buttonY = std::max(0, buttonY - buttonSize - button_padding());
	}
	auxButton.setBounds(button_padding(), buttonY, buttonSize, buttonSize);
}

void WorkingSetSelectorComponent::mouseUp(const MouseEvent &event)
{
	auto relativeEvent = event.getEventRelativeTo(this);

	if ((button_padding() <= relativeEvent.getMouseDownX()) && (relativeEvent.getMouseDownX() < button_padding() + BUTTON_WIDTH) && (button_padding() <= relativeEvent.getMouseDownY()) && (relativeEvent.getMouseDownY() < button_padding() + (button_padding() + BUTTON_HEIGHT) * children.size()))
	{
		int workingSetIndex = (relativeEvent.getMouseDownY() - button_padding()) / (BUTTON_HEIGHT + button_padding());

		if (workingSetIndex <= 255)
		{
			parentServer.change_selected_working_set(static_cast<std::uint8_t>(workingSetIndex));
		}
		redraw();
	}
}
