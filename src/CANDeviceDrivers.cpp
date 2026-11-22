/*******************************************************************************
** @file       CANDeviceDrivers.cpp
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "CANDeviceDrivers.hpp"

#include "isobus/hardware_integration/available_can_drivers.hpp"

#include <algorithm>
#include <cstring>

#ifdef ISOBUS_WINDOWSCANDLELIGHT_AVAILABLE
#include <Windows.h>

// initguid.h makes devpkey.h define the property keys instead of only declaring them, so it comes first
#include <initguid.h>

#include <devpkey.h>

#include <cfgmgr32.h>
#include <setupapi.h>

/// The WinUSB device interface GUID of candleLight firmware, the one the driver opens
static const GUID CANDLELIGHT_DEVICE_GUID = { 0xc15b4308, 0x04d3, 0x11e6, { 0xb3, 0xea, 0x60, 0x57, 0x18, 0x9e, 0x64, 0x43 } };

/// @brief Returns the name the adapter reports over USB, such as "candleLight USB to CAN adapter"
static std::string get_candlelight_name(HDEVINFO infoSet, SP_DEVINFO_DATA &deviceData)
{
	std::string name;
	WCHAR wideName[256] = {};
	DEVPROPTYPE propertyType = 0;

	if (SetupDiGetDevicePropertyW(infoSet, &deviceData, &DEVPKEY_Device_BusReportedDeviceDesc, &propertyType, reinterpret_cast<PBYTE>(wideName), sizeof(wideName) - sizeof(WCHAR), nullptr, 0) &&
	    (DEVPROP_TYPE_STRING == propertyType))
	{
		char utf8Name[512] = {};
		if (0 < WideCharToMultiByte(CP_UTF8, 0, wideName, -1, utf8Name, sizeof(utf8Name), nullptr, nullptr))
		{
			name = utf8Name;
		}
	}
	return name.empty() ? "candleLight adapter" : name;
}

/// @brief Returns the USB serial number of the adapter, taken from the device instance ID of the USB device
static std::string get_usb_serial(DEVINST deviceInstance)
{
	char instanceID[MAX_DEVICE_ID_LEN] = {};

	if (CR_SUCCESS != CM_Get_Device_IDA(deviceInstance, instanceID, MAX_DEVICE_ID_LEN, 0))
	{
		return "";
	}

	// On a composite device (CAN and firmware upgrade interfaces) the serial is on the parent USB device
	if (nullptr != std::strstr(instanceID, "&MI_"))
	{
		DEVINST parent = 0;
		if ((CR_SUCCESS != CM_Get_Parent(&parent, deviceInstance, 0)) ||
		    (CR_SUCCESS != CM_Get_Device_IDA(parent, instanceID, MAX_DEVICE_ID_LEN, 0)))
		{
			return "";
		}
	}

	// The instance ID is "USB\VID_xxxx&PID_xxxx\<serial>"
	const char *lastSeparator = std::strrchr(instanceID, '\\');
	return (nullptr != lastSeparator) ? std::string(lastSeparator + 1) : std::string();
}
#endif

/// The bit rate of ISOBUS
static constexpr std::uint32_t ISOBUS_BITRATE = 250000;

std::vector<CANDeviceList::Device> CANDeviceDrivers::list_devices(std::size_t driverIndex)
{
	std::vector<CANDeviceList::Device> devices;

	if (CANDeviceList::PCAN_DRIVER_INDEX == driverIndex)
	{
		devices = list_pcan_devices();
	}
	else if (CANDeviceList::CANDLELIGHT_DRIVER_INDEX == driverIndex)
	{
		devices = list_candlelight_devices();
	}

	if (devices.empty())
	{
		devices.push_back(CANDeviceList::get_default_device(driverIndex));
	}
	return devices;
}

std::shared_ptr<isobus::CANHardwarePlugin> CANDeviceDrivers::create_driver(std::size_t driverIndex, const CANDeviceList::Device &device)
{
	std::shared_ptr<isobus::CANHardwarePlugin> driver;

#ifdef ISOBUS_WINDOWSPCANBASIC_AVAILABLE
	if (CANDeviceList::PCAN_DRIVER_INDEX == driverIndex)
	{
		driver = std::make_shared<isobus::PCANBasicWindowsPlugin>(static_cast<WORD>(device.id));
	}
#endif
#ifdef ISOBUS_WINDOWSCANDLELIGHT_AVAILABLE
	if (CANDeviceList::CANDLELIGHT_DRIVER_INDEX == driverIndex)
	{
		driver = std::make_shared<isobus::CandleLightWindowsPlugin>(static_cast<std::uint8_t>(device.id), ISOBUS_BITRATE, 0);
	}
#endif
	(void)driverIndex;
	(void)device;
	return driver;
}

std::vector<CANDeviceList::Device> CANDeviceDrivers::list_pcan_devices()
{
	std::vector<CANDeviceList::Device> devices;

#ifdef ISOBUS_WINDOWSPCANBASIC_AVAILABLE
	DWORD count = 0;

	if ((PCAN_ERROR_OK == CAN_GetValue(PCAN_NONEBUS, PCAN_ATTACHED_CHANNELS_COUNT, &count, sizeof(count))) && (0 != count))
	{
		std::vector<TPCANChannelInformation> channels(count);

		if (PCAN_ERROR_OK == CAN_GetValue(PCAN_NONEBUS, PCAN_ATTACHED_CHANNELS, channels.data(), static_cast<DWORD>(channels.size() * sizeof(TPCANChannelInformation))))
		{
			for (const auto &channel : channels)
			{
				CANDeviceList::Device device;
				device.id = channel.channel_handle;
				device.serial = std::to_string(channel.device_id);
				device.description = std::string(channel.device_name, strnlen(channel.device_name, MAX_LENGTH_HARDWARE_NAME)) +
				  " channel " + std::to_string(channel.controller_number + 1) + ", device ID " + device.serial;
				if (0 != (channel.channel_condition & PCAN_CHANNEL_OCCUPIED))
				{
					device.description += " (in use)";
				}
				devices.push_back(device);
			}
		}
	}
#endif
	return devices;
}

std::vector<CANDeviceList::Device> CANDeviceDrivers::list_candlelight_devices()
{
	std::vector<CANDeviceList::Device> devices;

#ifdef ISOBUS_WINDOWSCANDLELIGHT_AVAILABLE
	// The same enumeration as the driver, so the index here is the device index the driver opens
	HDEVINFO infoSet = SetupDiGetClassDevsA(&CANDLELIGHT_DEVICE_GUID, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (INVALID_HANDLE_VALUE != infoSet)
	{
		SP_DEVICE_INTERFACE_DATA interfaceData = {};
		interfaceData.cbSize = sizeof(interfaceData);

		for (DWORD index = 0; SetupDiEnumDeviceInterfaces(infoSet, nullptr, &CANDLELIGHT_DEVICE_GUID, index, &interfaceData); index++)
		{
			SP_DEVINFO_DATA deviceData = {};
			deviceData.cbSize = sizeof(deviceData);
			DWORD requiredSize = 0;
			SetupDiGetDeviceInterfaceDetailA(infoSet, &interfaceData, nullptr, 0, &requiredSize, nullptr);
			std::vector<std::uint8_t> detailBuffer(std::max<DWORD>(requiredSize, sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A)));
			auto detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_A>(detailBuffer.data());
			detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

			CANDeviceList::Device device;
			device.id = index;
			if (SetupDiGetDeviceInterfaceDetailA(infoSet, &interfaceData, detail, static_cast<DWORD>(detailBuffer.size()), nullptr, &deviceData))
			{
				device.serial = get_usb_serial(deviceData.DevInst);
				device.description = get_candlelight_name(infoSet, deviceData);
			}
			else
			{
				device.description = "candleLight adapter";
			}
			device.description += device.serial.empty() ? (" " + std::to_string(index + 1)) : (", serial " + device.serial);
			devices.push_back(device);
		}
		SetupDiDestroyDeviceInfoList(infoSet);
	}
#endif
	return devices;
}
