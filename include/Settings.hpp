//================================================================================================
/// @file Settings.hpp
///
/// @brief Loads and holds the saved VT settings
/// @author Miklos Marton
///
/// @copyright 2025 The Open-Agriculture Developers
//================================================================================================

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "JuceHeader.h"

class Settings
{
public:
	Settings();
	~Settings();

	bool load_settings();
	std::shared_ptr<ValueTree> settingsValueTree();
	int vt_number() const;

private:
	std::shared_ptr<ValueTree> m_settings;
	int m_vtNumber = 1;
};

#endif // SETTINGS_HPP
