//================================================================================================
/// @file CANDeviceList.hpp
///
/// @brief Describes the PCAN and candleLight adapters, without calling their drivers.
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

/// @brief Describes the PCAN and candleLight adapters, so the operator can pick one when several are connected
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

	/// @brief Lists the attached adapters for the driver at this position
	using ListFunction = std::vector<Device> (*)(std::size_t driverIndex);

	/// @brief Creates the driver at this position for the given adapter
	using CreateDriverFunction = std::shared_ptr<isobus::CANHardwarePlugin> (*)(std::size_t driverIndex, const Device &device);

	/// @brief The position of the PCAN driver in the driver list built in Main.cpp
	static constexpr std::size_t PCAN_DRIVER_INDEX = 0;
	/// @brief The position of the candleLight driver in the driver list built in Main.cpp
	static constexpr std::size_t CANDLELIGHT_DRIVER_INDEX = 4;

	/// @brief Returns whether the driver at this position in the driver list picks one of several adapters
	static bool has_device_list(std::size_t driverIndex);

	/// @brief Returns the adapter the driver uses when nothing else was selected
	static Device get_default_device(std::size_t driverIndex);

	/// @brief Returns the listed adapter with this serial, or with this ID if it has no serial
	static std::optional<Device> find_device(const std::vector<Device> &devices, const Device &wanted);

private:
	/// @brief Returns whether a listed adapter is the wanted one
	static bool is_same_device(const Device &device, const Device &wanted);
};

#endif // CAN_DEVICE_LIST_HPP
