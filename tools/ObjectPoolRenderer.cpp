/*******************************************************************************
** @file       ObjectPoolRenderer.cpp
** @brief      Renders the masks of ISOBUS object pools to PNG files, without a
**             CAN bus or a window, and optionally compares them against a set
**             of reference images. Useful as a regression test for the VT's
**             drawing code.
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "JuceHeader.h"

#include "JuceManagedWorkingSetCache.hpp"

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include <iostream>
#include <regex>

namespace
{
	struct Options
	{
		File input;
		File outputDirectory;
		File referenceDirectory;
		int maxPixelDifference = 0;
	};

	struct Totals
	{
		int pools = 0;
		int poolsFailed = 0;
		int images = 0;
		int imagesMissingReference = 0;
		int imagesDifferent = 0;
	};

	const char *get_mask_type_name(isobus::VirtualTerminalObjectType type)
	{
		switch (type)
		{
			case isobus::VirtualTerminalObjectType::DataMask:
				return "DataMask";
			case isobus::VirtualTerminalObjectType::AlarmMask:
				return "AlarmMask";
			case isobus::VirtualTerminalObjectType::SoftKeyMask:
				return "SoftKeyMask";
			default:
				return nullptr;
		}
	}

	/// Returns the number of pixels whose colour differs from the reference, or -1 if the sizes differ.
	/// Differing pixels are painted red into the diff image.
	int compare_images(const Image &rendered, const Image &reference, Image &diff)
	{
		if ((rendered.getWidth() != reference.getWidth()) || (rendered.getHeight() != reference.getHeight()))
		{
			return -1;
		}

		diff = rendered.convertedToFormat(Image::ARGB);
		int differentPixels = 0;
		for (int y = 0; y < rendered.getHeight(); y++)
		{
			for (int x = 0; x < rendered.getWidth(); x++)
			{
				if (rendered.getPixelAt(x, y).getARGB() != reference.getPixelAt(x, y).getARGB())
				{
					differentPixels++;
					diff.setPixelAt(x, y, Colours::red);
				}
				else
				{
					diff.setPixelAt(x, y, diff.getPixelAt(x, y).withMultipliedAlpha(0.25f));
				}
			}
		}
		return differentPixels;
	}

	bool write_png(const Image &image, const File &file)
	{
		file.getParentDirectory().createDirectory();
		file.deleteFile();
		FileOutputStream stream(file);
		PNGImageFormat png;
		return stream.openedOk() && png.writeImageToStream(image, stream);
	}

	void render_pool(const File &iopFile, const String &poolName, const Options &options, Totals &totals)
	{
		totals.pools++;

		MemoryBlock fileData;
		if (!iopFile.loadFileAsData(fileData) || (0 == fileData.getSize()))
		{
			std::cout << "FAIL  " << poolName << ": could not read file" << std::endl;
			totals.poolsFailed++;
			return;
		}

		auto workingSet = std::make_shared<isobus::VirtualTerminalServerManagedWorkingSet>();
		auto data = static_cast<const std::uint8_t *>(fileData.getData());
		workingSet->add_iop_raw_data(std::vector<std::uint8_t>(data, data + fileData.getSize()));

		if (!workingSet->parse_iop_into_objects(workingSet->get_iop_raw_data(0).data(), static_cast<std::uint32_t>(workingSet->get_iop_raw_data(0).size())))
		{
			std::cout << "FAIL  " << poolName << ": parsing failed at object " << workingSet->get_object_pool_faulting_object_id() << std::endl;
			totals.poolsFailed++;
			return;
		}

		int poolImages = 0;
		for (const auto &entry : workingSet->get_object_tree())
		{
			const auto typeName = get_mask_type_name(entry.second->get_object_type());
			if (nullptr == typeName)
			{
				continue;
			}

			// JUCE caches rasterised glyphs across draws, which would make a render depend on what was drawn before it
			Typeface::clearTypefaceCache();

			auto component = JuceManagedWorkingSetCache::create_component(workingSet, entry.second);
			if ((nullptr == component) || component->getLocalBounds().isEmpty())
			{
				continue;
			}

			const auto image = component->createComponentSnapshot(component->getLocalBounds(), true, 1.0f);
			const auto relativePath = poolName + "/" + typeName + "_" + String(entry.first) + ".png";
			write_png(image, options.outputDirectory.getChildFile(relativePath));
			totals.images++;
			poolImages++;

			if (options.referenceDirectory != File())
			{
				const auto referenceFile = options.referenceDirectory.getChildFile(relativePath);
				if (!referenceFile.existsAsFile())
				{
					std::cout << "NEW   " << relativePath << std::endl;
					totals.imagesMissingReference++;
					continue;
				}

				Image diff;
				const auto differentPixels = compare_images(image, ImageFileFormat::loadFrom(referenceFile), diff);
				if (differentPixels < 0)
				{
					std::cout << "DIFF  " << relativePath << ": size differs from reference" << std::endl;
					totals.imagesDifferent++;
				}
				else if (differentPixels > options.maxPixelDifference)
				{
					std::cout << "DIFF  " << relativePath << ": " << differentPixels << " pixels differ" << std::endl;
					write_png(diff, options.outputDirectory.getChildFile(relativePath.replace(".png", ".diff.png")));
					totals.imagesDifferent++;
				}
			}
		}
		std::cout << "OK    " << poolName << ": " << poolImages << " masks" << std::endl;
	}

	void print_usage()
	{
		std::cout << "Usage: ObjectPoolRenderer <pool.iop | folder> <output folder> [--compare <reference folder>] [--tolerance <pixels>]\n"
		             "\n"
		             "Renders every data mask, alarm mask and soft key mask of the object pool(s) to PNG.\n"
		             "A folder is searched recursively for .iop files; split pools (*_partNN.iop) are skipped,\n"
		             "since the collection also has them joined into one file.\n"
		             "With --compare, each render is checked against the image with the same path in the\n"
		             "reference folder; the exit code is non-zero if any differ or are missing."
		          << std::endl;
	}
}

int main(int argc, char *argv[])
{
	ScopedJuceInitialiser_GUI juceInitialiser;
	Options options;
	StringArray positional;

	for (int i = 1; i < argc; i++)
	{
		const auto argument = String::fromUTF8(argv[i]);
		if ((argument == "--compare") && (i + 1 < argc))
		{
			options.referenceDirectory = File::getCurrentWorkingDirectory().getChildFile(String::fromUTF8(argv[++i]));
		}
		else if ((argument == "--tolerance") && (i + 1 < argc))
		{
			options.maxPixelDifference = String(argv[++i]).getIntValue();
		}
		else
		{
			positional.add(argument);
		}
	}

	if (positional.size() != 2)
	{
		print_usage();
		return 2;
	}
	options.input = File::getCurrentWorkingDirectory().getChildFile(positional[0]);
	options.outputDirectory = File::getCurrentWorkingDirectory().getChildFile(positional[1]);

	Totals totals;
	if (options.input.isDirectory())
	{
		const std::regex splitPart(".*_part[0-9]+\\.iop", std::regex::icase);
		auto files = options.input.findChildFiles(File::findFiles, true, "*.iop");
		files.sort();
		for (const auto &file : files)
		{
			if (!std::regex_match(file.getFileName().toStdString(), splitPart))
			{
				render_pool(file, file.getRelativePathFrom(options.input).upToLastOccurrenceOf(".", false, false).replaceCharacter('\\', '/'), options, totals);
			}
		}
	}
	else
	{
		render_pool(options.input, options.input.getFileNameWithoutExtension(), options, totals);
	}

	std::cout << "\n"
	          << totals.pools << " pools (" << totals.poolsFailed << " failed to parse), " << totals.images << " masks rendered";
	if (options.referenceDirectory != File())
	{
		std::cout << ", " << totals.imagesDifferent << " differ, " << totals.imagesMissingReference << " have no reference";
	}
	std::cout << std::endl;

	const bool comparisonFailed = (options.referenceDirectory != File()) && ((totals.imagesDifferent > 0) || (totals.imagesMissingReference > 0));
	return ((totals.poolsFailed > 0) || comparisonFailed) ? 1 : 0;
}
