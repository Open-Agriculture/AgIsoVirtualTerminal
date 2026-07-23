//================================================================================================
/// @file       UpdateChecker.hpp
///
/// @brief 		Asks GitHub if a newer release of this application has been published
/// @author     Sujan Dumaru
///
/// @copyright  The Open-Agriculture Developers
//================================================================================================

#pragma once

#include "JuceHeader.h"

#include <functional>

class UpdateChecker : private Thread
{
public:
	/// @brief The outcome of a check against the GitHub releases API
	struct Result
	{
		bool checkSucceeded = false; ///< False if the latest release could not be fetched or understood
		bool updateAvailable = false; ///< True if the latest release is newer than this build
		int statusCode = 0; ///< The HTTP response status, or 0 if no response was received
		String latestVersion; ///< The tag of the latest release, such as "1.4.0". Empty if the check failed.
		String releaseUrl; ///< A link to the latest release on GitHub. Empty if the check failed.
	};

	UpdateChecker();
	~UpdateChecker() override;

	/// @brief Fetches the latest release from GitHub on a background thread
	/// @param[in] callback Called on the message thread once the check has completed
	/// @returns True if the check was started, or false if one is already running or the worker could not start
	bool start(std::function<void(const Result &)> callback);

	/// @returns True if an update check is currently running
	bool is_check_in_progress() const;

	/// @brief Converts a release tag into the same packed form as ProjectInfo::versionNumber
	/// @param[in] versionTag The tag to convert, such as "1.4.0". A leading "v" is ignored.
	/// @returns The packed version, such as 0x010400, or 0 if the tag could not be understood
	static int parse_version(const String &versionTag);

private:
	void run() override;

	std::function<void(const Result &)> completionCallback;
	CriticalSection activeStreamLock;
	WebInputStream *activeStream = nullptr;
};
