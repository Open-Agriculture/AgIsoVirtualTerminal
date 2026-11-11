//================================================================================================
/// @file CANDeviceDrivers.hpp
///
/// @brief Finds the attached PCAN and candleLight adapters and creates their drivers.
/// @author The Open-Agriculture Developers
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef CAN_DEVICE_DRIVERS_HPP
#define CAN_DEVICE_DRIVERS_HPP

#include "CANDeviceList.hpp"

/// @brief The part of the adapter list that calls the drivers, so only the VT links it and not vt-render
class CANDeviceDrivers
{
public:
	/// @brief Returns the attached adapters for the driver at this position, or the default adapter if none is found
	static std::vector<CANDeviceList::Device> list_devices(std::size_t driverIndex);

	/// @brief Creates the driver at this position for the given adapter
	static std::shared_ptr<isobus::CANHardwarePlugin> create_driver(std::size_t driverIndex, const CANDeviceList::Device &device);

private:
	/// @brief Returns the attached PCAN channels
	static std::vector<CANDeviceList::Device> list_pcan_devices();

	/// @brief Returns the attached candleLight adapters, in the order the driver numbers them
	static std::vector<CANDeviceList::Device> list_candlelight_devices();
};

#endif // CAN_DEVICE_DRIVERS_HPP
