/*******************************************************************************
** @file       UpdateChecker.cpp
** @author     Sujan Dumaru
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "UpdateChecker.hpp"

namespace
{
	constexpr char LATEST_RELEASE_URL[] = "https://api.github.com/repos/Open-Agriculture/AgIsoVirtualTerminal/releases/latest";
	constexpr int REQUEST_TIMEOUT_MS = 5000;
	constexpr int NUMBER_OF_VERSION_FIELDS = 3; // Major, minor and patch, the same fields ProjectInfo::versionNumber packs
}

UpdateChecker::UpdateChecker() :
  Thread("Update Checker")
{
}

UpdateChecker::~UpdateChecker()
{
	signalThreadShouldExit();

	{
		const ScopedLock lock(activeStreamLock);

		if (nullptr != activeStream)
		{
			activeStream->cancel();
		}
	}

	stopThread(2000);
}

int UpdateChecker::parse_version(const String &versionTag)
{
	auto trimmedTag = versionTag.trim();

	if (trimmedTag.startsWithIgnoreCase("v"))
	{
		trimmedTag = trimmedTag.substring(1);
	}

	auto fields = StringArray::fromTokens(trimmedTag, ".", "");

	if (fields.isEmpty() || (fields.size() > NUMBER_OF_VERSION_FIELDS))
	{
		return 0;
	}

	int packedVersion = 0;

	for (int i = 0; i < NUMBER_OF_VERSION_FIELDS; i++)
	{
		// A tag may omit trailing fields, so "1.4" is treated as "1.4.0"
		auto field = (i < fields.size()) ? fields[i].trim() : String("0");

		if (field.isEmpty() || (field.length() > 3) || !field.containsOnly("0123456789"))
		{
			return 0; // Not a plain version tag, so we have nothing to compare against
		}

		auto value = field.getIntValue();

		if (value > 0xFF)
		{
			return 0; // Too large to pack the same way ProjectInfo::versionNumber does
		}
		packedVersion = (packedVersion << 8) | value;
	}
	return packedVersion;
}

bool UpdateChecker::start(std::function<void(const Result &)> callback)
{
	if (isThreadRunning())
	{
		return false;
	}

	completionCallback = std::move(callback);

	if (!startThread())
	{
		completionCallback = nullptr;
		return false;
	}

	return true;
}

bool UpdateChecker::is_check_in_progress() const
{
	return isThreadRunning();
}

void UpdateChecker::run()
{
	Result result;
	WebInputStream stream(URL(LATEST_RELEASE_URL), false);
	stream.withConnectionTimeout(REQUEST_TIMEOUT_MS)
	  .withExtraHeaders("Accept: application/vnd.github+json\r\n"
	                    "X-GitHub-Api-Version: 2022-11-28\r\n"
	                    "User-Agent: AgISOVirtualTerminal"); // GitHub rejects requests that don't identify themselves

	{
		const ScopedLock lock(activeStreamLock);
		activeStream = &stream;
	}

	if (!threadShouldExit())
	{
		const auto connected = stream.connect(nullptr);
		result.statusCode = stream.getStatusCode();

		if (connected && (200 == result.statusCode))
		{
			auto latestRelease = JSON::parse(stream.readEntireStreamAsString());
			auto latestTag = latestRelease.getProperty("tag_name", var()).toString();
			auto latestVersion = parse_version(latestTag);
			auto releaseUrl = latestRelease.getProperty("html_url", var()).toString();

			if ((0 != latestVersion) && releaseUrl.isNotEmpty())
			{
				result.checkSucceeded = true;
				result.updateAvailable = (latestVersion > ProjectInfo::versionNumber);
				result.latestVersion = latestTag;
				result.releaseUrl = releaseUrl;
			}
		}
	}

	{
		const ScopedLock lock(activeStreamLock);
		activeStream = nullptr;
	}

	if (threadShouldExit())
	{
		return;
	}

	auto callback = completionCallback;
	MessageManager::callAsync([callback, result]() {
		callback(result);
	});
}
