#pragma once

#include "DataMaskRenderAreaComponent.hpp"
#include "SoftKeyMaskRenderAreaComponent.hpp"
#include "WorkingSetSelectorComponent.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server.hpp"

#include <filesystem>

class ServerMainComponent : public juce::Component
  , public isobus::VirtualTerminalServer
  , public Timer
{
public:
	ServerMainComponent(std::shared_ptr<isobus::InternalControlFunction> serverControlFunction);
	~ServerMainComponent() override;

	bool get_is_enough_memory(std::uint32_t requestedMemory) const override;
	VTVersion get_version() const override;
	std::uint8_t get_number_of_navigation_soft_keys() const override;
	std::uint8_t get_soft_key_descriptor_x_pixel_width() const override;
	std::uint8_t get_soft_key_descriptor_y_pixel_width() const override;
	std::uint8_t get_number_of_possible_virtual_soft_keys_in_soft_key_mask() const override;
	std::uint8_t get_number_of_physical_soft_keys() const override;
	std::uint16_t get_data_mask_area_size_x_pixels() const override;
	std::uint16_t get_data_mask_area_size_y_pixels() const override;
	void suspend_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSetWithError) override;
	SupportedWideCharsErrorCode get_supported_wide_chars(std::uint8_t codePlane,
	                                                     std::uint16_t firstWideCharInInquiryRange,
	                                                     std::uint16_t lastWideCharInInquiryRange,
	                                                     std::uint8_t &numberOfRanges,
	                                                     std::vector<std::uint8_t> &wideCharRangeArray) override;

	std::vector<std::uint8_t> get_versions(isobus::NAME clientNAME) override;
	std::vector<std::uint8_t> get_supported_objects() const override;

	/// @brief This function is called when the client wants the server to load a previously stored object pool.
	/// If there exists in the VT's non-volatile memory an object pool matching the provided version label,
	/// return it. If one does not exist, return an empty vector.
	/// @param[in] versionLabel The object pool version to load for the given client NAME
	/// @param[in] clientNAME The client requesting the object pool
	/// @returns The requested object pool associated with the version label.
	virtual std::vector<std::uint8_t> load_version(const std::vector<std::uint8_t> &versionLabel, isobus::NAME clientNAME) override;

	/// @brief This function is called when the client wants the server to save an object pool
	/// to the VT's non-volatile memory.
	/// If the object pool is saved successfully, return true, otherwise return false.
	/// @note This may be called multiple times with the same version, but different data. When this
	/// happens, the expectation is that you will append each objectPool together into one large file.
	/// @param[in] objectPool The object pool data to save
	/// @param[in] versionLabel The object pool version to save for the given client NAME
	/// @param[in] clientNAME The client requesting the object pool
	/// @returns The requested object pool associated with the version label.
	virtual bool save_version(const std::vector<std::uint8_t> &objectPool, const std::vector<std::uint8_t> &versionLabel, isobus::NAME clientNAME) override;

	void timerCallback() override;

	void paint(juce::Graphics &g) override;
	void resized() override;

	std::shared_ptr<isobus::ControlFunction> get_client_control_function_for_working_set(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet) const;

private:
	// Your private member variables go here...
	std::size_t number_of_iop_files_in_directory(std::filesystem::path path);

	void on_change_active_mask_callback(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t workingSet, std::uint16_t newMask);
	void on_change_numeric_value_callback(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> affectedWorkingSet, std::uint16_t objectID, std::uint32_t value);
	void repaint_data_and_soft_key_mask();

	const std::string ISO_DATA_PATH = "iso_data";

	WorkingSetSelectorComponent workingSetSelector;
	DataMaskRenderAreaComponent dataMaskRenderer;
	SoftKeyMaskRenderAreaComponent softKeyMaskRenderer;
	SoundPlayer mSoundPlayer;
	AudioDeviceManager mAudioDeviceManager;
	std::uint8_t numberOfPoolsToRender = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ServerMainComponent)
};
