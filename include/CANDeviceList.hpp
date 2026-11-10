//================================================================================================
/// @file CANDeviceList.hpp
///
/// @brief Lists the PCAN and candleLight adapters attached to this computer.
/// @author The Open-Agriculture Developers
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef CAN_DEVICE_LIST_HPP
#define CAN_DEVICE_LIST_HPP

#include "isobus/hardware_integration/can_hardware_plugin.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/// @brief Finds the attached PCAN and candleLight adapters, so the operator can pick one when several are connected
class CANDeviceList
{
public:
	/// @brief An attached adapter
	struct Device
	{
		std::uint32_t id = 0; ///< The PCAN channel handle, or the candleLight device index
		std::string serial; ///< Identifies the adapter across restarts: the PCAN device ID or the USB serial number
		std::string description; ///< What the operator sees
	};

	/// @brief The position of the PCAN driver in the driver list built in Main.cpp
	static constexpr std::size_t PCAN_DRIVER_INDEX = 0;
	/// @brief The position of the candleLight driver in the driver list built in Main.cpp
	static constexpr std::size_t CANDLELIGHT_DRIVER_INDEX = 4;

	/// @brief Returns whether the driver at this position in the driver list picks one of several adapters
	static bool has_device_list(std::size_t driverIndex);

	/// @brief Returns the attached adapters for the driver at this position, or the default adapter if none is found
	static std::vector<Device> list_devices(std::size_t driverIndex);

	/// @brief Returns the adapter the driver uses when nothing else was selected
	static Device get_default_device(std::size_t driverIndex);

	/// @brief Returns the listed adapter with this serial, or with this ID if it has no serial
	static std::optional<Device> find_device(const std::vector<Device> &devices, const Device &wanted);

	/// @brief Creates the driver at this position for the given adapter
	static std::shared_ptr<isobus::CANHardwarePlugin> create_driver(std::size_t driverIndex, const Device &device);

private:
	/// @brief Returns whether a listed adapter is the wanted one
	static bool is_same_device(const Device &device, const Device &wanted);

	/// @brief Returns the attached PCAN channels
	static std::vector<Device> list_pcan_devices();

	/// @brief Returns the attached candleLight adapters, in the order the driver numbers them
	static std::vector<Device> list_candlelight_devices();
};

#endif // CAN_DEVICE_LIST_HPP
