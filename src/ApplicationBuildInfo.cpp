/*******************************************************************************
** @file       ApplicationBuildInfo.cpp
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "Main.hpp"
#include "git.h"

std::string AgISOVirtualTerminalApplication::getApplicationBuildInfo()
{
	std::string gitDescribe = std::string(git::Describe());
	if (gitDescribe.length() > 0)
	{
		return gitDescribe + (git::AnyUncommittedChanges() ? "-dirty" : "");
	}
	return ProjectInfo::versionString;
}

std::string AgISOVirtualTerminalApplication::getApplicationNameWithBuildInfo()
{
	std::string name = ProjectInfo::projectName;
	auto buildInfo = getApplicationBuildInfo();
	if (buildInfo.length() > 0)
	{
		name.append(" - " + buildInfo);
	}
	return name;
}
