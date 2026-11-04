//================================================================================================
/// @file AuxiliaryAssignmentComponent.hpp
///
/// @brief Declares the VT's auxiliary control (AUX-N) assignment screen.
/// @author Gunics Balázs
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef AUXILIARY_ASSIGNMENT_COMPONENT_HPP
#define AUXILIARY_ASSIGNMENT_COMPONENT_HPP

#include "SoftKeyMaskComponent.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server.hpp"

#include "JuceHeader.h"

#include <map>
#include <memory>
#include <utility>
#include <vector>

/// @brief The auxiliary functions and inputs of the working sets, and their assignments
struct AuxiliaryControlSnapshot
{
	using WorkingSetPointer = std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet>;

	/// @brief How far the assignment of a function has got
	enum class State : std::uint8_t
	{
		WaitingForImplement, ///< No Preferred Assignment yet, so it cannot be assigned
		NotAssigned,
		Assigning, ///< Sent, not accepted yet
		Assigned,
		Failed ///< Rejected or not answered
	};

	/// @brief One Auxiliary Function Type 2 and its input
	struct Function
	{
		bool operator==(const Function &other) const;

		WorkingSetPointer workingSet;
		WorkingSetPointer inputWorkingSet; ///< nullptr if not assigned
		std::uint16_t objectID = isobus::NULL_OBJECT_ID;
		std::uint16_t inputObjectID = isobus::NULL_OBJECT_ID;
		State state = State::NotAssigned;
	};

	/// @brief One Auxiliary Input Type 2
	struct Input
	{
		bool operator==(const Input &other) const;

		WorkingSetPointer workingSet;
		std::uint16_t objectID = isobus::NULL_OBJECT_ID;
	};

	/// @brief A working set with auxiliary functions or inputs
	struct Device
	{
		bool operator==(const Device &other) const;

		WorkingSetPointer workingSet;
		int functionCount = 0;
		int inputCount = 0;
		bool isInputUnitReady = false;
		bool hasPreferredAssignment = false;
	};

	/// @brief Collects the auxiliary objects of the working sets and their assignments
	/// @param[in] workingSets The managed working sets
	/// @param[in] assignments The server's assignments
	/// @param[in] isPrimaryVT True if this VT makes assignments
	/// @returns The snapshot
	static AuxiliaryControlSnapshot build(const std::vector<WorkingSetPointer> &workingSets,
	                                      const std::vector<isobus::VirtualTerminalServer::AuxiliaryAssignment> &assignments,
	                                      bool isPrimaryVT);

	/// @brief Returns if a working set's object pool has auxiliary functions or inputs
	/// @param[in] workingSet The working set to check
	/// @returns True if it has any
	static bool has_auxiliary_objects(const WorkingSetPointer &workingSet);

	bool operator==(const AuxiliaryControlSnapshot &other) const;
	bool operator!=(const AuxiliaryControlSnapshot &other) const;

	std::vector<Device> devices;
	std::vector<Function> functions;
	std::vector<Input> inputs;
	bool isPrimaryVT = false;
};

/// @brief The assignment screen, drawn over the data mask and soft key areas as a working set would be
class AuxiliaryAssignmentComponent : public juce::Component
{
public:
	/// @brief Constructor
	/// @param[in] server The VT server that makes the assignments
	explicit AuxiliaryAssignmentComponent(isobus::VirtualTerminalServer &server);

	/// @brief Opens the overview, or turns learn mode off when hidden
	/// @param[in] shown True when the screen is shown
	void set_shown(bool shown);

	/// @brief Redraws when the functions, inputs or assignments have changed
	/// @param[in] newSnapshot The current state
	void set_snapshot(const AuxiliaryControlSnapshot &newSnapshot);

	/// @brief Tells the operator what happened to an assignment
	/// @param[in] assignment The assignment
	/// @param[in] event What happened to it
	void on_assignment_event(const isobus::VirtualTerminalServer::AuxiliaryAssignment &assignment, isobus::VirtualTerminalServer::AuxiliaryAssignmentEvent event);

	/// @brief Takes a control the operator pressed in learn mode
	/// @param[in] status The input's status
	void on_input_status(const isobus::VirtualTerminalServer::AuxiliaryInputStatus &status);

	void paint(juce::Graphics &g) override;
	void paintOverChildren(juce::Graphics &g) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent &event) override;
	void mouseUp(const juce::MouseEvent &event) override;

private:
	using WorkingSetPointer = AuxiliaryControlSnapshot::WorkingSetPointer;
	using Result = isobus::VirtualTerminalServer::AuxiliaryAssignmentResult;

	enum class View : std::uint8_t
	{
		Overview, ///< Counts of the inputs and functions
		Assignments, ///< Every function and its input
		Inputs ///< The inputs to choose from for one function
	};

	enum class SoftKey : std::uint8_t
	{
		Assign,
		Back,
		PreviousPage,
		NextPage,
		ChooseInput,
		Unassign,
		ClearSelection
	};

	/// @brief A function or an input the operator picked
	struct Selection
	{
		bool is_set() const;
		bool operator==(const Selection &other) const;

		WorkingSetPointer workingSet;
		std::uint16_t objectID = isobus::NULL_OBJECT_ID;
	};

	/// @brief A soft key of the current view
	struct Key
	{
		SoftKey action;
		bool isEnabled;
	};

	static constexpr int HEADER_HEIGHT = 32;
	static constexpr int FOOTER_HEIGHT = 48;
	static constexpr int MARGIN = 4;
	static constexpr int STATISTICS_HEIGHT = 72;
	static constexpr int PRESSED_RING_WIDTH = 3;

	void change_view(View newView);
	void rebuild();
	void add_working_set_designator(const WorkingSetPointer &workingSet, juce::Rectangle<int> cell);
	void add_auxiliary_designator(const WorkingSetPointer &workingSet, std::uint16_t objectID, juce::Rectangle<int> cell);
	void add_row_designators(int row);
	void update_learn_mode();
	void press_soft_key(SoftKey action);
	void press_row(int row);
	void assign(const Selection &function, const Selection &input);
	void show_message(const juce::String &text, bool isError);
	void select_input(const Selection &input);

	void paint_header(juce::Graphics &g) const;
	void paint_statistics(juce::Graphics &g) const;
	void paint_row(juce::Graphics &g, int row) const;
	void paint_footer(juce::Graphics &g) const;
	void paint_soft_keys(juce::Graphics &g) const;

	std::vector<Key> get_soft_keys() const;
	juce::Rectangle<int> get_soft_key_bounds(int slot) const;
	int get_soft_key_slot_at(juce::Point<int> position) const;
	juce::Rectangle<int> get_row_bounds(int row) const;
	juce::Rectangle<int> get_footer_row() const;
	juce::Rectangle<int> get_cell(juce::Rectangle<int> row, int index) const;
	int get_row_at(juce::Point<int> position) const;
	int get_first_row_top() const;
	int get_rows_per_page() const;
	int get_item_count() const;
	int get_page_count() const;
	bool get_is_row_dimmed(int row) const;
	bool get_is_learn_mode_wanted() const;
	const AuxiliaryControlSnapshot::Function *get_selected_function() const;
	const AuxiliaryControlSnapshot::Function *get_function_on_row(int row) const;
	const AuxiliaryControlSnapshot::Input *get_input_on_row(int row) const;
	juce::String get_prompt() const;

	static juce::String get_device_name(const WorkingSetPointer &workingSet);
	static juce::String get_result_text(Result result);
	static juce::String get_state_text(AuxiliaryControlSnapshot::State state);
	static juce::String get_type_text(const WorkingSetPointer &workingSet, std::uint16_t objectID);
	static juce::Colour get_vt_colour(std::uint8_t colourIndex);
	static juce::Font get_font(float height, bool isBold = false);

	isobus::VirtualTerminalServer &vtServer;
	AuxiliaryControlSnapshot snapshot;
	std::vector<std::shared_ptr<juce::Component>> designators;
	std::vector<std::pair<juce::Rectangle<int>, juce::Colour>> designatorBackgrounds;
	std::map<std::pair<isobus::VirtualTerminalServerManagedWorkingSet *, std::uint16_t>, bool> activatedInputs; ///< Last learn mode state of each input
	Selection selectedFunction;
	Selection selectedInput;
	juce::String message;
	View view = View::Overview;
	int page = 0;
	int pressedSoftKeySlot = -1;
	bool isShown = false;
	bool isMessageAnError = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuxiliaryAssignmentComponent)
};

#endif // AUXILIARY_ASSIGNMENT_COMPONENT_HPP
