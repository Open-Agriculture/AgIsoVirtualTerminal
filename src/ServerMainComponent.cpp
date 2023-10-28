#include "ServerMainComponent.hpp"

#include "AlarmMaskAudio.h"
#include "JuceManagedWorkingSetCache.hpp"
#include "isobus/utility/system_timing.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

ServerMainComponent::ServerMainComponent(std::shared_ptr<isobus::InternalControlFunction> serverControlFunction) :
  VirtualTerminalServer(serverControlFunction),
  dataMaskRenderer(*this),
  softKeyMaskRenderer(*this)
{
	VirtualTerminalServer::initialize();
	mAudioDeviceManager.initialise(0, 1, nullptr, true);
	mAudioDeviceManager.addAudioCallback(&mSoundPlayer);
	addAndMakeVisible(workingSetSelector);
	addAndMakeVisible(dataMaskRenderer);
	addAndMakeVisible(softKeyMaskRenderer);
	// Make sure you set the size of the component after
	// you add any child components.
	setSize(800, 800);
	// setFramesPerSecond(60); // This sets the frequency of the update calls.
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
	return VTVersion::Version6;
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

isobus::VirtualTerminalBase::SupportedWideCharsErrorCode ServerMainComponent::get_supported_wide_chars(std::uint8_t codePlane,
                                                                                                       std::uint16_t firstWideCharInInquiryRange,
                                                                                                       std::uint16_t lastWideCharInInquiryRange,
                                                                                                       std::uint8_t &numberOfRanges,
                                                                                                       std::vector<std::uint8_t> &wideCharRangeArray)
{
	return isobus::VirtualTerminalBase::SupportedWideCharsErrorCode::AnyOtherError;
}

std::vector<std::uint8_t> ServerMainComponent::get_versions(isobus::NAME clientNAME)
{
	return std::vector<std::uint8_t>();
}

std::vector<std::uint8_t> ServerMainComponent::load_version(const std::vector<std::uint8_t> &versionLabel, isobus::NAME clientNAME)
{
	std::ostringstream nameString;
	std::vector<std::uint8_t> loadedIOPData;
	std::vector<std::uint8_t> loadedVersionLabel(7);
	std::streampos fileSize;
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
							iopxFile.seekg(0, std::ios::end);
							fileSize = iopxFile.tellg();
							iopxFile.seekg(7, std::ios::beg);

							loadedIOPData.reserve(fileSize);
							loadedIOPData.insert(loadedIOPData.begin(), std::istream_iterator<std::uint8_t>(iopxFile), std::istream_iterator<std::uint8_t>());
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
				activeWorkingSetMasterAddress = ws->get_control_function()->get_address();
				activeWorkingSetDataMaskObjectID = std::static_pointer_cast<isobus::WorkingSet>(ws->get_working_set_object())->get_active_mask();
				ws->set_working_set_maintenance_message_timestamp_ms(isobus::SystemTiming::get_timestamp_ms());

				dataMaskRenderer.on_change_active_mask(ws);
				softKeyMaskRenderer.on_change_active_mask(ws);
				activeWorkingSet = ws;
				ws->save_callback_handle(get_on_change_active_mask_event_dispatcher().add_listener([this](std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t workingSet, std::uint16_t newMask) { this->on_change_active_mask_callback(affectedWorkingSet, workingSet, newMask); }));
				ws->save_callback_handle(get_on_change_numeric_value_event_dispatcher().add_listener([this](std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t objectID, std::uint32_t value) { this->on_change_numeric_value_callback(affectedWorkingSet, objectID, value); }));
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
			//workingSetSelector.remove_working_set(std::static_pointer_cast<isobus::WorkingSet>(ws->get_working_set_object()));
		}
		else if (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == ws->get_object_pool_processing_state())
		{
			if (dataMaskRenderer.needsRepaint())
			{
				dataMaskRenderer.on_change_active_mask(ws);
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

void ServerMainComponent::on_change_active_mask_callback(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t workingSet, std::uint16_t newMask)
{
	if (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == affectedWorkingSet->get_object_pool_processing_state())
	{
		if (activeWorkingSet == affectedWorkingSet)
		{
			const MessageManagerLock mmLock;
			dataMaskRenderer.on_change_active_mask(activeWorkingSet);
			softKeyMaskRenderer.on_change_active_mask(activeWorkingSet);

			auto activeMask = affectedWorkingSet->get_object_by_id(newMask);

			if ((nullptr != activeMask) && (isobus::VirtualTerminalObjectType::AlarmMask == activeMask->get_object_type()))
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

void ServerMainComponent::on_change_numeric_value_callback(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t objectID, std::uint32_t value)
{
	if (isobus::VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == affectedWorkingSet->get_object_pool_processing_state())
	{
		if (activeWorkingSet == affectedWorkingSet)
		{
			const MessageManagerLock mmLock;
			auto affectedObject = affectedWorkingSet->get_object_by_id(objectID);

			switch (affectedObject->get_object_type())
			{
				case isobus::VirtualTerminalObjectType::InputBoolean:
				{
					std::static_pointer_cast<isobus::InputBoolean>(affectedObject)->set_value(value);
					repaint_data_and_soft_key_mask();
				}
				break;

				case isobus::VirtualTerminalObjectType::InputNumber:
				{
					std::static_pointer_cast<isobus::InputNumber>(affectedObject)->set_value(value);
					repaint_data_and_soft_key_mask();
				}
				break;

				case isobus::VirtualTerminalObjectType::InputList:
				{
					std::static_pointer_cast<isobus::InputList>(affectedObject)->set_value(value);
					repaint_data_and_soft_key_mask();
				}
				break;

				case isobus::VirtualTerminalObjectType::OutputNumber:
				{
					std::static_pointer_cast<isobus::OutputNumber>(affectedObject)->set_value(value);
					repaint_data_and_soft_key_mask();
				}
				break;

				case isobus::VirtualTerminalObjectType::OutputList:
				{
					std::static_pointer_cast<isobus::OutputList>(affectedObject)->set_value(value);
					repaint_data_and_soft_key_mask();
				}
				break;

				case isobus::VirtualTerminalObjectType::OutputMeter:
				{
					std::static_pointer_cast<isobus::OutputMeter>(affectedObject)->set_value(value);
					repaint_data_and_soft_key_mask();
				}
				break;

				case isobus::VirtualTerminalObjectType::OutputLinearBarGraph:
				{
					std::static_pointer_cast<isobus::OutputLinearBarGraph>(affectedObject)->set_value(value);
					repaint_data_and_soft_key_mask();
				}
				break;

				case isobus::VirtualTerminalObjectType::OutputArchedBarGraph:
				{
					std::static_pointer_cast<isobus::OutputArchedBarGraph>(affectedObject)->set_value(value);
					repaint_data_and_soft_key_mask();
				}
				break;

				case isobus::VirtualTerminalObjectType::NumberVariable:
				{
					std::static_pointer_cast<isobus::NumberVariable>(affectedObject)->set_value(value);
					repaint_data_and_soft_key_mask();
				}
				break;

				case isobus::VirtualTerminalObjectType::ObjectPointer:
				{
					std::static_pointer_cast<isobus::ObjectPointer>(affectedObject)->pop_child();
					std::static_pointer_cast<isobus::ObjectPointer>(affectedObject)->add_child(value, 0, 0);
					repaint_data_and_soft_key_mask();
				}
				break;

				case isobus::VirtualTerminalObjectType::ExternalObjectPointer:
				{
					// TODO
				}
				break;

				case isobus::VirtualTerminalObjectType::Animation:
				{
					//Todo std::static_pointer_cast<Animation>(lTargetObject)->set_value(value);
				}
				break;

				default:
				{
				}
				break;
			}
		}
	}
}

void ServerMainComponent::repaint_data_and_soft_key_mask()
{
	dataMaskRenderer.on_change_active_mask(activeWorkingSet);
	softKeyMaskRenderer.on_change_active_mask(activeWorkingSet);
}
