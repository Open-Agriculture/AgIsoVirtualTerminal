//================================================================================================
/// @file ServerMainComponent.hpp
///
/// @brief The main component of the application, which runs the VT server
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#ifndef SERVER_MAIN_COMPONENT_HPP
#define SERVER_MAIN_COMPONENT_HPP

#include "ConfigureHardwareWindow.hpp"
#include "DataMaskRenderAreaComponent.hpp"
#include "LoggerComponent.hpp"
#include "SoftKeyMaskComponent.hpp"
#include "SoftKeyMaskRenderAreaComponent.hpp"
#include "UpdateChecker.hpp"
#include "VT_NumberComponent.hpp"
#include "WorkingSetSelectorComponent.hpp"
#include "isobus/isobus/isobus_diagnostic_protocol.hpp"
#include "isobus/isobus/isobus_time_date_interface.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server.hpp"

#include <filesystem>
#include <set>

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
	                    const std::string &canLogPath_,
	                    std::uint8_t vtNumberArg = 0,
	                    std::string screenCaptureDir = "");
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

	/// @brief Returns the 32 byte version labels stored for a client
	std::vector<std::array<std::uint8_t, 32>> get_extended_versions(isobus::NAME clientNAME) override;

	/// @brief This function is called when the client wants the server to delete ALL stored object pools of one label length associated to its NAME.
	/// @param[in] versionLabelLength 7 for the Delete Version command, 32 for the Extended Delete Version command
	/// @param[in] clientNAME The NAME of the client that is requesting deletion
	/// @returns True if all relevant object pools were deleted from VT non-volatile storage, otherwise false.
	bool delete_all_versions(std::uint8_t versionLabelLength, isobus::NAME clientNAME) override;

	/// @brief Nothing to do, the server deletes the pool and then calls object_pool_deleted()
	/// @param[in] clientNAME The NAME of the client that is requesting deletion
	/// @returns Always true
	bool delete_object_pool(isobus::NAME clientNAME) override;

	/// @brief Stops presenting a working set whose master shut down unexpectedly
	void handle_unexpected_working_set_shutdown(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, UnexpectedShutdownReason) override;

	/// @brief Draws the working set in the selector, and remembers if its object pool was rejected
	void object_pool_parse_finished(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, bool success) override;

	/// @brief Activates a working set while none is active
	void select_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) override;

	/// @brief Stops presenting a working set whose object pool the client deleted
	void object_pool_deleted(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) override;

	/// @brief Closes the input dialog the working set has open, without taking its value
	std::uint16_t abort_input(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) override;

	std::uint8_t get_user_layout_datamask_bg_color() const override;
	std::uint8_t get_user_layout_softkeymask_bg_color() const override;

	void timerCallback() override;

	void paint(juce::Graphics &g) override;
	void resized() override;
	void parentHierarchyChanged() override;

	ApplicationCommandTarget *getNextCommandTarget() override;
	void getAllCommands(juce::Array<juce::CommandID> &allCommands) override;
	void getCommandInfo(juce::CommandID commandID, ApplicationCommandInfo &result) override;
	bool perform(const InvocationInfo &info) override;
	StringArray getMenuBarNames() override;
	PopupMenu getMenuForIndex(int, const juce::String &) override;
	void menuItemSelected(int, int) override;

	std::shared_ptr<isobus::ControlFunction> get_client_control_function_for_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) const;

	void change_selected_working_set(std::uint8_t index);

	void send_alarm_ack_command(isobus::VirtualTerminalBase::KeyActivationCode activationCode);

	void set_button_held(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t objectID, std::uint16_t maskObjectID, std::uint8_t keyCode, bool isSoftKey);
	void set_button_released(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t objectID, std::uint16_t maskObjectID, std::uint8_t keyCode, bool isSoftKey);
	void release_held_buttons(bool aborted = false);

	void repaint_on_next_update();

	void save_settings();

	void identify_vt() override;

	void screen_capture(std::uint8_t item, std::uint8_t path, std::shared_ptr<isobus::ControlFunction> requestor) override;

	static std::string getAppDataDir();

	/// @brief Gives up the keyboard focus and destroys the text caret which goes with it.
	/// @details Windows shows its touch keyboard whenever the thread owns a caret. JUCE creates
	/// one when a text field takes the focus, and destroys it when the focus goes away, but that
	/// clean up is deferred to an asynchronous callback and only runs while the field's window
	/// still has a peer. A dialog which is closed and destroyed before that callback runs
	/// therefore leaves the caret behind, and a caret belongs to the thread rather than to a
	/// window, so the touch keyboard then appears on any tap anywhere in the program until it is
	/// restarted. Call this whenever a dialog holding a text field goes away.
	static void release_text_input_focus();
	/**
   * @brief minimum_height
   * @return the on-screen height of the mask render areas, at the current display scale
   */
	int minimum_height() const;

	static constexpr double MINIMUM_DISPLAY_SCALE = 0.25; ///< Lower bound when fitting to the window
	static constexpr double MAXIMUM_DISPLAY_SCALE = 6.0; ///< Upper bound when fitting to the window

	/// @brief Returns how much the masks are magnified to fit the window, or 1 when that is turned off. Clients still get the reported size.
	/// @returns The display scale, where 1.0 is the reported size
	double display_scale() const;

	/// @brief Returns if object pools drawn for another data mask size should be scaled to the reported size
	/// @returns true when the pools should be scaled, false to draw them exactly as the working set sent them
	bool get_scale_pools_to_data_mask() const;

	static constexpr double MINIMUM_WORKING_SET_SELECTOR_SCALE = 0.75; ///< Floor for the picker column's own scale

	/// @brief Returns display_scale(), floored so the working-set picker stays usable on a touch screen
	/// @returns The working-set picker's display scale
	double working_set_selector_scale() const;

	/// @brief Stops the window from being made smaller than the masks need: their reported size, or the smallest
	/// scale when they are fitted to the window
	void apply_window_size_limits();

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
		GenerateLogPackageFromCurrentSession,
		ClearISOData,
		ConfigureCANHardware,
		StartStop,
		AutoStart,
		CheckForUpdates,
		AutoCheckForUpdates,
		AlwaysOnTop,
		FitMasksToWindow,
		ScalePoolsToDataMask
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
		bool operator==(const HeldButtonData &other) const;
		std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> associatedWorkingSet;
		std::uint32_t timestamp_ms;
		std::uint16_t buttonObjectID;
		std::uint16_t activeMaskObjectID;
		std::uint8_t buttonKeyCode;
		bool isSoftKey;
	};

	static VTVersion get_version_from_setting(std::uint8_t aVersion);

	std::size_t number_of_iop_files_in_directory(std::filesystem::path path);

	/// @brief The client's stored object pool directory, which may not exist yet
	std::string get_client_iso_data_path(isobus::NAME clientNAME) const;

	bool timeAndDateCallback(isobus::TimeDateInterface::TimeAndDate &timeAndDateToPopulate);
	void transferred_object_pool_parse_start(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet) const override;

	void on_change_active_mask_callback(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t workingSet, std::uint16_t newMask);

	/// @brief Called from the CAN thread when a client changes a soft key mask, hands the change to the message thread
	/// @param[in] affectedWorkingSet The working set whose mask was changed
	/// @param[in] mask The data or alarm mask that was given a new soft key mask
	/// @param[in] newSoftKeyMask The new soft key mask
	void on_change_active_softkey_mask_callback(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t mask, std::uint16_t newSoftKeyMask);

	/// @brief Redraws the soft keys when the changed mask is the one on screen. Runs on the message thread.
	/// @param[in] affectedWorkingSet The working set whose mask was changed
	/// @param[in] mask The data or alarm mask that was given a new soft key mask
	/// @param[in] newSoftKeyMask The new soft key mask
	void change_active_softkey_mask(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t mask, std::uint16_t newSoftKeyMask);

	/// @brief Restores the window position, size and maximised state which were saved the last
	/// time the program ran.
	/// @attention Like the always on top setting, this cannot be done during construction,
	/// because this component is not inside its window yet at that point.
	void apply_window_state();

	/// @brief Returns the position, size and maximised state of the window containing this
	/// component, in the form used by the settings file.
	/// @returns The window state, or an empty string if there is no window yet
	juce::String get_window_state() const;

	/// @brief Applies the always on top setting to the window which contains this component.
	/// @attention This cannot be done while this component is being constructed, because it is
	/// not inside its window yet at that point.
	void apply_always_on_top();

	void repaint_data_and_soft_key_mask();
	bool is_active_alarm_mask() const;
	void update_ack_button_visibility();
	void check_load_settings(std::shared_ptr<ValueTree> settings);

	/// @brief Asks GitHub if a newer release is available and tells the user if there is one
	/// @param[in] reportWhenUpToDate If true, also show a message when this is already the latest release
	void check_for_update(bool reportWhenUpToDate);

	/// @brief Handles the result of a check started by check_for_update(), on the message thread
	/// @param[in] result The outcome of the check
	/// @param[in] reportWhenUpToDate If true, also show a message when this is already the latest release
	void on_update_check_complete(const UpdateChecker::Result &result, bool reportWhenUpToDate);

	/// @brief Stops the CAN interface, keeps the configured frame handlers and resets the started state
	void stop_can_interface();

	/// @brief Stops the CAN interface and tells the user why, using the last error of the channel 0 driver
	/// @param[in] title The title of the alert
	void stop_can_interface_with_alert(const String &title);

	/// @brief Starts the CAN interface, and stops it again with an alert if a channel could not be opened
	void start_can_interface();

	/// @brief Removes a working set from the renderers and the selector, and selects another one if none is active
	void stop_presenting_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet);
	void send_held_button_activation(const HeldButtonData &button, KeyActivationCode activationCode);
	void clear_iso_data();

	static constexpr int CAN_STATUS_INDICATOR_WIDTH = 150;
	static constexpr juce::int64 UPDATE_CHECK_INTERVAL_MS = 60LL * 60LL * 1000LL;
	const std::string ISO_DATA_PATH = "iso_data";
	std::string screenCaptureDirArgument = "";
	std::string canLogPath;

	juce::ApplicationCommandManager mCommandManager;
	UpdateChecker updateChecker;
	WorkingSetSelectorComponent workingSetSelector;
	DataMaskRenderAreaComponent dataMaskRenderer;
	SoftKeyMaskRenderAreaComponent softKeyMaskRenderer;
	MenuBarComponent menuBar;
	LoggerComponent logger;
	Viewport loggerViewport;
	VT_NumberComponent vtNumberComponent;
	static constexpr int RESIZE_GRIP_SIZE = 44; ///< Large enough to drag with a finger
	std::unique_ptr<juce::ResizableCornerComponent> resizeGrip; ///< Replaces the window's 18 pixel corner resizer
	SoundPlayer mSoundPlayer;
	AudioDeviceManager mAudioDeviceManager;
	std::unique_ptr<isobus::TimeDateInterface> timeServingInterface;
	std::unique_ptr<isobus::DiagnosticProtocol> diagnosticProtocol;
	std::unique_ptr<AlertWindow> popupMenu;
	std::unique_ptr<ConfigureHardwareWindow> configureHardwareWindow;
	std::shared_ptr<isobus::ControlFunction> alarmAckKeyWs;
	std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &parentCANDrivers;
	std::vector<HeldButtonData> heldButtons;
	std::set<std::string> loadedNames;
	std::set<std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet>> rejectedWorkingSets; ///< Their last object pool was rejected, so they are never selected automatically
	std::uint32_t alarmAckKeyMaskId = isobus::NULL_OBJECT_ID;
	int alarmAckKeyCode = juce::KeyPress::escapeKey;
	juce::int64 lastUpdateCheck = 0;
	juce::String skippedUpdateVersion;
	std::uint8_t vtNumber = 1; // VT number in the range of 1-32
	std::uint8_t numberOfPoolsToRender = 0;
	VTVersion versionToReport = VTVersion::Version5;
	bool needToRepaint = false;
	bool canAdapterConnected = false;
	bool canInterfaceRunning = false;
	bool autostart = false;
	bool hasStartBeenCalled = false;
	bool alarmAckKeyPressed = false;
	bool showAckButton = false;
	bool saveIopBeforeParse = false;
	bool checkForUpdatesOnStartup = true;
	bool alwaysOnTop = false;
	bool fitMasksToWindow = false; ///< Scale the masks to fill the window instead of drawing them at their reported size
	bool scalePoolsToDataMask = false; ///< Scale pools drawn for another data mask size to the reported one
	bool needToApplyAlwaysOnTop = true; ///< Set when the window still has to be told about the setting
	juce::String savedWindowState; ///< The window geometry loaded from the settings file
	bool needToApplyWindowState = true; ///< Set until the saved geometry has been given to the window

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ServerMainComponent)
};

#endif // SERVER_MAIN_COMPONENT_HPP
