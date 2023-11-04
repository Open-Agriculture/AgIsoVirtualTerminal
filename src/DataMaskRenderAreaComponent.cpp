#include "DataMaskRenderAreaComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"
#include "ServerMainComponent.hpp"

DataMaskRenderAreaComponent::DataMaskRenderAreaComponent(ServerMainComponent &parentServer) :
  ownerServer(parentServer)
{
	addMouseListener(this, true);
}

void DataMaskRenderAreaComponent::on_change_active_mask(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	childComponents.clear();
	parentWorkingSet = workingSet;

	auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());
	setBounds(100, 0, 500, 500);

	if ((nullptr != workingSetObject) && (isobus::NULL_OBJECT_ID != workingSetObject->get_active_mask()))
	{
		auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());
		childComponents.emplace_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, activeMask));
		addAndMakeVisible(*childComponents.back());
	}
	repaint();
	needToRepaintActiveArea = false;
}

void DataMaskRenderAreaComponent::paint(Graphics &g)
{
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
	g.drawRect(0, 0, 480, 480, 1);
}

void DataMaskRenderAreaComponent::mouseDown(const MouseEvent &event)
{
	// Do a top down search to see if they clicked on some interactable object
	auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

	if (nullptr != workingSetObject)
	{
		auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());

		auto relativeEvent = event.getEventRelativeTo(this);
		auto clickedObject = getClickedChildRecursive(activeMask, relativeEvent.getMouseDownX(), relativeEvent.getMouseDownY());

		std::uint8_t keyCode = 1;

		if (nullptr != clickedObject)
		{
			if (isobus::VirtualTerminalObjectType::Button == clickedObject->get_object_type())
			{
				keyCode = std::static_pointer_cast<isobus::Button>(clickedObject)->get_key_code();
			}
			else if (isobus::VirtualTerminalObjectType::Key == clickedObject->get_object_type())
			{
				keyCode = std::static_pointer_cast<isobus::Key>(clickedObject)->get_key_code();
			}

			ownerServer.send_button_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonPressedOrLatched,
			                                           clickedObject->get_id(),
			                                           activeMask->get_id(),
			                                           keyCode,
			                                           ownerServer.get_active_working_set()->get_control_function());
		}
	}
}

// Used to calculate button release events
void DataMaskRenderAreaComponent::mouseUp(const MouseEvent &event)
{
	// Do a top down search to see if they clicked on some interactable object
	auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

	if (nullptr != workingSetObject)
	{
		auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());

		auto relativeEvent = event.getEventRelativeTo(this);
		auto clickedObject = getClickedChildRecursive(activeMask, relativeEvent.getMouseDownX(), relativeEvent.getMouseDownY());

		std::uint8_t keyCode = 1;

		if (nullptr != clickedObject)
		{
			switch (clickedObject->get_object_type())
			{
				case isobus::VirtualTerminalObjectType::Button:
				{
					if (false == std::static_pointer_cast<isobus::Button>(clickedObject)->get_option(isobus::Button::Options::Disabled))
					{
						keyCode = std::static_pointer_cast<isobus::Button>(clickedObject)->get_key_code();
						ownerServer.send_button_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased,
						                                           clickedObject->get_id(),
						                                           activeMask->get_id(),
						                                           keyCode,
						                                           ownerServer.get_active_working_set()->get_control_function());
					}
				}
				break;

				case isobus::VirtualTerminalObjectType::Key:
				{
					keyCode = std::static_pointer_cast<isobus::Key>(clickedObject)->get_key_code();
					ownerServer.send_button_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased,
					                                           clickedObject->get_id(),
					                                           activeMask->get_id(),
					                                           keyCode,
					                                           ownerServer.get_active_working_set()->get_control_function());
				}
				break;

				case isobus::VirtualTerminalObjectType::InputList:
				{
					auto clickedList = std::static_pointer_cast<isobus::InputList>(clickedObject);

					if ((clickedList->get_option(isobus::InputList::Options::Enabled)) && (clickedList->get_number_children() > 1))
					{
						// Need to display a modal combo selection
						inputListModal.reset(new AlertWindow("Input List Selection", "Select a List Item, then press OK.", MessageBoxIconType::QuestionIcon));
						inputListModal->addComboBox("Input List Combo", StringArray());
						currentModalComponentCache.clear();
						currentModalComponentCache.reserve(clickedList->get_number_children() - 1);

						// In order to handle things that are not strings being allowed in an input list, grab the popup menu itself and shove custom components in there
						auto combo = inputListModal->getComboBoxComponent("Input List Combo");
						auto comboPopup = combo->getRootMenu();

						for (std::uint32_t i = 0; i < clickedList->get_number_children(); i++)
						{
							auto child = clickedList->get_object_by_id(clickedList->get_child_id(static_cast<std::uint16_t>(i)));

							if (0 != i)
							{
								currentModalComponentCache.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));
								comboPopup->addCustomItem(i, *currentModalComponentCache.back().get(), currentModalComponentCache.back()->getWidth(), currentModalComponentCache.back()->getHeight(), true, nullptr, "Object " + std::to_string(clickedList->get_child_id(static_cast<std::uint16_t>(i))));
							}
						}
						inputListModal->addButton("OK", 0);
						auto resultCallback = [this](int /*result*/) {
							this->inputListModal->exitModalState();
							inputListModal.reset();
						};
						inputListModal->enterModalState(true, ModalCallbackFunction::create(std::move(resultCallback)), false);
					}
				}
				break;

				case isobus::VirtualTerminalObjectType::InputNumber:
				{
					auto clickedNumber = std::static_pointer_cast<isobus::InputNumber>(clickedObject);

					if (clickedNumber->get_option2(isobus::InputNumber::Options2::Enabled))
					{
						inputNumberModal.reset(new AlertWindow("Input Number", "Enter a value for this input number, then press OK.", MessageBoxIconType::QuestionIcon));

						float scaledValue = (clickedNumber->get_value() + clickedNumber->get_offset()) * clickedNumber->get_scale();

						for (std::uint16_t i = 0; i < clickedNumber->get_number_children(); i++)
						{
							auto child = clickedNumber->get_object_by_id(clickedNumber->get_child_id(i));

							if (nullptr != child)
							{
								if (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type())
								{
									scaledValue = (std::static_pointer_cast<isobus::NumberVariable>(child)->get_value() + clickedNumber->get_offset()) * clickedNumber->get_scale();
									break;
								}
							}
						}

						inputNumberSlider.reset(new Slider(Slider::SliderStyle::LinearHorizontal, Slider::TextBoxAbove));
						inputNumberSlider->setRange((clickedNumber->get_minimum_value() + clickedNumber->get_offset()) * clickedNumber->get_scale(), (clickedNumber->get_maximum_value() + clickedNumber->get_offset()) * clickedNumber->get_scale());
						inputNumberSlider->setNumDecimalPlacesToDisplay(clickedNumber->get_number_of_decimals());
						inputNumberSlider->setValue(scaledValue, NotificationType::dontSendNotification);
						inputNumberSlider->setSize(400, 80);

						inputNumberListener.set_last_value(clickedNumber->get_value());
						inputNumberListener.set_target(clickedNumber);
						inputNumberSlider->addListener(&inputNumberListener);

						inputNumberModal->addCustomComponent(inputNumberSlider.get());
						inputNumberModal->addButton("OK", 0);
						inputNumberModal->addButton("Cancel", 1); // TODO catch ESC as cancel
						auto resultCallback = [this, clickedNumber](int result) {
							this->inputNumberModal->exitModalState();

							std::uint16_t varNumID = 0xFFFF;
							if (0 == result)
							{
								clickedNumber->set_value(inputNumberListener.get_last_value());

								for (std::uint32_t i = 0; i < clickedNumber->get_number_children(); i++)
								{
									auto child = clickedNumber->get_object_by_id(clickedNumber->get_child_id(static_cast<std::uint16_t>(i)));

									if ((nullptr != child) && (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type()))
									{
										std::static_pointer_cast<isobus::NumberVariable>(child)->set_value(inputNumberListener.get_last_value());
										varNumID = child->get_id();
										break;
									}
								}
								this->needToRepaintActiveArea = true;
							}
							inputNumberListener.set_target(nullptr);
							inputNumberModal.reset();
							inputNumberSlider.reset();
							ownerServer.send_select_input_object_message(clickedNumber->get_id(), false, false, ownerServer.get_client_control_function_for_working_set(parentWorkingSet));

							if (0 == result)
							{
								if (0xFFFF != varNumID)
								{
									ownerServer.send_change_numeric_value_message(varNumID, clickedNumber->get_value(), ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
								}
								else
								{
									ownerServer.send_change_numeric_value_message(clickedNumber->get_id(), clickedNumber->get_value(), ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
								}
							}
							else
							{
								// TODO ESC
							}
						};
						inputNumberModal->enterModalState(true, ModalCallbackFunction::create(std::move(resultCallback)), false);
						ownerServer.send_select_input_object_message(clickedNumber->get_id(), true, true, ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
					}
				}
				break;

				default:
					break;
			}
		}
	}
}

bool DataMaskRenderAreaComponent::needsRepaint() const
{
	return needToRepaintActiveArea;
}

void DataMaskRenderAreaComponent::InputNumberListener::sliderValueChanged(Slider *slider)
{
	if ((nullptr != slider) && (nullptr != targetObject) && (0 != targetObject->get_scale()))
	{
		lastValue = static_cast<std::uint32_t>((slider->getValue() / targetObject->get_scale()) - targetObject->get_offset());
	}
}

std::uint32_t DataMaskRenderAreaComponent::InputNumberListener::get_last_value() const
{
	return lastValue;
}

void DataMaskRenderAreaComponent::InputNumberListener::set_last_value(std::uint32_t value)
{
	lastValue = value;
}

std::shared_ptr<isobus::InputNumber> DataMaskRenderAreaComponent::InputNumberListener::get_target()
{
	return targetObject;
}

void DataMaskRenderAreaComponent::InputNumberListener::set_target(std::shared_ptr<isobus::InputNumber> objectBeingModified)
{
	targetObject = objectBeingModified;
}

std::shared_ptr<isobus::VTObject> DataMaskRenderAreaComponent::getClickedChildRecursive(std::shared_ptr<isobus::VTObject> object, int x, int y)
{
	std::shared_ptr<isobus::VTObject> retVal;

	if ((nullptr == object) || (0 == object->get_number_children()))
	{
		return nullptr;
	}

	for (std::uint16_t i = 0; i < object->get_number_children(); i++)
	{
		auto child = object->get_object_by_id(object->get_child_id(i));

		if ((nullptr != child) &&
		    (objectCanBeClicked(child)) &&
		    (isClickWithinBounds(x, y, object->get_child_x(i), object->get_child_y(i), child->get_width(), child->get_height())))
		{
			return child;
		}
		else if (!objectCanBeClicked(child))
		{
			retVal = getClickedChildRecursive(child, x - object->get_child_x(i), y - object->get_child_y(i));

			if (nullptr != retVal)
			{
				break;
			}
		}
	}
	return retVal;
}

bool DataMaskRenderAreaComponent::objectCanBeClicked(std::shared_ptr<isobus::VTObject> object)
{
	bool retVal = false;

	if (nullptr != object)
	{
		switch (object->get_object_type())
		{
			case isobus::VirtualTerminalObjectType::Button:
			case isobus::VirtualTerminalObjectType::InputList:
			case isobus::VirtualTerminalObjectType::Key:
			case isobus::VirtualTerminalObjectType::InputNumber:
			case isobus::VirtualTerminalObjectType::InputBoolean:
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

bool DataMaskRenderAreaComponent::isClickWithinBounds(int clickXRelative, int clickYRelative, int objectX, int objectY, int objectWidth, int objectHeight)
{
	return ((clickXRelative >= objectX) && (clickXRelative <= (objectX + objectWidth))) &&
	  ((clickYRelative >= objectY) && (clickYRelative <= (objectY + objectHeight)));
}
