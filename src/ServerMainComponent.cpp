/*******************************************************************************
** @file       ServerMainComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ServerMainComponent.hpp"

#include "AlarmMaskAudio.h"
#include "JuceManagedWorkingSetCache.hpp"
#include "isobus/utility/system_timing.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

ServerMainComponent::ServerMainComponent(std::shared_ptr<isobus::InternalControlFunction> serverControlFunction) :
  VirtualTerminalServer(serverControlFunction),
  workingSetSelector(*this),
  dataMaskRenderer(*this),
  softKeyMaskRenderer(*this)
{
	VirtualTerminalServer::initialize();
	check_load_settings();

	if (languageCommandInterface.get_country_code().empty())
	{
		languageCommandInterface.set_country_code("US");
	}

	if (languageCommandInterface.get_language_code().empty())
	{
		languageCommandInterface.set_language_code("en");
	}

	languageCommandInterface.initialize();
	mAudioDeviceManager.initialise(0, 1, nullptr, true);
	mAudioDeviceManager.addAudioCallback(&mSoundPlayer);
	addAndMakeVisible(workingSetSelector);
	addAndMakeVisible(dataMaskRenderer);
	addAndMakeVisible(softKeyMaskRenderer);
	addAndMakeVisible(loggerViewport);

	menuBar.setModel(this);
	addAndMakeVisible(menuBar);

	// Make sure you set the size of the component after
	// you add any child components.
	setSize(800, 800);

	logger.setTopLeftPosition(0, 600);
	logger.setSize(800, 200);
	loggerViewport.setViewedComponent(&logger, false);
	logger.setVisible(true);

	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Info);
	setApplicationCommandManagerToWatch(&mCommandManager);
	mCommandManager.registerAllCommandsForTarget(this);
	startTimer(50);
}

ServerMainComponent::~ServerMainComponent()
{
}

bool ServerMainComponent::get_is_enough_memory(std::uint32_t) const
{
	return true;
}

isobus::VirtualTerminalBase::VTVersion ServerMainComponent::get_version() const
{
	return versionToReport;
}

std::uint8_t ServerMainComponent::get_number_of_navigation_soft_keys() const
{
	return 0;
}

std::uint8_t ServerMainComponent::get_soft_key_descriptor_x_pixel_width() const
{
	return 60;
}

std::uint8_t ServerMainComponent::get_soft_key_descriptor_y_pixel_width() const
{
	return 60;
}
std::uint8_t ServerMainComponent::get_number_of_possible_virtual_soft_keys_in_soft_key_mask() const
{
	return 64;
}
std::uint8_t ServerMainComponent::get_number_of_physical_soft_keys() const
{
	return 0;
}

std::uint16_t ServerMainComponent::get_data_mask_area_size_x_pixels() const
{
	return 480;
}

std::uint16_t ServerMainComponent::get_data_mask_area_size_y_pixels() const
{
	return 480;
}

void ServerMainComponent::suspend_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> /*workingSetWithError*/)
{
}

isobus::VirtualTerminalBase::SupportedWideCharsErrorCode ServerMainComponent::get_supported_wide_chars(std::uint8_t,
                                                                                                       std::uint16_t,
                                                                                                       std::uint16_t,
                                                                                                       std::uint8_t &,
                                                                                                       std::vector<std::uint8_t> &)
{
	return isobus::VirtualTerminalBase::SupportedWideCharsErrorCode::AnyOtherError;
}

std::vector<std::array<std::uint8_t, 7>> ServerMainComponent::get_versions(isobus::NAME clientNAME)
{
	std::ostringstream nameString;
	std::vector<std::array<std::uint8_t, 7>> retVal;
	nameString << std::hex << std::setfill('0') << std::setw(16) << clientNAME.get_full_name();
	File isoDirectory = (std::filesystem::current_path().string() + "/" + ISO_DATA_PATH + "/" + nameString.str());

	if (isoDirectory.exists() && isoDirectory.isDirectory())
	{
		auto directoryFiles = isoDirectory.findChildFiles(File::TypesOfFileToFind::findFiles, false, "*.iopx");

		for (auto &file : directoryFiles)
		{
			std::ifstream iopxFile(file.getFullPathName().toStdString(), std::ios::binary);

			if (iopxFile.is_open())
			{
				iopxFile.unsetf(std::ios::skipws);
				std::array<std::uint8_t, 7> versionLabel;
				iopxFile.read(reinterpret_cast<char *>(versionLabel.data()), 7);

				// Only add the version label if it is not already in the list
				bool versionAlreadyInList = false;
				for (const auto &version : retVal)
				{
					if (version == versionLabel)
					{
						versionAlreadyInList = true;
						break;
					}
				}

				if (!versionAlreadyInList)
				{
					retVal.push_back(versionLabel);
				}
			}
		}
	}
	else
	{
		isobus::CANStackLogger::info("[VT Server]: No saved object pool data for client: " + nameString.str());
	}
	return retVal;
}

std::vector<std::uint8_t> ServerMainComponent::get_supported_objects() const
{
	// These are defined by ISO 11783-6 Table A.1 "Virtual terminal objects"
	return { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 37, 39 };
}

std::vector<std::uint8_t> ServerMainComponent::load_version(const std::vector<std::uint8_t> &versionLabel, isobus::NAME clientNAME)
{
	std::ostringstream nameString;
	std::vector<std::uint8_t> loadedIOPData;
	std::vector<std::uint8_t> loadedVersionLabel(7);
	nameString << std::hex << std::setfill('0') << std::setw(16) << clientNAME.get_full_name();

	if ((std::filesystem::is_directory(ISO_DATA_PATH + "/" + nameString.str()) ||
	     std::filesystem::exists(ISO_DATA_PATH + "/" + nameString.str())) &&
	    (7 == versionLabel.size()))
	{
		for (const auto &entry : std::filesystem::directory_iterator(ISO_DATA_PATH + "/" + nameString.str()))
		{
			if (entry.path().has_extension() && entry.path().extension() == ".iopx")
			{
				std::ifstream iopxFile(entry.path(), std::ios::binary);

				if (iopxFile.is_open())
				{
					iopxFile.unsetf(std::ios::skipws);
					iopxFile.read(reinterpret_cast<char *>(loadedVersionLabel.data()), 7);

					if (7 == loadedVersionLabel.size())
					{
						bool versionMatches = true;
						for (std::uint8_t i = 0; i < 7; i++)
						{
							if (loadedVersionLabel.at(i) != versionLabel.at(i))
							{
								versionMatches = false;
								break;
							}
						}

						if (versionMatches)
						{
							iopxFile.seekg(7, std::ios::beg);
							loadedIOPData.insert(loadedIOPData.end(), std::istream_iterator<std::uint8_t>(iopxFile), std::istream_iterator<std::uint8_t>());
						}
					}
				}
			}
		}
	}
	return loadedIOPData;
}

bool ServerMainComponent::save_version(const std::vector<std::uint8_t> &objectPool, const std::vector<std::uint8_t> &versionLabel, isobus::NAME clientNAME)
{
	bool retVal = false;

	// Main saved data folder
	if (!std::filesystem::is_directory(ISO_DATA_PATH) || !std::filesystem::exists(ISO_DATA_PATH))
	{ // Check if src folder exists
		std::filesystem::create_directory(ISO_DATA_PATH); // create src folder
	}

	// NAME specific folder
	std::ostringstream nameString;
	nameString << std::hex << std::setfill('0') << std::setw(16) << clientNAME.get_full_name();

	if (!std::filesystem::is_directory(ISO_DATA_PATH + "/" + nameString.str()) || !std::filesystem::exists(ISO_DATA_PATH + "/" + nameString.str()))
	{ // Check if src folder exists
		std::filesystem::create_directory(ISO_DATA_PATH + "/" + nameString.str()); // create src folder
	}

	std::ofstream iopxFile(ISO_DATA_PATH + "/" + nameString.str() + "/object_pool_with_label_" + std::to_string(number_of_iop_files_in_directory(ISO_DATA_PATH + "/" + nameString.str())) + ".iopx", std::ios::trunc | std::ios::binary);
	std::ofstream iopFile(ISO_DATA_PATH + "/" + nameString.str() + "/object_pool_" + std::to_string(number_of_iop_files_in_directory(ISO_DATA_PATH + "/" + nameString.str())) + ".iop", std::ios::trunc | std::ios::binary);

	if (iopxFile.is_open())
	{
		iopxFile.write(reinterpret_cast<const char *>(versionLabel.data()), versionLabel.size());
		iopxFile.write(reinterpret_cast<const char *>(objectPool.data()), objectPool.size());
		iopxFile.close();
		retVal = true;
	}
	if (iopFile.is_open())
	{
		iopFile.write(reinterpret_cast<const char *>(objectPool.data()), objectPool.size());
		iopFile.close();
	}
	return retVal;
}

bool ServerMainComponent::delete_version(const std::vector<std::uint8_t> &versionLabel, isobus::NAME clientNAME)
{
	bool retVal = false;
	std::ostringstream nameString;
	std::vector<std::uint8_t> loadedVersionLabel(7);
	std::vector<std::filesystem::directory_entry> filesToRemove;
	nameString << std::hex << std::setfill('0') << std::setw(16) << clientNAME.get_full_name();

	if ((std::filesystem::is_directory(ISO_DATA_PATH + "/" + nameString.str()) ||
	     std::filesystem::exists(ISO_DATA_PATH + "/" + nameString.str())) &&
	    (7 == versionLabel.size()))
	{
		for (const auto &entry : std::filesystem::directory_iterator(ISO_DATA_PATH + "/" + nameString.str()))
		{
			if (entry.path().has_extension() && entry.path().extension() == ".iopx")
			{
				std::ifstream iopxFile(entry.path(), std::ios::binary);

				if (iopxFile.is_open())
				{
					iopxFile.unsetf(std::ios::skipws);
					iopxFile.read(reinterpret_cast<char *>(loadedVersionLabel.data()), 7);

					if (7 == loadedVersionLabel.size())
					{
						bool versionMatches = true;
						for (std::uint8_t i = 0; i < 7; i++)
						{
							if (loadedVersionLabel.at(i) != versionLabel.at(i))
							{
								versionMatches = false;
								break;
							}
						}

						if (versionMatches)
						{
							iopxFile.close();
							filesToRemove.push_back(entry);
							retVal = true;
						}
					}
				}
			}
		}

		for (const auto &entry : filesToRemove)
		{
			retVal &= std::filesystem::remove(entry);
		}
	}
	return retVal;
}

bool ServerMainComponent::delete_all_versions(isobus::NAME clientNAME)
{
	bool retVal = false;
	std::ostringstream nameString;
	std::vector<std::uint8_t> loadedVersionLabel(7);
	std::vector<std::filesystem::directory_entry> filesToRemove;
	nameString << std::hex << std::setfill('0') << std::setw(16) << clientNAME.get_full_name();

	if ((std::filesystem::is_directory(ISO_DATA_PATH + "/" + nameString.str()) ||
	     std::filesystem::exists(ISO_DATA_PATH + "/" + nameString.str())))
	{
		for (const auto &entry : std::filesystem::directory_iterator(ISO_DATA_PATH + "/" + nameString.str()))
		{
			if (entry.path().has_extension() && entry.path().extension() == ".iopx")
			{
				filesToRemove.push_back(entry);
			}
		}

		for (const auto &entry : filesToRemove)
		{
			retVal &= std::filesystem::remove(entry);
		}
	}
	return retVal;
}

void ServerMainComponent::timerCallback()
{
	// This function is called at the frequency specified by the setFramesPerSecond() call
	// in the constructor. You can use it to update counters, animate values, etc.
	if ((isobus::SystemTiming::time_expired_ms(statusMessageTimestamp_ms, 1000)) &&
	    (send_status_message()))
	{
		statusMessageTimestamp_ms = isobus::SystemTiming::get_timestamp_ms();
	}

	for (auto &ws : managedWorkingSetList)
	{
		if (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Success == ws->get_object_pool_processing_state())
		{
			ws->join_parsing_thread();
			send_end_of_object_pool_response(true, isobus::NULL_OBJECT_ID, isobus::NULL_OBJECT_ID, 0, ws->get_control_function());
			workingSetSelector.add_working_set_to_draw(ws);
			if (isobus::NULL_CAN_ADDRESS == activeWorkingSetMasterAddress)
			{
				ws->set_working_set_maintenance_message_timestamp_ms(isobus::SystemTiming::get_timestamp_ms());
				change_selected_working_set(0);
			}
		}
		else if (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Fail == ws->get_object_pool_processing_state())
		{
			ws->join_parsing_thread();
			///  @todo Get the parent object ID of the faulting object
			send_end_of_object_pool_response(true, isobus::NULL_OBJECT_ID, ws->get_object_pool_faulting_object_id(), 0, ws->get_control_function());
		}
		else if ((isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == ws->get_object_pool_processing_state()) &&
		         (isobus::SystemTiming::time_expired_ms(ws->get_working_set_maintenance_message_timestamp_ms(), 3000)))
		{
			workingSetSelector.remove_working_set(ws);
		}
		else if (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == ws->get_object_pool_processing_state())
		{
			if (dataMaskRenderer.needsRepaint())
			{
				repaint_data_and_soft_key_mask();
				dataMaskRenderer.on_change_active_mask(ws);
				needToRepaint = false;
			}
			else if (needToRepaint)
			{
				repaint_data_and_soft_key_mask();
				needToRepaint = false;
			}
		}
	}
}

void ServerMainComponent::paint(juce::Graphics &g)
{
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

	// You can add your drawing code here!
	//workingSetSelector->paint(g);
}

void ServerMainComponent::resized()
{
	// This is called when the MainContentComponent is resized.
	// If you add any child components, this is where you should
	// update their positions.
	auto lMenuBarHeight = juce::LookAndFeel::getDefaultLookAndFeel().getDefaultMenuBarHeight();
	auto lBounds = getLocalBounds();

	workingSetSelector.setBounds(0, lMenuBarHeight + 4, 100, 600);
	dataMaskRenderer.setBounds(100, lMenuBarHeight + 4, 500, 500);
	softKeyMaskRenderer.setBounds(580, lMenuBarHeight + 4, 100, 480);
	loggerViewport.setSize(getWidth(), getHeight() - 600);
	loggerViewport.setTopLeftPosition(0, 600);
	menuBar.setBounds(lBounds.removeFromTop(lMenuBarHeight));
}

ApplicationCommandTarget *ServerMainComponent::getNextCommandTarget()
{
	return nullptr;
}

void ServerMainComponent::getAllCommands(juce::Array<juce::CommandID> &allCommands)
{
	allCommands.add(static_cast<int>(CommandIDs::About));
	allCommands.add(static_cast<int>(CommandIDs::ConfigureLanguageCommand));
	allCommands.add(static_cast<int>(CommandIDs::ConfigureReportedVersion));
}

void ServerMainComponent::getCommandInfo(juce::CommandID commandID, ApplicationCommandInfo &result)
{
	switch (static_cast<CommandIDs>(commandID))
	{
		case CommandIDs::About:
		{
			result.setInfo("About", "Displays information about this application", "About", 0);
		}
		break;

		case CommandIDs::ConfigureLanguageCommand:
		{
			result.setInfo("Language Command", "Change the commanded language, units or country code", "Configure", 0);
		}
		break;

		case CommandIDs::ConfigureReportedVersion:
		{
			result.setInfo("Version", "Change the VT server's reported version", "Configure", 0);
		}
		break;

		default:
			break;
	}
}

bool ServerMainComponent::perform(const InvocationInfo &info)
{
	bool retVal = false;

	switch (info.commandID)
	{
		case static_cast<int>(CommandIDs::About):
		{
			retVal = true;
		}
		break;

		case static_cast<int>(CommandIDs::ConfigureLanguageCommand):
		{
			retVal = true;
			popupMenu = std::make_unique<AlertWindow>("Configure Language Command", "Use the following options to configure the units, language, and country that the VT will command from its clients.", MessageBoxIconType::NoIcon);
			popupMenu->addTextEditor("Language Code", languageCommandInterface.get_language_code(), "Language Code");
			popupMenu->addTextEditor("Country Code", languageCommandInterface.get_country_code(), "Country Code");
			popupMenu->addComboBox("Area Units", { "Metric", "Imperial/US" }, "Area Units");
			popupMenu->addComboBox("Date Format", { "ddmmyyyy", "ddyyyymm", "mmyyyydd", "mmddyyyy", "yyyymmdd", "yyyyddmm" }, "Date Format");
			popupMenu->addComboBox("Decimal Symbol", { "Comma", "Point" }, "Decimal Symbol");
			popupMenu->addComboBox("Distance Units", { "Metric", "Imperial/US" }, "Distance Units");
			popupMenu->addComboBox("Force Units", { "Metric", "Imperial/US" }, "Force Units");
			popupMenu->addComboBox("Generic Units", { "Metric", "Imperial", "US" }, "Generic Units");
			popupMenu->addComboBox("Mass Units", { "Metric", "Imperial", "US" }, "Mass Units");
			popupMenu->addComboBox("Pressure Units", { "Metric", "Imperial/US" }, "Pressure Units");
			popupMenu->addComboBox("Temperature Units", { "Metric", "Imperial/US" }, "Temperature Units");
			popupMenu->addComboBox("Time Format", { "24 hour", "12 hour" }, "Time Format");
			popupMenu->addComboBox("Volume Units", { "Metric", "Imperial", "US" }, "Volume Units ");
			popupMenu->getComboBoxComponent("Area Units")->setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_area_units()));
			popupMenu->getComboBoxComponent("Date Format")->setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_date_format()));
			popupMenu->getComboBoxComponent("Decimal Symbol")->setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_decimal_symbol()));
			popupMenu->getComboBoxComponent("Distance Units")->setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_distance_units()));
			popupMenu->getComboBoxComponent("Force Units")->setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_force_units()));
			popupMenu->getComboBoxComponent("Generic Units")->setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_generic_units()));
			popupMenu->getComboBoxComponent("Mass Units")->setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_mass_units()));
			popupMenu->getComboBoxComponent("Pressure Units")->setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_pressure_units()));
			popupMenu->getComboBoxComponent("Temperature Units")->setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_temperature_units()));
			popupMenu->getComboBoxComponent("Time Format")->setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_time_format()));
			popupMenu->getComboBoxComponent("Volume Units")->setSelectedItemIndex(static_cast<int>(languageCommandInterface.get_commanded_volume_units()));
			popupMenu->addButton("OK", 1, KeyPress(KeyPress::returnKey, 0, 0));
			popupMenu->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey, 0, 0));
			popupMenu->enterModalState(true, ModalCallbackFunction::create(LanguageCommandConfigClosed{ *this }));
		}
		break;

		case static_cast<int>(CommandIDs::ConfigureReportedVersion):
		{
			popupMenu = std::make_unique<AlertWindow>("Configure Reported VT Server Version", "You can use this setting to change the version of the ISO11783-6 standard that this server will claim to support in its status messages.", MessageBoxIconType::NoIcon);
			popupMenu->addComboBox("Version", { "Version 2 or Older", "Version 3", "Version 4", "Version 5", "Version 6" });
			popupMenu->addButton("OK", 2, KeyPress(KeyPress::returnKey, 0, 0));
			popupMenu->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey, 0, 0));
			popupMenu->enterModalState(true, ModalCallbackFunction::create(LanguageCommandConfigClosed{ *this }));
			retVal = true;
		}
		break;

		default:
			break;
	}
	return retVal;
}

StringArray ServerMainComponent::getMenuBarNames()
{
	return juce::StringArray("Configure", "About");
}

PopupMenu ServerMainComponent::getMenuForIndex(int index, const juce::String &)
{
	juce::PopupMenu retVal;

	switch (index)
	{
		case 0:
		{
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::ConfigureLanguageCommand));
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::ConfigureReportedVersion));
		}
		break;

		case 1:
		{
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::About));
		}
		break;

		default:
			break;
	}
	return retVal;
}

void ServerMainComponent::menuItemSelected(int, int)
{
	// Do nothing
}

std::shared_ptr<isobus::ControlFunction> ServerMainComponent::get_client_control_function_for_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) const
{
	std::shared_ptr<isobus::ControlFunction> retVal = nullptr;

	for (const auto &ws : managedWorkingSetList)
	{
		if (workingSet == ws)
		{
			retVal = workingSet->get_control_function();
			break;
		}
	}
	return retVal;
}

void ServerMainComponent::change_selected_working_set(std::uint8_t index)
{
	if (index < managedWorkingSetList.size())
	{
		for (auto &ws : managedWorkingSetList)
		{
			ws->clear_callback_handles();
		}
		auto &ws = managedWorkingSetList.at(index);
		activeWorkingSetMasterAddress = ws->get_control_function()->get_address();
		activeWorkingSetDataMaskObjectID = std::static_pointer_cast<isobus::WorkingSet>(ws->get_working_set_object())->get_active_mask();

		dataMaskRenderer.on_change_active_mask(ws);
		softKeyMaskRenderer.on_change_active_mask(ws);
		activeWorkingSet = ws;
		ws->save_callback_handle(get_on_repaint_event_dispatcher().add_listener([this](std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet>) { this->repaint_on_next_update(); }));
		ws->save_callback_handle(get_on_change_active_mask_event_dispatcher().add_listener([this](std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t workingSet, std::uint16_t newMask) { this->on_change_active_mask_callback(affectedWorkingSet, workingSet, newMask); }));

		if (send_status_message())
		{
			statusMessageTimestamp_ms = isobus::SystemTiming::get_timestamp_ms();
		}
		else
		{
			statusMessageTimestamp_ms = 0;
		}
	}
}

void ServerMainComponent::repaint_on_next_update()
{
	needToRepaint = true;
}

void ServerMainComponent::LanguageCommandConfigClosed::operator()(int result) const noexcept
{
	switch (result)
	{
		case 1: // Save Language Command
		case 2: // Save Version
		{
			mParent.save_settings();
		}
		break;

		default:
		{
			// Cancel. Do nothing
		}
		break;
	}
	mParent.exitModalState(result);
	mParent.popupMenu.reset();
}

std::size_t ServerMainComponent::number_of_iop_files_in_directory(std::filesystem::path path)
{
	std::size_t retVal = 0;

	for (const auto &entry : std::filesystem::directory_iterator(path))
	{
		if (entry.path().has_extension() && entry.path().extension() == ".iop")
		{
			retVal++;
		}
	}
	return retVal;
}

void ServerMainComponent::on_change_active_mask_callback(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t, std::uint16_t newMask)
{
	if (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == affectedWorkingSet->get_object_pool_processing_state())
	{
		const MessageManagerLock mmLock;

		dataMaskRenderer.on_change_active_mask(activeWorkingSet);
		softKeyMaskRenderer.on_change_active_mask(activeWorkingSet);

		auto activeMask = affectedWorkingSet->get_object_by_id(newMask);

		if (activeWorkingSetDataMaskObjectID != newMask)
		{
			activeWorkingSetDataMaskObjectID = newMask;

			if (send_status_message())
			{
				statusMessageTimestamp_ms = isobus::SystemTiming::get_timestamp_ms();
			}
			else
			{
				statusMessageTimestamp_ms = 0;
			}
		}

		if (nullptr != activeMask)
		{
			for (std::uint16_t i = 0; i < activeMask->get_number_children(); i++)
			{
				auto child = activeMask->get_object_by_id(activeMask->get_child_id(i));

				if ((nullptr != child) && (isobus::VirtualTerminalObjectType::SoftKeyMask == child->get_object_type()))
				{
					activeWorkingSetSoftkeyMaskObjectID = child->get_id();
					break;
				}
			}

			if (isobus::VirtualTerminalObjectType::AlarmMask == activeMask->get_object_type())
			{
				auto alarmMask = std::static_pointer_cast<isobus::AlarmMask>(activeMask);

				switch (alarmMask->get_signal_priority())
				{
					case isobus::AlarmMask::AcousticSignal::Highest:
					{
						mSoundPlayer.play(AlarmMaskAudio::alarmMaskHigh_mp3, AlarmMaskAudio::alarmMaskHigh_mp3Size);
					}
					break;

					case isobus::AlarmMask::AcousticSignal::Medium:
					{
						mSoundPlayer.play(AlarmMaskAudio::alarmMaskMedium_mp3, AlarmMaskAudio::alarmMaskMedium_mp3Size);
					}
					break;

					case isobus::AlarmMask::AcousticSignal::Lowest:
					{
						mSoundPlayer.play(AlarmMaskAudio::alarmMaskLow_mp3, AlarmMaskAudio::alarmMaskLow_mp3Size);
					}
					break;

					default:
						break;
				}
			}
		}
	}
}

void ServerMainComponent::repaint_data_and_soft_key_mask()
{
	dataMaskRenderer.on_change_active_mask(activeWorkingSet);
	softKeyMaskRenderer.on_change_active_mask(activeWorkingSet);
	workingSetSelector.redraw();
}

void ServerMainComponent::check_load_settings()
{
	auto lDefaultSaveLocation = File::getSpecialLocation(File::userApplicationDataDirectory);
	String lDataDirectoryPath = (lDefaultSaveLocation.getFullPathName().toStdString() + "/Open-Agriculture");
	File dataDirectory(lDataDirectoryPath);
	bool lCanLoadSettings = false;

	if (dataDirectory.exists() && dataDirectory.isDirectory())
	{
		lCanLoadSettings = true;
	}
	else
	{
		auto result = dataDirectory.createDirectory();
	}

	if (lCanLoadSettings)
	{
		String lFilePath = (lDefaultSaveLocation.getFullPathName().toStdString() + "/Open-Agriculture/" + "vt_settings.xml");
		File settingsFile = File(lFilePath);

		if (settingsFile.existsAsFile())
		{
			auto xml(XmlDocument::parse(settingsFile));
			ValueTree settings("Settings");

			if (nullptr != xml)
			{
				settings.copyPropertiesAndChildrenFrom(ValueTree::fromXml(*xml), nullptr);

				auto firstChild = settings.getChild(0);
				auto secondChild = settings.getChild(1);
				languageCommandInterface.set_commanded_area_units(static_cast<isobus::LanguageCommandInterface::AreaUnits>(int(firstChild.getProperty("AreaUnits"))));
				languageCommandInterface.set_commanded_date_format(static_cast<isobus::LanguageCommandInterface::DateFormats>(int(firstChild.getProperty("DateFormat"))));
				languageCommandInterface.set_commanded_decimal_symbol(static_cast<isobus::LanguageCommandInterface::DecimalSymbols>(int(firstChild.getProperty("DecimalSymbol"))));
				languageCommandInterface.set_commanded_distance_units(static_cast<isobus::LanguageCommandInterface::DistanceUnits>(int(firstChild.getProperty("DistanceUnits"))));
				languageCommandInterface.set_commanded_force_units(static_cast<isobus::LanguageCommandInterface::ForceUnits>(int(firstChild.getProperty("ForceUnits"))));
				languageCommandInterface.set_commanded_generic_units(static_cast<isobus::LanguageCommandInterface::UnitSystem>(int(firstChild.getProperty("UnitSystem"))));
				languageCommandInterface.set_commanded_mass_units(static_cast<isobus::LanguageCommandInterface::MassUnits>(int(firstChild.getProperty("MassUnits"))));
				languageCommandInterface.set_commanded_pressure_units(static_cast<isobus::LanguageCommandInterface::PressureUnits>(int(firstChild.getProperty("PressureUnits"))));
				languageCommandInterface.set_commanded_temperature_units(static_cast<isobus::LanguageCommandInterface::TemperatureUnits>(int(firstChild.getProperty("TemperatureUnits"))));
				languageCommandInterface.set_commanded_time_format(static_cast<isobus::LanguageCommandInterface::TimeFormats>(int(firstChild.getProperty("TimeFormat"))));
				languageCommandInterface.set_commanded_volume_units(static_cast<isobus::LanguageCommandInterface::VolumeUnits>(int(firstChild.getProperty("VolumeUnits"))));
				languageCommandInterface.set_country_code(String(firstChild.getProperty("CountryCode").toString()).toStdString());
				languageCommandInterface.set_language_code(String(firstChild.getProperty("LanguageCode").toString()).toStdString());
				versionToReport = static_cast<VTVersion>(int(secondChild.getProperty("Version")));
			}
		}
	}
}

void ServerMainComponent::save_settings()
{
	auto lDefaultSaveLocation = File::getSpecialLocation(File::userApplicationDataDirectory);
	String lDataDirectoryPath = (lDefaultSaveLocation.getFullPathName().toStdString() + "/Open-Agriculture");
	File dataDirectory(lDataDirectoryPath);
	bool lCanSaveSettings = false;

	if (dataDirectory.exists() && dataDirectory.isDirectory())
	{
		lCanSaveSettings = true;
	}
	else
	{
		auto result = dataDirectory.createDirectory();
		lCanSaveSettings = result.wasOk();
	}

	if (lCanSaveSettings)
	{
		String lFilePath = (lDefaultSaveLocation.getFullPathName().toStdString() + "/Open-Agriculture/" + "vt_settings.xml");
		File settingsFile = File(lFilePath);
		ValueTree settings("Settings");
		ValueTree languageCommandSettings("LanguageCommand");
		ValueTree compatibilitySettings("Compatibility");

		languageCommandSettings.setProperty("AreaUnits", static_cast<int>(languageCommandInterface.get_commanded_area_units()), nullptr);
		languageCommandSettings.setProperty("DateFormat", static_cast<int>(languageCommandInterface.get_commanded_date_format()), nullptr);
		languageCommandSettings.setProperty("DecimalSymbol", static_cast<int>(languageCommandInterface.get_commanded_decimal_symbol()), nullptr);
		languageCommandSettings.setProperty("DistanceUnits", static_cast<int>(languageCommandInterface.get_commanded_distance_units()), nullptr);
		languageCommandSettings.setProperty("ForceUnits", static_cast<int>(languageCommandInterface.get_commanded_force_units()), nullptr);
		languageCommandSettings.setProperty("UnitSystem", static_cast<int>(languageCommandInterface.get_commanded_generic_units()), nullptr);
		languageCommandSettings.setProperty("MassUnits", static_cast<int>(languageCommandInterface.get_commanded_mass_units()), nullptr);
		languageCommandSettings.setProperty("PressureUnits", static_cast<int>(languageCommandInterface.get_commanded_pressure_units()), nullptr);
		languageCommandSettings.setProperty("TemperatureUnits", static_cast<int>(languageCommandInterface.get_commanded_temperature_units()), nullptr);
		languageCommandSettings.setProperty("TimeFormat", static_cast<int>(languageCommandInterface.get_commanded_time_format()), nullptr);
		languageCommandSettings.setProperty("VolumeUnits", static_cast<int>(languageCommandInterface.get_commanded_volume_units()), nullptr);
		languageCommandSettings.setProperty("CountryCode", String(languageCommandInterface.get_country_code()), nullptr);
		languageCommandSettings.setProperty("LanguageCode", String(languageCommandInterface.get_language_code()), nullptr);
		compatibilitySettings.setProperty("Version", get_vt_version_byte(versionToReport), nullptr);
		settings.appendChild(languageCommandSettings, nullptr);
		settings.appendChild(compatibilitySettings, nullptr);
		std::unique_ptr<XmlElement> xml(settings.createXml());

		if (nullptr != xml)
		{
			xml->writeTo(settingsFile);
		}
	}
}
