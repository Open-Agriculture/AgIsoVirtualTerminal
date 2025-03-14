/*******************************************************************************
** @file       Settings.cpp
** @author     Miklos Marton
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "Settings.hpp"

Settings::Settings()
{
	m_settings = std::make_shared<ValueTree>("Settings");
}

Settings::~Settings()
{
}

bool Settings::load_settings()
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

			if (nullptr != xml)
			{
				m_settings->copyPropertiesAndChildrenFrom(ValueTree::fromXml(*xml), nullptr);

				int index = 0;
				auto child = m_settings->getChild(index);

				while (child.isValid())
				{
					if (Identifier("Hardware") == child.getType() && !child.getProperty("VT_Number").isVoid())
					{
						m_vtNumber = static_cast<std::uint8_t>(static_cast<int>(child.getProperty("VT_Number")));
						if (m_vtNumber == 0 || m_vtNumber > 32)
						{
							m_vtNumber = 1;
						}
						break;
					}
					index++;
					child = m_settings->getChild(index);
				}

				return true;
			}
		}
	}
	return false;
}

std::shared_ptr<ValueTree> Settings::settingsValueTree()
{
	return m_settings;
}

int Settings::vt_number() const
{
	return m_vtNumber;
}
