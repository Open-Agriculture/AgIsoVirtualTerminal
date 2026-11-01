/*******************************************************************************
** @file       VersionStorageTests.cpp
** @author     Gunics Balázs
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "VersionStorage.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;

static int failures = 0;

#define CHECK(condition)                                                              \
	do                                                                                  \
	{                                                                                   \
		if (!(condition))                                                                 \
		{                                                                                 \
			std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK(" #condition ") failed\n"; \
			failures++;                                                                     \
		}                                                                                 \
	} while (false)

static Bytes label_of(const std::string &text, std::size_t length)
{
	Bytes retVal(text.begin(), text.end());
	retVal.resize(length, ' ');
	return retVal;
}

static Bytes join(const Bytes &first, const Bytes &second)
{
	Bytes retVal = first;
	retVal.insert(retVal.end(), second.begin(), second.end());
	return retVal;
}

static void write_file(const fs::path &path, const Bytes &contents)
{
	std::ofstream file(path, std::ios::trunc | std::ios::binary);
	file.write(reinterpret_cast<const char *>(contents.data()), static_cast<std::streamsize>(contents.size()));
}

static std::size_t count_files(const fs::path &directory, const std::string &extension)
{
	std::size_t retVal = 0;

	for (const auto &entry : fs::directory_iterator(directory))
	{
		if (extension == entry.path().extension())
		{
			retVal++;
		}
	}
	return retVal;
}

// What the stack does for a Store Version: empty the label, then save each component in transfer order
static void store(VersionStorage &storage, const Bytes &label, const std::vector<Bytes> &components)
{
	storage.remove(label);

	for (const auto &component : components)
	{
		CHECK(storage.save(component, label));
	}
}

// The #271 sequence: the stored version must not grow when a client stores what it just loaded
static void store_after_load_keeps_the_version(const fs::path &directory)
{
	VersionStorage storage(directory);
	const auto label = label_of("POOL01", 7);
	const Bytes first = { 1, 2, 3, 4 };
	const Bytes second = { 5, 6 };

	store(storage, label, { first, second });
	const auto loaded = storage.load(label);
	CHECK(join(first, second) == loaded);

	store(storage, label, { loaded });
	CHECK(loaded == storage.load(label));

	store(storage, label, { storage.load(label) });
	CHECK(loaded == storage.load(label));
	CHECK(1 == count_files(directory, ".iopx"));
	CHECK(1 == count_files(directory, ".iop"));
}

// Directory order is unspecified, and 10 sorts before 2 as text
static void components_load_in_save_order(const fs::path &directory)
{
	VersionStorage storage(directory);
	const auto label = label_of("MANY", 7);
	std::vector<Bytes> components;
	Bytes expected;

	for (std::uint8_t i = 0; i < 12; i++)
	{
		components.push_back({ i, static_cast<std::uint8_t>(0xF0 | i) });
		expected = join(expected, components.back());
	}
	store(storage, label, components);
	CHECK(expected == storage.load(label));
}

static void label_lengths_are_separate_versions(const fs::path &directory)
{
	VersionStorage storage(directory);
	const auto shortLabel = label_of("SAME", 7);
	const auto longLabel = label_of("SAME", 32);
	const auto otherLabel = label_of("OTHER", 7);

	store(storage, shortLabel, { { 7 } });
	store(storage, longLabel, { { 32 } });
	store(storage, otherLabel, { { 8 } });

	CHECK(Bytes({ 7 }) == storage.load(shortLabel));
	CHECK(Bytes({ 32 }) == storage.load(longLabel));
	CHECK(2 == storage.get_labels(7).size());
	CHECK(1 == storage.get_labels(32).size());
	CHECK(longLabel == storage.get_labels(32).front());

	CHECK(storage.remove_all(7));
	CHECK(storage.get_labels(7).empty());
	CHECK(Bytes({ 32 }) == storage.load(longLabel));
	CHECK(1 == count_files(directory, ".iopx"));
	CHECK(1 == count_files(directory, ".iop"));
}

static void removing_what_is_not_stored(const fs::path &directory)
{
	VersionStorage storage(directory / "never-created");

	CHECK(!storage.remove(label_of("NONE", 7)));
	CHECK(storage.remove_all(7));
	CHECK(storage.load(label_of("NONE", 7)).empty());
	CHECK(storage.get_labels(7).empty());
}

// Files saved by older versions: object_pool_with_label_<N>.iopx, with a copy in object_pool_<N>.iop that delete left behind.
// A version in one file still loads. One in several files may have grown on every Store after a Load (#271), so the
// client has to upload it again, and its Store replaces the old files.
static void legacy_files_retire_or_load(const fs::path &directory)
{
	const auto grownLabel = label_of("GROWN", 7);
	const auto singleLabel = label_of("SINGLE", 7);
	const auto extendedLabel = label_of("SINGLE", 32);
	const Bytes first = { 0xA0, 0xA1 };
	const Bytes second = { 0xB0 };
	const Bytes single = { 0xC0 };
	const Bytes extended = { 0xE0, 0xE1 };

	// The pattern from #264: the third file is the first two stored again after a Load
	fs::create_directories(directory);
	write_file(directory / "object_pool_with_label_0.iopx", join(grownLabel, first));
	write_file(directory / "object_pool_with_label_1.iopx", join(grownLabel, second));
	write_file(directory / "object_pool_with_label_2.iopx", join(grownLabel, join(second, first)));
	write_file(directory / "object_pool_with_label_3.iopx", join(singleLabel, single));
	write_file(directory / "object_pool_with_label_4.iopx32", join(extendedLabel, extended));
	write_file(directory / "object_pool_0.iop", first);
	write_file(directory / "object_pool_1.iop", second);
	write_file(directory / "object_pool_2.iop", join(second, first));
	write_file(directory / "object_pool_3.iop", single);
	write_file(directory / "object_pool_4.iop", extended);
	write_file(directory / "object_pool_5.iop", { 0xDE, 0xAD }); // Left behind by an older delete

	VersionStorage storage(directory);
	CHECK(storage.load(grownLabel).empty());
	CHECK(std::vector<Bytes>({ singleLabel }) == storage.get_labels(7));
	CHECK(std::vector<Bytes>({ grownLabel }) == storage.get_retired_labels(7));
	CHECK(single == storage.load(singleLabel));
	CHECK(extended == storage.load(extendedLabel));
	CHECK(std::vector<Bytes>({ extendedLabel }) == storage.get_labels(32));
	CHECK(storage.get_retired_labels(32).empty());

	// The client uploads again
	store(storage, grownLabel, { first, second });
	CHECK(join(first, second) == storage.load(grownLabel));
	CHECK(storage.get_retired_labels(7).empty());
	CHECK(2 == storage.get_labels(7).size());
	for (int i = 0; i < 3; i++)
	{
		CHECK(!fs::exists(directory / ("object_pool_with_label_" + std::to_string(i) + ".iopx")));
		CHECK(!fs::exists(directory / ("object_pool_" + std::to_string(i) + ".iop")));
	}
	CHECK(!fs::exists(directory / "object_pool_5.iop"));
	CHECK(fs::exists(directory / "object_pool_with_label_3.iopx"));
	CHECK(fs::exists(directory / "object_pool_3.iop"));
	CHECK(single == storage.load(singleLabel));

	// A single file is replaced on the next Store as well
	store(storage, extendedLabel, { { 0xE2 } });
	CHECK(Bytes({ 0xE2 }) == storage.load(extendedLabel));
	CHECK(!fs::exists(directory / "object_pool_with_label_4.iopx32"));
	CHECK(!fs::exists(directory / "object_pool_4.iop"));

	// Delete Version removes a retired version too
	write_file(directory / "object_pool_with_label_6.iopx", join(grownLabel, first));
	write_file(directory / "object_pool_with_label_7.iopx", join(grownLabel, second));
	CHECK(storage.remove(grownLabel));
	CHECK(storage.get_retired_labels(7).empty());
	CHECK(storage.load(grownLabel).empty());
}

static void unreadable_component_fails_the_load(const fs::path &directory)
{
	VersionStorage storage(directory);
	const auto label = label_of("SHORT", 32);

	store(storage, label, { { 1, 2 } });
	CHECK(storage.save({ 3 }, label));

	// A component shorter than its label header
	for (const auto &entry : fs::directory_iterator(directory))
	{
		if ((".iopx" == entry.path().extension()) && ("_1" == entry.path().stem().string().substr(entry.path().stem().string().size() - 2)))
		{
			write_file(entry.path(), { 0x00 });
		}
	}
	CHECK(storage.load(label).empty());
}

int main()
{
	const auto root = fs::temp_directory_path() / ("agisovt-version-storage-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	int caseNumber = 0;
	const auto next_directory = [&root, &caseNumber]() { return root / std::to_string(caseNumber++); };

	store_after_load_keeps_the_version(next_directory());
	components_load_in_save_order(next_directory());
	label_lengths_are_separate_versions(next_directory());
	removing_what_is_not_stored(next_directory());
	legacy_files_retire_or_load(next_directory());
	unreadable_component_fails_the_load(next_directory());

	std::error_code error;
	fs::remove_all(root, error);

	if (0 != failures)
	{
		std::cerr << failures << " check(s) failed\n";
		return EXIT_FAILURE;
	}
	std::cout << "All version storage checks passed\n";
	return EXIT_SUCCESS;
}
