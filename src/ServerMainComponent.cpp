/*******************************************************************************
** @file       ServerMainComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ServerMainComponent.hpp"

#include "AlarmMaskAudio.h"
#include "JuceManagedWorkingSetCache.hpp"
#include "Main.hpp"
#include "ShortcutsWindow.hpp"
#include "isobus/utility/system_timing.hpp"

#include "SoftKeyMaskRenderAreaComponent.hpp"

#ifdef JUCE_WINDOWS
#include "isobus/hardware_integration/toucan_vscp_canal.hpp"
#elif JUCE_LINUX
#include "isobus/hardware_integration/socket_can_interface.hpp"
#endif

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>

ServerMainComponent::ServerMainComponent(
  std::shared_ptr<isobus::InternalControlFunction> serverControlFunction,
  std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers,
  std::shared_ptr<ValueTree> settings,
  uint8_t vtNumber) :
  VirtualTerminalServer(serverControlFunction), workingSetSelector(*this), dataMaskRenderer(*this), softKeyMaskRenderer(*this), parentCANDrivers(canDrivers)
{
	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Info);

	VirtualTerminalServer::initialize();
	check_load_settings(settings);

	if (languageCommandInterface.get_country_code().empty())
	{
		languageCommandInterface.set_country_code("US");
	}

	if (languageCommandInterface.get_language_code().empty())
	{
		languageCommandInterface.set_language_code("en");
	}

	languageCommandInterface.initialize();

	auto timerCallback = std::bind(&ServerMainComponent::timeAndDateCallback, this, std::placeholders::_1);
	timeServingInterface = std::make_unique<isobus::TimeDateInterface>(serverControlFunction, timerCallback);
	timeServingInterface->initialize();

	diagnosticProtocol = std::make_unique<isobus::DiagnosticProtocol>(serverControlFunction);
	diagnosticProtocol->set_product_identification_brand("Open-Agriculture");
	diagnosticProtocol->set_product_identification_model("AgIsoVirtualTerminal");
	diagnosticProtocol->set_software_id_field(0, AgISOVirtualTerminalApplication::getApplicationBuildInfo());
	diagnosticProtocol->initialize();

	isobus::CANHardwareInterface::get_periodic_update_event_dispatcher().add_listener([this]() {
		diagnosticProtocol->update();
	});

	mAudioDeviceManager.initialise(0, 1, nullptr, true);
	mAudioDeviceManager.addAudioCallback(&mSoundPlayer);
	addAndMakeVisible(workingSetSelector);
	addAndMakeVisible(dataMaskRenderer);
	addAndMakeVisible(softKeyMaskRenderer);
	addAndMakeVisible(loggerViewport);
	addChildComponent(vtNumberComponent);
	menuBar.setModel(this);
	addAndMakeVisible(menuBar);

	// Make sure you set the size of the component after
	// you add any child components.
	setSize(800, 800);

	logger.setTopLeftPosition(0, 600);
	logger.setSize(800, 200);
	loggerViewport.setViewedComponent(&logger, false);
	logger.setVisible(true);

	setApplicationCommandManagerToWatch(&mCommandManager);
	mCommandManager.registerAllCommandsForTarget(this);
	startTimer(50);

	setWantsKeyboardFocus(true);
	addKeyListener(this);
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
	std::uint8_t retVal = 32;

	if (versionToReport == VTVersion::Version6)
	{
		retVal = 60; // 60x60 is the minimum size for VT Version 6+
	}
	if (softKeyMaskDimensions.keyWidth > retVal)
	{
		retVal = softKeyMaskDimensions.keyWidth;
	}
	return retVal;
}

std::uint8_t ServerMainComponent::get_soft_key_descriptor_y_pixel_height() const
{
	// Min: 60 pixels wide
	std::uint8_t retVal = 60;

	if (softKeyMaskDimensions.keyHeight > retVal)
	{
		retVal = softKeyMaskDimensions.keyHeight;
	}
	return retVal;
}

std::uint8_t ServerMainComponent::get_number_of_possible_virtual_soft_keys_in_soft_key_mask() const
{
	constexpr std::uint8_t MAX_VIRTUAL_SOFT_KEYS = 64;
	std::uint8_t retVal = softKeyMaskDimensions.key_count();

	// VT Version 3 and prior shall support a maximum of 64 virtual Soft Keys per Soft Key Mask and shall
	// support as a minimum the number of reported physical Soft Keys
	if (versionToReport < VTVersion::Version3)
	{
		if (retVal > MAX_VIRTUAL_SOFT_KEYS)
		{
			retVal = MAX_VIRTUAL_SOFT_KEYS;
		}
		else if (retVal < get_number_of_physical_soft_keys())
		{
			retVal = get_number_of_physical_soft_keys();
		}
	}
	else
	{
		retVal = MAX_VIRTUAL_SOFT_KEYS; // VT Version 4 and later shall support exactly 64 virtual Soft Keys per Soft Key Mask
	}
	return retVal;
}
std::uint8_t ServerMainComponent::get_number_of_physical_soft_keys() const
{
	constexpr std::uint8_t MIN_PHYSICAL_SOFT_KEYS = 6;

	if (versionToReport >= isobus::VirtualTerminalBase::VTVersion::Version4)
	{
		// VT Version 4 and later VTs shall provide at least 6 physical Soft Keys.
		return softKeyMaskDimensions.key_count() < MIN_PHYSICAL_SOFT_KEYS ? MIN_PHYSICAL_SOFT_KEYS : softKeyMaskDimensions.key_count();
	}
	else
	{
		return softKeyMaskDimensions.key_count();
	}
}

uint8_t ServerMainComponent::get_physical_soft_key_columns() const
{
	return softKeyMaskDimensions.columnCount < 1 ? 1 : softKeyMaskDimensions.columnCount;
}

uint8_t ServerMainComponent::get_physical_soft_key_rows() const
{
	return softKeyMaskDimensions.rowCount < 1 ? 1 : softKeyMaskDimensions.rowCount;
}

std::uint16_t ServerMainComponent::get_data_mask_area_size_x_pixels() const
{
	return dataMaskRenderer.getWidth() != 0 ? dataMaskRenderer.getWidth() : 480;
}

std::uint16_t ServerMainComponent::get_data_mask_area_size_y_pixels() const
{
	return dataMaskRenderer.getHeight() != 0 ? dataMaskRenderer.getHeight() : 480;
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

bool ServerMainComponent::keyPressed(const KeyPress &key, Component *originatingComponent)
{
	if (key == alarmAckKeyCode && !alarmAckKeyPressed)
	{
		for (auto &ws : managedWorkingSetList)
		{
			if (activeWorkingSetMasterAddress == ws->get_control_function()->get_address())
			{
				alarmAckKeyPressed = true;
				alarmAckKeyMaskId = std::static_pointer_cast<isobus::WorkingSet>(ws->get_working_set_object())->get_active_mask();
				alarmAckKeyWs = ws->get_control_function();
				send_soft_key_activation_message(KeyActivationCode::ButtonPressedOrLatched, isobus::NULL_OBJECT_ID, alarmAckKeyMaskId, 0, alarmAckKeyWs);
				return true;
			}
		}
	}
	return false;
}

bool ServerMainComponent::keyStateChanged(bool isKeyDown, Component *originatingComponent)
{
	if (!isKeyDown && alarmAckKeyPressed && !juce::KeyPress::isKeyCurrentlyDown(alarmAckKeyCode))
	{
		alarmAckKeyPressed = false;
		send_soft_key_activation_message(KeyActivationCode::ButtonUnlatchedOrReleased, isobus::NULL_OBJECT_ID, alarmAckKeyMaskId, 0, alarmAckKeyWs);
	}
	return false;
}

std::vector<std::array<std::uint8_t, 7>> ServerMainComponent::get_versions(isobus::NAME clientNAME)
{
	std::ostringstream nameString;
	std::vector<std::array<std::uint8_t, 7>> retVal;
	nameString << std::hex << std::setfill('0') << std::setw(16) << clientNAME.get_full_name();
	File isoDirectory(File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName().toStdString() +
	                  File::getSeparatorString() +
	                  "Open-Agriculture" +
	                  File::getSeparatorString() +
	                  ISO_DATA_PATH +
	                  File::getSeparatorString() +
	                  nameString.str());

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
	std::string path = (File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName() +
	                    File::getSeparatorString() +
	                    "Open-Agriculture" +
	                    File::getSeparatorString() +
	                    ISO_DATA_PATH +
	                    File::getSeparatorString())
	                     .toStdString();
	nameString << std::hex << std::setfill('0') << std::setw(16) << clientNAME.get_full_name();

	if ((std::filesystem::is_directory(path + nameString.str()) ||
	     std::filesystem::exists(path + nameString.str())) &&
	    (7 == versionLabel.size()))
	{
		for (const auto &entry : std::filesystem::directory_iterator(path + nameString.str()))
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
	auto userAppData = File::getSpecialLocation(File::userApplicationDataDirectory);
	std::string path = (userAppData.getFullPathName() +
	                    File::getSeparatorString() +
	                    "Open-Agriculture" +
	                    File::getSeparatorString() +
	                    String(ISO_DATA_PATH))
	                     .toStdString();

	// Main saved data folder
	if (!std::filesystem::is_directory(path) || !std::filesystem::exists(path))
	{
		std::filesystem::create_directory(path);
	}

	// NAME specific folder
	std::ostringstream nameString;
	nameString << std::hex << std::setfill('0') << std::setw(16) << clientNAME.get_full_name();

	if (!std::filesystem::is_directory(path + "/" + nameString.str()) || !std::filesystem::exists(path + "/" + nameString.str()))
	{ // Check if src folder exists
		std::filesystem::create_directory(path + "/" + nameString.str()); // create src folder
	}

	std::ofstream iopxFile(path + "/" + nameString.str() + "/object_pool_with_label_" + std::to_string(number_of_iop_files_in_directory(path + "/" + nameString.str())) + ".iopx", std::ios::trunc | std::ios::binary);
	std::ofstream iopFile(path + "/" + nameString.str() + "/object_pool_" + std::to_string(number_of_iop_files_in_directory(path + "/" + nameString.str())) + ".iop", std::ios::trunc | std::ios::binary);

	if (iopxFile.is_open())
	{
		iopxFile.write(reinterpret_cast<const char *>(versionLabel.data()), static_cast<std::streamsize>(versionLabel.size()));
		iopxFile.write(reinterpret_cast<const char *>(objectPool.data()), static_cast<std::streamsize>(objectPool.size()));
		iopxFile.close();
		retVal = true;
	}
	if (iopFile.is_open())
	{
		iopFile.write(reinterpret_cast<const char *>(objectPool.data()), static_cast<std::streamsize>(objectPool.size()));
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
	std::string path = (File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName() +
	                    File::getSeparatorString() +
	                    "Open-Agriculture" +
	                    File::getSeparatorString() +
	                    ISO_DATA_PATH +
	                    File::getSeparatorString())
	                     .toStdString();
	nameString << std::hex << std::setfill('0') << std::setw(16) << clientNAME.get_full_name();

	if ((std::filesystem::is_directory(path + nameString.str()) ||
	     std::filesystem::exists(path + nameString.str())) &&
	    (7 == versionLabel.size()))
	{
		for (const auto &entry : std::filesystem::directory_iterator(path + nameString.str()))
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
	auto path = (File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName().toStdString() +
	             File::getSeparatorString() +
	             "Open-Agriculture" +
	             File::getSeparatorString() +
	             ISO_DATA_PATH +
	             File::getSeparatorString())
	              .toStdString();
	nameString << std::hex << std::setfill('0') << std::setw(16) << clientNAME.get_full_name();

	if ((std::filesystem::is_directory(path + nameString.str()) ||
	     std::filesystem::exists(path + nameString.str())))
	{
		for (const auto &entry : std::filesystem::directory_iterator(path + nameString.str()))
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

bool ServerMainComponent::delete_object_pool(isobus::NAME clientNAME)
{
	bool retVal = false;

	for (auto &ws : managedWorkingSetList)
	{
		if (ws->get_control_function()->get_NAME() == clientNAME)
		{
			ws->request_deletion(); // We'll delete it on our next update on our normal thread.
			retVal = true;
			break;
		}
	}
	return retVal;
}

void ServerMainComponent::timerCallback()
{
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

			workingSetSelector.update_drawn_working_sets(managedWorkingSetList);

			auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(ws->get_working_set_object());
			if ((isobus::NULL_CAN_ADDRESS == activeWorkingSetMasterAddress) &&
			    (nullptr != workingSetObject) &&
			    (workingSetObject->get_selectable()))
			{
				ws->set_working_set_maintenance_message_timestamp_ms(isobus::SystemTiming::get_timestamp_ms());
				change_selected_working_set(0);
			}

			if (ws->get_was_object_pool_loaded_from_non_volatile_memory())
			{
				send_load_version_response(0, ws->get_control_function());
			}
			else
			{
				send_end_of_object_pool_response(true, isobus::NULL_OBJECT_ID, isobus::NULL_OBJECT_ID, 0, ws->get_control_function());
			}
		}
		else if (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Fail == ws->get_object_pool_processing_state())
		{
			ws->join_parsing_thread();

			if (ws->get_was_object_pool_loaded_from_non_volatile_memory())
			{
				send_load_version_response(1, ws->get_control_function());
			}
			else
			{
				///  @todo Get the parent object ID of the faulting object
				send_end_of_object_pool_response(true, isobus::NULL_OBJECT_ID, ws->get_object_pool_faulting_object_id(), 0, ws->get_control_function());
			}
		}
		else if ((isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == ws->get_object_pool_processing_state()) &&
		         ((isobus::SystemTiming::time_expired_ms(ws->get_working_set_maintenance_message_timestamp_ms(), 3000)) ||
		          (ws->is_deletion_requested())))
		{
			workingSetSelector.update_drawn_working_sets(managedWorkingSetList);
			dataMaskRenderer.on_working_set_disconnect(ws);
			softKeyMaskRenderer.on_working_set_disconnect(ws);

			if (managedWorkingSetList.empty())
			{
				activeWorkingSetMasterAddress = isobus::NULL_CAN_ADDRESS;
				activeWorkingSetDataMaskObjectID = isobus::NULL_OBJECT_ID;
			}
			else if (ws->get_control_function()->get_address() == activeWorkingSetMasterAddress)
			{
				bool newWorkingSetFound = false;

				for (auto &nextWorkingSet : managedWorkingSetList)
				{
					if (nextWorkingSet->get_control_function()->get_address() != activeWorkingSetMasterAddress)
					{
						activeWorkingSetMasterAddress = nextWorkingSet->get_control_function()->get_address();
						auto nextWorkingSetObject = nextWorkingSet->get_working_set_object();
						if (nextWorkingSetObject)
						{
							activeWorkingSetDataMaskObjectID = std::static_pointer_cast<isobus::WorkingSet>(nextWorkingSetObject)->get_active_mask();
							newWorkingSetFound = true;
						}
						else
						{
							activeWorkingSetDataMaskObjectID = isobus::NULL_OBJECT_ID;
							newWorkingSetFound = false;
						}
						break;
					}
				}

				if (!newWorkingSetFound)
				{
					activeWorkingSetMasterAddress = isobus::NULL_CAN_ADDRESS;
					activeWorkingSetDataMaskObjectID = isobus::NULL_OBJECT_ID;
				}
			}
			remove_working_set(ws);
			break;
		}
		else if (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == ws->get_object_pool_processing_state())
		{
			if (dataMaskRenderer.needsRepaint() || needToRepaint)
			{
				needToRepaint = false;
				repaint_data_and_soft_key_mask();
			}

			for (auto &heldButton : heldButtons)
			{
				if (isobus::SystemTiming::time_expired_ms(heldButton.timestamp_ms, 200))
				{
					bool sentMessage = false;

					if (heldButton.isSoftKey)
					{
						sentMessage = send_soft_key_activation_message(KeyActivationCode::ButtonStillHeld, heldButton.buttonObjectID, heldButton.activeMaskObjectID, heldButton.buttonKeyCode, heldButton.associatedWorkingSet->get_control_function());
					}
					else
					{
						sentMessage = send_button_activation_message(KeyActivationCode::ButtonStillHeld, heldButton.buttonObjectID, heldButton.activeMaskObjectID, heldButton.buttonKeyCode, heldButton.associatedWorkingSet->get_control_function());
					}

					if (sentMessage)
					{
						heldButton.timestamp_ms = isobus::SystemTiming::get_timestamp_ms();
					}
				}
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
	dataMaskRenderer.setBounds(100, lMenuBarHeight + 4, get_data_mask_area_size_x_pixels(), get_data_mask_area_size_y_pixels());
	vtNumberComponent.setBounds(dataMaskRenderer.getBounds().getX() + (dataMaskRenderer.getWidth() / 4.0),
	                            dataMaskRenderer.getBounds().getY() + (dataMaskRenderer.getHeight() / 10.0),
	                            dataMaskRenderer.getBounds().getWidth() / 2.0,
	                            (dataMaskRenderer.getBounds().getHeight() / 10.0) * 8);
	softKeyMaskRenderer.setBounds(100 + get_data_mask_area_size_x_pixels(),
	                              lMenuBarHeight + 4,
	                              2 * SoftKeyMaskDimensions::PADDING + get_physical_soft_key_columns() * (SoftKeyMaskDimensions::PADDING + get_soft_key_descriptor_y_pixel_height()),
	                              get_data_mask_area_size_y_pixels());
	loggerViewport.setSize(getWidth(), getHeight() * .2f);
	loggerViewport.setTopLeftPosition(0, getHeight() * .8f);
	menuBar.setBounds(lBounds.removeFromTop(lMenuBarHeight));
	logger.setSize(loggerViewport.getWidth(), logger.getHeight());

	if (logger.getHeight() < loggerViewport.getHeight())
	{
		logger.setSize(loggerViewport.getWidth(), loggerViewport.getHeight());
	}
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
	allCommands.add(static_cast<int>(CommandIDs::ConfigureReportedHardware));
	allCommands.add(static_cast<int>(CommandIDs::ConfigureShortcuts));
	allCommands.add(static_cast<int>(CommandIDs::ConfigureLogging));
	allCommands.add(static_cast<int>(CommandIDs::GenerateLogPackage));
	allCommands.add(static_cast<int>(CommandIDs::ClearISOData));
	allCommands.add(static_cast<int>(CommandIDs::StartStop));
	allCommands.add(static_cast<int>(CommandIDs::AutoStart));
#ifdef JUCE_WINDOWS
	allCommands.add(static_cast<int>(CommandIDs::ConfigureCANHardware));
#elif JUCE_LINUX
	allCommands.add(static_cast<int>(CommandIDs::ConfigureCANHardware));
#endif
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

		case CommandIDs::ConfigureReportedHardware:
		{
			result.setInfo("Reported Hardware Capabilities", "Change info reported to clients in the get hardware message", "Configure", 0);
		}
		break;

		case CommandIDs::ConfigureLogging:
		{
			result.setInfo("Logging", "Change the logging level", "Configure", 0);
		}
		break;

		case CommandIDs::GenerateLogPackage:
		{
			result.setInfo("Generate Diagnostic Package", "Creates a zip file of diagnostic information", "Troubleshooting", 0);
		}
		break;

		case CommandIDs::ClearISOData:
		{
			result.setInfo("Clear ISO Data", "Clears all saved ISO data", "Troubleshooting", 0);
		}
		break;

		case CommandIDs::ConfigureShortcuts:
		{
			result.setInfo("Configure shortcuts", "Configure keyboard shortcuts", "Configure", 0);
		}
		break;

		case CommandIDs::ConfigureCANHardware:
		{
			result.setInfo("Configure CAN Hardware", "Selects which CAN hardware to connect to", "Configure", hasStartBeenCalled ? ApplicationCommandInfo::CommandFlags::isDisabled : 0);
		}
		break;

		case CommandIDs::StartStop:
		{
			result.setInfo("Start/Stop", "Starts or stops the CAN interface", "Control", hasStartBeenCalled ? ApplicationCommandInfo::CommandFlags::isTicked : 0);
		}
		break;

		case CommandIDs::AutoStart:
		{
			result.setInfo("Auto-Start VT on launch", "Controls whether or not the VT automatically starts when the program is launched", "Control", autostart ? ApplicationCommandInfo::CommandFlags::isTicked : 0);
		}
		break;

		case CommandIDs::NoCommand:
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
			popupMenu = std::make_unique<AlertWindow>("About", "", MessageBoxIconType::InfoIcon);
			popupMenu->addTextBlock("Version: " + String(ProjectInfo::versionString));
			popupMenu->addTextBlock("Copyright 2023 Adrian Del Grosso and the Open-Agriculture Developers.");
			popupMenu->addTextBlock("This software is licensed under the GPL-3.0. You may use or change this software, even commercially, but you may not include it as part of closed-source software. Refer to this project's GitHub page for more details on your license obligations. Please retain the original copyright statements found in this software.");
			popupMenu->addTextBlock("This is an ISO11783-6 virtual terminal server application based on AgIsoStack++ and the JUCE framework. This software is intended to be used for testing ISO11783 applications that consume AgIsoStack libraries, and serves as a reference implementation of our VT server files in AgIsoStack++.");
			popupMenu->addButton("OK", 0, KeyPress(KeyPress::returnKey, 0, 0));
			popupMenu->enterModalState(true, ModalCallbackFunction::create(LanguageCommandConfigClosed{ *this }));
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
			popupMenu->getComboBoxComponent("Version")->setSelectedItemIndex(static_cast<int>(versionToReport) - 2);
			popupMenu->enterModalState(true, ModalCallbackFunction::create(LanguageCommandConfigClosed{ *this }));
			retVal = true;
		}
		break;

		case static_cast<int>(CommandIDs::ConfigureReportedHardware):
		{
			popupMenu = std::make_unique<AlertWindow>("Configure Reported VT Capabilities", "You can use this menu to configure what the server will report to clients in the \"get hardware\" message response, as well as what will be displayed in the data/soft key mask render components of this application. Some of these settings may require you to close and reopen the application to avoid weird discrepancies with connected clients.", MessageBoxIconType::NoIcon);
			popupMenu->addTextEditor("VT number", String(vtNumber), "VT number (0-31, only applied on restart)");
			popupMenu->addTextEditor("Data Mask Size (height and width)", String(dataMaskRenderer.getWidth()), "Data Mask Size (height and width)");
			popupMenu->addTextEditor("Soft Key Designator Height", String(get_soft_key_descriptor_y_pixel_height()), "Soft Key Designator Height (min 60)");
			popupMenu->addTextEditor("Soft Key Designator Width", String(get_soft_key_descriptor_x_pixel_width()), "Soft Key Designator Width (min 60)");
			popupMenu->addTextEditor("Number of Physical Soft Key columns", String(get_physical_soft_key_columns()), "Number of Physical Soft Key columns (min 1)");
			popupMenu->addTextEditor("Number of Physical Soft Key rows", String(get_physical_soft_key_rows()), "Number of Physical Soft Key rows (min 1)");

			popupMenu->getTextEditor("VT number")->setInputRestrictions(2, "1234567890");
			popupMenu->getTextEditor("Data Mask Size (height and width)")->setInputRestrictions(4, "1234567890");
			popupMenu->getTextEditor("Soft Key Designator Height")->setInputRestrictions(4, "1234567890");
			popupMenu->getTextEditor("Soft Key Designator Width")->setInputRestrictions(4, "1234567890");
			popupMenu->getTextEditor("Number of Physical Soft Key columns")->setInputRestrictions(1, "1234567890");
			popupMenu->getTextEditor("Number of Physical Soft Key rows")->setInputRestrictions(2, "1234567890");

			popupMenu->addButton("OK", 3, KeyPress(KeyPress::returnKey, 0, 0));
			popupMenu->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey, 0, 0));
			popupMenu->enterModalState(true, ModalCallbackFunction::create(LanguageCommandConfigClosed{ *this }));
			retVal = true;
		}
		break;

		case static_cast<int>(CommandIDs::ConfigureLogging):
		{
			popupMenu = std::make_unique<AlertWindow>("Configure Logging", "You can use this to change the logging level, which affects what's shown in the logging area, and what is written to the log file. Setting logging to \"debug\" may impact performance, but will provide very verbose output.", MessageBoxIconType::NoIcon);
			popupMenu->addComboBox("Logging Level", { "Debug", "Info", "Warning", "Error", "Critical" });
			popupMenu->getComboBoxComponent("Logging Level")->setSelectedItemIndex(static_cast<int>(isobus::CANStackLogger::get_log_level()));
			popupMenu->addButton("OK", 4, KeyPress(KeyPress::returnKey, 0, 0));
			popupMenu->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey, 0, 0));
			popupMenu->enterModalState(true, ModalCallbackFunction::create(LanguageCommandConfigClosed{ *this }));
			retVal = true;
		}
		break;

		case static_cast<int>(CommandIDs::ConfigureShortcuts):
		{
			popupMenu = std::make_unique<ShortcutsWindow>(alarmAckKeyCode);
			popupMenu->enterModalState(true, ModalCallbackFunction::create(LanguageCommandConfigClosed{ *this }));
			retVal = true;
		}
		break;

		case static_cast<int>(CommandIDs::GenerateLogPackage):
		{
			auto diagnosticFileBuilder = std::make_unique<ZipFile::Builder>();
			bool anyFilesAdded = false;

			auto userDataFolder = File(File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName() + File::getSeparatorString() + "Open-Agriculture" + File::getSeparatorString());
			auto userDataFiles = userDataFolder.findChildFiles(File::TypesOfFileToFind::findFiles, false, "*");
			for (auto &file : userDataFiles)
			{
				auto fileExtension = file.getFileExtension();
				if (fileExtension != ".zip")
				{
					diagnosticFileBuilder->addFile(file, 9);
					anyFilesAdded = true;
				}
			}

			auto childDirectories = userDataFolder.findChildFiles(File::TypesOfFileToFind::findDirectories, false, "*");

			for (auto &directory : childDirectories)
			{
				auto childFiles = directory.findChildFiles(File::TypesOfFileToFind::findFiles, false, "*.iop");
				for (auto &file : childFiles)
				{
					diagnosticFileBuilder->addFile(file, 9);
					anyFilesAdded = true;
				}
			}

			if (anyFilesAdded)
			{
				auto currentTime = Time::getCurrentTime().toString(true, true, true, false);
				currentTime = currentTime.replaceCharacter(' ', '_');
				currentTime = currentTime.replaceCharacter(':', '_');
				const String fileName = File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName() +
				  File::getSeparatorString() +
				  "Open-Agriculture" +
				  File::getSeparatorString() +
				  "AgISOVirtualTerminalLogs_" +
				  currentTime +
				  ".zip";
				auto output = File(fileName).createOutputStream();
				diagnosticFileBuilder->writeToStream(*output.get(), nullptr);
				File(fileName).revealToUser();
			}
			else
			{
				AlertWindow::showAsync(MessageBoxOptions()
				                         .withIconType(MessageBoxIconType::WarningIcon)
				                         .withTitle("Export Failed")
				                         .withButton("OK"),
				                       nullptr);
			}
			retVal = true;
		}
		break;

		case static_cast<int>(CommandIDs::ClearISOData):
		{
			clear_iso_data();
			retVal = true;
		}
		break;

		case static_cast<int>(CommandIDs::ConfigureCANHardware):
		{
			configureHardwareWindow = std::make_unique<ConfigureHardwareWindow>(*this, parentCANDrivers);
			configureHardwareWindow->addToDesktop();
			Rectangle<int> area(0, 0, 400, 280);
			RectanglePlacement placement(RectanglePlacement::centred |
			                             RectanglePlacement::doNotResize);
			auto result = placement.appliedTo(area, Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.reduced(20));
			configureHardwareWindow->setBounds(result);

			configureHardwareWindow->setVisible(true);
			retVal = true;
		}
		break;

		case static_cast<int>(CommandIDs::StartStop):
		{
			if (hasStartBeenCalled)
			{
				isobus::CANStackLogger::info("Stopping CAN interface");

				// Save the frame handlers so we can re-add them after stopping the interface
#ifdef JUCE_WINDOWS
				auto canDriver0 = isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0);
				auto canDriver1 = isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(1);
				auto canDriver2 = isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(2);
				auto canDriver3 = isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(3);
#else
				auto canDriver = isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0);
#endif

				isobus::CANHardwareInterface::stop();

				// Since "Stop" clears all frame handlers, we need to re-add the ones we saved
#ifdef JUCE_WINDOWS
				isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver0);
				isobus::CANHardwareInterface::assign_can_channel_frame_handler(1, canDriver1);
				isobus::CANHardwareInterface::assign_can_channel_frame_handler(2, canDriver2);
				isobus::CANHardwareInterface::assign_can_channel_frame_handler(3, canDriver3);
#else
				isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);
#endif

				dataMaskRenderer.set_has_started(false);
				hasStartBeenCalled = false;
			}
			else if (nullptr == isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0))
			{
				AlertWindow::showAsync(MessageBoxOptions()
				                         .withIconType(MessageBoxIconType::InfoIcon)
				                         .withTitle("No CAN hardware has been configured yet! Before you start the VT for the first time, select which CAN driver to use in the \"configure\" menu.")
				                         .withButton("OK"),
				                       nullptr);
			}
			else
			{
				isobus::CANStackLogger::info("Starting CAN interface");
				isobus::CANHardwareInterface::start();
				dataMaskRenderer.set_has_started(true);
				hasStartBeenCalled = true;
			}
			mCommandManager.commandStatusChanged();
			retVal = true;
		}
		break;

		case static_cast<int>(CommandIDs::AutoStart):
		{
			autostart = !autostart;
			mCommandManager.commandStatusChanged();
			save_settings();
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
	return juce::StringArray("Control", "Configure", "Troubleshooting", "About");
}

PopupMenu ServerMainComponent::getMenuForIndex(int index, const juce::String &)
{
	juce::PopupMenu retVal;

	switch (index)
	{
		case 0:
		{
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::StartStop));
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::AutoStart));
		}
		break;

		case 1:
		{
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::ConfigureLanguageCommand));
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::ConfigureReportedVersion));
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::ConfigureReportedHardware));
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::ConfigureLogging));
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::ConfigureShortcuts));

#ifdef JUCE_WINDOWS
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::ConfigureCANHardware));
#elif JUCE_LINUX
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::ConfigureCANHardware));
#endif
		}
		break;

		case 2:
		{
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::GenerateLogPackage));
			retVal.addCommandItem(&mCommandManager, static_cast<int>(CommandIDs::ClearISOData));
		}
		break;

		case 3:
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
	if ((index < managedWorkingSetList.size()) &&
	    (nullptr != managedWorkingSetList.at(index)->get_working_set_object()) &&
	    (std::static_pointer_cast<isobus::WorkingSet>(managedWorkingSetList.at(index)->get_working_set_object())->get_selectable()))
	{
		bool lProcessActivateDeactivateMacros = false;

		for (auto &ws : managedWorkingSetList)
		{
			ws->clear_callback_handles();
		}
		auto &ws = managedWorkingSetList.at(index);

		if (activeWorkingSetMasterAddress != ws->get_control_function()->get_address())
		{
			lProcessActivateDeactivateMacros = true;

			if (nullptr != activeWorkingSet)
			{
				process_macro(activeWorkingSet->get_working_set_object(), isobus::EventID::OnDeactivate, isobus::VirtualTerminalObjectType::WorkingSet, activeWorkingSet);
				process_macro(ws->get_object_by_id(std::static_pointer_cast<isobus::WorkingSet>(ws->get_working_set_object())->get_active_mask()), isobus::EventID::OnHide, isobus::VirtualTerminalObjectType::DataMask, activeWorkingSet);
			}
		}

		activeWorkingSetMasterAddress = ws->get_control_function()->get_address();

		auto workingSetObject = std::static_pointer_cast<isobus::WorkingSet>(ws->get_working_set_object());
		std::uint16_t previousActiveMask = activeWorkingSetDataMaskObjectID;
		activeWorkingSetDataMaskObjectID = std::static_pointer_cast<isobus::WorkingSet>(ws->get_working_set_object())->get_active_mask();

		dataMaskRenderer.on_change_active_mask(ws);
		softKeyMaskRenderer.on_change_active_mask(ws);
		activeWorkingSet = ws;
		process_macro(activeWorkingSet->get_working_set_object(), isobus::EventID::OnActivate, isobus::VirtualTerminalObjectType::WorkingSet, activeWorkingSet);
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

		if (previousActiveMask != activeWorkingSetDataMaskObjectID)
		{
			process_macro(ws->get_object_by_id(previousActiveMask), isobus::EventID::OnHide, isobus::VirtualTerminalObjectType::DataMask, activeWorkingSet);
			process_macro(ws->get_object_by_id(previousActiveMask), isobus::EventID::OnHide, isobus::VirtualTerminalObjectType::AlarmMask, activeWorkingSet);
			process_macro(ws->get_object_by_id(activeWorkingSetDataMaskObjectID), isobus::EventID::OnShow, isobus::VirtualTerminalObjectType::DataMask, activeWorkingSet);
			process_macro(ws->get_object_by_id(activeWorkingSetDataMaskObjectID), isobus::EventID::OnShow, isobus::VirtualTerminalObjectType::AlarmMask, activeWorkingSet);
		}
	}
}

void ServerMainComponent::set_button_held(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t objectID, std::uint16_t maskObjectID, std::uint8_t keyCode, bool isSoftKey)
{
	HeldButtonData buttonData(workingSet, objectID, maskObjectID, keyCode, isSoftKey);
	bool alreadyHeld = false;

	for (auto &button : heldButtons)
	{
		if (buttonData == button)
		{
			button.timestamp_ms = isobus::SystemTiming::get_timestamp_ms();
			alreadyHeld = true;
		}
	}

	if (!alreadyHeld)
	{
		heldButtons.push_back(buttonData);
	}
}

void ServerMainComponent::set_button_released(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t objectID, std::uint16_t maskObjectID, std::uint8_t keyCode, bool isSoftKey)
{
	HeldButtonData buttonData(workingSet, objectID, maskObjectID, keyCode, isSoftKey);
	bool alreadyHeld = false;

	auto found = std::find(heldButtons.begin(), heldButtons.end(), buttonData);
	if (heldButtons.end() != found)
	{
		heldButtons.erase(found);
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
		{
			auto languageCode = mParent.popupMenu->getTextEditorContents("Language Code");
			auto countryCode = mParent.popupMenu->getTextEditorContents("Country Code");
			auto areaUnits = static_cast<isobus::LanguageCommandInterface::AreaUnits>(mParent.popupMenu->getComboBoxComponent("Area Units")->getSelectedItemIndex());
			auto dateFormat = static_cast<isobus::LanguageCommandInterface::DateFormats>(mParent.popupMenu->getComboBoxComponent("Date Format")->getSelectedItemIndex());
			auto decimalSymbol = static_cast<isobus::LanguageCommandInterface::DecimalSymbols>(mParent.popupMenu->getComboBoxComponent("Decimal Symbol")->getSelectedItemIndex());
			auto distanceUnits = static_cast<isobus::LanguageCommandInterface::DistanceUnits>(mParent.popupMenu->getComboBoxComponent("Distance Units")->getSelectedItemIndex());
			auto forceUnits = static_cast<isobus::LanguageCommandInterface::ForceUnits>(mParent.popupMenu->getComboBoxComponent("Force Units")->getSelectedItemIndex());
			auto genericUnits = static_cast<isobus::LanguageCommandInterface::UnitSystem>(mParent.popupMenu->getComboBoxComponent("Generic Units")->getSelectedItemIndex());
			auto massUnits = static_cast<isobus::LanguageCommandInterface::MassUnits>(mParent.popupMenu->getComboBoxComponent("Mass Units")->getSelectedItemIndex());
			auto pressureUnits = static_cast<isobus::LanguageCommandInterface::PressureUnits>(mParent.popupMenu->getComboBoxComponent("Pressure Units")->getSelectedItemIndex());
			auto temperatureUnits = static_cast<isobus::LanguageCommandInterface::TemperatureUnits>(mParent.popupMenu->getComboBoxComponent("Temperature Units")->getSelectedItemIndex());
			auto timeFormat = static_cast<isobus::LanguageCommandInterface::TimeFormats>(mParent.popupMenu->getComboBoxComponent("Time Format")->getSelectedItemIndex());
			auto volumeUnits = static_cast<isobus::LanguageCommandInterface::VolumeUnits>(mParent.popupMenu->getComboBoxComponent("Volume Units")->getSelectedItemIndex());

			mParent.languageCommandInterface.set_language_code(languageCode.toStdString());
			mParent.languageCommandInterface.set_country_code(countryCode.toStdString());
			mParent.languageCommandInterface.set_commanded_area_units(areaUnits);
			mParent.languageCommandInterface.set_commanded_date_format(dateFormat);
			mParent.languageCommandInterface.set_commanded_decimal_symbol(decimalSymbol);
			mParent.languageCommandInterface.set_commanded_distance_units(distanceUnits);
			mParent.languageCommandInterface.set_commanded_force_units(forceUnits);
			mParent.languageCommandInterface.set_commanded_generic_units(genericUnits);
			mParent.languageCommandInterface.set_commanded_mass_units(massUnits);
			mParent.languageCommandInterface.set_commanded_pressure_units(pressureUnits);
			mParent.languageCommandInterface.set_commanded_temperature_units(temperatureUnits);
			mParent.languageCommandInterface.set_commanded_time_format(timeFormat);
			mParent.languageCommandInterface.set_commanded_volume_units(volumeUnits);
			mParent.languageCommandInterface.send_language_command();

			mParent.save_settings();
		}
		break;

		case 2: // Save Version
		{
			auto version = mParent.popupMenu->getComboBoxComponent("Version")->getSelectedItemIndex() + 2;
			mParent.versionToReport = get_version_from_setting(version);

			mParent.save_settings();
		}
		break;

		case 3: // Save Reported Hardware
		{
			auto dataMaskSize = mParent.popupMenu->getTextEditorContents("Data Mask Size (height and width)");
			mParent.dataMaskRenderer.setSize(dataMaskSize.getIntValue(), dataMaskSize.getIntValue());
			mParent.softKeyMaskRenderer.setTopLeftPosition(100 + dataMaskSize.getIntValue(), 4 + juce::LookAndFeel::getDefaultLookAndFeel().getDefaultMenuBarHeight());

			mParent.softKeyMaskDimensions.columnCount = mParent.popupMenu->getTextEditorContents("Number of Physical Soft Key columns").getIntValue();
			mParent.softKeyMaskDimensions.rowCount = mParent.popupMenu->getTextEditorContents("Number of Physical Soft Key rows").getIntValue();
			if (mParent.get_number_of_physical_soft_keys() != mParent.softKeyMaskDimensions.key_count())
			{
				mParent.softKeyMaskDimensions.rowCount = (mParent.get_number_of_physical_soft_keys() / mParent.softKeyMaskDimensions.columnCount);
			}

			mParent.softKeyMaskDimensions.keyWidth = mParent.popupMenu->getTextEditorContents("Soft Key Designator Width").getIntValue();
			mParent.softKeyMaskDimensions.keyHeight = mParent.popupMenu->getTextEditorContents("Soft Key Designator Height").getIntValue();
			JuceManagedWorkingSetCache::set_softkey_mask_dimension_info(mParent.softKeyMaskDimensions);

			mParent.softKeyMaskRenderer.setSize(mParent.softKeyMaskDimensions.total_width(), dataMaskSize.getIntValue());

			mParent.vtNumber = mParent.popupMenu->getTextEditorContents("VT Number").getIntValue();
			if (mParent.vtNumber > 32)
			{
				mParent.vtNumber = 32;
			}
			else if (mParent.vtNumber == 0)
			{
				mParent.vtNumber = 1;
			}

			mParent.save_settings();
			mParent.repaint_data_and_soft_key_mask();
		}
		break;

		case 4: // Log level
		{
			isobus::CANStackLogger::set_log_level(static_cast<isobus::CANStackLogger::LoggingLevel>(mParent.popupMenu->getComboBoxComponent("Logging Level")->getSelectedItemIndex()));
			mParent.save_settings();
		}
		break;

		case 5: // Shortcuts
		{
			mParent.alarmAckKeyCode = dynamic_cast<ShortcutsWindow *>(mParent.popupMenu.get())->alarmAckKeyCode();
			mParent.save_settings();
		}

		default:
		{
			// Cancel. Do nothing
		}
		break;
	}
	mParent.exitModalState(result);
	mParent.popupMenu.reset();
}

ServerMainComponent::HeldButtonData::HeldButtonData(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::uint16_t objectID, std::uint16_t maskObjectID, std::uint8_t keyCode, bool isSoftKey) :
  isSoftKey(isSoftKey),
  associatedWorkingSet(workingSet),
  timestamp_ms(isobus::SystemTiming::get_timestamp_ms()),
  buttonObjectID(objectID),
  activeMaskObjectID(maskObjectID),
  buttonKeyCode(keyCode)
{
}

bool ServerMainComponent::HeldButtonData::operator==(const HeldButtonData &other)
{
	return ((other.activeMaskObjectID == activeMaskObjectID) &&
	        (other.associatedWorkingSet == associatedWorkingSet) &&
	        (other.buttonObjectID == buttonObjectID) &&
	        (other.buttonKeyCode == buttonKeyCode));
}

ServerMainComponent::VTVersion ServerMainComponent::get_version_from_setting(std::uint8_t aVersion)
{
	VTVersion retVal = VTVersion::Version2OrOlder;

	switch (aVersion)
	{
		case 3:
		{
			retVal = VTVersion::Version3;
		}
		break;

		case 4:
		{
			retVal = VTVersion::Version4;
		}
		break;

		case 5:
		{
			retVal = VTVersion::Version5;
		}
		break;

		case 6:
		{
			retVal = VTVersion::Version6;
		}
		break;

		default:
			break;
	}
	return retVal;
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

bool ServerMainComponent::timeAndDateCallback(isobus::TimeDateInterface::TimeAndDate &timeAndDate)
{
	auto now = std::chrono::system_clock::now();
	auto durationSinceEpoch = now.time_since_epoch();
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(durationSinceEpoch).count() % 1000;

	std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::tm localTime = *std::localtime(&t);

	timeAndDate.milliseconds = static_cast<std::uint16_t>((millis / 250) * 250);
	timeAndDate.seconds = static_cast<std::uint8_t>(localTime.tm_sec);
	timeAndDate.minutes = static_cast<std::uint8_t>(localTime.tm_min);
	timeAndDate.hours = static_cast<std::uint8_t>(localTime.tm_hour);
	timeAndDate.quarterDays = static_cast<std::uint8_t>(localTime.tm_hour / 6);
	timeAndDate.day = static_cast<std::uint8_t>(localTime.tm_mday);
	timeAndDate.month = static_cast<std::uint8_t>(localTime.tm_mon + 1);
	timeAndDate.year = static_cast<std::uint16_t>(localTime.tm_year + 1900);

	std::tm utcTime = *std::gmtime(&t);
	timeAndDate.localHourOffset = static_cast<std::int8_t>(localTime.tm_hour - utcTime.tm_hour);
	timeAndDate.localMinuteOffset = static_cast<std::int8_t>(localTime.tm_min - utcTime.tm_min);
	return true;
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
			if (isobus::VirtualTerminalObjectType::AlarmMask == activeMask->get_object_type())
			{
				auto alarmMask = std::static_pointer_cast<isobus::AlarmMask>(activeMask);
				activeWorkingSetSoftkeyMaskObjectID = alarmMask->get_soft_key_mask();

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

					case isobus::AlarmMask::AcousticSignal::None:
					default:
						break;
				}
				process_macro(activeMask, isobus::EventID::OnShow, isobus::VirtualTerminalObjectType::AlarmMask, activeWorkingSet);
				process_macro(activeMask, isobus::EventID::OnChangeActiveMask, isobus::VirtualTerminalObjectType::AlarmMask, activeWorkingSet);
			}
			else if (isobus::VirtualTerminalObjectType::DataMask == activeMask->get_object_type())
			{
				auto dataMask = std::static_pointer_cast<isobus::DataMask>(activeMask);
				activeWorkingSetSoftkeyMaskObjectID = dataMask->get_soft_key_mask();
				// Also process macros for the actual datamask (container) show event
				process_macro(activeMask, isobus::EventID::OnShow, isobus::VirtualTerminalObjectType::DataMask, activeWorkingSet);
				process_macro(activeMask, isobus::EventID::OnChangeActiveMask, isobus::VirtualTerminalObjectType::DataMask, activeWorkingSet);
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

void ServerMainComponent::check_load_settings(std::shared_ptr<ValueTree> settings)
{
	int index = 0;
	auto child = settings->getChild(index);

	while (child.isValid())
	{
		if (Identifier("LanguageCommand") == child.getType())
		{
			if (!child.getProperty("AreaUnits").isVoid())
			{
				languageCommandInterface.set_commanded_area_units(static_cast<isobus::LanguageCommandInterface::AreaUnits>(int(child.getProperty("AreaUnits"))));
			}
			if (!child.getProperty("DateFormat").isVoid())
			{
				languageCommandInterface.set_commanded_date_format(static_cast<isobus::LanguageCommandInterface::DateFormats>(int(child.getProperty("DateFormat"))));
			}
			if (!child.getProperty("DecimalSymbol").isVoid())
			{
				languageCommandInterface.set_commanded_decimal_symbol(static_cast<isobus::LanguageCommandInterface::DecimalSymbols>(int(child.getProperty("DecimalSymbol"))));
			}
			if (!child.getProperty("DistanceUnits").isVoid())
			{
				languageCommandInterface.set_commanded_distance_units(static_cast<isobus::LanguageCommandInterface::DistanceUnits>(int(child.getProperty("DistanceUnits"))));
			}
			if (!child.getProperty("ForceUnits").isVoid())
			{
				languageCommandInterface.set_commanded_force_units(static_cast<isobus::LanguageCommandInterface::ForceUnits>(int(child.getProperty("ForceUnits"))));
			}
			if (!child.getProperty("UnitSystem").isVoid())
			{
				languageCommandInterface.set_commanded_generic_units(static_cast<isobus::LanguageCommandInterface::UnitSystem>(int(child.getProperty("UnitSystem"))));
			}
			if (!child.getProperty("MassUnits").isVoid())
			{
				languageCommandInterface.set_commanded_mass_units(static_cast<isobus::LanguageCommandInterface::MassUnits>(int(child.getProperty("MassUnits"))));
			}
			if (!child.getProperty("PressureUnits").isVoid())
			{
				languageCommandInterface.set_commanded_pressure_units(static_cast<isobus::LanguageCommandInterface::PressureUnits>(int(child.getProperty("PressureUnits"))));
			}
			if (!child.getProperty("TemperatureUnits").isVoid())
			{
				languageCommandInterface.set_commanded_temperature_units(static_cast<isobus::LanguageCommandInterface::TemperatureUnits>(int(child.getProperty("TemperatureUnits"))));
			}
			if (!child.getProperty("TimeFormat").isVoid())
			{
				languageCommandInterface.set_commanded_time_format(static_cast<isobus::LanguageCommandInterface::TimeFormats>(int(child.getProperty("TimeFormat"))));
			}
			if (!child.getProperty("VolumeUnits").isVoid())
			{
				languageCommandInterface.set_commanded_volume_units(static_cast<isobus::LanguageCommandInterface::VolumeUnits>(int(child.getProperty("VolumeUnits"))));
			}
			if (!child.getProperty("CountryCode").isVoid())
			{
				languageCommandInterface.set_country_code(String(child.getProperty("CountryCode").toString()).toStdString());
			}
			if (!child.getProperty("LanguageCode").isVoid())
			{
				languageCommandInterface.set_language_code(String(child.getProperty("LanguageCode").toString()).toStdString());
			}
		}
		else if (Identifier("Compatibility") == child.getType())
		{
			if (!child.getProperty("Version").isVoid())
			{
				versionToReport = get_version_from_setting(static_cast<std::uint8_t>(static_cast<int>(child.getProperty("Version"))));
			}
		}
		else if (Identifier("Hardware") == child.getType())
		{
			if (!child.getProperty("SoftKeyDesignatorWidth").isVoid())
			{
				softKeyMaskDimensions.keyWidth = static_cast<std::uint16_t>(static_cast<int>(child.getProperty("SoftKeyDesignatorWidth")));
			}
			if (!child.getProperty("SoftKeyDesignatorHeight").isVoid())
			{
				softKeyMaskDimensions.keyHeight = static_cast<std::uint16_t>(static_cast<int>(child.getProperty("SoftKeyDesignatorHeight")));
			}
			if (!child.getProperty("SoftkeyColumnCount").isVoid())
			{
				softKeyMaskDimensions.columnCount = static_cast<std::uint16_t>(static_cast<int>(child.getProperty("SoftkeyColumnCount")));
			}
			if (!child.getProperty("SoftkeyRowCount").isVoid())
			{
				softKeyMaskDimensions.rowCount = static_cast<std::uint16_t>(static_cast<int>(child.getProperty("SoftkeyRowCount")));
			}
			if (!child.getProperty("DataMaskRenderAreaSize").isVoid())
			{
				dataMaskRenderer.setSize(static_cast<std::uint16_t>(static_cast<int>(child.getProperty("DataMaskRenderAreaSize"))), static_cast<std::uint16_t>(static_cast<int>(child.getProperty("DataMaskRenderAreaSize"))));
				softKeyMaskRenderer.setSize(2 * SoftKeyMaskDimensions::PADDING + get_physical_soft_key_columns() * (SoftKeyMaskDimensions::PADDING + get_soft_key_descriptor_y_pixel_height()),
				                            static_cast<int>(child.getProperty("DataMaskRenderAreaSize")));
			}
#ifdef JUCE_WINDOWS
			if (!child.getProperty("TouCANSerial").isVoid())
			{
				std::static_pointer_cast<isobus::TouCANPlugin>(parentCANDrivers.at(2))->reconfigure(0, static_cast<std::uint32_t>(static_cast<int>(child.getProperty("TouCANSerial"))));
			}

			if (!child.getProperty("CANDriver").isVoid())
			{
				auto index = static_cast<std::uint32_t>(static_cast<int>(child.getProperty("CANDriver")));

				if (index < parentCANDrivers.size())
				{
					isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, parentCANDrivers.at(index));
					isobus::CANStackLogger::debug("CAN Driver selection loaded from config file.");
				}
			}
#elif JUCE_LINUX
			if (!child.getProperty("SocketCANInterface").isVoid())
			{
				std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->set_name(static_cast<String>(child.getProperty("SocketCANInterface")).toStdString());
				isobus::CANStackLogger::info("Using Socket CAN interface name of: " + std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->get_device_name());
			}
			else
			{
				std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->set_name("can0");
				isobus::CANStackLogger::warn("Socket CAN interface name not yet configured. Using default of \"can0\"");
			}
#endif
			softKeyMaskRenderer.setTopLeftPosition(100 + dataMaskRenderer.getWidth(), 4 + juce::LookAndFeel::getDefaultLookAndFeel().getDefaultMenuBarHeight());
			JuceManagedWorkingSetCache::set_softkey_mask_dimension_info(softKeyMaskDimensions);
		}
		else if (Identifier("Logging") == child.getType())
		{
			if ((!child.getProperty("Level").isVoid()) && (static_cast<int>(child.getProperty("Level")) <= static_cast<int>(isobus::CANStackLogger::LoggingLevel::Critical)))
			{
				isobus::CANStackLogger::set_log_level(static_cast<isobus::CANStackLogger::LoggingLevel>(static_cast<int>(child.getProperty("Level"))));
			}
		}
		else if (Identifier("Control") == child.getType())
		{
			if (!child.getProperty("AutoStart").isVoid())
			{
				autostart = static_cast<bool>(static_cast<int>(child.getProperty("AutoStart")));

				if (autostart)
				{
					isobus::CANHardwareInterface::start();
					dataMaskRenderer.set_has_started(true);
					hasStartBeenCalled = true;
					isobus::CANStackLogger::info("AutoStart enabled. Starting CAN hardware interface.");
				}
			}

			if (!child.getProperty("AlarmAckKey").isVoid())
			{
				alarmAckKeyCode = static_cast<int>(child.getProperty("AlarmAckKey"));
			}
		}
		index++;
		child = settings->getChild(index);
	}

	if (!autostart)
	{
		isobus::CANStackLogger::info("AutoStart is disabled. Waiting for user to start the CAN hardware interface.");
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
		ValueTree hardwareSettings("Hardware");
		ValueTree loggingSettings("Logging");
		ValueTree controlSettings("Control");

		std::uint32_t hardwareDriverIndex = 0xFFFFFFFF;

		for (std::uint32_t i = 0; i < parentCANDrivers.size(); i++)
		{
			if (parentCANDrivers.at(i) == isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0))
			{
				hardwareDriverIndex = i;
				break;
			}
		}

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
		hardwareSettings.setProperty("DataMaskRenderAreaSize", dataMaskRenderer.getWidth(), nullptr);
		hardwareSettings.setProperty("VT_Number", vtNumber, nullptr);
		hardwareSettings.setProperty("SoftKeyDesignatorWidth", softKeyMaskDimensions.keyWidth, nullptr);
		hardwareSettings.setProperty("SoftKeyDesignatorHeight", softKeyMaskDimensions.keyHeight, nullptr);
		hardwareSettings.setProperty("SoftkeyColumnCount", softKeyMaskDimensions.columnCount, nullptr);
		hardwareSettings.setProperty("SoftkeyRowCount", softKeyMaskDimensions.rowCount, nullptr);

#ifdef JUCE_WINDOWS
		hardwareSettings.setProperty("TouCANSerial", static_cast<int>(std::static_pointer_cast<isobus::TouCANPlugin>(parentCANDrivers.at(2))->get_serial_number()), nullptr);
#elif JUCE_LINUX
		hardwareSettings.setProperty("SocketCANInterface", String(std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->get_device_name()), nullptr);
#endif

		if (0xFFFFFFFF != hardwareDriverIndex)
		{
			hardwareSettings.setProperty("CANDriver", static_cast<int>(hardwareDriverIndex), nullptr);
		}
		loggingSettings.setProperty("Level", static_cast<int>(isobus::CANStackLogger::get_log_level()), nullptr);
		controlSettings.setProperty("AutoStart", autostart, nullptr);
		controlSettings.setProperty("AlarmAckKey", alarmAckKeyCode, nullptr);
		settings.appendChild(languageCommandSettings, nullptr);
		settings.appendChild(compatibilitySettings, nullptr);
		settings.appendChild(hardwareSettings, nullptr);
		settings.appendChild(loggingSettings, nullptr);
		settings.appendChild(controlSettings, nullptr);
		std::unique_ptr<XmlElement> xml(settings.createXml());

		if (nullptr != xml)
		{
			xml->writeTo(settingsFile);
		}
	}
}

void ServerMainComponent::identify_vt()
{
	// first check if we have active alarm
	for (auto &ws : managedWorkingSetList)
	{
		if (activeWorkingSetMasterAddress == ws->get_control_function()->get_address())
		{
			auto activeMask = ws->get_object_by_id(std::static_pointer_cast<isobus::WorkingSet>(ws->get_working_set_object())->get_active_mask());
			if (nullptr != activeMask && isobus::VirtualTerminalObjectType::AlarmMask == activeMask->get_object_type())
			{
				return;
			}
		}
	}

	juce::MessageManager::callAsync([this] {
		vtNumberComponent.setVisible(true);
		repaint();
		juce::Timer::callAfterDelay(3000, [this]() { vtNumberComponent.setVisible(false); });
	});
}

void ServerMainComponent::remove_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSetToRemove)
{
	for (auto it = managedWorkingSetList.begin(); it != managedWorkingSetList.end(); it++)
	{
		if (workingSetToRemove == *it)
		{
			managedWorkingSetList.erase(it);
			break;
		}
	}
}

void ServerMainComponent::clear_iso_data()
{
	File isoDirectory(File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName().toStdString() +
	                  File::getSeparatorString() +
	                  "Open-Agriculture" +
	                  File::getSeparatorString() +
	                  ISO_DATA_PATH +
	                  File::getSeparatorString());

	if (isoDirectory.exists() && isoDirectory.isDirectory())
	{
		isoDirectory.deleteRecursively();
		isobus::CANStackLogger::info("ISO Data cleared");
	}
}
