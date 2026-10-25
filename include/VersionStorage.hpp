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
/// @details Each saved component is a file named after its label in hex and its index, such as
/// 4d79506f6f6c3031_0.iopx. The file holds the label and then the component. A copy of the component
/// alone is kept as a .iop file of the same name, for the diagnostics package.
/// The label length in the name keeps 7 and 32 byte labels apart, even if the padded text is the same.
/// Files from older versions are named object_pool_with_label_<N>.iopx (.iopx32 for a 32 byte label), with the copy
/// in object_pool_<N>.iop. Such a version in a single file is still listed and loaded. One in several files is retired:
/// those versions added every Store after a Load as one more file, so they may hold the pool many times over (#271),
/// and their order on disk is unknown. A retired version is not listed or loaded, so the client uploads its pool again
/// and the Store that follows replaces the files. Both kinds are removed like any other version.
class VersionStorage
{
public:
	/// @brief Constructor for a version store
	/// @param[in] clientDirectory The folder that holds the versions of one client. It is created on the first save.
	explicit VersionStorage(std::filesystem::path clientDirectory);

	/// @brief Returns the labels of one length that have something stored
	/// @param[in] labelLength 7 for versions, 32 for extended versions
	/// @returns Each stored label once
	std::vector<std::vector<std::uint8_t>> get_labels(std::size_t labelLength) const;

	/// @brief Returns the labels of one length whose version is retired, as it was saved by an older version in several files
	/// @param[in] labelLength 7 for versions, 32 for extended versions
	/// @returns Each retired label once
	std::vector<std::vector<std::uint8_t>> get_retired_labels(std::size_t labelLength) const;

	/// @brief Loads a stored version
	/// @param[in] label The version label
	/// @returns The components stored under the label, joined in the order they were saved, or nothing if none
	/// are stored or one cannot be read
	std::vector<std::uint8_t> load(const std::vector<std::uint8_t> &label) const;

	/// @brief Stores one component after the ones already stored under its label
	/// @param[in] objectPool The component
	/// @param[in] label The version label
	/// @returns True if the component was written
	bool save(const std::vector<std::uint8_t> &objectPool, const std::vector<std::uint8_t> &label);

	/// @brief Removes everything stored under a label
	/// @param[in] label The version label
	/// @returns True if something was stored under the label and all of it was removed
	bool remove(const std::vector<std::uint8_t> &label);

	/// @brief Removes every version with a label of one length
	/// @param[in] labelLength 7 for versions, 32 for extended versions
	/// @returns True unless a file could not be removed
	bool remove_all(std::size_t labelLength);

private:
	/// @brief A stored component
	struct StoredFile
	{
		std::filesystem::path path; ///< The .iopx file
		std::filesystem::path copyPath; ///< The .iop copy of the component
		std::vector<std::uint8_t> label; ///< The version label
		std::uint64_t index = 0; ///< Where the component goes in its version
		bool isLegacy = false; ///< True for a file named object_pool_with_label_<N>.iopx or .iopx32
	};

	/// @brief Finds every stored component, in the order they are loaded
	std::vector<StoredFile> find_stored_files() const;

	/// @brief Finds the stored components that can be loaded, which leaves out the retired versions
	std::vector<StoredFile> find_loadable_files() const;

	/// @brief Counts the files from older versions stored under a label
	static std::size_t count_legacy_files(const std::vector<StoredFile> &files, const std::vector<std::uint8_t> &label);

	/// @brief Removes stored components and their copies
	/// @returns True if every .iopx file was removed
	bool remove_files(const std::vector<StoredFile> &files) const;

	/// @brief Removes the .iop copies whose .iopx file is gone. Older versions left them behind on every delete.
	void remove_orphaned_copies() const;

	std::filesystem::path directory; ///< The folder that holds the versions of the client
};

#endif // VERSION_STORAGE_HPP
