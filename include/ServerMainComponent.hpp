#pragma once

#include "ConfigureHardwareWindow.hpp"
#include "DataMaskRenderAreaComponent.hpp"
#include "LoggerComponent.hpp"
#include "SoftKeyMaskComponent.hpp"
#include "SoftKeyMaskRenderAreaComponent.hpp"
#include "VT_NumberComponent.hpp"
#include "WorkingSetSelectorComponent.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server.hpp"

#include <filesystem>

class ServerMainComponent : public juce::Component
  , public juce::KeyListener
  , public isobus::VirtualTerminalServer
  , public Timer
  , public ApplicationCommandTarget
  , public MenuBarModel
{
public:
	ServerMainComponent(std::shared_ptr<isobus::InternalControlFunction> serverControlFunction,
	                    std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers,
	                    std::shared_ptr<ValueTree> settings,
	                    std::uint8_t vtNumber = 0);
	~ServerMainComponent() override;

	bool get_is_enough_memory(std::uint32_t requestedMemory) const override;
	VTVersion get_version() const override;
	std::uint8_t get_number_of_navigation_soft_keys() const override;
	std::uint8_t get_soft_key_descriptor_x_pixel_width() const override;
	std::uint8_t get_soft_key_descriptor_y_pixel_height() const override;
	std::uint8_t get_number_of_possible_virtual_soft_keys_in_soft_key_mask() const override;
	std::uint8_t get_number_of_physical_soft_keys() const override;
	std::uint8_t get_physical_soft_key_rows() const;
	std::uint8_t get_physical_soft_key_columns() const;
	std::uint16_t get_data_mask_area_size_x_pixels() const override;
	std::uint16_t get_data_mask_area_size_y_pixels() const override;
	void suspend_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSetWithError) override;
	SupportedWideCharsErrorCode get_supported_wide_chars(std::uint8_t codePlane,
	                                                     std::uint16_t firstWideCharInInquiryRange,
	                                                     std::uint16_t lastWideCharInInquiryRange,
	                                                     std::uint8_t &numberOfRanges,
	                                                     std::vector<std::uint8_t> &wideCharRangeArray) override;

	bool keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) override;
	bool keyStateChanged(bool isKeyDown, juce::Component *originatingComponent) override;

	std::vector<std::array<std::uint8_t, 7>> get_versions(isobus::NAME clientNAME) override;
	std::vector<std::uint8_t> get_supported_objects() const override;

	/// @brief This function is called when the client wants the server to load a previously stored object pool.
	/// If there exists in the VT's non-volatile memory an object pool matching the provided version label,
	/// return it. If one does not exist, return an empty vector.
	/// @param[in] versionLabel The object pool version to load for the given client NAME
	/// @param[in] clientNAME The client requesting the object pool
	/// @returns The requested object pool associated with the version label.
	std::vector<std::uint8_t> load_version(const std::vector<std::uint8_t> &versionLabel, isobus::NAME clientNAME) override;

	/// @brief This function is called when the client wants the server to save an object pool
	/// to the VT's non-volatile memory.
	/// If the object pool is saved successfully, return true, otherwise return false.
	/// @note This may be called multiple times with the same version, but different data. When this
	/// happens, the expectation is that you will append each objectPool together into one large file.
	/// @param[in] objectPool The object pool data to save
	/// @param[in] versionLabel The object pool version to save for the given client NAME
	/// @param[in] clientNAME The client requesting the object pool
	/// @returns The requested object pool associated with the version label.
	bool save_version(const std::vector<std::uint8_t> &objectPool, const std::vector<std::uint8_t> &versionLabel, isobus::NAME clientNAME) override;

	/// @brief This function is called when the client wants the server to delete a stored object pool.
	/// All object pool files matching the specified version label should then be deleted from the VT's
	/// non-volatile storage.
	/// @param[in] versionLabel The version label for the object pool(s) to delete
	/// @param[in] clientNAME The NAME of the client that is requesting deletion
	/// @returns True if the version was deleted from VT non-volatile storage, otherwise false.
	bool delete_version(const std::vector<std::uint8_t> &versionLabel, isobus::NAME clientNAME) override;

	/// @brief This function is called when the client wants the server to delete ALL stored object pools associated to it's NAME.
	/// All object pool files matching the specified client NAME should then be deleted from the VT's
	/// non-volatile storage.
	/// @param[in] clientNAME The NAME of the client that is requesting deletion
	/// @returns True if all relevant object pools were deleted from VT non-volatile storage, otherwise false.
	bool delete_all_versions(isobus::NAME clientNAME) override;

	/// @brief This function is called when the client wants the server to deactivate its object pool.
	/// You should treat this as a disconnection by the client, as it may be moving to another VT.
	/// @attention This does not mean to delete the pool from non-volatile memory!!! This only deactivates the active pool.
	/// @details This command is used to delete the entire object pool of this Working Set from volatile storage.
	/// This command can be used by an implement when it wants to move its object pool to another VT,
	/// or when it is shutting down or during the development of object pools.
	/// @param[in] clientNAME The NAME of the client that is requesting deletion
	/// @returns True if the client's active object pool was deactivated and removed from volatile storage, otherwise false.
	bool delete_object_pool(isobus::NAME clientNAME) override;

	void timerCallback() override;

	void paint(juce::Graphics &g) override;
	void resized() override;

	ApplicationCommandTarget *getNextCommandTarget() override;
	void getAllCommands(juce::Array<juce::CommandID> &allCommands) override;
	void getCommandInfo(juce::CommandID commandID, ApplicationCommandInfo &result) override;
	bool perform(const InvocationInfo &info) override;
	StringArray getMenuBarNames() override;
	PopupMenu getMenuForIndex(int, const juce::String &) override;
	void menuItemSelected(int, int) override;

	std::shared_ptr<isobus::ControlFunction> get_client_control_function_for_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) const;

	void change_selected_working_set(std::uint8_t index);

	void set_button_held(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t objectID, std::uint16_t maskObjectID, std::uint8_t keyCode, bool isSoftKey);
	void set_button_released(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t objectID, std::uint16_t maskObjectID, std::uint8_t keyCode, bool isSoftKey);

	void repaint_on_next_update();

	void save_settings();

	void identify_vt() override;

private:
	enum class CommandIDs : int
	{
		NoCommand = 0, /// 0 Is an invalid command ID
		About,
		ConfigureLanguageCommand,
		ConfigureReportedVersion,
		ConfigureReportedHardware,
		ConfigureLogging,
		ConfigureShortcuts,
		GenerateLogPackage,
		ClearISOData,
		ConfigureCANHardware,
		StartStop,
		AutoStart
	};

	SoftKeyMaskDimensions softKeyMaskDimensions;

	class LanguageCommandConfigClosed
	{
	public:
		void operator()(int result) const noexcept;
		ServerMainComponent &mParent;

	private:
	};
	friend class LanguageCommandConfigClosed;

	struct HeldButtonData
	{
		HeldButtonData(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t objectID, std::uint16_t maskObjectID, std::uint8_t keyCode, bool isSoftKey);
		bool operator==(const HeldButtonData &other);
		std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> associatedWorkingSet;
		std::uint32_t timestamp_ms;
		std::uint16_t buttonObjectID;
		std::uint16_t activeMaskObjectID;
		std::uint8_t buttonKeyCode;
		bool isSoftKey;
	};

	static VTVersion get_version_from_setting(std::uint8_t aVersion);

	std::size_t number_of_iop_files_in_directory(std::filesystem::path path);

	void on_change_active_mask_callback(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t workingSet, std::uint16_t newMask);
	void repaint_data_and_soft_key_mask();
	void check_load_settings(std::shared_ptr<ValueTree> settings);
	void remove_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSetToRemove);
	void clear_iso_data();

	const std::string ISO_DATA_PATH = "iso_data";

	juce::ApplicationCommandManager mCommandManager;
	WorkingSetSelectorComponent workingSetSelector;
	DataMaskRenderAreaComponent dataMaskRenderer;
	SoftKeyMaskRenderAreaComponent softKeyMaskRenderer;
	MenuBarComponent menuBar;
	LoggerComponent logger;
	Viewport loggerViewport;
	VT_NumberComponent vtNumberComponent;
	SoundPlayer mSoundPlayer;
	AudioDeviceManager mAudioDeviceManager;
	std::unique_ptr<AlertWindow> popupMenu;
	std::unique_ptr<ConfigureHardwareWindow> configureHardwareWindow;
	std::shared_ptr<isobus::ControlFunction> alarmAckKeyWs;
	std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &parentCANDrivers;
	std::vector<HeldButtonData> heldButtons;
	std::uint32_t alarmAckKeyMaskId = isobus::NULL_OBJECT_ID;
	int alarmAckKeyCode = juce::KeyPress::escapeKey;
	std::uint8_t vtNumber = 1;
	std::uint8_t numberOfPoolsToRender = 0;
	VTVersion versionToReport = VTVersion::Version5;
	bool needToRepaint = false;
	bool autostart = false;
	bool hasStartBeenCalled = false;
	bool alarmAckKeyPressed = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ServerMainComponent)
};
