#pragma once

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
