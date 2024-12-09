/*******************************************************************************
** @file       DataMaskRenderAreaComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "DataMaskRenderAreaComponent.hpp"
#include "AppImages.h"
#include "JuceManagedWorkingSetCache.hpp"
#include "ServerMainComponent.hpp"

DataMaskRenderAreaComponent::DataMaskRenderAreaComponent(ServerMainComponent &parentServer) :
  ownerServer(parentServer)
{
}

void DataMaskRenderAreaComponent::on_change_active_mask(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	childComponents.clear();
	parentWorkingSet = workingSet;

	auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

	if ((nullptr != workingSetObject) && (isobus::NULL_OBJECT_ID != workingSetObject->get_active_mask()))
	{
		auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());
		childComponents.emplace_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, activeMask));
		addAndMakeVisible(*childComponents.back());
	}
	repaint();
	needToRepaintActiveArea = false;
}

void DataMaskRenderAreaComponent::on_working_set_disconnect(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	if ((nullptr != workingSet) && (parentWorkingSet == workingSet))
	{
		childComponents.clear();
		parentWorkingSet.reset();
		repaint();
	}
}

void DataMaskRenderAreaComponent::paint(Graphics &g)
{
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

	if (nullptr != parentWorkingSet)
	{
		g.drawRect(0, 0, getWidth(), getHeight(), 1);
	}
	else
	{
		auto logoImage = ImageCache::getFromMemory(AppImages::logo2_png, AppImages::logo2_pngSize);
		g.drawImage(logoImage, 0, 0, 400, 400, 0, 0, logoImage.getWidth(), logoImage.getHeight());
		g.setColour(Colours::white);
		g.drawText("No Working Sets Are Active", 0, 400, 400, 80, Justification::centredTop, true);

		if (!hasStarted)
		{
			g.drawFittedText("To start the VT server, select \"Start/Stop\" from the control menu in the top left.", 0, 440, 400, 40, Justification::centredTop, 2);
		}
	}
}

void DataMaskRenderAreaComponent::mouseDown(const MouseEvent &event)
{
	if (nullptr != parentWorkingSet)
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
					ownerServer.process_macro(clickedObject, isobus::EventID::OnKeyPress, isobus::VirtualTerminalObjectType::Button, parentWorkingSet);
				}
				else if (isobus::VirtualTerminalObjectType::Key == clickedObject->get_object_type())
				{
					keyCode = std::static_pointer_cast<isobus::Key>(clickedObject)->get_key_code();
					ownerServer.process_macro(clickedObject, isobus::EventID::OnKeyPress, isobus::VirtualTerminalObjectType::Key, parentWorkingSet);
				}

				ownerServer.send_button_activation_message(isobus::VirtualTerminalBase::KeyActivationCode::ButtonPressedOrLatched,
				                                           clickedObject->get_id(),
				                                           activeMask->get_id(),
				                                           keyCode,
				                                           ownerServer.get_active_working_set()->get_control_function());
				if (isobus::VirtualTerminalObjectType::Key == clickedObject->get_object_type() ||
				    isobus::VirtualTerminalObjectType::Button == clickedObject->get_object_type())
				{
					ownerServer.set_button_held(ownerServer.get_active_working_set(),
					                            clickedObject->get_id(),
					                            activeMask->get_id(),
					                            keyCode,
					                            (isobus::VirtualTerminalObjectType::Key == clickedObject->get_object_type()));
				}
			}
		}
	}
}

// Used to calculate button release events
void DataMaskRenderAreaComponent::mouseUp(const MouseEvent &event)
{
	if (nullptr != parentWorkingSet)
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
							ownerServer.process_macro(clickedObject, isobus::EventID::OnKeyRelease, isobus::VirtualTerminalObjectType::Button, parentWorkingSet);
							ownerServer.set_button_released(ownerServer.get_active_working_set(),
							                                clickedObject->get_id(),
							                                activeMask->get_id(),
							                                keyCode,
							                                true);
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
						ownerServer.process_macro(clickedObject, isobus::EventID::OnKeyRelease, isobus::VirtualTerminalObjectType::Key, parentWorkingSet);
						ownerServer.set_button_released(ownerServer.get_active_working_set(),
						                                clickedObject->get_id(),
						                                activeMask->get_id(),
						                                keyCode,
						                                true);
					}
					break;

					case isobus::VirtualTerminalObjectType::InputList:
					{
						auto clickedList = std::static_pointer_cast<isobus::InputList>(clickedObject);

						if ((clickedList->get_option(isobus::InputList::Options::Enabled)) &&
						    (clickedList->get_number_children() > 0) &&
						    clickedList->get_option(isobus::InputList::Options::Enabled))
						{
							// Need to display a modal combo selection
							inputListModal.reset(new AlertWindow("Input List Selection", "Select a List Item, then press OK.", MessageBoxIconType::QuestionIcon));
							inputListModal->addComboBox("Input List Combo", StringArray());
							currentModalComponentCache.clear();
							currentModalComponentCache.reserve(clickedList->get_number_children());

							// In order to handle things that are not strings being allowed in an input list, grab the popup menu itself and shove custom components in there
							auto combo = inputListModal->getComboBoxComponent("Input List Combo");
							auto comboPopup = combo->getRootMenu();

							auto selectedIndex = -1;

							if (clickedList->get_variable_reference() != isobus::NULL_OBJECT_ID)
							{
								auto child = clickedList->get_object_by_id(clickedList->get_variable_reference(), parentWorkingSet->get_object_tree());
								if (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type())
								{
									auto child = clickedList->get_object_by_id(clickedList->get_variable_reference(), parentWorkingSet->get_object_tree());

									if (nullptr != child)
									{
										if (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type())
										{
											selectedIndex = std::static_pointer_cast<isobus::NumberVariable>(child)->get_value();
										}
									}
								}
							}

							for (std::uint32_t i = 0; i < clickedList->get_number_children(); i++)
							{
								auto child = clickedList->get_object_by_id(clickedList->get_child_id(static_cast<std::uint16_t>(i)), parentWorkingSet->get_object_tree());

								if (nullptr != child)
								{
									currentModalComponentCache.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));
									auto text = "Object " + std::to_string(clickedList->get_child_id(static_cast<std::uint16_t>(i)));
									if (child && child->get_object_type() == isobus::VirtualTerminalObjectType::OutputString)
									{
										text = std::static_pointer_cast<isobus::OutputString>(child)->displayed_value(parentWorkingSet);
									}

									comboPopup->addCustomItem(i + 1, *currentModalComponentCache.back().get(), currentModalComponentCache.back()->getWidth(), currentModalComponentCache.back()->getHeight(), true, nullptr, text);
								}
							}

							if (selectedIndex != -1)
							{
								combo->setSelectedItemIndex(selectedIndex);
							}

							inputListModal->addButton("OK", 0);
							auto resultCallback = [this, clickedList](int result) {
								auto inputCombo = inputListModal->getComboBoxComponent("Input List Combo");
								result = inputCombo->getSelectedItemIndex();

								// Remap the visible index to the actual index
								std::uint16_t numberOfNonNullsSeen = 0;
								for (std::uint16_t i = 0; i < clickedList->get_number_children(); i++)
								{
									if (isobus::NULL_OBJECT_ID != clickedList->get_child_id(i))
									{
										numberOfNonNullsSeen++;
									}

									if (numberOfNonNullsSeen == result + 1)
									{
										result = i;
										break;
									}
								}

								if (isobus::NULL_OBJECT_ID != clickedList->get_variable_reference())
								{
									auto child = clickedList->get_object_by_id(clickedList->get_variable_reference(), parentWorkingSet->get_object_tree());

									if (nullptr != child)
									{
										if (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type())
										{
											if (std::static_pointer_cast<isobus::NumberVariable>(child)->get_value() != static_cast<std::uint32_t>(result))
											{
												std::static_pointer_cast<isobus::NumberVariable>(child)->set_value(result);
												ownerServer.send_change_numeric_value_message(child->get_id(), result, ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
												ownerServer.process_macro(child, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::NumberVariable, parentWorkingSet);
											}
										}
									}
								}
								else
								{
									ownerServer.process_macro(clickedList, isobus::EventID::OnEntryOfAValue, isobus::VirtualTerminalObjectType::InputList, parentWorkingSet);
									if (clickedList->get_value() != result)
									{
										ownerServer.process_macro(clickedList, isobus::EventID::OnEntryOfANewValue, isobus::VirtualTerminalObjectType::InputList, parentWorkingSet);
										clickedList->set_value(static_cast<std::uint8_t>(result));
										ownerServer.send_change_numeric_value_message(clickedList->get_id(), result, ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
										ownerServer.process_macro(clickedList, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::InputList, parentWorkingSet);
									}
								}
								this->inputListModal->exitModalState();
								parentWorkingSet->set_object_focus(isobus::NULL_OBJECT_ID);
								ownerServer.process_macro(clickedList, isobus::EventID::OnInputFieldDeselection, isobus::VirtualTerminalObjectType::InputList, parentWorkingSet);
								inputListModal.reset();
								repaint();
							};
							inputListModal->enterModalState(true, ModalCallbackFunction::create(std::move(resultCallback)), false);
							parentWorkingSet->set_object_focus(clickedObject->get_id());
							ownerServer.process_macro(clickedObject, isobus::EventID::OnInputFieldSelection, isobus::VirtualTerminalObjectType::InputList, parentWorkingSet);
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

							if (isobus::NULL_OBJECT_ID != clickedNumber->get_variable_reference())
							{
								auto child = clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), parentWorkingSet->get_object_tree());

								if (nullptr != child)
								{
									if (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type())
									{
										scaledValue = (std::static_pointer_cast<isobus::NumberVariable>(child)->get_value() + clickedNumber->get_offset()) * clickedNumber->get_scale();
									}
								}
							}

							inputNumberSlider.reset(new Slider(Slider::SliderStyle::LinearHorizontal, Slider::TextBoxAbove));
							inputNumberSlider->setRange((static_cast<float>(clickedNumber->get_minimum_value()) + clickedNumber->get_offset()) * clickedNumber->get_scale(),
							                            (static_cast<float>(clickedNumber->get_maximum_value()) + clickedNumber->get_offset()) * clickedNumber->get_scale());
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
									ownerServer.process_macro(clickedNumber, isobus::EventID::OnEntryOfAValue, isobus::VirtualTerminalObjectType::InputNumber, parentWorkingSet);

									if (isobus::NULL_OBJECT_ID != clickedNumber->get_variable_reference())
									{
										auto child = clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), parentWorkingSet->get_object_tree());

										if ((nullptr != child) && (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type()))
										{
											varNumID = child->get_id();
										}
									}

									if (isobus::NULL_OBJECT_ID != varNumID)
									{
										if (std::static_pointer_cast<isobus::NumberVariable>(clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), parentWorkingSet->get_object_tree()))->get_value() != inputNumberListener.get_last_value())
										{
											ownerServer.process_macro(clickedNumber, isobus::EventID::OnEntryOfANewValue, isobus::VirtualTerminalObjectType::InputNumber, parentWorkingSet);
										}
										std::static_pointer_cast<isobus::NumberVariable>(clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), parentWorkingSet->get_object_tree()))->set_value(inputNumberListener.get_last_value());
									}
									else
									{
										if (clickedNumber->get_value() != inputNumberListener.get_last_value())
										{
											ownerServer.process_macro(clickedNumber, isobus::EventID::OnEntryOfANewValue, isobus::VirtualTerminalObjectType::InputNumber, parentWorkingSet);
											clickedNumber->set_value(inputNumberListener.get_last_value());
											ownerServer.process_macro(clickedNumber, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::InputNumber, parentWorkingSet);
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
										ownerServer.send_change_numeric_value_message(varNumID, std::static_pointer_cast<isobus::NumberVariable>(clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), parentWorkingSet->get_object_tree()))->get_value(), ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
										ownerServer.process_macro(clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), parentWorkingSet->get_object_tree()), isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::NumberVariable, parentWorkingSet);
									}
									else
									{
										ownerServer.send_change_numeric_value_message(clickedNumber->get_id(), clickedNumber->get_value(), ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
										ownerServer.process_macro(clickedNumber, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::InputNumber, parentWorkingSet);
									}
								}
								else
								{
									// TODO ESC
								}

								parentWorkingSet->set_object_focus(isobus::NULL_OBJECT_ID);
								ownerServer.process_macro(clickedNumber, isobus::EventID::OnInputFieldDeselection, isobus::VirtualTerminalObjectType::InputNumber, parentWorkingSet);
							};
							inputNumberModal->enterModalState(true, ModalCallbackFunction::create(std::move(resultCallback)), false);
							ownerServer.send_select_input_object_message(clickedNumber->get_id(), true, true, ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
							parentWorkingSet->set_object_focus(clickedObject->get_id());
							ownerServer.process_macro(clickedObject, isobus::EventID::OnInputFieldSelection, isobus::VirtualTerminalObjectType::InputNumber, parentWorkingSet);
						}
					}
					break;

					case isobus::VirtualTerminalObjectType::InputBoolean:
					{
						auto clickedBool = std::static_pointer_cast<isobus::InputBoolean>(clickedObject);

						if (clickedBool->get_enabled())
						{
							bool hasNumberVariable = false;
							ownerServer.process_macro(clickedBool, isobus::EventID::OnEntryOfAValue, isobus::VirtualTerminalObjectType::InputBoolean, parentWorkingSet);
							ownerServer.process_macro(clickedBool, isobus::EventID::OnEntryOfANewValue, isobus::VirtualTerminalObjectType::InputBoolean, parentWorkingSet);

							if (isobus::NULL_OBJECT_ID != clickedBool->get_variable_reference())
							{
								auto child = clickedBool->get_object_by_id(clickedBool->get_variable_reference(), parentWorkingSet->get_object_tree());

								if (nullptr != child)
								{
									if (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type())
									{
										hasNumberVariable = true;
										std::static_pointer_cast<isobus::NumberVariable>(child)->set_value(!(0 != std::static_pointer_cast<isobus::NumberVariable>(child)->get_value()));
										ownerServer.send_change_numeric_value_message(child->get_id(), std::static_pointer_cast<isobus::NumberVariable>(child)->get_value(), ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
										ownerServer.process_macro(child, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::NumberVariable, parentWorkingSet);
									}
								}
							}

							if (!hasNumberVariable)
							{
								clickedBool->set_value(~clickedBool->get_value());
								ownerServer.send_change_numeric_value_message(clickedBool->get_id(), clickedBool->get_value(), ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
								ownerServer.process_macro(clickedBool, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::InputBoolean, parentWorkingSet);
							}
							repaint();
						}
					}
					break;

					case isobus::VirtualTerminalObjectType::InputString:
					{
						auto clickedString = std::static_pointer_cast<isobus::InputString>(clickedObject);
						std::shared_ptr<isobus::StringVariable> stringVariable;

						if (clickedString->get_enabled())
						{
							bool hasStringVariable = false;

							if (isobus::NULL_OBJECT_ID != clickedString->get_variable_reference())
							{
								auto child = clickedString->get_object_by_id(clickedString->get_variable_reference(), parentWorkingSet->get_object_tree());

								if (nullptr != child)
								{
									if (isobus::VirtualTerminalObjectType::StringVariable == child->get_object_type())
									{
										hasStringVariable = true;
										stringVariable = std::static_pointer_cast<isobus::StringVariable>(child);
									}
								}
							}

							inputStringModal.reset(new AlertWindow("Input String", "Enter a value for this input string, then press OK.", MessageBoxIconType::QuestionIcon));
							inputStringModal->addTextEditor("Input String", hasStringVariable ? stringVariable->get_value() : clickedString->get_value());
							inputStringModal->addButton("OK", 0);
							inputStringModal->addButton("Cancel", 1); // TODO catch ESC as cancel
							auto resultCallback = [this, clickedString, stringVariable](int result) {
								this->inputStringModal->exitModalState();
								ownerServer.send_select_input_object_message(clickedString->get_id(), false, false, ownerServer.get_client_control_function_for_working_set(parentWorkingSet));

								if (0 == result) //OK
								{
									String newContent = this->inputStringModal->getTextEditor("Input String")->getText();
									ownerServer.process_macro(clickedString, isobus::EventID::OnEntryOfAValue, isobus::VirtualTerminalObjectType::InputString, parentWorkingSet);

									if (nullptr != stringVariable)
									{
										// It seems common practice to pad the string value out to the length in the object pool.
										while (newContent.length() < stringVariable->get_value().length())
										{
											newContent.append(" ", 1);
										}
										stringVariable->set_value(newContent.toStdString());
										ownerServer.send_change_string_value_message(stringVariable->get_id(), newContent.toStdString(), ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
										ownerServer.process_macro(stringVariable, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::StringVariable, parentWorkingSet);
									}
									else
									{
										// It seems common practice to pad the string value out to the length in the object pool.
										while (newContent.length() < clickedString->get_value().length())
										{
											newContent.append(" ", 1);
										}
										if (clickedString->get_value() != newContent)
										{
											ownerServer.process_macro(clickedString, isobus::EventID::OnEntryOfANewValue, isobus::VirtualTerminalObjectType::InputString, parentWorkingSet);
										}
										clickedString->set_value(newContent.toStdString());
										ownerServer.send_change_string_value_message(clickedString->get_id(), newContent.toStdString(), ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
										ownerServer.process_macro(clickedString, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::InputString, parentWorkingSet);
									}
									needToRepaintActiveArea = true;
								}
								inputStringModal->exitModalState();
								inputStringModal.reset();
								parentWorkingSet->set_object_focus(isobus::NULL_OBJECT_ID);
								ownerServer.process_macro(clickedString, isobus::EventID::OnInputFieldDeselection, isobus::VirtualTerminalObjectType::InputString, parentWorkingSet);
							};
							inputStringModal->enterModalState(true, ModalCallbackFunction::create(std::move(resultCallback)), false);
							ownerServer.send_select_input_object_message(clickedString->get_id(), true, true, ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
							parentWorkingSet->set_object_focus(clickedObject->get_id());
							ownerServer.process_macro(clickedObject, isobus::EventID::OnInputFieldSelection, isobus::VirtualTerminalObjectType::InputString, parentWorkingSet);
						}
					}
					break;

					default:
						break;
				}
			}
		}
	}
}

bool DataMaskRenderAreaComponent::needsRepaint() const
{
	return needToRepaintActiveArea;
}

void DataMaskRenderAreaComponent::set_has_started(bool started)
{
	hasStarted = started;
	repaint();
}

void DataMaskRenderAreaComponent::InputNumberListener::sliderValueChanged(Slider *slider)
{
	if ((nullptr != slider) && (nullptr != targetObject) && (0 != targetObject->get_scale()))
	{
		float scaledValue = (slider->getValue() / targetObject->get_scale()) - targetObject->get_offset();
		lastValue = static_cast<std::uint32_t>(scaledValue);
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
		    (isClickWithinBounds(x, y, 0, 0, child->get_width(), child->get_height())))
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
		for (std::uint16_t i = 0; i < object->get_number_children(); i++)
		{
			auto child = object->get_object_by_id(object->get_child_id(i), parentWorkingSet->get_object_tree());

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
			case isobus::VirtualTerminalObjectType::InputString:
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
