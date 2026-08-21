/*******************************************************************************
** @file       WorkingSetSelectorComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "WorkingSetSelectorComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"
#include "ServerMainComponent.hpp"
#include "WorkingSetLoadingIndicatorComponent.hpp"
#include "isobus/utility/system_timing.hpp"

// Clients author designators against whatever soft key size they assume, which across real pools runs from
// 28x26 up to 240x192, so the designator is scaled to fit the button rather than clipped to it. getUnion
// ignores empty rectangles, so children that resolved to nothing drop out of the measurement on their own.
static void fit_designator_to_button(juce::Component &designator, juce::Rectangle<int> button)
{
	auto children = designator.getChildren();
	juce::Rectangle<int> drawnArea;

	for (auto *child : children)
	{
		drawnArea = drawnArea.getUnion(child->getBounds());
	}

	if (drawnArea.isEmpty())
	{
		// nothing to measure, so leave the component at the button-sized bounds it was built with
		return;
	}

	for (auto *child : children)
	{
		child->setTopLeftPosition(child->getPosition() - drawnArea.getPosition());
	}
	designator.setSize(drawnArea.getWidth(), drawnArea.getHeight());
	designator.setTransform(juce::RectanglePlacement(juce::RectanglePlacement::centred)
	                          .getTransformToFit(designator.getBounds().toFloat(), button.toFloat()));
}

WorkingSetSelectorComponent::AckButton::AckButton() :
  juce::TextButton("ACK")
{
}

void WorkingSetSelectorComponent::AckButton::paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
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
  parentServer(server)
{
	setOpaque(false);
	setBounds(0, 0, WIDTH, server.minimum_height());
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
}

void WorkingSetSelectorComponent::update_drawn_working_sets(std::vector<std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet>> &managedWorkingSetList)
{
	children.clear();

	for (std::size_t i = 0; i < managedWorkingSetList.size(); i++)
	{
		children.push_back({ managedWorkingSetList.at(i) });

		if ((
		      (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == managedWorkingSetList.at(i)->get_object_pool_processing_state()) ||
		      managedWorkingSetList.at(i)->is_object_pool_transfer_in_progress()) &&
		    (!isobus::SystemTiming::time_expired_ms(managedWorkingSetList.at(i)->get_working_set_maintenance_message_timestamp_ms(), 3000)) &&
		    (!managedWorkingSetList.at(i)->is_deletion_requested()))
		{
			children.back().childComponents.push_back(getWorkingSetChildComponent(managedWorkingSetList.at(i), i));
		}
	}

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
	setBounds(0, 0, WIDTH, parentServer.minimum_height());
	update_ack_button_bounds();
}

void WorkingSetSelectorComponent::redraw()
{
	int workingSetIndex = 0;
	for (auto &workingSet : children)
	{
		workingSet.childComponents.clear();
		workingSet.childComponents.push_back(getWorkingSetChildComponent(workingSet.workingSet, workingSetIndex));
		workingSetIndex++;
	}
	repaint();
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
	update_ack_button_bounds();
	ackButton.toFront(false);
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
	fit_designator_to_button(*workingSetComponent, bounds);
	addAndMakeVisible(*workingSetComponent);
	return workingSetComponent;
}

void WorkingSetSelectorComponent::update_ack_button_bounds()
{
	const auto ackButtonSize = BUTTON_WIDTH;
	const auto ackButtonY = std::max(0, getHeight() - ackButtonSize - button_padding());
	ackButton.setBounds(button_padding(), ackButtonY, ackButtonSize, ackButtonSize);
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
