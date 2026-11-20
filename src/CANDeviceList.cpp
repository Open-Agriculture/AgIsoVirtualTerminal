/*******************************************************************************
** @file       CANDeviceList.cpp
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "CANDeviceList.hpp"

#include "isobus/hardware_integration/available_can_drivers.hpp"

#include <algorithm>

bool CANDeviceList::has_device_list(std::size_t driverIndex)
{
#ifdef _WIN32
	return (PCAN_DRIVER_INDEX == driverIndex) || (CANDLELIGHT_DRIVER_INDEX == driverIndex);
#else
	(void)driverIndex;
	return false;
#endif
}

CANDeviceList::Device CANDeviceList::get_default_device(std::size_t driverIndex)
{
	Device device;

	if (PCAN_DRIVER_INDEX == driverIndex)
	{
#ifdef ISOBUS_WINDOWSPCANBASIC_AVAILABLE
		device.id = PCAN_USBBUS1;
#endif
		device.description = "PCAN-USB channel 1 (no adapter found)";
	}
	else
	{
		device.description = "First candleLight adapter (no adapter found)";
	}
	return device;
}

std::optional<CANDeviceList::Device> CANDeviceList::find_device(const std::vector<Device> &devices, const Device &wanted)
{
	const auto match = std::find_if(devices.begin(), devices.end(), [&wanted](const Device &device) { return is_same_device(device, wanted); });

	if (devices.end() == match)
	{
		return std::nullopt;
	}
	return *match;
}

bool CANDeviceList::is_same_device(const Device &device, const Device &wanted)
{
	// The ID changes when adapters are plugged in another order, the serial doesn't
	return wanted.serial.empty() ? (device.id == wanted.id) : (device.serial == wanted.serial);
}
