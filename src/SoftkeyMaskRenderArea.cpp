#include "JuceManagedWorkingSetCache.hpp"
#include "ServerMainComponent.hpp"
#include "SoftKeyMaskRenderAreaComponent.hpp"

SoftKeyMaskRenderAreaComponent::SoftKeyMaskRenderAreaComponent(ServerMainComponent &parentServer) :
  ownerServer(parentServer)
{
	addMouseListener(this, true);
}

void SoftKeyMaskRenderAreaComponent::on_change_active_mask(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	childComponents.clear();
	parentWorkingSet = workingSet;

	auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());
	setBounds(580, 0, 100, 480);

	if ((nullptr != workingSetObject) && (isobus::NULL_OBJECT_ID != workingSetObject->get_active_mask()))
	{
		auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());

		if (nullptr != activeMask)
		{
			for (std::uint16_t i = 0; i < activeMask->get_number_children(); i++)
			{
				auto child = activeMask->get_object_by_id(activeMask->get_child_id(i));

				if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
				{
					childComponents.emplace_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));
					addAndMakeVisible(*childComponents.back());
					break;
				}
			}
		}
	}
	repaint();
}

void SoftKeyMaskRenderAreaComponent::paint(Graphics &g)
{
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
	g.drawRect(0, 0, 100, 480, 1);
}

void SoftKeyMaskRenderAreaComponent::mouseDown(const MouseEvent &event)
{
	// Do a top down search to see if they clicked on some interactable object
	auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

	if (nullptr != workingSetObject)
	{
		auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());

		for (std::uint16_t i = 0; i < activeMask->get_number_children(); i++)
		{
			auto child = activeMask->get_object_by_id(activeMask->get_child_id(i));

			if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
			{
				activeMask = child;
				break;
			}
		}

		auto relativeEvent = event.getEventRelativeTo(this);
		auto clickedObject = getClickedChildRecursive(activeMask, relativeEvent.getMouseDownX(), relativeEvent.getMouseDownY());

		std::uint8_t keyCode = 1;

		if (nullptr != clickedObject)
		{
			if (isobus::VirtualTerminalObjectType::Key == clickedObject->get_object_type())
			{
				keyCode = std::static_pointer_cast<isobus::Button>(clickedObject)->get_key_code();
			}

			ownerServer.send_soft_key_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonPressedOrLatched,
			                                             clickedObject->get_id(),
			                                             activeMask->get_id(),
			                                             keyCode,
			                                             ownerServer.get_active_working_set()->get_control_function());
		}
	}
}

void SoftKeyMaskRenderAreaComponent::mouseUp(const MouseEvent &event)
{
	// Do a top down search to see if they clicked on some interactable object
	auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

	if (nullptr != workingSetObject)
	{
		auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());

		for (std::uint16_t i = 0; i < activeMask->get_number_children(); i++)
		{
			auto child = activeMask->get_object_by_id(activeMask->get_child_id(i));

			if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
			{
				activeMask = child;
				break;
			}
		}

		auto relativeEvent = event.getEventRelativeTo(this);
		auto clickedObject = getClickedChildRecursive(activeMask, relativeEvent.getPosition().x, relativeEvent.getPosition().y);

		std::uint8_t keyCode = 1;

		if (nullptr != clickedObject)
		{
			if (isobus::VirtualTerminalObjectType::Key == clickedObject->get_object_type())
			{
				keyCode = std::static_pointer_cast<isobus::Button>(clickedObject)->get_key_code();
			}

			ownerServer.send_soft_key_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased,
			                                             clickedObject->get_id(),
			                                             activeMask->get_id(),
			                                             keyCode,
			                                             ownerServer.get_active_working_set()->get_control_function());
		}
	}
}

std::shared_ptr<isobus::VTObject> SoftKeyMaskRenderAreaComponent::getClickedChildRecursive(std::shared_ptr<isobus::VTObject> object, int x, int y)
{
	std::shared_ptr<isobus::VTObject> retVal;

	if ((nullptr == object) || (0 == object->get_number_children()))
	{
		return nullptr;
	}

	for (std::uint16_t i = 0; i < object->get_number_children(); i++)
	{
		auto child = object->get_object_by_id(object->get_child_id(i));

		// Knowing the location requires some knowledge of how the mask is displaying each key...

		if ((nullptr != child) &&
		    (objectCanBeClicked(child)) &&
		    (isClickWithinBounds(x, y, 10, 10 + (60 * i) + (10 * i), ownerServer.get_soft_key_descriptor_x_pixel_width(), ownerServer.get_soft_key_descriptor_y_pixel_width())))
		{
			return child;
		}
		else if (!objectCanBeClicked(child))
		{
			retVal = getClickedChildRecursive(child, x - 10, y - (10 + (60 * i) + (10 * i)));

			if (nullptr != retVal)
			{
				break;
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
