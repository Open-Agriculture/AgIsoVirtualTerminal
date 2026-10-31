/*******************************************************************************
** @file       VersionStorage.cpp
** @author     Gunics Balázs
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "VersionStorage.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
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

// A name such as 4d79506f6f6c3031_0, the label in hex and the index of the component
static bool parse_stored_name(const std::string &stem, std::vector<std::uint8_t> &label, std::uint64_t &index)
{
	const auto separator = stem.rfind('_');
	return (std::string::npos != separator) &&
	  from_hex(stem.substr(0, separator), label) &&
	  parse_index(stem.substr(separator + 1), index);
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

std::size_t VersionStorage::count_legacy_files(const std::vector<StoredFile> &files, const std::vector<std::uint8_t> &label)
{
	std::size_t retVal = 0;

	for (const auto &file : files)
	{
		if (file.isLegacy && (file.label == label))
		{
			retVal++;
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
	std::vector<std::vector<std::uint8_t>> retVal;

	for (const auto &file : find_loadable_files())
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
	const auto storedFiles = find_stored_files();
	std::vector<std::vector<std::uint8_t>> retVal;

	for (const auto &file : storedFiles)
	{
		if ((labelLength == file.label.size()) &&
		    (count_legacy_files(storedFiles, file.label) > 1) &&
		    (retVal.end() == std::find(retVal.begin(), retVal.end(), file.label)))
		{
			retVal.push_back(file.label);
		}
	}
	return retVal;
}

std::vector<std::uint8_t> VersionStorage::load(const std::vector<std::uint8_t> &label) const
{
	std::vector<std::uint8_t> retVal;

	for (const auto &file : find_loadable_files())
	{
		if (file.label == label)
		{
			std::ifstream iopxFile(file.path, std::ios::binary);
			std::vector<std::uint8_t> contents;

			if (iopxFile.is_open())
			{
				contents.assign(std::istreambuf_iterator<char>(iopxFile), std::istreambuf_iterator<char>());
			}

			// Part of a version is no version, as the client would get a pool with objects missing
			if ((!iopxFile.is_open()) || iopxFile.bad() || (contents.size() < label.size()))
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
	std::uint64_t index = 0;
	std::error_code error;

	if (label.empty())
	{
		return false;
	}
	std::filesystem::create_directories(directory, error);

	for (const auto &file : find_stored_files())
	{
		if ((!file.isLegacy) && (file.label == label))
		{
			index = std::max(index, file.index + 1);
		}
	}

	const auto name = to_hex(label) + "_" + std::to_string(index);
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
	return retVal;
}

bool VersionStorage::remove(const std::vector<std::uint8_t> &label)
{
	std::vector<StoredFile> filesToRemove;

	for (const auto &file : find_stored_files())
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
	std::vector<StoredFile> filesToRemove;

	for (const auto &file : find_stored_files())
	{
		if (labelLength == file.label.size())
		{
			filesToRemove.push_back(file);
		}
	}

	const bool retVal = remove_files(filesToRemove);
	remove_orphaned_copies();
	return retVal;
}

std::vector<VersionStorage::StoredFile> VersionStorage::find_stored_files() const
{
	std::vector<StoredFile> retVal;
	std::error_code error;

	for (auto entry = std::filesystem::directory_iterator(directory, error); (!error) && (std::filesystem::directory_iterator() != entry); entry.increment(error))
	{
		std::error_code entryError;
		const auto extension = entry->path().extension();
		const bool isExtendedLegacyFile = (".iopx32" == extension);

		if (entry->is_regular_file(entryError) && ((".iopx" == extension) || isExtendedLegacyFile))
		{
			const auto stem = entry->path().stem().u8string();
			StoredFile file;
			file.path = entry->path();

			if (starts_with(stem, LEGACY_FILE_PREFIX) &&
			    parse_index(stem.substr(LEGACY_FILE_PREFIX.size()), file.index))
			{
				std::ifstream iopxFile(file.path, std::ios::binary);
				file.label.resize(isExtendedLegacyFile ? LEGACY_EXTENDED_LABEL_LENGTH : LEGACY_LABEL_LENGTH);
				file.copyPath = directory / (LEGACY_COPY_PREFIX + stem.substr(LEGACY_FILE_PREFIX.size()) + ".iop");
				file.isLegacy = true;

				if (iopxFile.read(reinterpret_cast<char *>(file.label.data()), static_cast<std::streamsize>(file.label.size())))
				{
					retVal.push_back(file);
				}
			}
			else if ((!isExtendedLegacyFile) && parse_stored_name(stem, file.label, file.index))
			{
				file.copyPath = file.path;
				file.copyPath.replace_extension(".iop");
				retVal.push_back(file);
			}
		}
	}

	// Older versions saved first, so their files come first
	std::sort(retVal.begin(), retVal.end(), [](const StoredFile &first, const StoredFile &second) {
		return (first.isLegacy != second.isLegacy) ? first.isLegacy : (first.index < second.index);
	});
	return retVal;
}

std::vector<VersionStorage::StoredFile> VersionStorage::find_loadable_files() const
{
	const auto storedFiles = find_stored_files();
	std::vector<StoredFile> retVal;

	for (const auto &file : storedFiles)
	{
		if ((!file.isLegacy) || (1 == count_legacy_files(storedFiles, file.label)))
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
