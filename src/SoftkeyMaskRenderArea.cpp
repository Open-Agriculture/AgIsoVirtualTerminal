/*******************************************************************************
** @file       SoftKeyMaskRenderAreaComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "JuceManagedWorkingSetCache.hpp"
#include "ServerMainComponent.hpp"
#include "SoftKeyMaskRenderAreaComponent.hpp"

SoftKeyMaskRenderAreaComponent::SoftKeyMaskRenderAreaComponent(ServerMainComponent &parentServer) :
  ownerServer(parentServer)
{
	//addMouseListener(this, true);
}

void SoftKeyMaskRenderAreaComponent::on_change_active_mask(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	childComponents.clear();
	parentWorkingSet = workingSet;

	if (parentWorkingSet)
	{
		auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

		if ((nullptr != workingSetObject) && (isobus::NULL_OBJECT_ID != workingSetObject->get_active_mask()))
		{
			auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());

			if (nullptr != activeMask)
			{
				if (isobus::VirtualTerminalObjectType::AlarmMask == activeMask->get_object_type())
				{
					auto child = activeMask->get_object_by_id(std::static_pointer_cast<isobus::AlarmMask>(activeMask)->get_soft_key_mask(), parentWorkingSet->get_object_tree());

					if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
					{
						childComponents.emplace_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));
						addAndMakeVisible(*childComponents.back());
					}
				}
				else if (isobus::VirtualTerminalObjectType::DataMask == activeMask->get_object_type())
				{
					auto child = activeMask->get_object_by_id(std::static_pointer_cast<isobus::DataMask>(activeMask)->get_soft_key_mask(), parentWorkingSet->get_object_tree());

					if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
					{
						childComponents.emplace_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));
						addAndMakeVisible(*childComponents.back());
					}
				}
			}
		}
	}
	repaint();
}

void SoftKeyMaskRenderAreaComponent::on_working_set_disconnect(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	if ((nullptr != workingSet) && (workingSet == parentWorkingSet))
	{
		parentWorkingSet = nullptr;
		childComponents.clear();
		repaint();
	}
}

void SoftKeyMaskRenderAreaComponent::paint(Graphics &g)
{
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

	if (nullptr != parentWorkingSet)
	{
		g.drawRect(0, 0, getWidth(), getHeight(), 1);
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
			auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());
			auto parentMask = activeMask;

			if (isobus::VirtualTerminalObjectType::AlarmMask == activeMask->get_object_type())
			{
				auto child = activeMask->get_object_by_id(std::static_pointer_cast<isobus::AlarmMask>(activeMask)->get_soft_key_mask(), parentWorkingSet->get_object_tree());

				if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
				{
					activeMask = child;
				}
			}
			else if (isobus::VirtualTerminalObjectType::DataMask == activeMask->get_object_type())
			{
				auto child = activeMask->get_object_by_id(std::static_pointer_cast<isobus::DataMask>(activeMask)->get_soft_key_mask(), parentWorkingSet->get_object_tree());

				if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
				{
					activeMask = child;
				}
			}

			auto relativeEvent = event.getEventRelativeTo(this);
			auto clickedObject = getClickedChildRecursive(activeMask, relativeEvent.getMouseDownX(), relativeEvent.getMouseDownY());

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
				                                             parentMask->get_id(),
				                                             keyCode,
				                                             ownerServer.get_active_working_set()->get_control_function());
				ownerServer.set_button_held(ownerServer.get_active_working_set(),
				                            clickedObject->get_id(),
				                            activeMask->get_id(),
				                            keyCode,
				                            true);
			}
		}
	}
}

void SoftKeyMaskRenderAreaComponent::mouseUp(const MouseEvent &event)
{
	if (nullptr != parentWorkingSet)
	{
		// Do a top down search to see if they clicked on some interactable object
		auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

		if (nullptr != workingSetObject)
		{
			auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());
			auto parentMask = activeMask;

			if (isobus::VirtualTerminalObjectType::AlarmMask == activeMask->get_object_type())
			{
				auto child = activeMask->get_object_by_id(std::static_pointer_cast<isobus::AlarmMask>(activeMask)->get_soft_key_mask(), parentWorkingSet->get_object_tree());

				if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
				{
					activeMask = child;
				}
			}
			else if (isobus::VirtualTerminalObjectType::DataMask == activeMask->get_object_type())
			{
				auto child = activeMask->get_object_by_id(std::static_pointer_cast<isobus::DataMask>(activeMask)->get_soft_key_mask(), parentWorkingSet->get_object_tree());

				if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
				{
					activeMask = child;
				}
			}

			auto relativeEvent = event.getEventRelativeTo(this);
			auto clickedObject = getClickedChildRecursive(activeMask, relativeEvent.getPosition().x, relativeEvent.getPosition().y);

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
				                                             parentMask->get_id(),
				                                             keyCode,
				                                             ownerServer.get_active_working_set()->get_control_function());
				ownerServer.set_button_released(ownerServer.get_active_working_set(),
				                                clickedObject->get_id(),
				                                activeMask->get_id(),
				                                keyCode,
				                                true);
			}
		}
	}
}

std::shared_ptr<isobus::VTObject> SoftKeyMaskRenderAreaComponent::getClickedChildRecursive(std::shared_ptr<isobus::VTObject> object, int x, int y)
{
	std::shared_ptr<isobus::VTObject> retVal;

	if ((nullptr == object) ||
	    ((isobus::VirtualTerminalObjectType::ObjectPointer != object->get_object_type()) &&
	     (0 == object->get_number_children())))
	{
		return nullptr;
	}

	if (isobus::VirtualTerminalObjectType::ObjectPointer == object->get_object_type())
	{
		auto child = object->get_object_by_id(std::static_pointer_cast<isobus::ObjectPointer>(object)->get_value(), parentWorkingSet->get_object_tree());

		// Knowing the location requires some knowledge of how the mask is displaying each key...

		if ((nullptr != child) &&
		    (objectCanBeClicked(child)) &&
		    (isClickWithinBounds(x, y, 0, 0, ownerServer.get_soft_key_descriptor_x_pixel_width(), ownerServer.get_soft_key_descriptor_y_pixel_height())))
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
		int row = 0, col = (ownerServer.get_physical_soft_key_columns() - 1);
		for (std::uint16_t i = 0; i < object->get_number_children(); i++)
		{
			auto child = object->get_object_by_id(object->get_child_id(i), parentWorkingSet->get_object_tree());

			// Knowing the location requires some knowledge of how the mask is displaying each key...

			int colX = SoftKeyMaskDimensions::padding + col * (ownerServer.get_soft_key_descriptor_x_pixel_width() + SoftKeyMaskDimensions::padding);
			int rowY = SoftKeyMaskDimensions::padding + row * (ownerServer.get_soft_key_descriptor_y_pixel_height() + SoftKeyMaskDimensions::padding);
			if ((nullptr != child) &&
			    (objectCanBeClicked(child)) &&
					(isClickWithinBounds(x, y, colX, rowY, ownerServer.get_soft_key_descriptor_x_pixel_width(), ownerServer.get_soft_key_descriptor_y_pixel_height())))
			{
				return child;
			}
			else if (!objectCanBeClicked(child))
			{
				retVal = getClickedChildRecursive(child, x - colX, y - rowY);

				if (nullptr != retVal)
				{
					break;
				}
			}
			row++;
			if (row >= ownerServer.get_physical_soft_key_rows())
			{
				row = 0;
				col--;
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
	return ((clickXRelative >= objectX) && (clickXRelative <= (objectX + objectWidth))) &&
	  ((clickYRelative >= objectY) && (clickYRelative <= (objectY + objectHeight)));
}
