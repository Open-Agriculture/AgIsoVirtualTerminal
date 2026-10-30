/*******************************************************************************
** @file       DataMaskRenderAreaComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "DataMaskRenderAreaComponent.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include "AppImages.h"
#include "ButtonComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"
#include "ServerMainComponent.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

constexpr int DESIGNED_MASK_SIZES[] = { 200, 240, 480 };

// Pools often overhang their mask by a few percent and rely on the VT to clip it.
constexpr int MASK_OVERHANG_TOLERANCE_DIVISOR = 20;

// The median, because pools park objects outside the mask and wrap children in oversized containers.
static int measure_median_mask_extent(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet)
{
	std::vector<int> maskExtents;

	for (const auto &currentObject : workingSet->get_object_tree())
	{
		auto object = currentObject.second;

		if ((nullptr == object) ||
		    ((isobus::VirtualTerminalObjectType::DataMask != object->get_object_type()) &&
		     (isobus::VirtualTerminalObjectType::AlarmMask != object->get_object_type())))
		{
			continue;
		}

		int extent = 0;

		for (std::uint16_t i = 0; i < object->get_number_children(); i++)
		{
			auto child = object->get_object_by_id(object->get_child_id(i), workingSet->get_object_tree());

			if ((nullptr != child) && (isobus::VirtualTerminalObjectType::ObjectPointer == child->get_object_type()))
			{
				child = object->get_object_by_id(std::static_pointer_cast<isobus::ObjectPointer>(child)->get_value(), workingSet->get_object_tree());
			}

			if ((nullptr == child) ||
			    (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
			{
				continue;
			}
			extent = std::max(extent, object->get_child_x(i) + static_cast<int>(child->get_width()));
			extent = std::max(extent, object->get_child_y(i) + static_cast<int>(child->get_height()));
		}

		if (0 != extent)
		{
			maskExtents.push_back(extent);
		}
	}

	auto retVal = 0;

	if (!maskExtents.empty())
	{
		auto medianPosition = maskExtents.begin() + (maskExtents.size() / 2);
		std::nth_element(maskExtents.begin(), medianPosition, maskExtents.end());
		retVal = *medianPosition;
	}
	return retVal;
}

static int detect_designed_mask_size(int medianMaskExtent, int renderAreaSize)
{
	auto retVal = renderAreaSize;

	if (0 != medianMaskExtent)
	{
		const bool drawnLargerThanRenderArea = (medianMaskExtent > renderAreaSize + (renderAreaSize / MASK_OVERHANG_TOLERANCE_DIVISOR));

		if (drawnLargerThanRenderArea)
		{
			retVal = medianMaskExtent;
		}

		for (auto designedSize : DESIGNED_MASK_SIZES)
		{
			const auto toleratedSize = designedSize + (designedSize / MASK_OVERHANG_TOLERANCE_DIVISOR);

			// A size whose overhang reaches the render area would clip a pool drawn for the render area.
			if ((drawnLargerThanRenderArea || (toleratedSize < renderAreaSize)) && (medianMaskExtent <= toleratedSize))
			{
				retVal = designedSize;
				break;
			}
		}
	}
	return retVal;
}

// A pool can show the same Button more than once
static void show_button_pressed(Component &parent, std::uint16_t buttonID, bool pressed)
{
	for (auto child : parent.getChildren())
	{
		auto button = dynamic_cast<ButtonComponent *>(child);

		if ((nullptr != button) && (buttonID == button->get_id()))
		{
			button->set_pressed(pressed);
		}
		show_button_pressed(*child, buttonID, pressed);
	}
}

DataMaskRenderAreaComponent::DataMaskRenderAreaComponent(ServerMainComponent &parentServer) :
  ownerServer(parentServer)
{
}

int DataMaskRenderAreaComponent::to_pool_coordinate(float renderedCoordinate) const
{
	double poolCoordinate = renderedCoordinate;

	if (0 != designedMaskSize)
	{
		poolCoordinate = poolCoordinate * designedMaskSize / ownerServer.get_data_mask_area_size_x_pixels();
	}
	// Rounding before the whole inverse is done can land an edge pixel in the neighbouring object.
	return static_cast<int>(std::floor(poolCoordinate));
}

void DataMaskRenderAreaComponent::on_change_active_mask(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	const auto previousWorkingSet = parentWorkingSet;
	const auto previousDesignedMaskSize = designedMaskSize;
	const auto previousActiveMaskID = activeMaskID;
	needToRepaintActiveArea = false;
	childComponents.clear();
	parentWorkingSet = workingSet;
	designedMaskSize = 0;
	activeMaskID = isobus::NULL_OBJECT_ID;

	if (parentWorkingSet)
	{
		auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

		if ((nullptr != workingSetObject) && (isobus::NULL_OBJECT_ID != workingSetObject->get_active_mask()))
		{
			activeMaskID = workingSetObject->get_active_mask();
			const int renderAreaSize = ownerServer.get_data_mask_area_size_x_pixels();
			designedMaskSize = renderAreaSize;

			if (ownerServer.get_scale_pools_to_data_mask())
			{
				// Measured once per pool, so commands that move objects cannot flip the scale. Switching working sets measures again.
				if (measuredWorkingSetObject != workingSetObject)
				{
					measuredWorkingSetObject = workingSetObject;
					medianMaskExtent = measure_median_mask_extent(parentWorkingSet);
				}
				designedMaskSize = detect_designed_mask_size(medianMaskExtent, renderAreaSize);
			}

			// Runs on every mask change, so log only a new working set or size.
			if ((renderAreaSize != designedMaskSize) &&
			    ((previousWorkingSet != parentWorkingSet) || (previousDesignedMaskSize != designedMaskSize)))
			{
				LOG_INFO("[VT Server]: Object pool appears drawn for a %d px data mask, scaling it to %d px", designedMaskSize, renderAreaSize);
			}
			auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());
			childComponents.emplace_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, activeMask));

			if (nullptr != childComponents.back())
			{
				addAndMakeVisible(*childComponents.back());

				// The cache builds every mask at 480.
				childComponents.back()->setSize(designedMaskSize, designedMaskSize);
				childComponents.back()->setTransform(AffineTransform::scale(static_cast<float>(renderAreaSize) / designedMaskSize));
			}
		}
	}
	// Every repaint comes through here, so only a new mask or working set ends the edit or the press
	if ((previousWorkingSet != parentWorkingSet) || (previousActiveMaskID != activeMaskID))
	{
		pressedButtonID = isobus::NULL_OBJECT_ID;
		close_input_dialogs();
	}
	// Rebuilt Buttons start unpressed
	show_button_pressed(*this, pressedButtonID, true);
	repaint();
}

void DataMaskRenderAreaComponent::on_working_set_disconnect(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	if ((nullptr != workingSet) && (parentWorkingSet == workingSet))
	{
		childComponents.clear();
		parentWorkingSet.reset();
		close_input_dialogs();
		repaint();
	}
}

std::uint16_t DataMaskRenderAreaComponent::abort_input(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet)
{
	std::uint16_t abortedObjectID = isobus::NULL_OBJECT_ID;
	{
		const std::lock_guard<std::mutex> lock(openInputMutex);
		if ((nullptr != workingSet) && (openInputWorkingSet == workingSet))
		{
			abortedObjectID = openInputObjectID;
			openInputWorkingSet.reset();
			openInputObjectID = isobus::NULL_OBJECT_ID;
		}
	}

	if (isobus::NULL_OBJECT_ID != abortedObjectID)
	{
		ownerServer.send_select_input_object_message(abortedObjectID, false, false, ownerServer.get_client_control_function_for_working_set(workingSet));
	}
	return abortedObjectID;
}

void DataMaskRenderAreaComponent::close_aborted_input_dialogs()
{
	bool reopened;
	{
		const std::lock_guard<std::mutex> lock(openInputMutex);
		reopened = (nullptr != openInputWorkingSet);
	}

	// An input opened after abort_input() is not the one the VT closed
	if (!reopened)
	{
		close_input_dialogs();
	}
}

void DataMaskRenderAreaComponent::close_input_dialogs()
{
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet;
	std::uint16_t objectID;
	{
		const std::lock_guard<std::mutex> lock(openInputMutex);
		workingSet = std::move(openInputWorkingSet);
		objectID = openInputObjectID;
		openInputObjectID = isobus::NULL_OBJECT_ID;
	}

	// The callbacks will see the input as ended, so the close messages go out here
	if (isobus::NULL_OBJECT_ID != objectID)
	{
		const auto destination = ownerServer.get_client_control_function_for_working_set(workingSet);
		ownerServer.send_select_input_object_message(objectID, false, false, destination);
		ownerServer.send_esc_message(objectID, 0, destination);
	}

	// 0 = Cancel, the same result JUCE gives a window close or Escape
	for (auto *dialog : { inputListModal.get(), inputNumberModal.get(), inputStringModal.get() })
	{
		if (nullptr != dialog)
		{
			dialog->exitModalState(0);
		}
	}
}

void DataMaskRenderAreaComponent::begin_open_input(std::uint16_t objectID)
{
	const std::lock_guard<std::mutex> lock(openInputMutex);
	openInputWorkingSet = parentWorkingSet;
	openInputObjectID = objectID;
}

bool DataMaskRenderAreaComponent::end_open_input()
{
	const std::lock_guard<std::mutex> lock(openInputMutex);
	const bool wasOpen = (nullptr != openInputWorkingSet);
	openInputWorkingSet.reset();
	openInputObjectID = isobus::NULL_OBJECT_ID;
	return wasOpen;
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
			auto clickedObject = getClickedChildRecursive(activeMask,
			                                              to_pool_coordinate(relativeEvent.mouseDownPosition.x),
			                                              to_pool_coordinate(relativeEvent.mouseDownPosition.y));

			if (nullptr != clickedObject)
			{
				if (isobus::VirtualTerminalObjectType::Button == clickedObject->get_object_type())
				{
					auto clickedButton = std::static_pointer_cast<isobus::Button>(clickedObject);

					if (false == clickedButton->get_option(isobus::Button::Options::Disabled))
					{
						const bool latchable = clickedButton->get_option(isobus::Button::Options::Latchable);
						// Latched is sent as pressed and unlatched as released, with no repeats
						const bool pressed = (false == latchable) || (false == clickedButton->get_option(isobus::Button::Options::CurrentButtonStateIfLatchable));
						std::uint8_t keyCode = clickedButton->get_key_code();

						if (true == latchable)
						{
							clickedButton->set_option(isobus::Button::Options::CurrentButtonStateIfLatchable, pressed);
							needToRepaintActiveArea = true;
						}
						else
						{
							// Set before the macro, which can change the mask and so clear it
							pressedButtonID = clickedObject->get_id();
						}
						ownerServer.process_macro(clickedObject, pressed ? isobus::EventID::OnKeyPress : isobus::EventID::OnKeyRelease, isobus::VirtualTerminalObjectType::Button, parentWorkingSet);
						ownerServer.send_button_activation_message(pressed ? isobus::VirtualTerminalBase::KeyActivationCode::ButtonPressedOrLatched : isobus::VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased,
						                                           clickedObject->get_id(),
						                                           activeMask->get_id(),
						                                           keyCode,
						                                           ownerServer.get_active_working_set()->get_control_function());

						if (false == latchable)
						{
							ownerServer.set_button_held(ownerServer.get_active_working_set(), clickedObject->get_id(), activeMask->get_id(), keyCode, false);
							show_button_pressed(*this, pressedButtonID, true);
						}
					}
				}
			}
		}
	}
}

void DataMaskRenderAreaComponent::mouseDrag(const MouseEvent &event)
{
	if ((isobus::NULL_OBJECT_ID != pressedButtonID) && (nullptr != parentWorkingSet))
	{
		auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

		if (nullptr != workingSetObject)
		{
			auto relativeEvent = event.getEventRelativeTo(this);
			auto objectUnderPointer = getClickedChildRecursive(parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask()),
			                                                   to_pool_coordinate(relativeEvent.position.x),
			                                                   to_pool_coordinate(relativeEvent.position.y));

			if ((nullptr == objectUnderPointer) || (pressedButtonID != objectUnderPointer->get_id()))
			{
				show_button_pressed(*this, pressedButtonID, false);
				pressedButtonID = isobus::NULL_OBJECT_ID;
				ownerServer.release_held_buttons(true);
			}
		}
	}
}

// Used to calculate button release events
void DataMaskRenderAreaComponent::mouseUp(const MouseEvent &event)
{
	const auto releasedButtonID = pressedButtonID;
	show_button_pressed(*this, pressedButtonID, false);
	pressedButtonID = isobus::NULL_OBJECT_ID;

	if (nullptr != parentWorkingSet)
	{
		// Do a top down search to see if they clicked on some interactable object
		auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(parentWorkingSet->get_working_set_object());

		if (nullptr != workingSetObject)
		{
			auto activeMask = parentWorkingSet->get_object_by_id(workingSetObject->get_active_mask());

			auto relativeEvent = event.getEventRelativeTo(this);
			auto clickedObject = getClickedChildRecursive(activeMask,
			                                              to_pool_coordinate(relativeEvent.mouseDownPosition.x),
			                                              to_pool_coordinate(relativeEvent.mouseDownPosition.y));

			// A result callback runs after its dialog stops being modal, and would end an input opened before it ran
			const bool inputDialogPending = (nullptr != inputListModal) || (nullptr != inputNumberModal) || (nullptr != inputStringModal);

			if (nullptr != clickedObject)
			{
				switch (clickedObject->get_object_type())
				{
					case isobus::VirtualTerminalObjectType::Button:
					{
						// A press cut off by a mask change is released by release_held_buttons() below
						if (releasedButtonID == clickedObject->get_id())
						{
							std::uint8_t keyCode = std::static_pointer_cast<isobus::Button>(clickedObject)->get_key_code();
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

					case isobus::VirtualTerminalObjectType::InputList:
					{
						auto clickedList = std::static_pointer_cast<isobus::InputList>(clickedObject);

						if (!inputDialogPending &&
						    clickedList->get_option(isobus::InputList::Options::Enabled) &&
						    (clickedList->get_number_children() > 0))
						{
							// Need to display a modal combo selection
							inputListModal.reset(new AlertWindow("Input List Selection", "Select a List Item, then press OK.", MessageBoxIconType::QuestionIcon));
							inputListModal->addComboBox("Input List Combo", StringArray());
							currentModalComponentCache.clear();
							currentModalComponentCache.reserve(clickedList->get_number_children());

							// In order to handle things that are not strings being allowed in an input list, grab the popup menu itself and shove custom components in there
							auto combo = inputListModal->getComboBoxComponent("Input List Combo");
							auto comboPopup = combo->getRootMenu();

							std::uint32_t selectedIndex = std::numeric_limits<std::uint32_t>::max();

							if (clickedList->get_variable_reference() != isobus::NULL_OBJECT_ID)
							{
								auto referencedVariable = clickedList->get_object_by_id(clickedList->get_variable_reference(), parentWorkingSet->get_object_tree());

								if (nullptr != referencedVariable)
								{
									if (isobus::VirtualTerminalObjectType::NumberVariable == referencedVariable->get_object_type())
									{
										selectedIndex = std::static_pointer_cast<isobus::NumberVariable>(referencedVariable)->get_value();
									}
								}
							}
							else
							{
								selectedIndex = clickedList->get_value();
							}

							for (std::uint32_t i = 0; i < clickedList->get_number_children(); i++)
							{
								auto child = clickedList->get_object_by_id(clickedList->get_child_id(static_cast<std::uint16_t>(i)), parentWorkingSet->get_object_tree());

								if (nullptr != child)
								{
									currentModalComponentCache.push_back(JuceManagedWorkingSetCache::create_component(parentWorkingSet, child));

									if (nullptr != currentModalComponentCache.back())
									{
										auto text = "Object " + std::to_string(clickedList->get_child_id(static_cast<std::uint16_t>(i)));
										if (child->get_object_type() == isobus::VirtualTerminalObjectType::OutputString)
										{
											text = std::static_pointer_cast<isobus::OutputString>(child)->displayed_value(parentWorkingSet->get_object_tree());
										}

										comboPopup->addCustomItem(i + 1, *currentModalComponentCache.back().get(), currentModalComponentCache.back()->getWidth(), currentModalComponentCache.back()->getHeight(), true, nullptr, text);
									}
								}
							}

							// Item IDs are the list index + 1, so NULL children, which have no item, cannot shift the selection
							if (selectedIndex < clickedList->get_number_children())
							{
								combo->setSelectedId(static_cast<int>(selectedIndex) + 1, dontSendNotification);
							}

							inputListModal->addButton("OK", 1, KeyPress(KeyPress::returnKey, 0, 0));
							inputListModal->addButton("Cancel", 0);

							// Only a chosen index (0-254) may be sent, so OK waits for a selection
							auto okButton = inputListModal->getButton("OK");
							okButton->setEnabled(0 != combo->getSelectedId());
							combo->onChange = [okButton, combo]() { okButton->setEnabled(0 != combo->getSelectedId()); };
							auto resultCallback = [this, clickedList, workingSet = parentWorkingSet](int result) {
								const bool closedByVT = !end_open_input();
								const int selectedId = inputListModal->getComboBoxComponent("Input List Combo")->getSelectedId();
								std::shared_ptr<isobus::NumberVariable> variable;

								if (isobus::NULL_OBJECT_ID != clickedList->get_variable_reference())
								{
									auto child = clickedList->get_object_by_id(clickedList->get_variable_reference(), workingSet->get_object_tree());

									if ((nullptr != child) && (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type()))
									{
										variable = std::static_pointer_cast<isobus::NumberVariable>(child);
									}
								}

								if ((1 == result) && !closedByVT && (0 != selectedId) && ((isobus::NULL_OBJECT_ID == clickedList->get_variable_reference()) || (nullptr != variable)))
								{
									const std::uint32_t oldValue = (nullptr != variable) ? variable->get_value() : clickedList->get_value();
									const std::uint32_t newValue = static_cast<std::uint32_t>(selectedId - 1);

									ownerServer.process_macro(clickedList, isobus::EventID::OnEntryOfAValue, isobus::VirtualTerminalObjectType::InputList, workingSet);
									if (oldValue != newValue)
									{
										ownerServer.process_macro(clickedList, isobus::EventID::OnEntryOfANewValue, isobus::VirtualTerminalObjectType::InputList, workingSet);
									}

									if (nullptr != variable)
									{
										variable->set_value(newValue);
										ownerServer.send_change_numeric_value_message(variable->get_id(), newValue, ownerServer.get_client_control_function_for_working_set(workingSet));
										if (oldValue != newValue)
										{
											ownerServer.process_macro(variable, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::NumberVariable, workingSet);
										}
									}
									else
									{
										clickedList->set_value(static_cast<std::uint8_t>(newValue));
										ownerServer.send_change_numeric_value_message(clickedList->get_id(), newValue, ownerServer.get_client_control_function_for_working_set(workingSet));
										if (oldValue != newValue)
										{
											ownerServer.process_macro(clickedList, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::InputList, workingSet);
										}
									}
								}
								if (!closedByVT)
								{
									ownerServer.send_select_input_object_message(clickedList->get_id(), false, false, ownerServer.get_client_control_function_for_working_set(workingSet));
									if (0 == result)
									{
										ownerServer.send_esc_message(clickedList->get_id(), 0, ownerServer.get_client_control_function_for_working_set(workingSet));
									}
								}
								workingSet->set_object_focus(isobus::NULL_OBJECT_ID);
								ownerServer.repaint_on_next_update();
								ownerServer.process_macro(clickedList, isobus::EventID::OnInputFieldDeselection, isobus::VirtualTerminalObjectType::InputList, workingSet);
								inputListModal.reset();
								repaint();
							};
							inputListModal->enterModalState(true, ModalCallbackFunction::create(std::move(resultCallback)), false);
							begin_open_input(clickedList->get_id());
							ownerServer.send_select_input_object_message(clickedList->get_id(), true, true, ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
							if (nullptr != parentWorkingSet)
							{
								parentWorkingSet->set_object_focus(clickedObject->get_id());
							}
							ownerServer.process_macro(clickedObject, isobus::EventID::OnInputFieldSelection, isobus::VirtualTerminalObjectType::InputList, parentWorkingSet);
						}
					}
					break;

					case isobus::VirtualTerminalObjectType::InputNumber:
					{
						auto clickedNumber = std::static_pointer_cast<isobus::InputNumber>(clickedObject);

						if (!inputDialogPending && clickedNumber->get_option2(isobus::InputNumber::Options2::Enabled))
						{
							inputNumberModal.reset(new AlertWindow("Input Number", "", MessageBoxIconType::NoIcon));

							std::uint32_t currentRawValue = clickedNumber->get_value();

							if (isobus::NULL_OBJECT_ID != clickedNumber->get_variable_reference())
							{
								auto child = clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), parentWorkingSet->get_object_tree());

								if (nullptr != child)
								{
									if (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type())
									{
										currentRawValue = std::static_pointer_cast<isobus::NumberVariable>(child)->get_value();
									}
								}
							}

							// A keypad rather than a slider, because an exact value cannot be hit by
							// dragging a finger, especially over the ranges an input number can have
							inputNumberKeypad.reset(new NumericKeypadComponent((static_cast<double>(currentRawValue) + clickedNumber->get_offset()) * clickedNumber->get_scale(),
							                                                   (static_cast<double>(clickedNumber->get_minimum_value()) + clickedNumber->get_offset()) * clickedNumber->get_scale(),
							                                                   (static_cast<double>(clickedNumber->get_maximum_value()) + clickedNumber->get_offset()) * clickedNumber->get_scale(),
							                                                   clickedNumber->get_number_of_decimals(),
							                                                   (isobus::LanguageCommandInterface::DecimalSymbols::Comma == ownerServer.get_language_command_interface().get_commanded_decimal_symbol()) ? ',' : '.'));

							inputNumberListener.set_target(clickedNumber);

							// Seeded with what the object holds now, so confirming without typing
							// anything cannot write a stale value from a previous edit
							inputNumberListener.set_last_value(currentRawValue);
							inputNumberModal->addCustomComponent(inputNumberKeypad.get());
							inputNumberKeypad->onConfirm = [this]() { inputNumberModal->exitModalState(1); };
							inputNumberKeypad->onCancel = [this]() { inputNumberModal->exitModalState(0); };
							auto resultCallback = [this, clickedNumber, workingSet = parentWorkingSet](int result) {
								const bool closedByVT = !end_open_input();

								std::uint16_t varNumID = 0xFFFF;
								if ((1 == result) && !closedByVT)
								{
									// Back from what the operator sees to what the object stores.
									// Rounded rather than truncated, and clamped, so the value can
									// never land outside what the object said it accepts. Skipped if nothing
									// was typed, or the keypad's rounding to the object's decimals is written back.
									if ((nullptr != inputNumberKeypad) && inputNumberKeypad->is_edited() && (0 != clickedNumber->get_scale()))
									{
										const double rawValue = (inputNumberKeypad->get_value() / clickedNumber->get_scale()) - clickedNumber->get_offset();
										const double clamped = std::min(std::max(rawValue, static_cast<double>(clickedNumber->get_minimum_value())),
										                                static_cast<double>(clickedNumber->get_maximum_value()));
										inputNumberListener.set_last_value(static_cast<std::uint32_t>(std::llround(clamped)));
									}

									ownerServer.process_macro(clickedNumber, isobus::EventID::OnEntryOfAValue, isobus::VirtualTerminalObjectType::InputNumber, workingSet);

									if (isobus::NULL_OBJECT_ID != clickedNumber->get_variable_reference())
									{
										auto child = clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), workingSet->get_object_tree());

										if ((nullptr != child) && (isobus::VirtualTerminalObjectType::NumberVariable == child->get_object_type()))
										{
											varNumID = child->get_id();
										}
									}

									if (isobus::NULL_OBJECT_ID != varNumID)
									{
										if (std::static_pointer_cast<isobus::NumberVariable>(clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), workingSet->get_object_tree()))->get_value() != inputNumberListener.get_last_value())
										{
											ownerServer.process_macro(clickedNumber, isobus::EventID::OnEntryOfANewValue, isobus::VirtualTerminalObjectType::InputNumber, workingSet);
										}
										std::static_pointer_cast<isobus::NumberVariable>(clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), workingSet->get_object_tree()))->set_value(inputNumberListener.get_last_value());
									}
									else
									{
										if (clickedNumber->get_value() != inputNumberListener.get_last_value())
										{
											ownerServer.process_macro(clickedNumber, isobus::EventID::OnEntryOfANewValue, isobus::VirtualTerminalObjectType::InputNumber, workingSet);
											clickedNumber->set_value(inputNumberListener.get_last_value());
											ownerServer.process_macro(clickedNumber, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::InputNumber, workingSet);
										}
									}

									this->needToRepaintActiveArea = true;
								}
								inputNumberListener.set_target(nullptr);
								inputNumberModal.reset();
								inputNumberKeypad.reset();
								// On Linux a dialog closed by abort_input() stays drawn until something repaints
								repaint();
								if (!closedByVT)
								{
									ownerServer.send_select_input_object_message(clickedNumber->get_id(), false, false, ownerServer.get_client_control_function_for_working_set(workingSet));
								}

								if ((1 == result) && !closedByVT)
								{
									if (0xFFFF != varNumID)
									{
										ownerServer.send_change_numeric_value_message(varNumID, std::static_pointer_cast<isobus::NumberVariable>(clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), workingSet->get_object_tree()))->get_value(), ownerServer.get_client_control_function_for_working_set(workingSet));
										ownerServer.process_macro(clickedNumber->get_object_by_id(clickedNumber->get_variable_reference(), workingSet->get_object_tree()), isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::NumberVariable, workingSet);
									}
									else
									{
										ownerServer.send_change_numeric_value_message(clickedNumber->get_id(), clickedNumber->get_value(), ownerServer.get_client_control_function_for_working_set(workingSet));
										ownerServer.process_macro(clickedNumber, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::InputNumber, workingSet);
									}
								}
								else if (!closedByVT)
								{
									ownerServer.send_esc_message(clickedNumber->get_id(), 0, ownerServer.get_client_control_function_for_working_set(workingSet));
								}
								workingSet->set_object_focus(isobus::NULL_OBJECT_ID);
								ownerServer.process_macro(clickedNumber, isobus::EventID::OnInputFieldDeselection, isobus::VirtualTerminalObjectType::InputNumber, workingSet);
							};
							inputNumberModal->enterModalState(true, ModalCallbackFunction::create(std::move(resultCallback)), false);
							begin_open_input(clickedNumber->get_id());
							inputNumberKeypad->grabKeyboardFocus();
							ownerServer.send_select_input_object_message(clickedNumber->get_id(), true, true, ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
							if (parentWorkingSet)
							{
								parentWorkingSet->set_object_focus(clickedObject->get_id());
							}
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
										auto numVar = std::static_pointer_cast<isobus::NumberVariable>(child);
										hasNumberVariable = true;
										numVar->set_value(numVar->get_value() != 0 ? 0 : 1);
										ownerServer.send_change_numeric_value_message(child->get_id(), numVar->get_value(), ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
										ownerServer.process_macro(child, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::NumberVariable, parentWorkingSet);
									}
								}
							}

							if (!hasNumberVariable)
							{
								clickedBool->set_value(clickedBool->get_value() != 0 ? 0 : 1);
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

						if (!inputDialogPending && clickedString->get_enabled())
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
							inputStringModal->addButton("OK", 1);
							inputStringModal->addButton("Cancel", 0);
							auto resultCallback = [this, clickedString, stringVariable, workingSet = parentWorkingSet](int result) {
								const bool closedByVT = !end_open_input();

								if (!closedByVT)
								{
									ownerServer.send_select_input_object_message(clickedString->get_id(), false, false, ownerServer.get_client_control_function_for_working_set(workingSet));
									if (0 == result)
									{
										ownerServer.send_esc_message(clickedString->get_id(), 0, ownerServer.get_client_control_function_for_working_set(workingSet));
									}
								}

								if ((1 == result) && !closedByVT)
								{
									String newContent = this->inputStringModal->getTextEditor("Input String")->getText();
									ownerServer.process_macro(clickedString, isobus::EventID::OnEntryOfAValue, isobus::VirtualTerminalObjectType::InputString, workingSet);

									if (nullptr != stringVariable)
									{
										// It seems common practice to pad the string value out to the length in the object pool.
										while (newContent.length() < stringVariable->get_value().length())
										{
											newContent.append(" ", 1);
										}
										stringVariable->set_value(newContent.toStdString());
										ownerServer.send_change_string_value_message(stringVariable->get_id(), newContent.toStdString(), ownerServer.get_client_control_function_for_working_set(workingSet));
										ownerServer.process_macro(stringVariable, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::StringVariable, workingSet);
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
											ownerServer.process_macro(clickedString, isobus::EventID::OnEntryOfANewValue, isobus::VirtualTerminalObjectType::InputString, workingSet);
										}
										clickedString->set_value(newContent.toStdString());
										ownerServer.send_change_string_value_message(clickedString->get_id(), newContent.toStdString(), ownerServer.get_client_control_function_for_working_set(workingSet));
										ownerServer.process_macro(clickedString, isobus::EventID::OnChangeValue, isobus::VirtualTerminalObjectType::InputString, workingSet);
									}
									needToRepaintActiveArea = true;
								}
								inputStringModal.reset();
								repaint();

								// Make sure no text caret survives the dialog, otherwise Windows
								// keeps re-invoking the touch keyboard on every later tap
								ServerMainComponent::release_text_input_focus();
								workingSet->set_object_focus(isobus::NULL_OBJECT_ID);
								ownerServer.process_macro(clickedString, isobus::EventID::OnInputFieldDeselection, isobus::VirtualTerminalObjectType::InputString, workingSet);
							};
							inputStringModal->enterModalState(true, ModalCallbackFunction::create(std::move(resultCallback)), false);
							begin_open_input(clickedString->get_id());
							ownerServer.send_select_input_object_message(clickedString->get_id(), true, true, ownerServer.get_client_control_function_for_working_set(parentWorkingSet));
							if (parentWorkingSet)
							{
								parentWorkingSet->set_object_focus(clickedObject->get_id());
							}
							ownerServer.process_macro(clickedObject, isobus::EventID::OnInputFieldSelection, isobus::VirtualTerminalObjectType::InputString, parentWorkingSet);
						}
					}
					break;

					default:
						break;
				}
			}
			ownerServer.release_held_buttons();
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
	    (nullptr == parentWorkingSet) ||
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
		// none of the childs of the hidden containers should send clicked events
		if (isobus::VirtualTerminalObjectType::Container == object->get_object_type() &&
		    std::static_pointer_cast<const isobus::Container>(object)->get_hidden())
		{
			return nullptr;
		}

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
	return ((clickXRelative >= objectX) && (clickXRelative < (objectX + objectWidth))) &&
	  ((clickYRelative >= objectY) && (clickYRelative < (objectY + objectHeight)));
}
