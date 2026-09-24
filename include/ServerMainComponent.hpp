#pragma once

#include "ConfigureHardwareWindow.hpp"
#include "DataMaskRenderAreaComponent.hpp"
#include "LoggerComponent.hpp"
#include "SettingsPageComponent.hpp"
#include "SoftKeyMaskComponent.hpp"
#include "SoftKeyMaskRenderAreaComponent.hpp"
#include "VT_NumberComponent.hpp"
#include "WorkingSetSelectorComponent.hpp"
#include "isobus/isobus/isobus_diagnostic_protocol.hpp"
#include "isobus/isobus/isobus_language_command_interface.hpp"
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

	std::uint8_t get_user_layout_datamask_bg_color() const override;
	std::uint8_t get_user_layout_softkeymask_bg_color() const override;

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

	void send_alarm_ack_command(isobus::VirtualTerminalBase::KeyActivationCode activationCode);

	void set_button_held(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t objectID, std::uint16_t maskObjectID, std::uint8_t keyCode, bool isSoftKey);
	void set_button_released(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t objectID, std::uint16_t maskObjectID, std::uint8_t keyCode, bool isSoftKey);

	void repaint_on_next_update();

	void save_settings();

	void identify_vt() override;

	void screen_capture(std::uint8_t item, std::uint8_t path, std::shared_ptr<isobus::ControlFunction> requestor) override;

	static std::string getAppDataDir();
	/**
   * @brief minimum_height
   * @return the height of the softkey- or the datamask size, whichever is bigger
   */
	int minimum_height() const;

	/// @brief Gives the settings page access to invoke/query the existing menu commands, so
	/// simple rows (toggles, actions) stay backed by the same logic as the menu bar rather than
	/// duplicating it. See docs/touch-settings-page.md.
	/// @returns The command manager that also backs the menu bar
	juce::ApplicationCommandManager &get_command_manager();

	/// @brief Shows the full-window touch settings page, hiding the normal VT view behind it.
	/// See docs/touch-settings-page.md.
	void open_settings_page();

	/// @brief Hides the touch settings page and restores the normal VT view.
	void close_settings_page();

	// The Control checkboxes on the settings page are small dedicated wrappers, rather than the
	// settings page invoking the menu bar's CommandIDs directly, so the enum itself does not need
	// to become public. perform() calls these same methods, so there is exactly one place each
	// setting's toggle logic lives.
	bool get_autostart() const;
	void toggle_autostart();
	bool get_always_on_top() const;
	void toggle_always_on_top();

	/// @brief Whether the desktop menu bar is currently hidden in favour of the touch settings
	/// page's cogwheel. Not yet persisted between runs - see docs/touch-settings-page.md.
	bool get_menu_bar_hidden() const;
	void toggle_menu_bar_hidden();

	// These three route through mCommandManager.invokeDirectly() to the same CommandIDs the menu
	// bar's Troubleshooting items use, so the settings page's buttons and the menu bar share one
	// implementation instead of two.
	void generate_diagnostic_package();
	void generate_diagnostic_package_from_current_session();
	void request_clear_iso_data();

	// Configuration section wrappers - see docs/touch-settings-page.md. VT version and VT number
	// only take effect on restart, matching the popup they replace; the settings page is
	// responsible for saying so, not this class.
	int get_reported_version_index() const;
	void set_reported_version_index(int index);
	int get_vt_number() const;
	void set_vt_number(int number);

	/// @brief Opens the existing CAN hardware selection dialog. Not yet migrated to an inline row -
	/// see docs/touch-settings-page.md's phasing.
	void open_can_hardware_configuration();
	bool get_can_hardware_configurable() const;

	/// @brief Whether the CAN interface is currently running. Hardware can only be reconfigured
	/// while it is stopped, so the settings page offers this right next to the CAN hardware row.
	bool get_can_interface_started() const;
	void toggle_can_interface();

	int get_log_level_index() const;
	void set_log_level_index(int index);
	bool get_log_window_visible() const;
	void toggle_log_window_visible();
	bool get_save_iop_before_parse() const;
	void toggle_save_iop_before_parse();

	bool get_show_ack_button() const;
	void toggle_show_ack_button();
	int get_alarm_ack_key_code() const;
	void set_alarm_ack_key_code(int keyCode);

	/// @brief Gives the settings page direct access to the ISOBUS language/units settings, so it
	/// can reuse the same get/set methods the popup this replaces already used, rather than adding
	/// a pass-through wrapper for each of the dozen fields.
	isobus::LanguageCommandInterface &get_language_command_interface();

	/// @brief Applies new data mask / soft key mask dimensions, reproducing the reported-hardware
	/// popup's exact logic (mask sizing, soft key dimension recompute, JuceManagedWorkingSetCache
	/// update) - see docs/touch-settings-page.md. The individual current values are already
	/// available via the public get_data_mask_area_size_x_pixels(), get_soft_key_descriptor_x_
	/// pixel_width(), get_soft_key_descriptor_y_pixel_height(), get_physical_soft_key_columns() and
	/// get_physical_soft_key_rows(), so no new getters are needed alongside this setter.
	/// @attention Like the popup this replaces, some clients may show discrepancies until the app
	/// is restarted.
	void set_hardware_capabilities(int dataMaskSize, int softKeyDesignatorWidth, int softKeyDesignatorHeight, int softKeyColumns, int softKeyRows);

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
		AlwaysOnTop
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

	/// @brief A larger, touch-friendly grab handle that resizes the window from its bottom-left
	/// corner, mirroring juce::ResizableCornerComponent's bottom-right behaviour (which cannot be
	/// reused directly - it always grows from the top-left, regardless of where it is placed).
	/// Lives over the working set selector column rather than the data mask, so it never sits on
	/// top of rendered VT content - see setup_touch_resize_corner().
	class BottomLeftResizeCorner : public juce::Component
	{
	public:
		BottomLeftResizeCorner(juce::Component &componentToResize, juce::ComponentBoundsConstrainer &boundsConstrainer);

		void paint(juce::Graphics &g) override;
		void mouseDown(const juce::MouseEvent &event) override;
		void mouseDrag(const juce::MouseEvent &event) override;
		void mouseUp(const juce::MouseEvent &event) override;

	private:
		juce::Component &component;
		juce::ComponentBoundsConstrainer &constrainer;
		juce::Rectangle<int> originalBounds;
	};

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

	bool timeAndDateCallback(isobus::TimeDateInterface::TimeAndDate &timeAndDateToPopulate);
	void transferred_object_pool_parse_start(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet) const override;

	void on_change_active_mask_callback(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t workingSet, std::uint16_t newMask);

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

	/// @brief Creates a larger touch-friendly grab handle in the bottom-right corner for resizing
	/// the window, since the native OS resize border is thin and easy to miss with a finger.
	/// @attention Like the always on top setting, this cannot be done during construction, because
	/// this component is not inside its window yet at that point.
	void setup_touch_resize_corner();

	void repaint_data_and_soft_key_mask();
	bool is_active_alarm_mask() const;
	void update_ack_button_visibility();
	void check_load_settings(std::shared_ptr<ValueTree> settings);
	void remove_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSetToRemove);
	void clear_iso_data();

	static constexpr int CAN_STATUS_INDICATOR_WIDTH = 150;
	const std::string ISO_DATA_PATH = "iso_data";
	std::string screenCaptureDirArgument = "";
	std::string canLogPath;

	juce::ApplicationCommandManager mCommandManager;
	WorkingSetSelectorComponent workingSetSelector;
	DataMaskRenderAreaComponent dataMaskRenderer;
	SoftKeyMaskRenderAreaComponent softKeyMaskRenderer;
	SettingsPageComponent settingsPage;
	MenuBarComponent menuBar;
	LoggerComponent logger;
	Viewport loggerViewport;
	VT_NumberComponent vtNumberComponent;
	SoundPlayer mSoundPlayer;
	AudioDeviceManager mAudioDeviceManager;
	std::unique_ptr<isobus::TimeDateInterface> timeServingInterface;
	std::unique_ptr<isobus::DiagnosticProtocol> diagnosticProtocol;
	std::unique_ptr<AlertWindow> popupMenu;
	std::unique_ptr<ConfigureHardwareWindow> configureHardwareWindow;
	juce::ComponentBoundsConstrainer touchResizeConstrainer;
	std::unique_ptr<BottomLeftResizeCorner> touchResizeCorner;
	std::shared_ptr<isobus::ControlFunction> alarmAckKeyWs;
	std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &parentCANDrivers;
	std::vector<HeldButtonData> heldButtons;
	std::set<std::string> loadedNames;
	std::set<const isobus::VirtualTerminalServerManagedWorkingSet *> loadVersionResponsesSent;
	std::uint32_t alarmAckKeyMaskId = isobus::NULL_OBJECT_ID;
	int alarmAckKeyCode = juce::KeyPress::escapeKey;
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
	bool alwaysOnTop = false;
	bool menuBarHidden = false; ///< See get_menu_bar_hidden() - not yet persisted between runs
	bool needToApplyAlwaysOnTop = true; ///< Set when the window still has to be told about the setting
	juce::String savedWindowState; ///< The window geometry loaded from the settings file
	bool needToApplyWindowState = true; ///< Set until the saved geometry has been given to the window
	bool needToSetupTouchResizeCorner = true; ///< Set until setup_touch_resize_corner() has run

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ServerMainComponent)
};
