/*******************************************************************************
** @file       SoftkeyMaskRenderArea.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "JuceManagedWorkingSetCache.hpp"
#include "ServerMainComponent.hpp"
#include "SoftKeyMaskRenderAreaComponent.hpp"

SoftKeyMaskRenderAreaComponent::SoftKeyMaskRenderAreaComponent(ServerMainComponent &parentServer) :
  ownerServer(parentServer)
{
	pageBar.onPageChange = [this](int delta) { change_page(delta); };
	addChildComponent(pageBar);
}

void SoftKeyMaskRenderAreaComponent::on_change_active_mask(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	std::uint16_t activeMaskID = isobus::NULL_OBJECT_ID;
	std::shared_ptr<isobus::VTObject> softKeyMask;

	if (workingSet)
	{
		auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(workingSet->get_working_set_object());

		if ((nullptr != workingSetObject) && (isobus::NULL_OBJECT_ID != workingSetObject->get_active_mask()))
		{
			activeMaskID = workingSetObject->get_active_mask();
			auto activeMask = workingSet->get_object_by_id(activeMaskID);

			if (nullptr != activeMask)
			{
				if (isobus::VirtualTerminalObjectType::AlarmMask == activeMask->get_object_type())
				{
					softKeyMask = activeMask->get_object_by_id(std::static_pointer_cast<isobus::AlarmMask>(activeMask)->get_soft_key_mask(), workingSet->get_object_tree());
				}
				else if (isobus::VirtualTerminalObjectType::DataMask == activeMask->get_object_type())
				{
					softKeyMask = activeMask->get_object_by_id(std::static_pointer_cast<isobus::DataMask>(activeMask)->get_soft_key_mask(), workingSet->get_object_tree());
				}

				if ((nullptr != softKeyMask) && (isobus::VirtualTerminalObjectType::SoftKeyMask != softKeyMask->get_object_type()))
				{
					softKeyMask = nullptr;
				}
			}
		}
	}

	const std::uint16_t softKeyMaskID = (nullptr != softKeyMask) ? softKeyMask->get_id() : isobus::NULL_OBJECT_ID;

	if ((workingSet != parentWorkingSet) || (activeMaskID != previousActiveMaskID) || (softKeyMaskID != previousSoftKeyMaskID))
	{
		currentPage = 0;
	}
	previousActiveMaskID = activeMaskID;
	previousSoftKeyMaskID = softKeyMaskID;
	childComponents.clear();
	parentWorkingSet = workingSet;

	int pageCount = 0;

	if (nullptr != softKeyMask)
	{
		const SoftKeyMaskDimensions &dimensions = JuceManagedWorkingSetCache::get_softkey_mask_dimension_info();
		const int slots = dimensions.key_count();
		std::uint16_t firstKey = 0;

		if (0 < slots)
		{
			pageCount = SoftKeyMaskComponent::get_page_count(softKeyMask, parentWorkingSet, dimensions);

			if (currentPage >= pageCount)
			{
				currentPage = static_cast<std::uint16_t>(std::max(pageCount - 1, 0));
			}
			firstKey = static_cast<std::uint16_t>(currentPage * slots);
		}

		auto softKeyMaskComponent = std::make_shared<SoftKeyMaskComponent>(parentWorkingSet, *std::static_pointer_cast<isobus::SoftKeyMask>(softKeyMask), dimensions, firstKey);
		softKeyMaskComponent->setInterceptsMouseClicks(false, false);
		const auto vtColour = parentWorkingSet->get_colour(softKeyMaskComponent->get_background_color());
		pageBar.set_mask_background(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
		childComponents.emplace_back(softKeyMaskComponent);
		addAndMakeVisible(*childComponents.back());
	}
	update_page_bar(pageCount);
	repaint();
}

void SoftKeyMaskRenderAreaComponent::on_working_set_disconnect(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	if ((nullptr != workingSet) && (workingSet == parentWorkingSet))
	{
		parentWorkingSet = nullptr;
		childComponents.clear();
		update_page_bar(0);
		repaint();
	}
}

void SoftKeyMaskRenderAreaComponent::change_page(int delta)
{
	currentPage = static_cast<std::uint16_t>(currentPage + delta);
	on_change_active_mask(parentWorkingSet);
}

void SoftKeyMaskRenderAreaComponent::update_page_bar(int pageCount)
{
	pageBar.setBounds(SoftKeyPageBar::get_area(JuceManagedWorkingSetCache::get_softkey_mask_dimension_info()));
	pageBar.set_page(currentPage, pageCount);

	// Otherwise the rebuilt mask component covers it
	pageBar.toFront(false);
}

void SoftKeyMaskRenderAreaComponent::paint(Graphics &g)
{
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

	if (nullptr != parentWorkingSet)
	{
		g.drawRect(0, 0, getWidth(), getHeight(), 1);
	}
}

void SoftKeyMaskRenderAreaComponent::paintOverChildren(Graphics &g)
{
	// The key face belongs to the pool
	if ((!pressedKeyBounds.isEmpty()) && (nullptr != parentWorkingSet) && (!childComponents.empty()))
	{
		auto vtColour = parentWorkingSet->get_colour(std::static_pointer_cast<SoftKeyMaskComponent>(childComponents.front())->get_background_color());

		g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f).contrasting());
		g.drawRect(pressedKeyBounds.expanded(PRESSED_RING_WIDTH), PRESSED_RING_WIDTH);
	}
}

void SoftKeyMaskRenderAreaComponent::mouseDown(const MouseEvent &event)
{
	if (nullptr != parentWorkingSet)
	{
		// Do a top down search to see if they clicked on some interactable object
		auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

		if (nullptr != workingSetObject)
		{
			auto activeDataOrAlarmMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());
			std::shared_ptr<isobus::VTObject> softkeyMask;

			if (isobus::VirtualTerminalObjectType::AlarmMask == activeDataOrAlarmMask->get_object_type())
			{
				auto child = activeDataOrAlarmMask->get_object_by_id(std::static_pointer_cast<isobus::AlarmMask>(activeDataOrAlarmMask)->get_soft_key_mask(), parentWorkingSet->get_object_tree());

				if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
				{
					softkeyMask = child;
				}
			}
			else if (isobus::VirtualTerminalObjectType::DataMask == activeDataOrAlarmMask->get_object_type())
			{
				auto child = activeDataOrAlarmMask->get_object_by_id(std::static_pointer_cast<isobus::DataMask>(activeDataOrAlarmMask)->get_soft_key_mask(), parentWorkingSet->get_object_tree());

				if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
				{
					softkeyMask = child;
				}
			}

			auto relativeEvent = event.getEventRelativeTo(this);
			auto clickedObject = getClickedChildRecursive(softkeyMask, relativeEvent.getMouseDownX(), relativeEvent.getMouseDownY());

			ownerServer.process_macro(clickedObject, isobus::EventID::OnKeyPress, isobus::VirtualTerminalObjectType::Key, parentWorkingSet);

			std::uint8_t keyCode = 1;

			if (nullptr != clickedObject)
			{
				if (isobus::VirtualTerminalObjectType::Key == clickedObject->get_object_type())
				{
					keyCode = std::static_pointer_cast<isobus::Key>(clickedObject)->get_key_code();
				}

				ownerServer.send_soft_key_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonPressedOrLatched,
				                                             clickedObject->get_id(),
				                                             activeDataOrAlarmMask->get_id(),
				                                             keyCode,
				                                             ownerServer.get_active_working_set()->get_control_function());
				ownerServer.set_button_held(ownerServer.get_active_working_set(),
				                            clickedObject->get_id(),
				                            activeDataOrAlarmMask->get_id(),
				                            keyCode,
				                            true);

				const SoftKeyMaskDimensions &keyDimensions = JuceManagedWorkingSetCache::get_softkey_mask_dimension_info();

				for (int slot = 0; slot < keyDimensions.key_count(); slot++)
				{
					const auto keyBounds = juce::Rectangle<int>(keyDimensions.keyWidth, keyDimensions.keyHeight).withPosition(keyDimensions.key_position(slot));

					if (keyBounds.contains(relativeEvent.getMouseDownPosition()))
					{
						pressedKeyBounds = keyBounds;
						repaint();
					}
				}
			}
		}
	}
}

void SoftKeyMaskRenderAreaComponent::mouseDrag(const MouseEvent &event)
{
	if ((!pressedKeyBounds.isEmpty()) && (!pressedKeyBounds.contains(event.getEventRelativeTo(this).getPosition())))
	{
		pressedKeyBounds = {};
		repaint();
		ownerServer.release_held_buttons(true);
	}
}

void SoftKeyMaskRenderAreaComponent::mouseUp(const MouseEvent &event)
{
	if ((!pressedKeyBounds.isEmpty()) && (nullptr != parentWorkingSet))
	{
		// Do a top down search to see if they clicked on some interactable object
		auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

		if (nullptr != workingSetObject)
		{
			auto activeDataOrAlarmMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());
			std::shared_ptr<isobus::VTObject> softkeyMask;

			if (isobus::VirtualTerminalObjectType::AlarmMask == activeDataOrAlarmMask->get_object_type())
			{
				auto child = activeDataOrAlarmMask->get_object_by_id(std::static_pointer_cast<isobus::AlarmMask>(activeDataOrAlarmMask)->get_soft_key_mask(), parentWorkingSet->get_object_tree());

				if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
				{
					softkeyMask = child;
				}
			}
			else if (isobus::VirtualTerminalObjectType::DataMask == activeDataOrAlarmMask->get_object_type())
			{
				auto child = activeDataOrAlarmMask->get_object_by_id(std::static_pointer_cast<isobus::DataMask>(activeDataOrAlarmMask)->get_soft_key_mask(), parentWorkingSet->get_object_tree());

				if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
				{
					softkeyMask = child;
				}
			}

			auto relativeEvent = event.getEventRelativeTo(this);
			auto clickedObject = getClickedChildRecursive(softkeyMask, relativeEvent.getMouseDownX(), relativeEvent.getMouseDownY());

			ownerServer.process_macro(clickedObject, isobus::EventID::OnKeyRelease, isobus::VirtualTerminalObjectType::Key, parentWorkingSet);

			std::uint8_t keyCode = 1;

			if (nullptr != clickedObject)
			{
				if (isobus::VirtualTerminalObjectType::Key == clickedObject->get_object_type())
				{
					keyCode = std::static_pointer_cast<isobus::Key>(clickedObject)->get_key_code();
				}

				ownerServer.send_soft_key_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased,
				                                             clickedObject->get_id(),
				                                             activeDataOrAlarmMask->get_id(),
				                                             keyCode,
				                                             ownerServer.get_active_working_set()->get_control_function());
				ownerServer.set_button_released(ownerServer.get_active_working_set(),
				                                clickedObject->get_id(),
				                                activeDataOrAlarmMask->get_id(),
				                                keyCode,
				                                true);
			}
			ownerServer.release_held_buttons();
		}
	}
	pressedKeyBounds = {};
	repaint();
}

std::shared_ptr<isobus::VTObject> SoftKeyMaskRenderAreaComponent::getClickedChildRecursive(std::shared_ptr<isobus::VTObject> object, int x, int y)
{
	std::shared_ptr<isobus::VTObject> retVal;
	const SoftKeyMaskDimensions &keyDimensions = JuceManagedWorkingSetCache::get_softkey_mask_dimension_info();

	if ((nullptr == object) ||
	    ((isobus::VirtualTerminalObjectType::ObjectPointer != object->get_object_type()) &&
	     (0 == object->get_number_children())))
	{
		return nullptr;
	}

	if (isobus::VirtualTerminalObjectType::ObjectPointer == object->get_object_type())
	{
		auto child = object->get_object_by_id(std::static_pointer_cast<isobus::ObjectPointer>(object)->get_value(), parentWorkingSet->get_object_tree());

		if ((nullptr != child) &&
		    (objectCanBeClicked(child)) &&
		    (isClickWithinBounds(x, y, 0, 0, keyDimensions.keyWidth, keyDimensions.keyHeight)))
		{
			return child;
		}
		else if (!objectCanBeClicked(child))
		{
			retVal = getClickedChildRecursive(child, x, y);
		}
	}
	else
	{
		const int slots = keyDimensions.key_count();
		const int firstKey = currentPage * slots;

		for (int i = firstKey; (i < object->get_number_children()) && (i < firstKey + slots); i++)
		{
			auto child = object->get_object_by_id(object->get_child_id(static_cast<std::uint16_t>(i)), parentWorkingSet->get_object_tree());
			const auto position = keyDimensions.key_position(i - firstKey);

			if ((nullptr != child) &&
			    (objectCanBeClicked(child)) &&
			    (isClickWithinBounds(x, y, position.x, position.y, keyDimensions.keyWidth, keyDimensions.keyHeight)))
			{
				return child;
			}
			else if (!objectCanBeClicked(child))
			{
				retVal = getClickedChildRecursive(child, x - position.x, y - position.y);

				if (nullptr != retVal)
				{
					break;
				}
			}
		}
	}
	return retVal;
}

bool SoftKeyMaskRenderAreaComponent::objectCanBeClicked(std::shared_ptr<isobus::VTObject> object)
{
	bool retVal = false;

	if (nullptr != object)
	{
		switch (object->get_object_type())
		{
			case isobus::VirtualTerminalObjectType::Key:
			{
				retVal = true;
			}
			break;

			default:
			{
				retVal = false;
			}
			break;
		}
	}
	return retVal;
}

bool SoftKeyMaskRenderAreaComponent::isClickWithinBounds(int clickXRelative, int clickYRelative, int objectX, int objectY, int objectWidth, int objectHeight)
{
	return ((clickXRelative >= objectX) && (clickXRelative < (objectX + objectWidth))) &&
	  ((clickYRelative >= objectY) && (clickYRelative < (objectY + objectHeight)));
}
