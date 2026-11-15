//================================================================================================
/// @file VersionStorage.hpp
///
/// @brief Declares the store for the object pool versions of one client.
/// @author Gunics Balázs
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#ifndef VERSION_STORAGE_HPP
#define VERSION_STORAGE_HPP

#include <cstdint>
#include <filesystem>
#include <vector>

/// @brief Keeps the object pool versions of one client in a folder of its own.
/// @details Each component is stored as <label in hex>_<index>.iopx, which holds the label and then the component,
/// with a .iop copy of the component for the diagnostics package. A version only loads when its components are
/// numbered 0 to n - 1. Older releases stored object_pool_with_label_<N>.iopx files. A version in one such file still
/// loads. One in several files is retired, as it may have grown on every Store (#271), so its client uploads it again.
class VersionStorage
{
public:
	/// @param[in] clientDirectory The folder of one client, created on the first save
	explicit VersionStorage(std::filesystem::path clientDirectory);

	/// @brief Returns each label of one length, 7 or 32, with a version that can be loaded
	std::vector<std::vector<std::uint8_t>> get_labels(std::size_t labelLength) const;

	/// @brief Returns each label of one length, 7 or 32, whose version is retired
	std::vector<std::vector<std::uint8_t>> get_retired_labels(std::size_t labelLength) const;

	/// @brief Returns the components of a version joined in index order, or nothing if it is incomplete or cannot be read
	std::vector<std::uint8_t> load(const std::vector<std::uint8_t> &label) const;

	/// @brief Stores one component after the ones already stored under its label
	/// @returns True if it was written. If not, the whole label is removed, as the stack ends the Store there.
	bool save(const std::vector<std::uint8_t> &objectPool, const std::vector<std::uint8_t> &label);

	/// @brief Removes everything stored under a label
	/// @returns True if something was stored and all of it was removed
	bool remove(const std::vector<std::uint8_t> &label);

	/// @brief Removes every version with a label of one length, 7 or 32
	/// @returns True unless a file could not be removed or the folder could not be read
	bool remove_all(std::size_t labelLength);

private:
	struct StoredFile
	{
		std::filesystem::path path; ///< The .iopx file
		std::filesystem::path copyPath; ///< The .iop copy
		std::vector<std::uint8_t> label; ///< Empty for an older file too short to hold one
		std::size_t labelLength = 0; ///< Known from the name, so a file without a label is still removed
		std::uint64_t index = 0;
		bool isLegacy = false; ///< Named object_pool_with_label_<N>.iopx or .iopx32
	};

	struct LabelContents
	{
		std::size_t legacyFiles = 0;
		std::uint64_t components = 0;
		std::uint64_t nextIndex = 0; ///< One past the highest component index
	};

	/// @returns False if the folder could not be read. A folder that is not there yet holds nothing.
	bool find_stored_files(std::vector<StoredFile> &files) const;

	/// @brief Leaves out the retired and incomplete versions
	static std::vector<StoredFile> find_loadable_files(const std::vector<StoredFile> &files);

	static LabelContents get_label_contents(const std::vector<StoredFile> &files, const std::vector<std::uint8_t> &label);

	/// @returns True if every .iopx file was removed
	bool remove_files(const std::vector<StoredFile> &files) const;

	/// @brief Removes the .iop copies whose .iopx file is gone, which older releases left behind on every delete
	void remove_orphaned_copies() const;

	std::filesystem::path directory;
};

#endif // VERSION_STORAGE_HPP
