/*******************************************************************************
** @file       AuxiliaryAssignmentComponent.cpp
** @author     Gunics Balázs
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "AuxiliaryAssignmentComponent.hpp"

#include "DesignatorFit.hpp"
#include "JuceManagedWorkingSetCache.hpp"
#include "KeyComponent.hpp"
#include "ManufacturerMap.hpp"

#include <algorithm>
#include <cstdlib>

namespace
{
	// Colour indices of the VT's standard palette
	constexpr std::uint8_t BLACK = 0;
	constexpr std::uint8_t WHITE = 1;
	constexpr std::uint8_t GREEN = 2;
	constexpr std::uint8_t SILVER = 7;
	constexpr std::uint8_t GREY = 8;
	constexpr std::uint8_t BLUE = 9;
	constexpr std::uint8_t RED = 12;
	constexpr std::uint8_t YELLOW = 14;
	constexpr std::uint8_t NAVY = 15;

	constexpr float TITLE_FONT_HEIGHT = 18.0f;
	constexpr float TEXT_FONT_HEIGHT = 14.0f;
	constexpr float SMALL_FONT_HEIGHT = 11.0f;
	constexpr float NUMBER_FONT_HEIGHT = 30.0f;
	constexpr int ARROW_WIDTH = 16;
	constexpr int MINIMUM_CELL_SIZE = 24;

	using Snapshot = AuxiliaryControlSnapshot;

	bool is_parsed(const Snapshot::WorkingSetPointer &workingSet)
	{
		return (nullptr != workingSet) && (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == workingSet->get_object_pool_processing_state());
	}

	bool is_auxiliary_object(const std::pair<const std::uint16_t, std::shared_ptr<isobus::VTObject>> &object)
	{
		return (nullptr != object.second) &&
		  ((isobus::VirtualTerminalObjectType::AuxiliaryFunctionType2 == object.second->get_object_type()) ||
		   (isobus::VirtualTerminalObjectType::AuxiliaryInputType2 == object.second->get_object_type()));
	}

	// Analogue values run up to 0xFAFF (ISO 11783-6 J.7.9)
	constexpr std::uint16_t ANALOGUE_MAXIMUM = 0xFAFF;
	constexpr int LEARN_MOVEMENT = ANALOGUE_MAXIMUM / 10; ///< A lever moved this far is pressed
	constexpr int LEARN_STILL = ANALOGUE_MAXIMUM / 50; ///< A pressed lever moving less than this has stopped

	using FunctionType = isobus::AuxiliaryFunctionType2::FunctionType;

	// A reserved type if the object is not an Auxiliary Input Type 2
	FunctionType get_input_function_type(const Snapshot::WorkingSetPointer &workingSet, std::uint16_t inputObjectID)
	{
		auto object = is_parsed(workingSet) ? workingSet->get_object_by_id(inputObjectID) : nullptr;
		auto retVal = FunctionType::ReservedRangeStart;

		if ((nullptr != object) && (isobus::VirtualTerminalObjectType::AuxiliaryInputType2 == object->get_object_type()))
		{
			retVal = std::static_pointer_cast<isobus::AuxiliaryInputType2>(object)->get_function_type();
		}
		return retVal;
	}

	bool has_moved(std::uint16_t from, std::uint16_t to, int distance)
	{
		return (ANALOGUE_MAXIMUM >= from) && (ANALOGUE_MAXIMUM >= to) && (distance <= std::abs(static_cast<int>(to) - static_cast<int>(from)));
	}

	bool uses_input(const Snapshot::Function &function, const Snapshot::WorkingSetPointer &inputWorkingSet, std::uint16_t inputObjectID)
	{
		return (nullptr != inputWorkingSet) && (inputWorkingSet == function.inputWorkingSet) && (inputObjectID == function.inputObjectID);
	}

	bool is_assigned_function_of(const Snapshot::Function &function, const Snapshot::WorkingSetPointer &workingSet)
	{
		return (workingSet == function.workingSet) && (Snapshot::State::Assigned == function.state);
	}

	Snapshot::State get_assignment_state(const isobus::VirtualTerminalServer::AuxiliaryAssignment &assignment)
	{
		auto state = Snapshot::State::NotAssigned;

		if (assignment.hasFailed)
		{
			state = Snapshot::State::Failed;
		}
		else if (nullptr != assignment.inputWorkingSet)
		{
			state = assignment.isAcknowledged ? Snapshot::State::Assigned : Snapshot::State::Assigning;
		}
		return state;
	}

	Snapshot::Function make_function(const Snapshot::WorkingSetPointer &workingSet,
	                                 std::uint16_t objectID,
	                                 const std::vector<isobus::VirtualTerminalServer::AuxiliaryAssignment> &assignments)
	{
		Snapshot::Function function;
		function.workingSet = workingSet;
		function.objectID = objectID;
		function.state = workingSet->get_was_preferred_assignment_received() ? Snapshot::State::NotAssigned : Snapshot::State::WaitingForImplement;

		for (const auto &assignment : assignments)
		{
			if ((workingSet == assignment.functionWorkingSet) && (objectID == assignment.functionObjectID))
			{
				function.inputWorkingSet = assignment.inputWorkingSet;
				function.inputObjectID = assignment.inputObjectID;
				function.state = get_assignment_state(assignment);
			}
		}
		return function;
	}

	int count_functions_using(const Snapshot &snapshot, const Snapshot::WorkingSetPointer &inputWorkingSet, std::uint16_t inputObjectID)
	{
		return static_cast<int>(std::count_if(snapshot.functions.begin(), snapshot.functions.end(), [&inputWorkingSet, inputObjectID](const Snapshot::Function &function) { return uses_input(function, inputWorkingSet, inputObjectID); }));
	}

	void draw_text(juce::Graphics &g, const juce::String &text, juce::Rectangle<int> area, const juce::Font &font, juce::Colour colour)
	{
		g.setColour(colour);
		g.setFont(font);
		g.drawFittedText(text, area, juce::Justification::centredLeft, 1, 0.7f);
	}

	void draw_triangle(juce::Graphics &g, juce::Rectangle<float> area, float rotation)
	{
		juce::Path triangle;
		triangle.addTriangle(area.getX(), area.getY(), area.getRight(), area.getCentreY(), area.getX(), area.getBottom());
		g.fillPath(triangle, juce::AffineTransform::rotation(rotation, area.getCentreX(), area.getCentreY()));
	}
} // namespace

bool AuxiliaryControlSnapshot::Function::operator==(const Function &other) const
{
	return (workingSet == other.workingSet) &&
	  (inputWorkingSet == other.inputWorkingSet) &&
	  (objectID == other.objectID) &&
	  (inputObjectID == other.inputObjectID) &&
	  (state == other.state);
}

bool AuxiliaryControlSnapshot::Input::operator==(const Input &other) const
{
	return (workingSet == other.workingSet) && (objectID == other.objectID);
}

bool AuxiliaryControlSnapshot::Device::operator==(const Device &other) const
{
	return (workingSet == other.workingSet) &&
	  (functionCount == other.functionCount) &&
	  (inputCount == other.inputCount) &&
	  (isInputUnitReady == other.isInputUnitReady) &&
	  (hasPreferredAssignment == other.hasPreferredAssignment);
}

AuxiliaryControlSnapshot AuxiliaryControlSnapshot::build(const std::vector<WorkingSetPointer> &workingSets,
                                                         const std::vector<isobus::VirtualTerminalServer::AuxiliaryAssignment> &assignments,
                                                         bool isPrimaryVT)
{
	AuxiliaryControlSnapshot snapshot;
	snapshot.isPrimaryVT = isPrimaryVT;

	for (const auto &workingSet : workingSets)
	{
		if (is_parsed(workingSet))
		{
			Device device;
			device.workingSet = workingSet;
			device.isInputUnitReady = workingSet->get_is_auxiliary_input_ready();
			device.hasPreferredAssignment = workingSet->get_was_preferred_assignment_received();

			for (const auto &object : workingSet->get_object_tree())
			{
				if (nullptr == object.second)
				{
				}
				else if (isobus::VirtualTerminalObjectType::AuxiliaryFunctionType2 == object.second->get_object_type())
				{
					snapshot.functions.push_back(make_function(workingSet, object.first, assignments));
					device.functionCount++;
				}
				else if (isobus::VirtualTerminalObjectType::AuxiliaryInputType2 == object.second->get_object_type())
				{
					snapshot.inputs.push_back({ workingSet, object.first });
					device.inputCount++;
				}
			}

			if ((0 < device.functionCount) || (0 < device.inputCount))
			{
				snapshot.devices.push_back(device);
			}
		}
	}
	return snapshot;
}

bool AuxiliaryControlSnapshot::has_auxiliary_objects(const WorkingSetPointer &workingSet)
{
	return is_parsed(workingSet) && std::any_of(workingSet->get_object_tree().begin(), workingSet->get_object_tree().end(), is_auxiliary_object);
}

bool AuxiliaryControlSnapshot::operator==(const AuxiliaryControlSnapshot &other) const
{
	return (devices == other.devices) && (functions == other.functions) && (inputs == other.inputs) && (isPrimaryVT == other.isPrimaryVT);
}

bool AuxiliaryControlSnapshot::operator!=(const AuxiliaryControlSnapshot &other) const
{
	return !(*this == other);
}

bool AuxiliaryAssignmentComponent::Selection::is_set() const
{
	return nullptr != workingSet;
}

bool AuxiliaryAssignmentComponent::Selection::operator==(const Selection &other) const
{
	return (workingSet == other.workingSet) && (objectID == other.objectID);
}

AuxiliaryAssignmentComponent::AuxiliaryAssignmentComponent(isobus::VirtualTerminalServer &server) :
  vtServer(server)
{
	setOpaque(true);
}

void AuxiliaryAssignmentComponent::set_shown(bool shown)
{
	isShown = shown;
	view = View::Overview;
	page = 0;
	selectedFunction = {};
	selectedInput = {};
	learnedInputs.clear();
	isLearnModeSwitchedOn = true;

	// A problem reported while hidden waits for the operator
	if ((!shown) || (!isMessageAnError))
	{
		message.clear();
	}
	rebuild();
	update_learn_mode();
}

void AuxiliaryAssignmentComponent::set_snapshot(const AuxiliaryControlSnapshot &newSnapshot)
{
	if (newSnapshot != snapshot)
	{
		snapshot = newSnapshot;

		// A selection whose working set left is dropped
		if ((nullptr == get_selected_function()) && selectedFunction.is_set())
		{
			selectedFunction = {};
		}
		if (selectedInput.is_set() &&
		    std::none_of(snapshot.inputs.begin(), snapshot.inputs.end(), [this](const Snapshot::Input &input) { return Selection{ input.workingSet, input.objectID } == selectedInput; }))
		{
			selectedInput = {};
		}
		if ((View::Inputs == view) && (!selectedFunction.is_set()))
		{
			view = View::Assignments;
		}
		rebuild();
		update_learn_mode();
	}
}

void AuxiliaryAssignmentComponent::on_assignment_event(const isobus::VirtualTerminalServer::AuxiliaryAssignment &assignment, isobus::VirtualTerminalServer::AuxiliaryAssignmentEvent event)
{
	using Event = isobus::VirtualTerminalServer::AuxiliaryAssignmentEvent;

	switch (event)
	{
		case Event::Assigned:
		{
			if (assignment.isMadeByOperator)
			{
				show_message("Assigned", false);
			}
		}
		break;

		case Event::Unassigned:
		{
			if (assignment.isMadeByOperator)
			{
				show_message("Assignment removed", false);
			}
		}
		break;

		case Event::Removed:
		{
			show_message("An assignment was removed: it conflicted, or its input is gone", true);
		}
		break;

		case Event::Rejected:
		{
			show_message("The implement rejected the assignment", true);
		}
		break;

		case Event::NotAvailable:
		default:
		{
			show_message("The implement or the input unit did not answer", true);
		}
		break;
	}
}

void AuxiliaryAssignmentComponent::on_input_status(const isobus::VirtualTerminalServer::AuxiliaryInputStatus &status)
{
	auto &learned = learnedInputs[std::make_pair(status.inputWorkingSet.get(), status.inputObjectID)];
	const bool isPress = learned.update(status, get_input_function_type(status.inputWorkingSet, status.inputObjectID));

	// Units repeat the status while a control is held, only the press picks it
	if (isPress && get_is_learn_mode_wanted())
	{
		const Selection input{ status.inputWorkingSet, status.inputObjectID };

		if (selectedFunction.is_set())
		{
			assign(selectedFunction, input);
		}
		else
		{
			select_input(input);
		}
	}
}

bool AuxiliaryAssignmentComponent::LearnedInput::update(const isobus::VirtualTerminalServer::AuxiliaryInputStatus &status, isobus::AuxiliaryFunctionType2::FunctionType type)
{
	// Units also flag releases and ADC noise as activation, so the values tell a press (ISO 11783-6 Table J.5)
	bool isPress = false;

	switch (type)
	{
		// Value 2 counts the presses
		case FunctionType::BooleanLatchingOnOff:
		case FunctionType::BooleanNonLatchingIncreaseValue:
		case FunctionType::DualBooleanBothLatching:
		case FunctionType::DualBooleanBothNonLatching:
		case FunctionType::DualBooleanLatchingUp:
		case FunctionType::DualBooleanLatchingDown:
		case FunctionType::QuadratureBooleanNonLatching:
		{
			isPress = status.isActivated && hasValues && (lastValue2 != status.value2);
		}
		break;

		// Value 1 is the count
		case FunctionType::BidirectionalEncoder:
		{
			isPress = status.isActivated && hasValues && (lastValue1 != status.value1);
		}
		break;

		case FunctionType::Analouge:
		case FunctionType::AnalougeReturnTo50Percent:
		case FunctionType::AnalougeReturnTo0PercentIncreaseValue:
		case FunctionType::QuadratureAnalouge:
		case FunctionType::QuadratureAnalougeReturnTo50Percent:
		{
			isPress = update_lever(status, (FunctionType::QuadratureAnalouge == type) || (FunctionType::QuadratureAnalougeReturnTo50Percent == type));
		}
		break;

		// A lever that latches at its ends, value 2 counts the latches
		case FunctionType::CombinedAnalougeReturnTo50PercentWithDualBooleanLatching:
		case FunctionType::CombinedAnalougeMaintainsPositionWithDualBooleanLatching:
		{
			const bool isMoved = update_lever(status, false);
			isPress = isMoved || (status.isActivated && hasValues && (lastValue2 != status.value2));
		}
		break;

		default:
		{
			isPress = status.isActivated && (!wasPressed);
			wasPressed = status.isActivated;
		}
		break;
	}
	lastValue1 = status.value1;
	lastValue2 = status.value2;
	hasValues = true;
	return isPress;
}

bool AuxiliaryAssignmentComponent::LearnedInput::update_lever(const isobus::VirtualTerminalServer::AuxiliaryInputStatus &status, bool hasSecondAxis)
{
	// A lever has to move to count, and once it counted it has to stop
	const int distance = wasPressed ? LEARN_STILL : LEARN_MOVEMENT;
	const bool isMoved = has_moved(restValue1, status.value1, distance) || (hasSecondAxis && has_moved(restValue2, status.value2, distance));
	const bool isPressed = status.isActivated && hasValues && isMoved;

	if ((!hasValues) || (!status.isActivated) || wasPressed || isPressed)
	{
		restValue1 = status.value1;
		restValue2 = status.value2;
	}
	const bool isPress = isPressed && (!wasPressed);
	wasPressed = isPressed;
	return isPress;
}

void AuxiliaryAssignmentComponent::paint(juce::Graphics &g)
{
	g.fillAll(get_vt_colour(WHITE));
	paint_header(g);

	if (View::Overview == view)
	{
		paint_statistics(g);
	}
	else if (View::Inputs == view)
	{
		// The function the inputs are listed for stays above them
		paint_row(g, -1);
	}

	for (int row = 0; row < get_rows_per_page(); row++)
	{
		paint_row(g, row);
	}

	for (const auto &background : designatorBackgrounds)
	{
		g.setColour(background.second);
		g.fillRect(background.first);
	}
	paint_footer(g);
	paint_soft_keys(g);
}

void AuxiliaryAssignmentComponent::paintOverChildren(juce::Graphics &g)
{
	for (int row = 0; row < get_rows_per_page(); row++)
	{
		if (get_is_row_dimmed(row))
		{
			g.setColour(get_vt_colour(WHITE).withAlpha(0.6f));
			g.fillRect(get_row_bounds(row));
		}
	}

	if (0 <= pressedSoftKeySlot)
	{
		g.setColour(get_vt_colour(GREY).contrasting());
		g.drawRect(get_soft_key_bounds(pressedSoftKeySlot).expanded(PRESSED_RING_WIDTH), PRESSED_RING_WIDTH);
	}
}

void AuxiliaryAssignmentComponent::resized()
{
	rebuild();
}

void AuxiliaryAssignmentComponent::mouseDown(const juce::MouseEvent &event)
{
	const int slot = get_soft_key_slot_at(event.getPosition());
	const auto keys = get_soft_keys();

	if ((0 <= slot) && (slot < static_cast<int>(keys.size())) && keys.at(static_cast<std::size_t>(slot)).isEnabled)
	{
		pressedSoftKeySlot = slot;
		repaint();
	}
}

void AuxiliaryAssignmentComponent::mouseUp(const juce::MouseEvent &event)
{
	if (0 <= pressedSoftKeySlot)
	{
		const auto keys = get_soft_keys();
		const int slot = pressedSoftKeySlot;
		pressedSoftKeySlot = -1;
		repaint();

		// Sliding off the key aborts the press
		if ((slot == get_soft_key_slot_at(event.getPosition())) && (slot < static_cast<int>(keys.size())))
		{
			press_soft_key(keys.at(static_cast<std::size_t>(slot)).action);
		}
	}
	else
	{
		const int row = get_row_at(event.getMouseDownPosition());

		if ((0 <= row) && (row == get_row_at(event.getPosition())))
		{
			press_row(row);
		}
	}
}

void AuxiliaryAssignmentComponent::change_view(View newView)
{
	if (View::Overview == newView)
	{
		selectedFunction = {};
		selectedInput = {};
	}
	view = newView;
	page = 0;
	message.clear();
	rebuild();
	update_learn_mode();
}

void AuxiliaryAssignmentComponent::rebuild()
{
	designators.clear();
	designatorBackgrounds.clear();
	page = juce::jlimit(0, get_page_count() - 1, page);

	if (View::Inputs == view)
	{
		add_row_designators(-1);
	}

	for (int row = 0; row < get_rows_per_page(); row++)
	{
		add_row_designators(row);
	}

	if ((View::Assignments == view) && selectedInput.is_set())
	{
		const auto footer = get_footer_row();
		add_working_set_designator(selectedInput.workingSet, get_cell(footer, 0));
		add_auxiliary_designator(selectedInput.workingSet, selectedInput.objectID, get_cell(footer, 1));
	}
	repaint();
}

void AuxiliaryAssignmentComponent::add_working_set_designator(const WorkingSetPointer &workingSet, juce::Rectangle<int> cell)
{
	auto workingSetObject = (nullptr != workingSet) ? workingSet->get_working_set_object() : nullptr;

	if (nullptr != workingSetObject)
	{
		auto component = JuceManagedWorkingSetCache::create_component(workingSet, workingSetObject);
		const auto vtColour = workingSet->get_colour(workingSetObject->get_background_color());
		designatorBackgrounds.emplace_back(cell, juce::Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));

		if (nullptr != component)
		{
			component->setTopLeftPosition(cell.getPosition());
			fit_designator_to_button(*component, cell, juce::RectanglePlacement::centred);

			// Artwork that could not be measured keeps the soft key size, which would overlap the next cell
			if (component->getTransform().isIdentity() && (!cell.contains(component->getBounds())))
			{
				component->setTransform(juce::RectanglePlacement(juce::RectanglePlacement::centred).getTransformToFit(component->getBounds().toFloat(), cell.toFloat()));
			}
			component->setInterceptsMouseClicks(false, false);
			addAndMakeVisible(*component);
			designators.push_back(component);
		}
	}
}

void AuxiliaryAssignmentComponent::add_auxiliary_designator(const WorkingSetPointer &workingSet, std::uint16_t objectID, juce::Rectangle<int> cell)
{
	auto object = (nullptr != workingSet) ? workingSet->get_object_by_id(objectID) : nullptr;

	if (nullptr != object)
	{
		// The designator fits a soft key (ISO 11783-6 J.4.3), so the VT's key draws it
		isobus::Key key;
		key.set_id(object->get_id());
		key.set_background_color(object->get_background_color());
		for (std::uint16_t i = 0; i < object->get_number_children(); i++)
		{
			key.add_child(object->get_child_id(i), object->get_child_x(i), object->get_child_y(i));
		}

		const auto &dimensions = JuceManagedWorkingSetCache::get_softkey_mask_dimension_info();
		auto component = std::make_shared<KeyComponent>(workingSet, key, dimensions.keyWidth, dimensions.keyHeight);
		component->setTransform(juce::RectanglePlacement(juce::RectanglePlacement::centred).getTransformToFit(component->getBounds().toFloat(), cell.toFloat()));
		component->setInterceptsMouseClicks(false, false);
		addAndMakeVisible(*component);
		designators.push_back(component);
	}
}

void AuxiliaryAssignmentComponent::add_row_designators(int row)
{
	const auto bounds = get_row_bounds(row);

	if (View::Overview == view)
	{
		const int index = page * get_rows_per_page() + row;

		if ((0 <= row) && (index < static_cast<int>(snapshot.devices.size())))
		{
			add_working_set_designator(snapshot.devices.at(static_cast<std::size_t>(index)).workingSet, get_cell(bounds, 0));
		}
	}
	else if (const auto *input = get_input_on_row(row))
	{
		add_working_set_designator(input->workingSet, get_cell(bounds, 0));
		add_auxiliary_designator(input->workingSet, input->objectID, get_cell(bounds, 1));
	}
	else if (const auto *function = get_function_on_row(row))
	{
		add_working_set_designator(function->workingSet, get_cell(bounds, 0));
		add_auxiliary_designator(function->workingSet, function->objectID, get_cell(bounds, 1));

		if (View::Assignments == view)
		{
			add_working_set_designator(function->inputWorkingSet, get_cell(bounds, 2));
			add_auxiliary_designator(function->inputWorkingSet, function->inputObjectID, get_cell(bounds, 3));
		}
	}
}

void AuxiliaryAssignmentComponent::update_learn_mode()
{
	vtServer.set_auxiliary_learn_mode(get_is_learn_mode_wanted());
}

void AuxiliaryAssignmentComponent::press_soft_key(SoftKey action)
{
	switch (action)
	{
		case SoftKey::Assign:
		{
			change_view(View::Assignments);
		}
		break;

		case SoftKey::Back:
		{
			change_view((View::Inputs == view) ? View::Assignments : View::Overview);
		}
		break;

		case SoftKey::PreviousPage:
		case SoftKey::NextPage:
		{
			page += (SoftKey::NextPage == action) ? 1 : -1;
			rebuild();
		}
		break;

		case SoftKey::ChooseInput:
		{
			selectedInput = {};
			change_view(View::Inputs);
		}
		break;

		case SoftKey::Unassign:
		{
			const auto result = vtServer.unassign_auxiliary_function(selectedFunction.workingSet, selectedFunction.objectID);
			selectedFunction = {};
			show_message((Result::Allowed == result) ? juce::String("Removing the assignment") : get_result_text(result), Result::Allowed != result);
		}
		break;

		case SoftKey::Learn:
		{
			isLearnModeSwitchedOn = !isLearnModeSwitchedOn;
			learnedInputs.clear();
			update_learn_mode();
			show_message(isLearnModeSwitchedOn ? "Learn mode on, press a control to pick it" : "Learn mode off, the controls drive their functions", false);
		}
		break;

		case SoftKey::ClearSelection:
		default:
		{
			selectedFunction = {};
			selectedInput = {};
			show_message({}, false);
		}
		break;
	}
}

void AuxiliaryAssignmentComponent::press_row(int row)
{
	if (const auto *input = get_input_on_row(row))
	{
		assign(selectedFunction, { input->workingSet, input->objectID });
	}
	else if (const auto *function = get_function_on_row(row))
	{
		const Selection pressed{ function->workingSet, function->objectID };

		if (selectedInput.is_set())
		{
			assign(pressed, selectedInput);
		}
		else
		{
			selectedFunction = (pressed == selectedFunction) ? Selection{} : pressed;
			show_message({}, false);
		}
	}
}

void AuxiliaryAssignmentComponent::assign(const Selection &function, const Selection &input)
{
	const auto result = vtServer.assign_auxiliary_function(function.workingSet, function.objectID, input.workingSet, input.objectID);

	if (Result::Allowed == result)
	{
		selectedFunction = {};
		selectedInput = {};

		if (View::Inputs == view)
		{
			change_view(View::Assignments);
		}
		show_message("Assigning", false);
	}
	else
	{
		// The selection stays, so another control can be tried
		show_message(get_result_text(result), true);
	}
}

void AuxiliaryAssignmentComponent::show_message(const juce::String &text, bool isError)
{
	message = text;
	isMessageAnError = isError;
	rebuild();
}

void AuxiliaryAssignmentComponent::select_input(const Selection &input)
{
	selectedInput = input;
	message.clear();
	rebuild();
}

void AuxiliaryAssignmentComponent::paint_header(juce::Graphics &g) const
{
	const juce::Rectangle<int> header(0, 0, getWidth() - JuceManagedWorkingSetCache::get_softkey_mask_dimension_info().total_width(), HEADER_HEIGHT);
	auto area = header.reduced(2 * MARGIN, 0);
	juce::String title = "Auxiliary control";

	if (View::Assignments == view)
	{
		title = "Assignments";
	}
	else if (View::Inputs == view)
	{
		title = "Choose an input";
	}

	g.setColour(get_vt_colour(NAVY));
	g.fillRect(header);

	if (1 < get_page_count())
	{
		const juce::String pageText = juce::String(page + 1) + "/" + juce::String(get_page_count());
		const auto font = get_font(TEXT_FONT_HEIGHT);
		const auto pageArea = area.removeFromRight(juce::GlyphArrangement::getStringWidthInt(font, pageText) + MARGIN);
		g.setColour(get_vt_colour(WHITE));
		g.setFont(font);
		g.drawText(pageText, pageArea, juce::Justification::centredRight, false);
	}

	if (vtServer.get_auxiliary_learn_mode() && get_is_learn_mode_wanted())
	{
		const auto font = get_font(TEXT_FONT_HEIGHT, true);
		const auto badge = area.removeFromRight(juce::GlyphArrangement::getStringWidthInt(font, "LEARN") + 4 * MARGIN).reduced(MARGIN, 2 * MARGIN - 1);
		g.setColour(get_vt_colour(YELLOW));
		g.fillRect(badge);
		g.setColour(get_vt_colour(BLACK));
		g.setFont(font);
		g.drawText("LEARN", badge, juce::Justification::centred, false);
	}
	draw_text(g, title, area, get_font(TITLE_FONT_HEIGHT, true), get_vt_colour(WHITE));
}

void AuxiliaryAssignmentComponent::paint_statistics(juce::Graphics &g) const
{
	const int width = getWidth() - JuceManagedWorkingSetCache::get_softkey_mask_dimension_info().total_width();
	const int assigned = static_cast<int>(std::count_if(snapshot.functions.begin(), snapshot.functions.end(), [](const Snapshot::Function &function) { return Snapshot::State::Assigned == function.state; }));
	const std::pair<int, const char *> tiles[] = {
		{ static_cast<int>(snapshot.inputs.size()), "inputs" },
		{ static_cast<int>(snapshot.functions.size()), "functions" },
		{ assigned, "assigned" }
	};
	auto area = juce::Rectangle<int>(0, HEADER_HEIGHT + MARGIN, width, STATISTICS_HEIGHT).reduced(MARGIN, 0);
	const int tileWidth = (area.getWidth() - 2 * MARGIN) / 3;

	for (const auto &tile : tiles)
	{
		auto tileArea = area.removeFromLeft(tileWidth);
		area.removeFromLeft(MARGIN);
		g.setColour(get_vt_colour(NAVY));
		g.drawRect(tileArea, 2);
		tileArea.reduce(2 * MARGIN, MARGIN);
		g.setFont(get_font(NUMBER_FONT_HEIGHT, true));
		g.drawText(juce::String(tile.first), tileArea.removeFromTop(tileArea.getHeight() * 2 / 3), juce::Justification::centred, false);
		g.setColour(get_vt_colour(BLACK));
		g.setFont(get_font(TEXT_FONT_HEIGHT));
		g.drawText(tile.second, tileArea, juce::Justification::centred, false);
	}
}

void AuxiliaryAssignmentComponent::paint_row(juce::Graphics &g, int row) const
{
	const auto bounds = get_row_bounds(row);
	const auto *function = get_function_on_row(row);
	const auto *input = get_input_on_row(row);
	juce::String firstLine;
	juce::String secondLine;
	auto firstLineColour = get_vt_colour(BLACK);
	int textCell = 2;

	if (View::Overview == view)
	{
		const int index = page * get_rows_per_page() + row;

		if ((0 <= row) && (index < static_cast<int>(snapshot.devices.size())))
		{
			const auto &device = snapshot.devices.at(static_cast<std::size_t>(index));
			juce::StringArray details;

			if (0 < device.functionCount)
			{
				const int assigned = static_cast<int>(std::count_if(snapshot.functions.begin(), snapshot.functions.end(), [&device](const Snapshot::Function &each) { return is_assigned_function_of(each, device.workingSet); }));
				details.add(juce::String(device.functionCount) + " functions, " + (device.hasPreferredAssignment ? (juce::String(assigned) + " assigned") : juce::String("waiting for the implement")));
			}
			if (0 < device.inputCount)
			{
				details.add(juce::String(device.inputCount) + " inputs, " + (device.isInputUnitReady ? "ready" : "not ready"));
			}
			firstLine = get_device_name(device.workingSet);
			secondLine = details.joinIntoString("; ");
			textCell = 1;
		}
	}
	else if (nullptr != input)
	{
		const int users = count_functions_using(snapshot, input->workingSet, input->objectID);
		const auto result = vtServer.check_auxiliary_assignment(selectedFunction.workingSet, selectedFunction.objectID, input->workingSet, input->objectID);
		firstLine = (Result::Allowed == result) ? juce::String("Can be assigned") : get_result_text(result);
		firstLineColour = get_vt_colour((Result::Allowed == result) ? GREEN : GREY);
		secondLine = get_type_text(input->workingSet, input->objectID) + ((0 < users) ? (", drives " + juce::String(users)) : juce::String());
	}
	else if (nullptr != function)
	{
		firstLine = get_state_text(function->state);
		secondLine = get_type_text(function->workingSet, function->objectID);
		textCell = (View::Assignments == view) ? 4 : 2;

		switch (function->state)
		{
			case Snapshot::State::Assigned:
			{
				firstLineColour = get_vt_colour(GREEN);
			}
			break;

			case Snapshot::State::Assigning:
			{
				firstLineColour = get_vt_colour(BLUE);
			}
			break;

			case Snapshot::State::Failed:
			{
				firstLineColour = get_vt_colour(RED);
			}
			break;

			default:
			{
				firstLineColour = get_vt_colour(GREY);
			}
			break;
		}

		if ((View::Assignments == view) && (Selection{ function->workingSet, function->objectID } == selectedFunction))
		{
			g.setColour(get_vt_colour(YELLOW));
			g.fillRect(bounds);
		}
	}

	if (firstLine.isNotEmpty())
	{
		if ((View::Assignments == view) && (nullptr != function))
		{
			const auto from = get_cell(bounds, 1);
			const auto to = get_cell(bounds, 2);
			g.setColour(get_vt_colour(GREY));
			draw_triangle(g, juce::Rectangle<float>(static_cast<float>(from.getRight() + MARGIN), static_cast<float>(from.getCentreY() - 6), static_cast<float>(ARROW_WIDTH - 2 * MARGIN), 12.0f), 0.0f);

			if (nullptr == function->inputWorkingSet)
			{
				g.setColour(get_vt_colour(SILVER));
				g.drawRect(to.getUnion(get_cell(bounds, 3)), 1);
			}
		}

		auto textArea = bounds.withLeft(get_cell(bounds, textCell).getX()).reduced(MARGIN, MARGIN);
		draw_text(g, firstLine, textArea.removeFromTop(textArea.getHeight() / 2), get_font(TEXT_FONT_HEIGHT, true), firstLineColour);
		draw_text(g, secondLine, textArea, get_font(SMALL_FONT_HEIGHT), get_vt_colour(GREY));
		g.setColour(get_vt_colour(SILVER));
		g.drawHorizontalLine(bounds.getBottom(), static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));
	}
}

void AuxiliaryAssignmentComponent::paint_footer(juce::Graphics &g) const
{
	auto footer = get_footer_row();
	g.setColour(get_vt_colour(SILVER));
	g.fillRect(get_row_bounds(get_rows_per_page()).withBottom(getHeight()));

	// The pressed control is drawn first
	if ((View::Assignments == view) && selectedInput.is_set() && message.isEmpty())
	{
		footer.setLeft(get_cell(footer, 2).getX());
	}
	draw_text(g, message.isNotEmpty() ? message : get_prompt(), footer.reduced(2 * MARGIN, 0), get_font(TEXT_FONT_HEIGHT, isMessageAnError), get_vt_colour((message.isNotEmpty() && isMessageAnError) ? RED : BLACK));
}

void AuxiliaryAssignmentComponent::paint_soft_keys(juce::Graphics &g) const
{
	const auto &dimensions = JuceManagedWorkingSetCache::get_softkey_mask_dimension_info();
	const auto keys = get_soft_keys();
	g.setColour(get_vt_colour(GREY));
	g.fillRect(getLocalBounds().removeFromRight(dimensions.total_width()));

	for (std::size_t slot = 0; slot < keys.size(); slot++)
	{
		const auto bounds = get_soft_key_bounds(static_cast<int>(slot));
		const auto &key = keys.at(slot);
		const auto labelColour = get_vt_colour(key.isEnabled ? BLACK : GREY);
		auto background = key.isEnabled ? WHITE : SILVER;

		// Lit like the LEARN badge while learn mode is on
		if ((SoftKey::Learn == key.action) && get_is_learn_mode_wanted())
		{
			background = YELLOW;
		}
		g.setColour(get_vt_colour(background));
		g.fillRect(bounds);
		g.setColour(get_vt_colour(BLACK));
		g.drawRect(bounds, 1);
		g.setColour(labelColour);

		if ((SoftKey::PreviousPage == key.action) || (SoftKey::NextPage == key.action))
		{
			const auto arrow = bounds.toFloat().withSizeKeepingCentre(bounds.getWidth() * 0.4f, bounds.getHeight() * 0.4f);
			draw_triangle(g, arrow, juce::MathConstants<float>::halfPi * ((SoftKey::PreviousPage == key.action) ? -1.0f : 1.0f));
		}
		else
		{
			const char *labels[] = { "Assign", "Back", "", "", "Inputs", "Unassign", "Clear", "Learn" };
			g.setFont(get_font(std::min(TEXT_FONT_HEIGHT, bounds.getHeight() * 0.3f), true));
			g.drawFittedText(labels[static_cast<std::size_t>(key.action)], bounds.reduced(2), juce::Justification::centred, 2, 0.6f);
		}
	}
}

std::vector<AuxiliaryAssignmentComponent::Key> AuxiliaryAssignmentComponent::get_soft_keys() const
{
	std::vector<Key> keys;
	const auto *function = get_selected_function();

	if (View::Overview == view)
	{
		keys.push_back({ SoftKey::Assign, snapshot.isPrimaryVT && (!snapshot.functions.empty()) });
	}
	else
	{
		keys.push_back({ SoftKey::Back, true });
		keys.push_back({ SoftKey::PreviousPage, 0 < page });
		keys.push_back({ SoftKey::NextPage, page + 1 < get_page_count() });
		keys.push_back({ SoftKey::Learn, snapshot.isPrimaryVT });

		if (View::Assignments == view)
		{
			keys.push_back({ SoftKey::ChooseInput, (nullptr != function) && (Snapshot::State::WaitingForImplement != function->state) });
			keys.push_back({ SoftKey::Unassign, (nullptr != function) && (nullptr != function->inputWorkingSet) });
			keys.push_back({ SoftKey::ClearSelection, selectedFunction.is_set() || selectedInput.is_set() });
		}
	}

	// A VT with fewer soft keys keeps the first ones
	const auto &dimensions = JuceManagedWorkingSetCache::get_softkey_mask_dimension_info();
	keys.resize(std::min(keys.size(), static_cast<std::size_t>(std::max(dimensions.key_count(), 0))));
	return keys;
}

juce::Rectangle<int> AuxiliaryAssignmentComponent::get_soft_key_bounds(int slot) const
{
	const auto &dimensions = JuceManagedWorkingSetCache::get_softkey_mask_dimension_info();
	const int left = getWidth() - dimensions.total_width();
	return juce::Rectangle<int>(dimensions.keyWidth, dimensions.keyHeight).withPosition(dimensions.key_position(slot).translated(left, 0));
}

int AuxiliaryAssignmentComponent::get_soft_key_slot_at(juce::Point<int> position) const
{
	int retVal = -1;

	for (int slot = 0; slot < static_cast<int>(get_soft_keys().size()); slot++)
	{
		if (get_soft_key_bounds(slot).contains(position))
		{
			retVal = slot;
		}
	}
	return retVal;
}

juce::Rectangle<int> AuxiliaryAssignmentComponent::get_row_bounds(int row) const
{
	const auto &dimensions = JuceManagedWorkingSetCache::get_softkey_mask_dimension_info();
	const int rowHeight = std::max(MINIMUM_CELL_SIZE, dimensions.keyHeight * 3 / 4) + 2 * MARGIN;
	const int width = getWidth() - dimensions.total_width();

	// Row -1 is the fixed row above the list
	const int top = (0 <= row) ? (get_first_row_top() + row * rowHeight) : (HEADER_HEIGHT + MARGIN);
	return { 0, top, width, rowHeight };
}

juce::Rectangle<int> AuxiliaryAssignmentComponent::get_footer_row() const
{
	// One row high, centred in the space below the list
	const auto row = get_row_bounds(get_rows_per_page());
	return row.withY(row.getY() + std::max(0, getHeight() - row.getBottom()) / 2);
}

juce::Rectangle<int> AuxiliaryAssignmentComponent::get_cell(juce::Rectangle<int> row, int index) const
{
	const auto &dimensions = JuceManagedWorkingSetCache::get_softkey_mask_dimension_info();
	const int cellHeight = row.getHeight() - 2 * MARGIN;
	const int cellWidth = std::max(MINIMUM_CELL_SIZE, cellHeight * dimensions.keyWidth / std::max(dimensions.keyHeight, 1));

	// Function and input are a pair of cells each, with an arrow between them
	const int left = row.getX() + MARGIN + index * (cellWidth + MARGIN) + ((2 <= index) ? ARROW_WIDTH : 0);
	return { left, row.getY() + MARGIN, cellWidth, cellHeight };
}

int AuxiliaryAssignmentComponent::get_row_at(juce::Point<int> position) const
{
	int retVal = -1;

	for (int row = 0; row < get_rows_per_page(); row++)
	{
		if (get_row_bounds(row).contains(position))
		{
			retVal = row;
		}
	}
	return retVal;
}

int AuxiliaryAssignmentComponent::get_first_row_top() const
{
	const auto &dimensions = JuceManagedWorkingSetCache::get_softkey_mask_dimension_info();
	const int rowHeight = std::max(MINIMUM_CELL_SIZE, dimensions.keyHeight * 3 / 4) + 2 * MARGIN;
	int top = HEADER_HEIGHT + MARGIN;

	if (View::Overview == view)
	{
		top += STATISTICS_HEIGHT + MARGIN;
	}
	else if (View::Inputs == view)
	{
		top += rowHeight + MARGIN;
	}
	return top;
}

int AuxiliaryAssignmentComponent::get_rows_per_page() const
{
	const auto &dimensions = JuceManagedWorkingSetCache::get_softkey_mask_dimension_info();
	const int rowHeight = std::max(MINIMUM_CELL_SIZE, dimensions.keyHeight * 3 / 4) + 2 * MARGIN;

	// The footer takes one row
	return std::max(1, (getHeight() - get_first_row_top()) / rowHeight - 1);
}

int AuxiliaryAssignmentComponent::get_item_count() const
{
	std::size_t count = snapshot.functions.size();

	if (View::Overview == view)
	{
		count = snapshot.devices.size();
	}
	else if (View::Inputs == view)
	{
		count = snapshot.inputs.size();
	}
	return static_cast<int>(count);
}

int AuxiliaryAssignmentComponent::get_page_count() const
{
	const int rows = get_rows_per_page();
	return std::max(1, (get_item_count() + rows - 1) / rows);
}

bool AuxiliaryAssignmentComponent::get_is_row_dimmed(int row) const
{
	bool retVal = false;

	if (const auto *input = get_input_on_row(row))
	{
		retVal = Result::Allowed != vtServer.check_auxiliary_assignment(selectedFunction.workingSet, selectedFunction.objectID, input->workingSet, input->objectID);
	}
	else if (const auto *function = get_function_on_row(row))
	{
		// Functions the pressed control cannot drive
		retVal = (View::Assignments == view) && selectedInput.is_set() &&
		  (Result::Allowed != vtServer.check_auxiliary_assignment(function->workingSet, function->objectID, selectedInput.workingSet, selectedInput.objectID));
	}
	return retVal;
}

bool AuxiliaryAssignmentComponent::get_is_learn_mode_wanted() const
{
	// Only while assignments are made (ISO 11783-6 J.8)
	return isShown && (View::Overview != view) && snapshot.isPrimaryVT && isLearnModeSwitchedOn;
}

const AuxiliaryControlSnapshot::Function *AuxiliaryAssignmentComponent::get_selected_function() const
{
	const AuxiliaryControlSnapshot::Function *retVal = nullptr;

	for (const auto &function : snapshot.functions)
	{
		if (Selection{ function.workingSet, function.objectID } == selectedFunction)
		{
			retVal = &function;
		}
	}
	return retVal;
}

const AuxiliaryControlSnapshot::Function *AuxiliaryAssignmentComponent::get_function_on_row(int row) const
{
	const AuxiliaryControlSnapshot::Function *retVal = nullptr;
	const int index = page * get_rows_per_page() + row;

	if ((View::Inputs == view) && (-1 == row))
	{
		retVal = get_selected_function();
	}
	else if ((View::Assignments == view) && (0 <= row) && (row < get_rows_per_page()) && (index < static_cast<int>(snapshot.functions.size())))
	{
		retVal = &snapshot.functions.at(static_cast<std::size_t>(index));
	}
	return retVal;
}

const AuxiliaryControlSnapshot::Input *AuxiliaryAssignmentComponent::get_input_on_row(int row) const
{
	const AuxiliaryControlSnapshot::Input *retVal = nullptr;
	const int index = page * get_rows_per_page() + row;

	if ((View::Inputs == view) && (0 <= row) && (row < get_rows_per_page()) && (index < static_cast<int>(snapshot.inputs.size())))
	{
		retVal = &snapshot.inputs.at(static_cast<std::size_t>(index));
	}
	return retVal;
}

juce::String AuxiliaryAssignmentComponent::get_prompt() const
{
	const bool canPressControl = get_is_learn_mode_wanted();
	juce::String prompt = canPressControl ? "Press a control to assign it, or Inputs to choose one" : "Press Inputs to choose its input";

	if (!snapshot.isPrimaryVT)
	{
		// ISO 11783-6 J.3.1
		prompt = "Only the primary VT (function instance 0) makes assignments";
	}
	else if (View::Overview == view)
	{
		prompt = "Press Assign to change the assignments";

		if (snapshot.functions.empty())
		{
			prompt = "No implement offers auxiliary functions";
		}
		else if (snapshot.inputs.empty())
		{
			prompt = "No auxiliary input unit is connected";
		}
	}
	else if (View::Inputs == view)
	{
		prompt = canPressControl ? "Pick an input, or press a control" : "Pick an input";
	}
	else if (selectedInput.is_set())
	{
		const int users = count_functions_using(snapshot, selectedInput.workingSet, selectedInput.objectID);
		prompt = "Pick a function for this control" + ((0 < users) ? (", it drives " + juce::String(users)) : juce::String());
	}
	else if (!selectedFunction.is_set())
	{
		prompt = canPressControl ? "Pick a function, or press a control" : "Pick a function. Learn is off, the controls work";
	}
	return prompt;
}

juce::String AuxiliaryAssignmentComponent::get_device_name(const WorkingSetPointer &workingSet)
{
	juce::String name = "Unknown";
	const auto controlFunction = (nullptr != workingSet) ? workingSet->get_control_function() : nullptr;

	if (nullptr != controlFunction)
	{
		const auto manufacturer = manufacturerMap.find(controlFunction->get_NAME().get_manufacturer_code());
		name = (manufacturerMap.end() != manufacturer) ? juce::String(manufacturer->second) : ("Manufacturer " + juce::String(controlFunction->get_NAME().get_manufacturer_code()));
		name += " (" + juce::String::toHexString(controlFunction->get_address()).toUpperCase().paddedLeft('0', 2) + ")";
	}
	return name;
}

juce::String AuxiliaryAssignmentComponent::get_result_text(Result result)
{
	switch (result)
	{
		case Result::Allowed:
			return "Can be assigned";
		case Result::NotPrimaryVT:
			return "Only the primary VT makes assignments";
		case Result::NoPreferredAssignment:
			return "The implement has not sent its assignments yet";
		case Result::InvalidFunction:
			return "The function is gone";
		case Result::InvalidInput:
			return "The input is gone, or its unit is not ready";
		case Result::TypeMismatch:
			return "The control is of another type";
		case Result::CriticalControl:
			return "The function needs a critical control";
		case Result::AssignmentRestriction:
			return "The function only takes its preferred control";
		case Result::SingleAssignment:
		default:
			return "The control may drive only one function";
	}
}

juce::String AuxiliaryAssignmentComponent::get_state_text(AuxiliaryControlSnapshot::State state)
{
	switch (state)
	{
		case Snapshot::State::WaitingForImplement:
			return "Waiting for the implement";
		case Snapshot::State::Assigning:
			return "Assigning";
		case Snapshot::State::Assigned:
			return "Assigned";
		case Snapshot::State::Failed:
			return "Failed";
		case Snapshot::State::NotAssigned:
		default:
			return "Not assigned";
	}
}

juce::String AuxiliaryAssignmentComponent::get_type_text(const WorkingSetPointer &workingSet, std::uint16_t objectID)
{
	static const char *const typeNames[] = { "Latching", "Analogue", "Momentary", "Analogue, centre return", "Analogue, zero return", "3-position latching", "3-position momentary", "3-position, up latches", "3-position, down latches", "Analogue, latching ends", "Analogue held, latching ends", "Quadrature momentary", "Quadrature analogue", "Quadrature, centre return", "Encoder" };
	juce::StringArray parts;
	auto object = (nullptr != workingSet) ? workingSet->get_object_by_id(objectID) : nullptr;

	if ((nullptr != object) && (isobus::VirtualTerminalObjectType::AuxiliaryFunctionType2 == object->get_object_type()))
	{
		using Function = isobus::AuxiliaryFunctionType2;
		const auto function = std::static_pointer_cast<Function>(object);
		const auto type = static_cast<std::size_t>(function->get_function_type());
		parts.add((type < std::size(typeNames)) ? typeNames[type] : "Unknown type");
		parts.add(function->get_function_attribute(Function::CriticalControl) ? "critical" : "");
		parts.add(function->get_function_attribute(Function::AssignmentRestriction) ? "restricted" : "");
		parts.add(function->get_function_attribute(Function::SingleAssignment) ? "single" : "");
	}
	else if ((nullptr != object) && (isobus::VirtualTerminalObjectType::AuxiliaryInputType2 == object->get_object_type()))
	{
		using Input = isobus::AuxiliaryInputType2;
		const auto input = std::static_pointer_cast<Input>(object);
		const auto type = static_cast<std::size_t>(input->get_function_type());
		parts.add((type < std::size(typeNames)) ? typeNames[type] : "Unknown type");
		parts.add(input->get_function_attribute(Input::CriticalControl) ? "critical" : "");
		parts.add(input->get_function_attribute(Input::SingleAssignment) ? "single" : "");
	}
	parts.removeEmptyStrings();
	return parts.joinIntoString(", ");
}

juce::Colour AuxiliaryAssignmentComponent::get_vt_colour(std::uint8_t colourIndex)
{
	static const isobus::VTColourTable colourTable;
	const auto colour = colourTable.get_colour(colourIndex);
	return juce::Colour::fromFloatRGBA(colour.r, colour.g, colour.b, 1.0f);
}

juce::Font AuxiliaryAssignmentComponent::get_font(float height, bool isBold)
{
	// The VT draws the pools' text in this font
	return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), height, isBold ? juce::Font::bold : juce::Font::plain));
}
