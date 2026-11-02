/*******************************************************************************
** @file       VersionStorage.cpp
** @author     Gunics Balázs
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "VersionStorage.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <system_error>

static const std::string LEGACY_FILE_PREFIX = "object_pool_with_label_";
static const std::string LEGACY_COPY_PREFIX = "object_pool_";
static constexpr std::size_t LEGACY_LABEL_LENGTH = 7;
static constexpr std::size_t LEGACY_EXTENDED_LABEL_LENGTH = 32;

static bool parse_index(const std::string &text, std::uint64_t &index)
{
	// Short enough that it cannot overflow
	if (text.empty() ||
	    (text.size() > 18) ||
	    (!std::all_of(text.begin(), text.end(), [](char character) { return 0 != std::isdigit(static_cast<unsigned char>(character)); })))
	{
		return false;
	}
	index = std::stoull(text);
	return true;
}

static std::string to_hex(const std::vector<std::uint8_t> &bytes)
{
	static const char digits[] = "0123456789abcdef";
	std::string retVal;

	for (const auto byte : bytes)
	{
		retVal.push_back(digits[byte >> 4]);
		retVal.push_back(digits[byte & 0x0F]);
	}
	return retVal;
}

static bool from_hex(const std::string &text, std::vector<std::uint8_t> &bytes)
{
	if (text.empty() ||
	    (0 != (text.size() % 2)) ||
	    (!std::all_of(text.begin(), text.end(), [](char character) { return 0 != std::isxdigit(static_cast<unsigned char>(character)); })))
	{
		return false;
	}

	bytes.clear();
	for (std::size_t i = 0; i < text.size(); i += 2)
	{
		bytes.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(i, 2), nullptr, 16)));
	}
	return true;
}

static std::string get_stored_name(const std::vector<std::uint8_t> &label, std::uint64_t index)
{
	return to_hex(label) + "_" + std::to_string(index);
}

// A name such as 4d79506f6f6c3031_0, the label in hex and the index of the component.
// Only the spelling save() writes is read, so no two files hold the same component.
static bool parse_stored_name(const std::string &stem, std::vector<std::uint8_t> &label, std::uint64_t &index)
{
	const auto separator = stem.rfind('_');
	return (std::string::npos != separator) &&
	  from_hex(stem.substr(0, separator), label) &&
	  parse_index(stem.substr(separator + 1), index) &&
	  (get_stored_name(label, index) == stem);
}

// The whole file or nothing, as a short read would load a component cut off
static bool read_file(const std::filesystem::path &path, std::vector<std::uint8_t> &contents)
{
	std::error_code error;
	const auto size = std::filesystem::file_size(path, error);
	std::ifstream file(path, std::ios::binary);

	if (error || (!file.is_open()))
	{
		return false;
	}
	contents.resize(static_cast<std::size_t>(size));
	return static_cast<bool>(file.read(reinterpret_cast<char *>(contents.data()), static_cast<std::streamsize>(contents.size())));
}

// A file that cannot be checked counts as there, so its copy is kept
static bool may_exist(const std::filesystem::path &path)
{
	std::error_code error;
	return std::filesystem::exists(path, error) || error;
}

static bool starts_with(const std::string &text, const std::string &prefix)
{
	return 0 == text.compare(0, prefix.size(), prefix);
}

VersionStorage::LabelContents VersionStorage::get_label_contents(const std::vector<StoredFile> &files, const std::vector<std::uint8_t> &label)
{
	LabelContents retVal;

	for (const auto &file : files)
	{
		if (file.label == label)
		{
			if (file.isLegacy)
			{
				retVal.legacyFiles++;
			}
			else
			{
				retVal.components++;
				retVal.nextIndex = std::max(retVal.nextIndex, file.index + 1);
			}
		}
	}
	return retVal;
}

VersionStorage::VersionStorage(std::filesystem::path clientDirectory) :
  directory(std::move(clientDirectory))
{
}

std::vector<std::vector<std::uint8_t>> VersionStorage::get_labels(std::size_t labelLength) const
{
	std::vector<StoredFile> storedFiles;
	std::vector<std::vector<std::uint8_t>> retVal;

	if (!find_stored_files(storedFiles))
	{
		return {};
	}

	for (const auto &file : find_loadable_files(storedFiles))
	{
		if ((labelLength == file.label.size()) &&
		    (retVal.end() == std::find(retVal.begin(), retVal.end(), file.label)))
		{
			retVal.push_back(file.label);
		}
	}
	return retVal;
}

std::vector<std::vector<std::uint8_t>> VersionStorage::get_retired_labels(std::size_t labelLength) const
{
	std::vector<StoredFile> storedFiles;
	std::vector<std::vector<std::uint8_t>> retVal;

	if (!find_stored_files(storedFiles))
	{
		return {};
	}

	for (const auto &file : storedFiles)
	{
		const auto contents = get_label_contents(storedFiles, file.label);

		if ((labelLength == file.label.size()) &&
		    (0 == contents.components) &&
		    (contents.legacyFiles > 1) &&
		    (retVal.end() == std::find(retVal.begin(), retVal.end(), file.label)))
		{
			retVal.push_back(file.label);
		}
	}
	return retVal;
}

std::vector<std::uint8_t> VersionStorage::load(const std::vector<std::uint8_t> &label) const
{
	std::vector<StoredFile> storedFiles;
	std::vector<std::uint8_t> retVal;

	if (!find_stored_files(storedFiles))
	{
		return {};
	}

	for (const auto &file : find_loadable_files(storedFiles))
	{
		if (file.label == label)
		{
			std::vector<std::uint8_t> contents;

			// Part of a version is no version, as the client would get a pool with objects missing
			if ((!read_file(file.path, contents)) || (contents.size() < label.size()))
			{
				return {};
			}
			retVal.insert(retVal.end(), contents.begin() + static_cast<std::ptrdiff_t>(label.size()), contents.end());
		}
	}
	return retVal;
}

bool VersionStorage::save(const std::vector<std::uint8_t> &objectPool, const std::vector<std::uint8_t> &label)
{
	bool retVal = false;
	std::vector<StoredFile> storedFiles;
	std::error_code error;

	if (label.empty())
	{
		return false;
	}
	std::filesystem::create_directories(directory, error);

	// The index from a folder that cannot be read could be one already saved
	if (find_stored_files(storedFiles))
	{
		const auto name = get_stored_name(label, get_label_contents(storedFiles, label).nextIndex);
		const auto iopxPath = directory / (name + ".iopx");
		std::ofstream iopxFile(iopxPath, std::ios::trunc | std::ios::binary);

		if (iopxFile.is_open())
		{
			iopxFile.write(reinterpret_cast<const char *>(label.data()), static_cast<std::streamsize>(label.size()));
			iopxFile.write(reinterpret_cast<const char *>(objectPool.data()), static_cast<std::streamsize>(objectPool.size()));
			iopxFile.close();
			retVal = !iopxFile.fail();
		}

		if (retVal)
		{
			std::ofstream iopFile(directory / (name + ".iop"), std::ios::trunc | std::ios::binary);

			if (iopFile.is_open())
			{
				iopFile.write(reinterpret_cast<const char *>(objectPool.data()), static_cast<std::streamsize>(objectPool.size()));
			}
		}
		else
		{
			// A cut off component would be loaded as part of the version
			std::filesystem::remove(iopxPath, error);
		}
	}

	if (!retVal)
	{
		// The stack ends the Store here, so the components saved before this one would be a version with objects missing
		remove(label);
	}
	return retVal;
}

bool VersionStorage::remove(const std::vector<std::uint8_t> &label)
{
	std::vector<StoredFile> storedFiles;
	std::vector<StoredFile> filesToRemove;

	if (!find_stored_files(storedFiles))
	{
		return false;
	}

	for (const auto &file : storedFiles)
	{
		if (file.label == label)
		{
			filesToRemove.push_back(file);
		}
	}

	const bool retVal = (!filesToRemove.empty()) && remove_files(filesToRemove);
	remove_orphaned_copies();
	return retVal;
}

bool VersionStorage::remove_all(std::size_t labelLength)
{
	std::vector<StoredFile> storedFiles;
	std::vector<StoredFile> filesToRemove;

	if (!find_stored_files(storedFiles))
	{
		return false;
	}

	for (const auto &file : storedFiles)
	{
		if (labelLength == file.labelLength)
		{
			filesToRemove.push_back(file);
		}
	}

	const bool retVal = remove_files(filesToRemove);
	remove_orphaned_copies();
	return retVal;
}

bool VersionStorage::find_stored_files(std::vector<StoredFile> &files) const
{
	std::error_code error;
	std::error_code entryError;
	files.clear();

	if (!std::filesystem::exists(directory, error))
	{
		return !error;
	}

	for (auto entry = std::filesystem::directory_iterator(directory, error); (!error) && (!entryError) && (std::filesystem::directory_iterator() != entry); entry.increment(error))
	{
		const auto extension = entry->path().extension();
		const bool isExtendedLegacyFile = (".iopx32" == extension);

		if (((".iopx" == extension) || isExtendedLegacyFile) && entry->is_regular_file(entryError))
		{
			const auto stem = entry->path().stem().u8string();
			StoredFile file;
			file.path = entry->path();

			if (starts_with(stem, LEGACY_FILE_PREFIX) &&
			    parse_index(stem.substr(LEGACY_FILE_PREFIX.size()), file.index))
			{
				std::ifstream iopxFile(file.path, std::ios::binary);
				file.labelLength = isExtendedLegacyFile ? LEGACY_EXTENDED_LABEL_LENGTH : LEGACY_LABEL_LENGTH;
				file.label.resize(file.labelLength);
				file.copyPath = directory / (LEGACY_COPY_PREFIX + stem.substr(LEGACY_FILE_PREFIX.size()) + ".iop");
				file.isLegacy = true;

				// A file too short to hold a label is in no version, but its label length still removes it
				if (!iopxFile.read(reinterpret_cast<char *>(file.label.data()), static_cast<std::streamsize>(file.label.size())))
				{
					file.label.clear();
				}
				files.push_back(file);
			}
			else if ((!isExtendedLegacyFile) && parse_stored_name(stem, file.label, file.index))
			{
				file.labelLength = file.label.size();
				file.copyPath = file.path;
				file.copyPath.replace_extension(".iop");
				files.push_back(file);
			}
		}
	}

	// A folder that was only partly read could leave out a component of a version
	if (error || entryError)
	{
		files.clear();
		return false;
	}

	// Directory order is unspecified, and 10 sorts before 2 as text
	std::sort(files.begin(), files.end(), [](const StoredFile &first, const StoredFile &second) { return first.index < second.index; });
	return true;
}

std::vector<VersionStorage::StoredFile> VersionStorage::find_loadable_files(const std::vector<StoredFile> &files)
{
	std::vector<StoredFile> retVal;

	for (const auto &file : files)
	{
		const auto contents = get_label_contents(files, file.label);
		// No two components have the same index, so n of them that end at n - 1 are all there
		const bool isComplete = (contents.components == contents.nextIndex);
		const bool isSingleLegacyFile = (0 == contents.components) && (1 == contents.legacyFiles);

		if ((!file.label.empty()) && (file.isLegacy ? isSingleLegacyFile : isComplete))
		{
			retVal.push_back(file);
		}
	}
	return retVal;
}

bool VersionStorage::remove_files(const std::vector<StoredFile> &files) const
{
	bool retVal = true;

	for (const auto &file : files)
	{
		std::error_code error;
		std::filesystem::remove(file.path, error);

		if (error)
		{
			retVal = false;
		}

		// The copy is only for the diagnostics package, so a copy that cannot be removed fails nothing
		std::filesystem::remove(file.copyPath, error);
	}
	return retVal;
}

void VersionStorage::remove_orphaned_copies() const
{
	std::vector<std::filesystem::path> copiesToRemove;
	std::error_code error;

	for (auto entry = std::filesystem::directory_iterator(directory, error); (!error) && (std::filesystem::directory_iterator() != entry); entry.increment(error))
	{
		std::error_code entryError;

		if (entry->is_regular_file(entryError) && (".iop" == entry->path().extension()))
		{
			const auto stem = entry->path().stem().u8string();
			std::vector<std::uint8_t> label;
			std::uint64_t index = 0;
			std::vector<std::filesystem::path> iopxPaths;

			if (starts_with(stem, LEGACY_COPY_PREFIX) &&
			    parse_index(stem.substr(LEGACY_COPY_PREFIX.size()), index))
			{
				const auto legacyName = LEGACY_FILE_PREFIX + stem.substr(LEGACY_COPY_PREFIX.size());
				iopxPaths = { directory / (legacyName + ".iopx"), directory / (legacyName + ".iopx32") };
			}
			else if (parse_stored_name(stem, label, index))
			{
				iopxPaths = { entry->path() };
				iopxPaths.back().replace_extension(".iopx");
			}

			if ((!iopxPaths.empty()) && (!std::any_of(iopxPaths.begin(), iopxPaths.end(), may_exist)))
			{
				copiesToRemove.push_back(entry->path());
			}
		}
	}

	for (const auto &copy : copiesToRemove)
	{
		std::filesystem::remove(copy, error);
	}
}
