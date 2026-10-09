//================================================================================================
/// @file SettingsPageComponent.cpp
///
/// @brief Implements the full-window touch settings page. See docs/touch-settings-page.md for the
/// design this implements.
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include "SettingsPageComponent.hpp"
#include "ServerMainComponent.hpp"

#include <cmath>
#include <iterator>

namespace
{
	constexpr int TOP_BAR_HEIGHT = 56;
	constexpr int BOTTOM_BAR_HEIGHT = 64;
	constexpr int PANE_PADDING = 16;
	constexpr int ROW_HEIGHT = 32;
	constexpr int ROW_GAP = 6;
	constexpr int SECTION_GAP = 16;
	constexpr int ROW_LABEL_WIDTH = 160;

	/// @brief One entry in the curated locale list - see the class comment on the flag-icons
	/// follow-up. A flag+name picker needs that vendoring; this is the same curated set and the
	/// same Custom escape hatch, in plain text until that follow-up work lands.
	struct LocaleEntry
	{
		const char *name;
		const char *languageCode;
		const char *countryCode;
	};

	// Taken from the languages a real commercial implement pool ships, per the design doc - not
	// exhaustive by design. A pairing this list does not cover is entered via Custom instead.
	constexpr LocaleEntry LOCALE_ENTRIES[] = {
		{ "Bulgarian", "bg", "BG" },
		{ "Czech", "cs", "CZ" },
		{ "Danish", "da", "DK" },
		{ "German", "de", "DE" },
		{ "Greek", "el", "GR" },
		{ "English", "en", "US" },
		{ "Spanish", "es", "ES" },
		{ "Estonian", "et", "EE" },
		{ "Finnish", "fi", "FI" },
		{ "French", "fr", "FR" },
		{ "Croatian", "hr", "HR" },
		{ "Hungarian", "hu", "HU" },
		{ "Italian", "it", "IT" },
		{ "Lithuanian", "lt", "LT" },
		{ "Latvian", "lv", "LV" },
		{ "Dutch", "nl", "NL" },
		{ "Norwegian", "no", "NO" },
		{ "Polish", "pl", "PL" },
		{ "Portuguese", "pt", "PT" },
		{ "Romanian", "ro", "RO" },
		{ "Russian", "ru", "RU" },
		{ "Slovak", "sk", "SK" },
		{ "Slovenian", "sl", "SI" },
		{ "Serbian", "sr", "RS" },
		{ "Swedish", "sv", "SE" },
		{ "Turkish", "tr", "TR" },
		{ "Ukrainian", "uk", "UA" },
	};
	constexpr int LOCALE_CUSTOM_INDEX = static_cast<int>(std::size(LOCALE_ENTRIES));

	constexpr int UNITS_SYSTEM_METRIC = 0;
	constexpr int UNITS_SYSTEM_US = 1;
	constexpr int UNITS_SYSTEM_IMPERIAL = 2;
	constexpr int UNITS_SYSTEM_CUSTOM = 3;
}

SettingsPageComponent::SettingsPageComponent(ServerMainComponent &server) :
  ownerServer(server)
{
	backButton.onClick = [this]() { ownerServer.close_settings_page(); };
	addAndMakeVisible(backButton);

	titleLabel.setText("Settings", juce::dontSendNotification);
	titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
	titleLabel.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(titleLabel);

	// Tab buttons live in the top bar, next to the title - see the class comment on why this
	// replaced the two side-by-side panes.
	const auto setUpTabButton = [this](juce::TextButton &button, Tab tab) {
		button.setClickingTogglesState(false);
		button.onClick = [this, tab]() { set_tab(tab); };
		addAndMakeVisible(button);
	};
	setUpTabButton(appTabButton, Tab::App);
	setUpTabButton(isobusTabButton, Tab::Isobus);
	setUpTabButton(hardwareTabButton, Tab::Hardware);
	setUpTabButton(troubleshootingTabButton, Tab::Troubleshooting);

	addAndMakeVisible(pageViewport);
	pageViewport.setViewedComponent(&pageContent, false);
	pageViewport.setScrollBarThickness(32); // wide enough to grab on a touchscreen

	const auto setUpRow = [this](juce::Label &label, juce::Component &control) {
		label.setJustificationType(juce::Justification::centredLeft);
		pageContent.addAndMakeVisible(label);
		pageContent.addAndMakeVisible(control);
	};
	const auto styleSectionHeader = [](juce::Label &label) {
		label.setFont(juce::Font(14.0f, juce::Font::bold));
		label.setColour(juce::Label::textColourId, juce::Colours::white);
		label.setJustificationType(juce::Justification::centredLeft);
	};

	// App tab
	appLanguageLabel.setJustificationType(juce::Justification::centredLeft);
	pageContent.addAndMakeVisible(appLanguageLabel);
	appLanguageValueLabel.setJustificationType(juce::Justification::centredRight);
	appLanguageValueLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
	pageContent.addAndMakeVisible(appLanguageValueLabel);

	logLevelLabel.setJustificationType(juce::Justification::centredLeft);
	pageContent.addAndMakeVisible(logLevelLabel);
	logLevelCombo.addItemList({ "Debug", "Info", "Warning", "Error", "Critical" }, 1);
	logLevelCombo.onChange = [this]() { ownerServer.set_log_level_index(logLevelCombo.getSelectedItemIndex()); };
	pageContent.addAndMakeVisible(logLevelCombo);

	logWindowToggle.onClick = [this]() { ownerServer.toggle_log_window_visible(); };
	pageContent.addAndMakeVisible(logWindowToggle);

	autoStartToggle.onClick = [this]() { ownerServer.toggle_autostart(); };
	pageContent.addAndMakeVisible(autoStartToggle);

	alwaysOnTopToggle.onClick = [this]() { ownerServer.toggle_always_on_top(); };
	pageContent.addAndMakeVisible(alwaysOnTopToggle);

	hideMenuBarToggle.onClick = [this]() { ownerServer.toggle_menu_bar_hidden(); };
	pageContent.addAndMakeVisible(hideMenuBarToggle);

	showAckButtonToggle.onClick = [this]() { ownerServer.toggle_show_ack_button(); };
	pageContent.addAndMakeVisible(showAckButtonToggle);

	alarmAckKeyLabel.setJustificationType(juce::Justification::centredLeft);
	pageContent.addAndMakeVisible(alarmAckKeyLabel);
	alarmAckKeyButton.onClick = [this]() { begin_alarm_ack_key_capture(); };
	pageContent.addAndMakeVisible(alarmAckKeyButton);

	// ISOBUS tab - VT identity
	vtVersionLabel.setJustificationType(juce::Justification::centredLeft);
	pageContent.addAndMakeVisible(vtVersionLabel);
	vtVersionCombo.addItemList({ "Version 2 or Older", "Version 3", "Version 4", "Version 5", "Version 6" }, 1);
	vtVersionCombo.onChange = [this]() { ownerServer.set_reported_version_index(vtVersionCombo.getSelectedItemIndex()); };
	pageContent.addAndMakeVisible(vtVersionCombo);

	vtNumberLabel.setJustificationType(juce::Justification::centredLeft);
	pageContent.addAndMakeVisible(vtNumberLabel);
	vtNumberButton.onClick = [this]() {
		begin_numeric_edit(ownerServer.get_vt_number(), 1.0, 32.0, 0, [this](double value) {
			ownerServer.set_vt_number(static_cast<int>(std::lround(value)));
			update_vt_number_button_text();
		});
	};
	pageContent.addAndMakeVisible(vtNumberButton);

	// ISOBUS tab - locale
	styleSectionHeader(localeSectionLabel);
	pageContent.addAndMakeVisible(localeSectionLabel);

	for (const auto &entry : LOCALE_ENTRIES)
	{
		localeCombo.addItem(juce::String(entry.name) + " (" + entry.languageCode + "/" + entry.countryCode + ")",
		                    localeCombo.getNumItems() + 1);
	}
	localeCombo.addItem("Custom...", localeCombo.getNumItems() + 1);
	localeCombo.onChange = [this]() { apply_locale_selection(); };
	setUpRow(localeLabel, localeCombo);

	languageCodeEditor.setInputRestrictions(2);
	languageCodeEditor.onFocusLost = [this]() { apply_language_and_units_pane(); };
	languageCodeEditor.onReturnKey = [this]() { apply_language_and_units_pane(); };
	setUpRow(languageCodeLabel, languageCodeEditor);

	countryCodeEditor.setInputRestrictions(2);
	countryCodeEditor.onFocusLost = [this]() { apply_language_and_units_pane(); };
	countryCodeEditor.onReturnKey = [this]() { apply_language_and_units_pane(); };
	setUpRow(countryCodeLabel, countryCodeEditor);

	decimalSymbolCombo.addItemList({ "Comma", "Point" }, 1);
	decimalSymbolCombo.onChange = [this]() { apply_language_and_units_pane(); };
	setUpRow(decimalSymbolLabel, decimalSymbolCombo);

	dateFormatCombo.addItemList({ "ddmmyyyy", "ddyyyymm", "mmyyyydd", "mmddyyyy", "yyyymmdd", "yyyyddmm" }, 1);
	dateFormatCombo.onChange = [this]() { apply_language_and_units_pane(); };
	setUpRow(dateFormatLabel, dateFormatCombo);

	timeFormatCombo.addItemList({ "24 hour", "12 hour" }, 1);
	timeFormatCombo.onChange = [this]() { apply_language_and_units_pane(); };
	setUpRow(timeFormatLabel, timeFormatCombo);

	// ISOBUS tab - units
	styleSectionHeader(unitsSectionLabel);
	pageContent.addAndMakeVisible(unitsSectionLabel);

	unitsSystemCombo.addItemList({ "Metric", "US", "Imperial", "Custom..." }, 1);
	unitsSystemCombo.onChange = [this]() { apply_units_system_selection(); };
	setUpRow(unitsSystemLabel, unitsSystemCombo);

	genericUnitsCombo.addItemList({ "Metric", "Imperial", "US" }, 1);
	genericUnitsCombo.onChange = [this]() { apply_language_and_units_pane(); };
	setUpRow(genericUnitsLabel, genericUnitsCombo);

	distanceUnitsCombo.addItemList({ "Metric", "Imperial/US" }, 1);
	distanceUnitsCombo.onChange = [this]() { apply_language_and_units_pane(); };
	setUpRow(distanceUnitsLabel, distanceUnitsCombo);

	areaUnitsCombo.addItemList({ "Metric", "Imperial/US" }, 1);
	areaUnitsCombo.onChange = [this]() { apply_language_and_units_pane(); };
	setUpRow(areaUnitsLabel, areaUnitsCombo);

	volumeUnitsCombo.addItemList({ "Metric", "Imperial", "US" }, 1);
	volumeUnitsCombo.onChange = [this]() { apply_language_and_units_pane(); };
	setUpRow(volumeUnitsLabel, volumeUnitsCombo);

	massUnitsCombo.addItemList({ "Metric", "Imperial", "US" }, 1);
	massUnitsCombo.onChange = [this]() { apply_language_and_units_pane(); };
	setUpRow(massUnitsLabel, massUnitsCombo);

	temperatureUnitsCombo.addItemList({ "Metric", "Imperial/US" }, 1);
	temperatureUnitsCombo.onChange = [this]() { apply_language_and_units_pane(); };
	setUpRow(temperatureUnitsLabel, temperatureUnitsCombo);

	pressureUnitsCombo.addItemList({ "Metric", "Imperial/US" }, 1);
	pressureUnitsCombo.onChange = [this]() { apply_language_and_units_pane(); };
	setUpRow(pressureUnitsLabel, pressureUnitsCombo);

	forceUnitsCombo.addItemList({ "Metric", "Imperial/US" }, 1);
	forceUnitsCombo.onChange = [this]() { apply_language_and_units_pane(); };
	setUpRow(forceUnitsLabel, forceUnitsCombo);

	// Hardware tab - CAN interface
	canHardwareLabel.setJustificationType(juce::Justification::centredLeft);
	pageContent.addAndMakeVisible(canHardwareLabel);
	canInterfaceToggleButton.onClick = [this]() {
		ownerServer.toggle_can_interface();
		update_can_interface_controls();
	};
	pageContent.addAndMakeVisible(canInterfaceToggleButton);
	canHardwareButton.onClick = [this]() { ownerServer.open_can_hardware_configuration(); };
	pageContent.addAndMakeVisible(canHardwareButton);

	// Hardware tab - data mask / soft keys
	styleSectionHeader(hardwareSectionLabel);
	pageContent.addAndMakeVisible(hardwareSectionLabel);

	hardwareRestartNoteLabel.setText("Some changes here need an app restart to fully apply.", juce::dontSendNotification);
	hardwareRestartNoteLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
	hardwareRestartNoteLabel.setJustificationType(juce::Justification::centredLeft);
	hardwareRestartNoteLabel.setMinimumHorizontalScale(1.0f);
	pageContent.addAndMakeVisible(hardwareRestartNoteLabel);

	dataMaskSizeButton.onClick = [this]() { begin_hardware_field_edit(HardwareField::DataMaskSize); };
	setUpRow(dataMaskSizeLabel, dataMaskSizeButton);
	softKeyWidthButton.onClick = [this]() { begin_hardware_field_edit(HardwareField::SoftKeyWidth); };
	setUpRow(softKeyWidthLabel, softKeyWidthButton);
	softKeyHeightButton.onClick = [this]() { begin_hardware_field_edit(HardwareField::SoftKeyHeight); };
	setUpRow(softKeyHeightLabel, softKeyHeightButton);
	softKeyColumnsButton.onClick = [this]() { begin_hardware_field_edit(HardwareField::SoftKeyColumns); };
	setUpRow(softKeyColumnsLabel, softKeyColumnsButton);
	softKeyRowsButton.onClick = [this]() { begin_hardware_field_edit(HardwareField::SoftKeyRows); };
	setUpRow(softKeyRowsLabel, softKeyRowsButton);

	// Troubleshooting tab
	diagnosticsButton.onClick = [this]() { ownerServer.generate_diagnostic_package(); };
	pageContent.addAndMakeVisible(diagnosticsButton);

	sessionDiagnosticsButton.onClick = [this]() { ownerServer.generate_diagnostic_package_from_current_session(); };
	pageContent.addAndMakeVisible(sessionDiagnosticsButton);

	saveIopToggle.onClick = [this]() { ownerServer.toggle_save_iop_before_parse(); };
	pageContent.addAndMakeVisible(saveIopToggle);

	clearIsoDataButton.onClick = [this]() {
		if (clearIsoDataArmed)
		{
			clearIsoDataArmed = false;
			ownerServer.request_clear_iso_data();
		}
		else
		{
			clearIsoDataArmed = true;
		}
		clearIsoDataButton.setButtonText(clearIsoDataArmed ? "Tap again to confirm" : "Clear ISO Data");
	};
	pageContent.addAndMakeVisible(clearIsoDataButton);

	pageContent.addAndMakeVisible(reportIssueButton);

	// Bottom bar
	restoreDefaultsButton.onClick = [this]() { ownerServer.close_settings_page(); };
	addAndMakeVisible(restoreDefaultsButton);

	cancelButton.onClick = [this]() { ownerServer.close_settings_page(); };
	addAndMakeVisible(cancelButton);

	applyButton.onClick = [this]() { ownerServer.close_settings_page(); };
	addAndMakeVisible(applyButton);

	versionLabel.setJustificationType(juce::Justification::centredRight);
	versionLabel.setText("Version: " + juce::String(ProjectInfo::versionString), juce::dontSendNotification);
	addAndMakeVisible(versionLabel);

	// Numeric keypad overlay - built hidden, shown by begin_numeric_edit()
	numericEditBackdrop.setInterceptsMouseClicks(true, false);
	addChildComponent(numericEditBackdrop);
	numericKeypadOkButton.onClick = [this]() { end_numeric_edit(true); };
	addChildComponent(numericKeypadOkButton);
	numericKeypadCancelButton.onClick = [this]() { end_numeric_edit(false); };
	addChildComponent(numericKeypadCancelButton);

	set_tab(currentTab);
}

void SettingsPageComponent::paint(juce::Graphics &g)
{
	auto bounds = getLocalBounds();
	g.fillAll(juce::Colours::black);

	auto topBar = bounds.removeFromTop(TOP_BAR_HEIGHT);
	g.setColour(juce::Colours::darkgrey);
	g.drawHorizontalLine(topBar.getBottom(), 0.0f, static_cast<float>(getWidth()));

	auto bottomBar = bounds.removeFromBottom(BOTTOM_BAR_HEIGHT);
	g.drawHorizontalLine(bottomBar.getY(), 0.0f, static_cast<float>(getWidth()));

	if (numericEditBackdrop.isVisible())
	{
		g.setColour(juce::Colours::black.withAlpha(0.75f));
		g.fillRect(numericEditBackdrop.getBounds());
	}
}

void SettingsPageComponent::resized()
{
	auto bounds = getLocalBounds();

	auto topBar = bounds.removeFromTop(TOP_BAR_HEIGHT).reduced(PANE_PADDING, 8);
	backButton.setBounds(topBar.removeFromLeft(40));
	topBar.removeFromLeft(PANE_PADDING);
	troubleshootingTabButton.setBounds(topBar.removeFromRight(150));
	topBar.removeFromRight(6);
	hardwareTabButton.setBounds(topBar.removeFromRight(110));
	topBar.removeFromRight(6);
	isobusTabButton.setBounds(topBar.removeFromRight(100));
	topBar.removeFromRight(6);
	appTabButton.setBounds(topBar.removeFromRight(90));
	titleLabel.setBounds(topBar);

	auto bottomBar = bounds.removeFromBottom(BOTTOM_BAR_HEIGHT).reduced(PANE_PADDING, 12);
	restoreDefaultsButton.setBounds(bottomBar.removeFromLeft(140));
	applyButton.setBounds(bottomBar.removeFromRight(100));
	bottomBar.removeFromRight(PANE_PADDING);
	cancelButton.setBounds(bottomBar.removeFromRight(100));
	versionLabel.setBounds(bottomBar);

	constexpr int GENEROUS_HEIGHT = 10000; // shrunk back down to the actual content height below

	const auto layoutRow = [&](juce::Rectangle<int> &area, juce::Label &label, juce::Component &control, int controlWidth) {
		auto row = area.removeFromTop(ROW_HEIGHT);
		label.setBounds(row.removeFromLeft(ROW_LABEL_WIDTH));
		control.setBounds(row.removeFromRight(controlWidth));
		area.removeFromTop(ROW_GAP);
	};

	auto pageOuter = bounds.reduced(PANE_PADDING, 0);
	pageViewport.setBounds(pageOuter);

	const int contentWidth = juce::jmax(1, pageOuter.getWidth() - pageViewport.getScrollBarThickness());
	auto content = juce::Rectangle<int>(0, 0, contentWidth, GENEROUS_HEIGHT);
	content.removeFromTop(PANE_PADDING);

	switch (currentTab)
	{
		case Tab::App:
		{
			layoutRow(content, appLanguageLabel, appLanguageValueLabel, 200);
			{
				auto row = content.removeFromTop(ROW_HEIGHT);
				logLevelLabel.setBounds(row.removeFromLeft(ROW_LABEL_WIDTH));
				logWindowToggle.setBounds(row.removeFromRight(160));
				logLevelCombo.setBounds(row);
				content.removeFromTop(ROW_GAP);
			}
			autoStartToggle.setBounds(content.removeFromTop(ROW_HEIGHT));
			content.removeFromTop(ROW_GAP);
			alwaysOnTopToggle.setBounds(content.removeFromTop(ROW_HEIGHT));
			content.removeFromTop(ROW_GAP);
			hideMenuBarToggle.setBounds(content.removeFromTop(ROW_HEIGHT));
			content.removeFromTop(ROW_GAP);
			showAckButtonToggle.setBounds(content.removeFromTop(ROW_HEIGHT));
			content.removeFromTop(ROW_GAP);
			layoutRow(content, alarmAckKeyLabel, alarmAckKeyButton, 160);
		}
		break;

		case Tab::Isobus:
		{
			layoutRow(content, vtVersionLabel, vtVersionCombo, 200);
			layoutRow(content, vtNumberLabel, vtNumberButton, 120);
			content.removeFromTop(SECTION_GAP - ROW_GAP);

			localeSectionLabel.setBounds(content.removeFromTop(20));
			layoutRow(content, localeLabel, localeCombo, 180);
			if (languageCodeLabel.isVisible())
			{
				layoutRow(content, languageCodeLabel, languageCodeEditor, 100);
				layoutRow(content, countryCodeLabel, countryCodeEditor, 100);
			}
			layoutRow(content, decimalSymbolLabel, decimalSymbolCombo, 140);
			layoutRow(content, dateFormatLabel, dateFormatCombo, 140);
			layoutRow(content, timeFormatLabel, timeFormatCombo, 140);
			content.removeFromTop(SECTION_GAP - ROW_GAP);

			unitsSectionLabel.setBounds(content.removeFromTop(20));
			layoutRow(content, unitsSystemLabel, unitsSystemCombo, 140);
			if (genericUnitsLabel.isVisible())
			{
				layoutRow(content, genericUnitsLabel, genericUnitsCombo, 140);
				layoutRow(content, distanceUnitsLabel, distanceUnitsCombo, 140);
				layoutRow(content, areaUnitsLabel, areaUnitsCombo, 140);
				layoutRow(content, volumeUnitsLabel, volumeUnitsCombo, 140);
				layoutRow(content, massUnitsLabel, massUnitsCombo, 140);
				layoutRow(content, temperatureUnitsLabel, temperatureUnitsCombo, 140);
				layoutRow(content, pressureUnitsLabel, pressureUnitsCombo, 140);
				layoutRow(content, forceUnitsLabel, forceUnitsCombo, 140);
			}
		}
		break;

		case Tab::Hardware:
		{
			{
				auto row = content.removeFromTop(ROW_HEIGHT);
				canHardwareLabel.setBounds(row.removeFromLeft(ROW_LABEL_WIDTH));
				canHardwareButton.setBounds(row.removeFromRight(120));
				row.removeFromRight(PANE_PADDING);
				canInterfaceToggleButton.setBounds(row.removeFromRight(90));
				content.removeFromTop(ROW_GAP);
			}
			content.removeFromTop(SECTION_GAP - ROW_GAP);

			hardwareSectionLabel.setBounds(content.removeFromTop(20));
			hardwareRestartNoteLabel.setBounds(content.removeFromTop(ROW_HEIGHT));
			content.removeFromTop(ROW_GAP);
			layoutRow(content, dataMaskSizeLabel, dataMaskSizeButton, 100);
			layoutRow(content, softKeyWidthLabel, softKeyWidthButton, 100);
			layoutRow(content, softKeyHeightLabel, softKeyHeightButton, 100);
			layoutRow(content, softKeyColumnsLabel, softKeyColumnsButton, 100);
			layoutRow(content, softKeyRowsLabel, softKeyRowsButton, 100);
		}
		break;

		case Tab::Troubleshooting:
		{
			auto diagnosticsRow = content.removeFromTop(ROW_HEIGHT);
			diagnosticsButton.setBounds(diagnosticsRow.removeFromLeft(150));
			diagnosticsRow.removeFromLeft(PANE_PADDING);
			sessionDiagnosticsButton.setBounds(diagnosticsRow.removeFromLeft(170));
			content.removeFromTop(ROW_GAP);
			saveIopToggle.setBounds(content.removeFromTop(ROW_HEIGHT));
			content.removeFromTop(ROW_GAP);
			clearIsoDataButton.setBounds(content.removeFromTop(ROW_HEIGHT).removeFromLeft(150));
			content.removeFromTop(SECTION_GAP);
			reportIssueButton.setBounds(content.removeFromTop(ROW_HEIGHT).removeFromLeft(170));
		}
		break;
	}

	content.removeFromTop(PANE_PADDING);
	pageContent.setSize(contentWidth, GENEROUS_HEIGHT - content.getHeight());

	numericEditBackdrop.setBounds(getLocalBounds());
	if (nullptr != numericKeypad)
	{
		const auto centre = getLocalBounds().getCentre();
		numericKeypad->setCentrePosition(centre.x, centre.y - 20);
		numericKeypadOkButton.setBounds(numericKeypad->getX(), numericKeypad->getBottom() + PANE_PADDING, (numericKeypad->getWidth() - PANE_PADDING) / 2, 44);
		numericKeypadCancelButton.setBounds(numericKeypadOkButton.getRight() + PANE_PADDING, numericKeypadOkButton.getY(), numericKeypadOkButton.getWidth(), 44);
	}
}

void SettingsPageComponent::refresh_from_current_settings()
{
	refresh_language_and_units_pane();
	refresh_hardware_capabilities_pane();

	vtVersionCombo.setSelectedItemIndex(ownerServer.get_reported_version_index(), juce::dontSendNotification);
	update_vt_number_button_text();
	update_can_interface_controls();
	logLevelCombo.setSelectedItemIndex(ownerServer.get_log_level_index(), juce::dontSendNotification);
	logWindowToggle.setToggleState(ownerServer.get_log_window_visible(), juce::dontSendNotification);

	autoStartToggle.setToggleState(ownerServer.get_autostart(), juce::dontSendNotification);
	alwaysOnTopToggle.setToggleState(ownerServer.get_always_on_top(), juce::dontSendNotification);
	hideMenuBarToggle.setToggleState(ownerServer.get_menu_bar_hidden(), juce::dontSendNotification);
	saveIopToggle.setToggleState(ownerServer.get_save_iop_before_parse(), juce::dontSendNotification);
	showAckButtonToggle.setToggleState(ownerServer.get_show_ack_button(), juce::dontSendNotification);

	capturingAlarmAckKey = false;
	update_alarm_ack_key_button_text();

	clearIsoDataArmed = false;
	clearIsoDataButton.setButtonText("Clear ISO Data");

	if (nullptr != numericKeypad)
	{
		end_numeric_edit(false);
	}

	resized();
}

void SettingsPageComponent::set_tab(Tab tab)
{
	currentTab = tab;
	update_tab_button_states();

	const bool app = (Tab::App == tab);
	const bool isobus = (Tab::Isobus == tab);
	const bool hardware = (Tab::Hardware == tab);
	const bool troubleshooting = (Tab::Troubleshooting == tab);

	appLanguageLabel.setVisible(app);
	appLanguageValueLabel.setVisible(app);
	logLevelLabel.setVisible(app);
	logLevelCombo.setVisible(app);
	logWindowToggle.setVisible(app);
	autoStartToggle.setVisible(app);
	alwaysOnTopToggle.setVisible(app);
	hideMenuBarToggle.setVisible(app);
	showAckButtonToggle.setVisible(app);
	alarmAckKeyLabel.setVisible(app);
	alarmAckKeyButton.setVisible(app);

	vtVersionLabel.setVisible(isobus);
	vtVersionCombo.setVisible(isobus);
	vtNumberLabel.setVisible(isobus);
	vtNumberButton.setVisible(isobus);
	localeSectionLabel.setVisible(isobus);
	localeLabel.setVisible(isobus);
	localeCombo.setVisible(isobus);
	decimalSymbolLabel.setVisible(isobus);
	decimalSymbolCombo.setVisible(isobus);
	dateFormatLabel.setVisible(isobus);
	dateFormatCombo.setVisible(isobus);
	timeFormatLabel.setVisible(isobus);
	timeFormatCombo.setVisible(isobus);
	unitsSectionLabel.setVisible(isobus);
	unitsSystemLabel.setVisible(isobus);
	unitsSystemCombo.setVisible(isobus);

	// The language/country override rows and the eight unit detail rows are only shown when their
	// respective master combo is set to Custom - see set_custom_locale_rows_visible() and
	// set_unit_detail_rows_visible(). When this tab is hidden entirely, hide these too regardless
	// of that state; when it is shown again, restore them by re-checking that state.
	const bool showCustomLocaleRows = isobus && (LOCALE_CUSTOM_INDEX == localeCombo.getSelectedItemIndex());
	set_custom_locale_rows_visible(showCustomLocaleRows);
	const bool showUnitDetailRows = isobus && (UNITS_SYSTEM_CUSTOM == unitsSystemCombo.getSelectedItemIndex());
	set_unit_detail_rows_visible(showUnitDetailRows);

	canHardwareLabel.setVisible(hardware);
	canInterfaceToggleButton.setVisible(hardware);
	canHardwareButton.setVisible(hardware);
	hardwareSectionLabel.setVisible(hardware);
	hardwareRestartNoteLabel.setVisible(hardware);
	dataMaskSizeLabel.setVisible(hardware);
	dataMaskSizeButton.setVisible(hardware);
	softKeyWidthLabel.setVisible(hardware);
	softKeyWidthButton.setVisible(hardware);
	softKeyHeightLabel.setVisible(hardware);
	softKeyHeightButton.setVisible(hardware);
	softKeyColumnsLabel.setVisible(hardware);
	softKeyColumnsButton.setVisible(hardware);
	softKeyRowsLabel.setVisible(hardware);
	softKeyRowsButton.setVisible(hardware);

	diagnosticsButton.setVisible(troubleshooting);
	sessionDiagnosticsButton.setVisible(troubleshooting);
	saveIopToggle.setVisible(troubleshooting);
	clearIsoDataButton.setVisible(troubleshooting);
	reportIssueButton.setVisible(troubleshooting);

	resized();
}

void SettingsPageComponent::update_tab_button_states()
{
	const auto colourFor = [this](Tab tab) {
		return currentTab == tab ? juce::Colours::darkslateblue : juce::Colours::darkgrey;
	};
	appTabButton.setColour(juce::TextButton::buttonColourId, colourFor(Tab::App));
	isobusTabButton.setColour(juce::TextButton::buttonColourId, colourFor(Tab::Isobus));
	hardwareTabButton.setColour(juce::TextButton::buttonColourId, colourFor(Tab::Hardware));
	troubleshootingTabButton.setColour(juce::TextButton::buttonColourId, colourFor(Tab::Troubleshooting));
}

void SettingsPageComponent::begin_alarm_ack_key_capture()
{
	capturingAlarmAckKey = true;
	alarmAckKeyButton.setButtonText("Press a key...");
	setWantsKeyboardFocus(true);
	grabKeyboardFocus();
}

void SettingsPageComponent::update_alarm_ack_key_button_text()
{
	alarmAckKeyButton.setButtonText(juce::KeyPress(ownerServer.get_alarm_ack_key_code()).getTextDescription());
}

bool SettingsPageComponent::keyPressed(const juce::KeyPress &key)
{
	if (!capturingAlarmAckKey)
	{
		return false;
	}

	ownerServer.set_alarm_ack_key_code(key.getKeyCode());
	capturingAlarmAckKey = false;
	update_alarm_ack_key_button_text();
	setWantsKeyboardFocus(false);
	if (hasKeyboardFocus(false))
	{
		giveAwayKeyboardFocus();
	}
	return true;
}

void SettingsPageComponent::update_vt_number_button_text()
{
	const int number = ownerServer.get_vt_number();
	vtNumberButton.setButtonText(juce::String(number) + (1 == number ? " (Primary)" : ""));
}

void SettingsPageComponent::update_can_interface_controls()
{
	const bool started = ownerServer.get_can_interface_started();
	canInterfaceToggleButton.setButtonText(started ? "Stop" : "Start");
	canHardwareButton.setEnabled(ownerServer.get_can_hardware_configurable());
}

void SettingsPageComponent::begin_numeric_edit(double currentValue, double minimum, double maximum, std::uint8_t decimals, std::function<void(double)> onApply)
{
	numericEditApplyCallback = std::move(onApply);
	numericKeypad = std::make_unique<NumericKeypadComponent>(currentValue, minimum, maximum, decimals);
	addAndMakeVisible(*numericKeypad);
	numericEditBackdrop.setVisible(true);
	numericKeypadOkButton.setVisible(true);
	numericKeypadCancelButton.setVisible(true);
	numericEditBackdrop.toFront(false);
	numericKeypad->toFront(false);
	numericKeypadOkButton.toFront(false);
	numericKeypadCancelButton.toFront(false);
	resized();
	repaint();
}

void SettingsPageComponent::end_numeric_edit(bool apply)
{
	if (apply && (nullptr != numericKeypad) && numericEditApplyCallback)
	{
		numericEditApplyCallback(numericKeypad->get_value());
	}

	numericEditApplyCallback = nullptr;
	numericKeypad.reset();
	numericEditBackdrop.setVisible(false);
	numericKeypadOkButton.setVisible(false);
	numericKeypadCancelButton.setVisible(false);
	repaint();
}

void SettingsPageComponent::begin_hardware_field_edit(HardwareField field)
{
	double currentValue = 0.0;
	double minimum = 1.0;
	double maximum = 999.0;

	switch (field)
	{
		case HardwareField::DataMaskSize:
			currentValue = ownerServer.get_data_mask_area_size_x_pixels();
			minimum = 100.0;
			maximum = 2000.0;
			break;
		case HardwareField::SoftKeyWidth:
			currentValue = ownerServer.get_soft_key_descriptor_x_pixel_width();
			minimum = 60.0;
			break;
		case HardwareField::SoftKeyHeight:
			currentValue = ownerServer.get_soft_key_descriptor_y_pixel_height();
			minimum = 60.0;
			break;
		case HardwareField::SoftKeyColumns:
			currentValue = ownerServer.get_physical_soft_key_columns();
			maximum = 9.0;
			break;
		case HardwareField::SoftKeyRows:
			currentValue = ownerServer.get_physical_soft_key_rows();
			maximum = 99.0;
			break;
	}

	begin_numeric_edit(currentValue, minimum, maximum, 0, [this, field](double value) {
		const int newValue = static_cast<int>(std::lround(value));
		const int dataMaskSize = (HardwareField::DataMaskSize == field) ? newValue : ownerServer.get_data_mask_area_size_x_pixels();
		const int softKeyWidth = (HardwareField::SoftKeyWidth == field) ? newValue : ownerServer.get_soft_key_descriptor_x_pixel_width();
		const int softKeyHeight = (HardwareField::SoftKeyHeight == field) ? newValue : ownerServer.get_soft_key_descriptor_y_pixel_height();
		const int softKeyColumns = (HardwareField::SoftKeyColumns == field) ? newValue : ownerServer.get_physical_soft_key_columns();
		const int softKeyRows = (HardwareField::SoftKeyRows == field) ? newValue : ownerServer.get_physical_soft_key_rows();

		ownerServer.set_hardware_capabilities(dataMaskSize, softKeyWidth, softKeyHeight, softKeyColumns, softKeyRows);
		refresh_hardware_capabilities_pane();
	});
}

void SettingsPageComponent::refresh_hardware_capabilities_pane()
{
	dataMaskSizeButton.setButtonText(juce::String(ownerServer.get_data_mask_area_size_x_pixels()));
	softKeyWidthButton.setButtonText(juce::String(ownerServer.get_soft_key_descriptor_x_pixel_width()));
	softKeyHeightButton.setButtonText(juce::String(ownerServer.get_soft_key_descriptor_y_pixel_height()));
	softKeyColumnsButton.setButtonText(juce::String(ownerServer.get_physical_soft_key_columns()));
	softKeyRowsButton.setButtonText(juce::String(ownerServer.get_physical_soft_key_rows()));
}

void SettingsPageComponent::refresh_language_and_units_pane()
{
	auto &languageCommandInterface = ownerServer.get_language_command_interface();

	const auto currentLanguageCode = languageCommandInterface.get_language_code();
	const auto currentCountryCode = languageCommandInterface.get_country_code();
	languageCodeEditor.setText(currentLanguageCode, juce::dontSendNotification);
	countryCodeEditor.setText(currentCountryCode, juce::dontSendNotification);

	int matchedLocaleIndex = LOCALE_CUSTOM_INDEX;
	for (std::size_t i = 0; i < std::size(LOCALE_ENTRIES); i++)
	{
		if ((currentLanguageCode == LOCALE_ENTRIES[i].languageCode) && (currentCountryCode == LOCALE_ENTRIES[i].countryCode))
		{
			matchedLocaleIndex = static_cast<int>(i);
			break;
		}
	}
	localeCombo.setSelectedItemIndex(matchedLocaleIndex, juce::dontSendNotification);
	set_custom_locale_rows_visible(LOCALE_CUSTOM_INDEX == matchedLocaleIndex);

	decimalSymbolCombo.setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_decimal_symbol()), juce::dontSendNotification);
	dateFormatCombo.setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_date_format()), juce::dontSendNotification);
	timeFormatCombo.setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_time_format()), juce::dontSendNotification);

	const int generic = static_cast<int>(languageCommandInterface.get_commanded_generic_units());
	const int distance = static_cast<int>(languageCommandInterface.get_commanded_distance_units());
	const int area = static_cast<int>(languageCommandInterface.get_commanded_area_units());
	const int volume = static_cast<int>(languageCommandInterface.get_commanded_volume_units());
	const int mass = static_cast<int>(languageCommandInterface.get_commanded_mass_units());
	const int temperature = static_cast<int>(languageCommandInterface.get_commanded_temperature_units());
	const int pressure = static_cast<int>(languageCommandInterface.get_commanded_pressure_units());
	const int force = static_cast<int>(languageCommandInterface.get_commanded_force_units());

	genericUnitsCombo.setSelectedItemIndex(generic, juce::dontSendNotification);
	distanceUnitsCombo.setSelectedItemIndex(distance, juce::dontSendNotification);
	areaUnitsCombo.setSelectedItemIndex(area, juce::dontSendNotification);
	volumeUnitsCombo.setSelectedItemIndex(volume, juce::dontSendNotification);
	massUnitsCombo.setSelectedItemIndex(mass, juce::dontSendNotification);
	temperatureUnitsCombo.setSelectedItemIndex(temperature, juce::dontSendNotification);
	pressureUnitsCombo.setSelectedItemIndex(pressure, juce::dontSendNotification);
	forceUnitsCombo.setSelectedItemIndex(force, juce::dontSendNotification);

	// The two-option fields (distance/area/temperature/pressure/force) do not distinguish
	// Imperial from US, so only the three-option fields (generic/volume/mass) can tell them apart
	const bool allMetric = (0 == generic) && (0 == distance) && (0 == area) && (0 == volume) && (0 == mass) && (0 == temperature) && (0 == pressure) && (0 == force);
	const bool allImperial = (1 == generic) && (1 == distance) && (1 == area) && (1 == volume) && (1 == mass) && (1 == temperature) && (1 == pressure) && (1 == force);
	const bool allUS = (2 == generic) && (1 == distance) && (1 == area) && (2 == volume) && (2 == mass) && (1 == temperature) && (1 == pressure) && (1 == force);

	int unitsSystemSelection = UNITS_SYSTEM_CUSTOM;
	if (allMetric)
	{
		unitsSystemSelection = UNITS_SYSTEM_METRIC;
	}
	else if (allUS)
	{
		unitsSystemSelection = UNITS_SYSTEM_US;
	}
	else if (allImperial)
	{
		unitsSystemSelection = UNITS_SYSTEM_IMPERIAL;
	}
	unitsSystemCombo.setSelectedItemIndex(unitsSystemSelection, juce::dontSendNotification);
	set_unit_detail_rows_visible(UNITS_SYSTEM_CUSTOM == unitsSystemSelection);
}

void SettingsPageComponent::set_custom_locale_rows_visible(bool visible)
{
	languageCodeLabel.setVisible(visible);
	languageCodeEditor.setVisible(visible);
	countryCodeLabel.setVisible(visible);
	countryCodeEditor.setVisible(visible);
}

void SettingsPageComponent::apply_locale_selection()
{
	const int selection = localeCombo.getSelectedItemIndex();
	const bool isCustom = (LOCALE_CUSTOM_INDEX == selection);
	set_custom_locale_rows_visible(isCustom);

	if (!isCustom)
	{
		languageCodeEditor.setText(LOCALE_ENTRIES[selection].languageCode, juce::dontSendNotification);
		countryCodeEditor.setText(LOCALE_ENTRIES[selection].countryCode, juce::dontSendNotification);
		apply_language_and_units_pane();
	}

	resized();
}

void SettingsPageComponent::set_unit_detail_rows_visible(bool visible)
{
	genericUnitsLabel.setVisible(visible);
	genericUnitsCombo.setVisible(visible);
	distanceUnitsLabel.setVisible(visible);
	distanceUnitsCombo.setVisible(visible);
	areaUnitsLabel.setVisible(visible);
	areaUnitsCombo.setVisible(visible);
	volumeUnitsLabel.setVisible(visible);
	volumeUnitsCombo.setVisible(visible);
	massUnitsLabel.setVisible(visible);
	massUnitsCombo.setVisible(visible);
	temperatureUnitsLabel.setVisible(visible);
	temperatureUnitsCombo.setVisible(visible);
	pressureUnitsLabel.setVisible(visible);
	pressureUnitsCombo.setVisible(visible);
	forceUnitsLabel.setVisible(visible);
	forceUnitsCombo.setVisible(visible);
}

void SettingsPageComponent::apply_units_system_selection()
{
	const int selection = unitsSystemCombo.getSelectedItemIndex();
	const bool isCustom = (UNITS_SYSTEM_CUSTOM == selection);
	set_unit_detail_rows_visible(isCustom);

	if (!isCustom)
	{
		// The two-option fields collapse Imperial and US into one "Imperial/US" choice (index 1);
		// only the three-option fields (generic/volume/mass) actually distinguish them.
		const int twoOptionIndex = (UNITS_SYSTEM_METRIC == selection) ? 0 : 1;
		const int threeOptionIndex = (UNITS_SYSTEM_METRIC == selection) ? 0 : ((UNITS_SYSTEM_US == selection) ? 2 : 1);

		genericUnitsCombo.setSelectedItemIndex(threeOptionIndex, juce::dontSendNotification);
		distanceUnitsCombo.setSelectedItemIndex(twoOptionIndex, juce::dontSendNotification);
		areaUnitsCombo.setSelectedItemIndex(twoOptionIndex, juce::dontSendNotification);
		volumeUnitsCombo.setSelectedItemIndex(threeOptionIndex, juce::dontSendNotification);
		massUnitsCombo.setSelectedItemIndex(threeOptionIndex, juce::dontSendNotification);
		temperatureUnitsCombo.setSelectedItemIndex(twoOptionIndex, juce::dontSendNotification);
		pressureUnitsCombo.setSelectedItemIndex(twoOptionIndex, juce::dontSendNotification);
		forceUnitsCombo.setSelectedItemIndex(twoOptionIndex, juce::dontSendNotification);

		apply_language_and_units_pane();
	}

	resized();
}

void SettingsPageComponent::apply_language_and_units_pane()
{
	auto &languageCommandInterface = ownerServer.get_language_command_interface();

	languageCommandInterface.set_language_code(languageCodeEditor.getText().toStdString());
	languageCommandInterface.set_country_code(countryCodeEditor.getText().toStdString());
	languageCommandInterface.set_commanded_decimal_symbol(static_cast<isobus::LanguageCommandInterface::DecimalSymbols>(decimalSymbolCombo.getSelectedItemIndex()));
	languageCommandInterface.set_commanded_date_format(static_cast<isobus::LanguageCommandInterface::DateFormats>(dateFormatCombo.getSelectedItemIndex()));
	languageCommandInterface.set_commanded_time_format(static_cast<isobus::LanguageCommandInterface::TimeFormats>(timeFormatCombo.getSelectedItemIndex()));
	languageCommandInterface.set_commanded_generic_units(static_cast<isobus::LanguageCommandInterface::UnitSystem>(genericUnitsCombo.getSelectedItemIndex()));
	languageCommandInterface.set_commanded_distance_units(static_cast<isobus::LanguageCommandInterface::DistanceUnits>(distanceUnitsCombo.getSelectedItemIndex()));
	languageCommandInterface.set_commanded_area_units(static_cast<isobus::LanguageCommandInterface::AreaUnits>(areaUnitsCombo.getSelectedItemIndex()));
	languageCommandInterface.set_commanded_volume_units(static_cast<isobus::LanguageCommandInterface::VolumeUnits>(volumeUnitsCombo.getSelectedItemIndex()));
	languageCommandInterface.set_commanded_mass_units(static_cast<isobus::LanguageCommandInterface::MassUnits>(massUnitsCombo.getSelectedItemIndex()));
	languageCommandInterface.set_commanded_temperature_units(static_cast<isobus::LanguageCommandInterface::TemperatureUnits>(temperatureUnitsCombo.getSelectedItemIndex()));
	languageCommandInterface.set_commanded_pressure_units(static_cast<isobus::LanguageCommandInterface::PressureUnits>(pressureUnitsCombo.getSelectedItemIndex()));
	languageCommandInterface.set_commanded_force_units(static_cast<isobus::LanguageCommandInterface::ForceUnits>(forceUnitsCombo.getSelectedItemIndex()));

	languageCommandInterface.send_language_command();
	ownerServer.save_settings();
}
