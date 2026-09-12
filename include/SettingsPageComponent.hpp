//================================================================================================
/// @file SettingsPageComponent.hpp
///
/// @brief Defines the full-window touch settings page. See docs/touch-settings-page.md for the
/// design this implements.
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef SETTINGS_PAGE_COMPONENT_HPP
#define SETTINGS_PAGE_COMPONENT_HPP

#include "JuceHeader.h"
#include "NumericKeypadComponent.hpp"

#include <functional>
#include <memory>

class ServerMainComponent;

/// @brief Full-window settings page shown in place of the menu bar's popups, opened by the
/// cogwheel next to the ACK button (see WorkingSetSelectorComponent).
///
/// @details See docs/touch-settings-page.md for the full design and phasing:
/// - Four tabs (App/ISOBUS/Hardware/Troubleshooting) each show one full-width scrollable page,
///   rather than the two side-by-side panes the design doc originally sketched - the doc's own
///   Configuration/Control split didn't leave enough room once ISOBUS Language/Units grew to
///   twenty-odd rows, and cramming unrelated settings into fixed-width columns read as arbitrary.
/// - CAN hardware selection still opens the existing dialog rather than being an inline row -
///   migrating that dialog (it enumerates OS-specific adapters) is follow-up work.
/// - The locale and units rows are a curated picker/master-switch with a "Custom..." escape hatch
///   that reveals the underlying fields for direct override - see apply_locale_selection() and
///   apply_units_system_selection(). The curated locale list is plain text for now; a flag+name
///   picker needs the flag-icons vendoring described in the design doc and is follow-up work.
/// - RESTORE DEFAULTS/CANCEL/APPLY do not yet stage changes; every row currently applies
///   immediately, and the buttons only close the page. Staged apply (and the "restart required"
///   warning APPLY needs to give for settings like VT number/version and Hide Menu Bar) is
///   follow-up work.
class SettingsPageComponent : public juce::Component
{
public:
	explicit SettingsPageComponent(ServerMainComponent &server);

	void paint(juce::Graphics &g) override;
	void resized() override;
	bool keyPressed(const juce::KeyPress &key) override;

	/// @brief Refreshes every row from the underlying settings and resets the delete-pools and
	/// ACK-key-capture transient states, since the settings may have changed since this page was
	/// last shown (e.g. via the menu bar, while both remain available). Call before showing the page.
	void refresh_from_current_settings();

private:
	enum class Tab
	{
		App,
		Isobus,
		Hardware,
		Troubleshooting
	};

	void set_tab(Tab tab);
	void update_tab_button_states();
	void begin_alarm_ack_key_capture();
	void update_alarm_ack_key_button_text();
	void update_vt_number_button_text();
	void update_can_interface_controls();

	/// @brief Reads every ISOBUS Language/Units row from ownerServer's LanguageCommandInterface.
	/// Called on refresh_from_current_settings(), since the interface is the single source of
	/// truth (the language command can also be reconfigured via the menu bar while this page
	/// remains open elsewhere).
	void refresh_language_and_units_pane();

	/// @brief Applies every ISOBUS Language/Units row to ownerServer's LanguageCommandInterface and
	/// re-sends the language command - called from each row's own onChange, matching how every
	/// other row on this page applies immediately (see the class comment on staged apply).
	void apply_language_and_units_pane();

	/// @brief Applies the localeCombo selection: a curated entry fills in and applies the language/
	/// country codes directly; "Custom" instead reveals languageCodeEditor/countryCodeEditor for
	/// manual entry and leaves the codes as they were until the operator types something.
	void apply_locale_selection();
	void set_custom_locale_rows_visible(bool visible);

	/// @brief Applies the unitsSystemCombo selection: Metric/US/Imperial sets and applies every
	/// individual unit combo below to match (they differ only where the protocol actually
	/// distinguishes US from Imperial - see apply_units_system_selection()'s implementation);
	/// "Custom" instead reveals them for individual override, unchanged.
	void apply_units_system_selection();
	void set_unit_detail_rows_visible(bool visible);

	/// @brief Refreshes every Hardware tab button's displayed value from ownerServer's current
	/// renderer/soft key dimensions. See refresh_language_and_units_pane() for why this is a
	/// separate read pass.
	void refresh_hardware_capabilities_pane();

	/// @brief Opens the numeric keypad overlay pre-filled with one Hardware dimension, applying it
	/// via ownerServer.set_hardware_capabilities() (re-reading the other four current values from
	/// their own getters) when confirmed - called from that field's own button.
	enum class HardwareField
	{
		DataMaskSize,
		SoftKeyWidth,
		SoftKeyHeight,
		SoftKeyColumns,
		SoftKeyRows
	};
	void begin_hardware_field_edit(HardwareField field);

	/// @brief Opens the numeric keypad overlay - see docs/touch-settings-page.md's "No popups
	/// anywhere" principle; this is a child component shown over the page rather than a separate
	/// modal window, matching how the same keypad is used elsewhere in the app. Shared by every
	/// numeric field on this page (VT number, data mask size, soft key dimensions) rather than each
	/// having its own copy - onApply receives the entered value, already clamped to [minimum, maximum].
	void begin_numeric_edit(double currentValue, double minimum, double maximum, std::uint8_t decimals, std::function<void(double)> onApply);
	void end_numeric_edit(bool apply);

	ServerMainComponent &ownerServer;
	Tab currentTab = Tab::App;
	bool clearIsoDataArmed = false; ///< True after the first tap on Clear ISO Data, until the second tap or a timeout
	bool capturingAlarmAckKey = false; ///< True between tapping the Alarm ACK key button and the next keypress

	// Top bar - the tab buttons live here rather than their own row, using the horizontal space
	// next to the title that four side-by-side panes would otherwise have left empty.
	juce::TextButton backButton{ "<" };
	juce::Label titleLabel;
	juce::TextButton appTabButton{ "App" };
	juce::TextButton isobusTabButton{ "ISOBUS" };
	juce::TextButton hardwareTabButton{ "Hardware" };
	juce::TextButton troubleshootingTabButton{ "Troubleshooting" };

	// Every tab's content lives in one Viewport, full width, showing only the current tab's rows -
	// see set_tab(). Scrolled because the ISOBUS tab alone is over twenty rows once locale and
	// units are both expanded to Custom.
	juce::Viewport pageViewport;
	juce::Component pageContent;

	// App tab
	juce::Label appLanguageLabel{ {}, "App language" };
	juce::Label appLanguageValueLabel{ {}, "Follow System" }; ///< Static placeholder - app language selection isn't implemented yet
	juce::Label logLevelLabel{ {}, "Log level" };
	juce::ComboBox logLevelCombo;
	juce::ToggleButton logWindowToggle{ "Show log window" };
	juce::ToggleButton autoStartToggle{ "Auto-start VT on launch" };
	juce::ToggleButton alwaysOnTopToggle{ "Always on top" };
	juce::ToggleButton hideMenuBarToggle{ "Hide menu bar" };
	juce::ToggleButton showAckButtonToggle{ "Show ACK button on alarms" };
	juce::Label alarmAckKeyLabel{ {}, "Alarm ACK key" };
	juce::TextButton alarmAckKeyButton;

	// ISOBUS tab - VT identity
	juce::Label vtVersionLabel{ {}, "VT version" };
	juce::ComboBox vtVersionCombo;
	juce::Label vtNumberLabel{ {}, "VT number" };
	juce::TextButton vtNumberButton; ///< Opens the numeric keypad overlay, rather than a text field

	// ISOBUS tab - locale. localeCombo picks from a curated list of common language/country pairs
	// (see the class comment on the flag-icons follow-up). Selecting "Custom..." (the last entry)
	// reveals languageCodeEditor/countryCodeEditor for direct entry instead - the protocol permits
	// pairs no curated list will cover.
	juce::Label localeSectionLabel{ {}, "LOCALE" };
	juce::Label localeLabel{ {}, "Language" };
	juce::ComboBox localeCombo;
	juce::Label languageCodeLabel{ {}, "Language code" };
	juce::TextEditor languageCodeEditor;
	juce::Label countryCodeLabel{ {}, "Country code" };
	juce::TextEditor countryCodeEditor;
	juce::Label decimalSymbolLabel{ {}, "Decimal symbol" };
	juce::ComboBox decimalSymbolCombo;
	juce::Label dateFormatLabel{ {}, "Date format" };
	juce::ComboBox dateFormatCombo;
	juce::Label timeFormatLabel{ {}, "Time format" };
	juce::ComboBox timeFormatCombo;

	// ISOBUS tab - units. unitsSystemCombo picks Metric/US/Imperial, which sets and applies every
	// unit category below it in one go - most installations use one consistent system throughout,
	// so requiring all eight to be set individually is needless friction. Selecting "Custom..."
	// (the last entry) reveals them for individual override instead.
	juce::Label unitsSectionLabel{ {}, "UNITS" };
	juce::Label unitsSystemLabel{ {}, "Units" };
	juce::ComboBox unitsSystemCombo;
	juce::Label genericUnitsLabel{ {}, "Generic units" };
	juce::ComboBox genericUnitsCombo;
	juce::Label distanceUnitsLabel{ {}, "Distance units" };
	juce::ComboBox distanceUnitsCombo;
	juce::Label areaUnitsLabel{ {}, "Area units" };
	juce::ComboBox areaUnitsCombo;
	juce::Label volumeUnitsLabel{ {}, "Volume units" };
	juce::ComboBox volumeUnitsCombo;
	juce::Label massUnitsLabel{ {}, "Mass units" };
	juce::ComboBox massUnitsCombo;
	juce::Label temperatureUnitsLabel{ {}, "Temperature units" };
	juce::ComboBox temperatureUnitsCombo;
	juce::Label pressureUnitsLabel{ {}, "Pressure units" };
	juce::ComboBox pressureUnitsCombo;
	juce::Label forceUnitsLabel{ {}, "Force units" };
	juce::ComboBox forceUnitsCombo;

	// Hardware tab - CAN interface
	juce::Label canHardwareLabel{ {}, "CAN hardware" };
	juce::TextButton canInterfaceToggleButton; ///< Hardware can only be reconfigured while stopped
	juce::TextButton canHardwareButton{ "Configure..." };

	// Hardware tab - data mask / soft keys. Reproduces the "Reported Hardware Capabilities"
	// popup's fields (VT number moved to the ISOBUS tab instead). Numeric keypad buttons, like VT
	// number, rather than the popup's plain text editors.
	juce::Label hardwareSectionLabel{ {}, "DATA MASK & SOFT KEYS" };
	juce::Label hardwareRestartNoteLabel; ///< "restart to fully apply" caveat, replacing the popup's description text
	juce::Label dataMaskSizeLabel{ {}, "Data mask size" };
	juce::TextButton dataMaskSizeButton;
	juce::Label softKeyWidthLabel{ {}, "Soft key width" };
	juce::TextButton softKeyWidthButton;
	juce::Label softKeyHeightLabel{ {}, "Soft key height" };
	juce::TextButton softKeyHeightButton;
	juce::Label softKeyColumnsLabel{ {}, "Soft key columns" };
	juce::TextButton softKeyColumnsButton;
	juce::Label softKeyRowsLabel{ {}, "Soft key rows" };
	juce::TextButton softKeyRowsButton;

	// Troubleshooting tab
	juce::TextButton diagnosticsButton{ "Full Diagnostics" };
	juce::TextButton sessionDiagnosticsButton{ "Session Diagnostics" };
	juce::ToggleButton saveIopToggle{ "Save IOP before parsing" };
	juce::TextButton clearIsoDataButton{ "Clear ISO Data" };
	juce::HyperlinkButton reportIssueButton{ "Report an issue", juce::URL("https://github.com/Open-Agriculture/AgIsoVirtualTerminal/issues/new") };

	// Bottom bar
	juce::TextButton restoreDefaultsButton{ "Restore Defaults" };
	juce::TextButton cancelButton{ "Cancel" };
	juce::TextButton applyButton{ "Apply" };
	juce::Label versionLabel;

	// Numeric keypad overlay - see begin_numeric_edit(). numericEditBackdrop sits behind the
	// keypad and OK/Cancel buttons and simply catches clicks so they cannot reach the page below.
	juce::Component numericEditBackdrop;
	std::unique_ptr<NumericKeypadComponent> numericKeypad;
	juce::TextButton numericKeypadOkButton{ "OK" };
	juce::TextButton numericKeypadCancelButton{ "Cancel" };
	std::function<void(double)> numericEditApplyCallback;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPageComponent)
};

#endif // SETTINGS_PAGE_COMPONENT_HPP
