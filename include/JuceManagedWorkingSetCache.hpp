//================================================================================================
/// @file JuceManagedWorkingSetCache.hpp
///
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef JUCE_MANAGED_WORKING_SET_HPP
#define JUCE_MANAGED_WORKING_SET_HPP

#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"

class JuceManagedWorkingSetCache
{
public:
	static std::shared_ptr<Component> create_component(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, std::shared_ptr<isobus::VTObject> sourceObject);

private:
	class ComponentCacheClass
	{
	public:
		ComponentCacheClass(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> associatedWorkingSet) :
		  workingSet(associatedWorkingSet){};

		std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet;
		//std::map<std::uint16_t, std::shared_ptr<Component>> componentLookup;
	};
	static std::vector<ComponentCacheClass> workingSetComponentCache;
};

#endif // JUCE_MANAGED_WORKING_SET_HPP
