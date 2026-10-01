/*******************************************************************************
** @file       vt_render.cpp
** @brief      vt-render: renders the working set designator and the masks of an
**             ISOBUS object pool to PNG files, plus a manifest.json with the hash
**             of every image, using the VT's own drawing components and without a
**             CAN bus or a window.
**
**             The output depends only on the pool, its meta.yaml and the VT source:
**             fonts are bundled, drawing is done by JUCE's software renderer at one
**             image pixel per VT pixel, and nothing time or machine dependent is
**             written. Two builds, such as the target and the head of a pull
**             request, can therefore be compared by their manifests.
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "JuceHeader.h"
#include "VtRenderFonts.h"
#include "git.h"

#include "JuceManagedWorkingSetCache.hpp"
#include "PictureGraphicComponent.hpp"
#include "SoftKeyMaskComponent.hpp"
#include "TextDrawingComponent.hpp"
#include "WorkingSetComponent.hpp"
#include "WorkingSetSelectorComponent.hpp"

#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>

namespace
{
	enum ExitCode
	{
		EXIT_OK = 0, ///< The pool loaded and every image was written
		EXIT_RENDER_FAILED = 1, ///< The pool loaded, but at least one image could not be rendered or written
		EXIT_LOAD_FAILED = 2, ///< The pool or its meta.yaml could not be loaded; only manifest.json was written
		EXIT_USAGE = 3
	};

	constexpr int MANIFEST_VERSION = 1;

	const char *get_object_type_name(isobus::VirtualTerminalObjectType type)
	{
		using Type = isobus::VirtualTerminalObjectType;
		switch (type)
		{
			case Type::WorkingSet:
				return "WorkingSet";
			case Type::DataMask:
				return "DataMask";
			case Type::AlarmMask:
				return "AlarmMask";
			case Type::Container:
				return "Container";
			case Type::WindowMask:
				return "WindowMask";
			case Type::SoftKeyMask:
				return "SoftKeyMask";
			case Type::Key:
				return "Key";
			case Type::Button:
				return "Button";
			case Type::KeyGroup:
				return "KeyGroup";
			case Type::InputBoolean:
				return "InputBoolean";
			case Type::InputString:
				return "InputString";
			case Type::InputNumber:
				return "InputNumber";
			case Type::InputList:
				return "InputList";
			case Type::OutputString:
				return "OutputString";
			case Type::OutputNumber:
				return "OutputNumber";
			case Type::OutputList:
				return "OutputList";
			case Type::OutputLine:
				return "OutputLine";
			case Type::OutputRectangle:
				return "OutputRectangle";
			case Type::OutputEllipse:
				return "OutputEllipse";
			case Type::OutputPolygon:
				return "OutputPolygon";
			case Type::OutputMeter:
				return "OutputMeter";
			case Type::OutputLinearBarGraph:
				return "OutputLinearBarGraph";
			case Type::OutputArchedBarGraph:
				return "OutputArchedBarGraph";
			case Type::GraphicsContext:
				return "GraphicsContext";
			case Type::Animation:
				return "Animation";
			case Type::PictureGraphic:
				return "PictureGraphic";
			case Type::GraphicData:
				return "GraphicData";
			case Type::ScaledGraphic:
				return "ScaledGraphic";
			case Type::NumberVariable:
				return "NumberVariable";
			case Type::StringVariable:
				return "StringVariable";
			case Type::FontAttributes:
				return "FontAttributes";
			case Type::LineAttributes:
				return "LineAttributes";
			case Type::FillAttributes:
				return "FillAttributes";
			case Type::InputAttributes:
				return "InputAttributes";
			case Type::ExtendedInputAttributes:
				return "ExtendedInputAttributes";
			case Type::ColourMap:
				return "ColourMap";
			case Type::ObjectLabelRefrenceList:
				return "ObjectLabelReferenceList";
			case Type::ObjectPointer:
				return "ObjectPointer";
			case Type::ExternalObjectDefinition:
				return "ExternalObjectDefinition";
			case Type::ExternalReferenceNAME:
				return "ExternalReferenceNAME";
			case Type::ExternalObjectPointer:
				return "ExternalObjectPointer";
			case Type::Macro:
				return "Macro";
			case Type::AuxiliaryFunctionType1:
				return "AuxiliaryFunctionType1";
			case Type::AuxiliaryInputType1:
				return "AuxiliaryInputType1";
			case Type::AuxiliaryFunctionType2:
				return "AuxiliaryFunctionType2";
			case Type::AuxiliaryInputType2:
				return "AuxiliaryInputType2";
			case Type::AuxiliaryControlDesignatorType2:
				return "AuxiliaryControlDesignatorType2";
			case Type::ManufacturerDefined1:
				return "ManufacturerDefined1";
			case Type::ManufacturerDefined2:
				return "ManufacturerDefined2";
			default:
				return "Unknown";
		}
	}

	String get_object_type_label(isobus::VirtualTerminalObjectType type)
	{
		const String name = get_object_type_name(type);
		return (name == "Unknown") ? ("Unknown(" + String(static_cast<int>(type)) + ")") : name;
	}

	/// Objects that can appear in the rendered images but that the VT has no component for, or that never
	/// appear in any image. Everything else is either drawn or only referenced by drawn objects (fonts,
	/// variables, macros, ...). Returns nullptr for those.
	const char *get_skipped_object_status(isobus::VirtualTerminalObjectType type)
	{
		using Type = isobus::VirtualTerminalObjectType;
		switch (type)
		{
			case Type::WindowMask:
			case Type::KeyGroup:
			case Type::OutputList:
			case Type::OutputArchedBarGraph:
			case Type::GraphicsContext:
			case Type::Animation:
			case Type::GraphicData:
			case Type::ScaledGraphic:
			case Type::ExternalObjectPointer:
			case Type::ManufacturerDefined1:
			case Type::ManufacturerDefined2:
				return "unsupported";

			case Type::ExternalObjectDefinition:
			case Type::ExternalReferenceNAME:
			case Type::AuxiliaryFunctionType1:
			case Type::AuxiliaryInputType1:
			case Type::AuxiliaryFunctionType2:
			case Type::AuxiliaryInputType2:
			case Type::AuxiliaryControlDesignatorType2:
				return "ignored";

			case Type::WorkingSet:
			case Type::DataMask:
			case Type::AlarmMask:
			case Type::Container:
			case Type::SoftKeyMask:
			case Type::Key:
			case Type::Button:
			case Type::InputBoolean:
			case Type::InputString:
			case Type::InputNumber:
			case Type::InputList:
			case Type::OutputString:
			case Type::OutputNumber:
			case Type::OutputLine:
			case Type::OutputRectangle:
			case Type::OutputEllipse:
			case Type::OutputPolygon:
			case Type::OutputMeter:
			case Type::OutputLinearBarGraph:
			case Type::PictureGraphic:
			case Type::NumberVariable:
			case Type::StringVariable:
			case Type::FontAttributes:
			case Type::LineAttributes:
			case Type::FillAttributes:
			case Type::InputAttributes:
			case Type::ExtendedInputAttributes:
			case Type::ColourMap:
			case Type::ObjectLabelRefrenceList:
			case Type::ObjectPointer:
			case Type::Macro:
				return nullptr;

			default:
				return "unsupported";
		}
	}

	/// Resolves every font to the bundled DejaVu Sans Mono, so that no render depends on the fonts installed
	class BundledFontLookAndFeel : public LookAndFeel_V4
	{
	public:
		BundledFontLookAndFeel()
		{
			regular = load("DejaVuSansMono.ttf");
			bold = load("DejaVuSansMono-Bold.ttf");
			italic = load("DejaVuSansMono-Oblique.ttf");
			boldItalic = load("DejaVuSansMono-BoldOblique.ttf");
		}

		bool is_loaded() const
		{
			return (nullptr != regular) && (nullptr != bold) && (nullptr != italic) && (nullptr != boldItalic);
		}

		Typeface::Ptr getTypefaceForFont(const Font &font) override
		{
			if (font.isBold())
			{
				return font.isItalic() ? boldItalic : bold;
			}
			return font.isItalic() ? italic : regular;
		}

		const Array<var> &get_font_descriptions() const
		{
			return descriptions;
		}

	private:
		Typeface::Ptr load(const String &fileName)
		{
			for (int i = 0; i < VtRenderFonts::namedResourceListSize; i++)
			{
				if (fileName == VtRenderFonts::getNamedResourceOriginalFilename(VtRenderFonts::namedResourceList[i]))
				{
					int size = 0;
					const auto data = VtRenderFonts::getNamedResource(VtRenderFonts::namedResourceList[i], size);

					DynamicObject::Ptr description = new DynamicObject();
					description->setProperty("file", fileName);
					description->setProperty("sha256", SHA256(data, static_cast<size_t>(size)).toHexString());
					descriptions.add(description.get());
					return Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
				}
			}
			return nullptr;
		}

		Typeface::Ptr regular;
		Typeface::Ptr bold;
		Typeface::Ptr italic;
		Typeface::Ptr boldItalic;
		Array<var> descriptions;
	};

	/// Collects the CAN stack's error messages, which carry the reason a pool failed to parse
	class ErrorCollector : public isobus::CANStackLogger
	{
	public:
		void sink_CAN_stack_log(LoggingLevel level, const std::string &text) override
		{
			if (level >= LoggingLevel::Error)
			{
				messages.add(String::fromUTF8(text.c_str()));
			}
		}

		StringArray messages;
	};

	struct PoolMeta
	{
		String id;
		String name;
		String manufacturer;
		bool isPublic = false;
		StringArray knownIssues;
		std::map<String, int> render; ///< The render: values that are not TODO
	};

	struct RenderSettings
	{
		int dataMaskSize = 480; // what the VT uses until the user sets another size
		int softKeyWidth = SoftKeyMaskDimensions().keyWidth;
		int softKeyHeight = SoftKeyMaskDimensions().keyHeight;
		int softKeyCount = SoftKeyMaskDimensions().key_count();
		int softKeyRows = 0;
		int softKeyColumns = 0;
		StringArray fromMeta;

		SoftKeyMaskDimensions get_dimensions() const
		{
			SoftKeyMaskDimensions dimensions;
			dimensions.keyWidth = softKeyWidth;
			dimensions.keyHeight = softKeyHeight;
			dimensions.rowCount = softKeyRows;
			dimensions.columnCount = softKeyColumns;
			dimensions.height = dataMaskSize;
			return dimensions;
		}

		/// Width of the VT's soft key area, the same formula as ServerMainComponent::resized
		int get_soft_key_area_width() const
		{
			return 2 * SoftKeyMaskDimensions::PADDING + softKeyColumns * (SoftKeyMaskDimensions::PADDING + softKeyHeight);
		}
	};

	/// Everything that is known about a pool before it is parsed
	struct PoolInput
	{
		String id;
		PoolMeta meta;
		MemoryBlock data;
		String sha256;
		String error; ///< Set if the pool or its meta.yaml could not be read
	};

	String unquote(const String &value)
	{
		if ((value.length() >= 2) &&
		    ((value.startsWithChar('"') && value.endsWithChar('"')) || (value.startsWithChar('\'') && value.endsWithChar('\''))))
		{
			return value.substring(1, value.length() - 1);
		}
		return value;
	}

	/// Reads the fields of meta.yaml that vt-render uses. It is not a YAML parser: it understands the flat
	/// top-level keys and the render: block that the collection's schema allows, and skips everything else.
	bool read_meta(const File &file, PoolMeta &meta, String &error)
	{
		if (!file.existsAsFile())
		{
			error = "meta.yaml not found";
			return false;
		}

		StringArray lines;
		lines.addLines(file.loadFileAsString());
		String block; // the top-level key whose indented lines follow

		for (const auto &line : lines)
		{
			if (line.trim().isEmpty() || line.trimStart().startsWithChar('#'))
			{
				continue;
			}

			const bool indented = line.startsWithChar(' ') || line.startsWithChar('\t');
			const auto key = line.upToFirstOccurrenceOf(":", false, false).trim();
			const auto value = unquote(line.fromFirstOccurrenceOf(":", false, false).trim());

			if (!indented)
			{
				block = key;

				if (key == "id")
				{
					meta.id = value;
				}
				else if (key == "name")
				{
					meta.name = value;
				}
				else if (key == "manufacturer")
				{
					meta.manufacturer = value;
				}
				else if (key == "public")
				{
					if ((value != "true") && (value != "false"))
					{
						error = "meta.yaml: public is neither true nor false: " + value;
						return false;
					}
					meta.isPublic = (value == "true");
				}
			}
			else if ((block == "known_issues") && line.trimStart().startsWith("- "))
			{
				meta.knownIssues.add(unquote(line.trimStart().substring(2).trim()));
			}
			else if ((block == "render") && line.containsChar(':') && (value != "TODO"))
			{
				if (value.isEmpty() || !value.containsOnly("0123456789") || (value.getIntValue() < 1))
				{
					error = "meta.yaml: render." + key + " is neither a positive integer nor TODO: " + value;
					return false;
				}
				meta.render[key] = value.getIntValue();
			}
		}

		if ((8 != meta.id.length()) || !meta.id.containsOnly("0123456789abcdef"))
		{
			error = "meta.yaml: id is not 8 lower case hex digits: " + meta.id;
			return false;
		}
		return true;
	}

	RenderSettings resolve_settings(const PoolMeta &meta)
	{
		RenderSettings settings;
		const std::pair<const char *, int *> fields[] = {
			{ "data_mask_size", &settings.dataMaskSize },
			{ "softkey_count", &settings.softKeyCount },
			{ "softkey_width", &settings.softKeyWidth },
			{ "softkey_height", &settings.softKeyHeight }
		};

		for (const auto &field : fields)
		{
			const auto value = meta.render.find(field.first);
			if (meta.render.end() != value)
			{
				*field.second = value->second;
				settings.fromMeta.add(field.first);
			}
		}

		// The keys go in one column, top to bottom, as many as fit next to the data mask, then in the next
		// column to the left. With the VT's defaults (480, 60 x 60, 6 keys) that is its own 6 x 1 layout.
		const int rowsThatFit = std::max(1, (settings.dataMaskSize - SoftKeyMaskDimensions::PADDING) / (settings.softKeyHeight + SoftKeyMaskDimensions::PADDING));
		settings.softKeyRows = std::min(settings.softKeyCount, rowsThatFit);
		settings.softKeyColumns = (settings.softKeyCount + settings.softKeyRows - 1) / settings.softKeyRows;
		return settings;
	}

	PoolInput read_pool_input(const File &iopFile, const File &metaFile)
	{
		PoolInput input;

		if (iopFile.loadFileAsData(input.data) && (input.data.getSize() > 0))
		{
			input.sha256 = SHA256(input.data).toHexString();
		}
		else
		{
			input.data.reset();
			input.error = "could not read the pool file, or it is empty";
		}

		String metaError;
		if (!read_meta(metaFile, input.meta, metaError) && input.error.isEmpty())
		{
			input.error = metaError;
		}

		// The collection's id rule, so that a pool with a broken meta.yaml still gets a stable folder
		if (input.meta.id.length() == 8 && input.meta.id.containsOnly("0123456789abcdef"))
		{
			input.id = input.meta.id;
		}
		else if (input.sha256.isNotEmpty())
		{
			input.id = input.sha256.substring(0, 8);
		}
		else
		{
			input.id = "unknown";
		}
		return input;
	}

	/// SHA-256 of the width and height (32-bit big endian) followed by the pixels as unpremultiplied
	/// RGBA8, row by row. Unlike a hash of the PNG file, it does not depend on the PNG encoder.
	String get_pixel_sha256(const Image &image)
	{
		MemoryOutputStream stream(static_cast<size_t>(8 + 4 * image.getWidth() * image.getHeight()));
		stream.writeIntBigEndian(image.getWidth());
		stream.writeIntBigEndian(image.getHeight());

		const Image::BitmapData pixels(image, Image::BitmapData::readOnly);
		for (int y = 0; y < image.getHeight(); y++)
		{
			for (int x = 0; x < image.getWidth(); x++)
			{
				const auto colour = pixels.getPixelColour(x, y);
				stream.writeByte(static_cast<char>(colour.getRed()));
				stream.writeByte(static_cast<char>(colour.getGreen()));
				stream.writeByte(static_cast<char>(colour.getBlue()));
				stream.writeByte(static_cast<char>(colour.getAlpha()));
			}
		}
		return SHA256(stream.getData(), stream.getDataSize()).toHexString();
	}

	/// Paints into a software image. The glyph cache is cleared first: it matches fonts approximately, so
	/// otherwise text would depend on what was drawn before, and an image on the order it was rendered in.
	Image paint_image(int width, int height, const std::function<void(Graphics &)> &paint)
	{
		Typeface::clearTypefaceCache();
		Image image(Image::ARGB, width, height, true, SoftwareImageType());
		{
			Graphics g(image);
			paint(g);
		}
		return image;
	}

	bool write_png(const Image &image, const File &file)
	{
		file.deleteFile();
		FileOutputStream stream(file);
		PNGImageFormat png;
		return stream.openedOk() && png.writeImageToStream(image, stream) && (stream.flush(), stream.getStatus().wasOk());
	}

	/// Removes what an earlier run left, so that a mask that no longer exists does not leave its image behind
	void remove_previous_output(const File &directory)
	{
		const StringArray prefixes = { "ws_designator", "dm_", "am_", "skm_", "screen_" };
		for (const auto &file : directory.findChildFiles(File::findFiles, false, "*.png;manifest.json"))
		{
			const auto name = file.getFileName();
			bool isOurs = (name == "manifest.json");
			for (const auto &prefix : prefixes)
			{
				isOurs = isOurs || name.startsWith(prefix);
			}
			if (isOurs)
			{
				file.deleteFile();
			}
		}
	}

	/// Unlike VirtualTerminalWorkingSetBase::get_object_by_id, does not add an entry for a missing id
	std::shared_ptr<isobus::VTObject> find_object(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, std::uint16_t id)
	{
		const auto &tree = workingSet->get_object_tree();
		const auto object = tree.find(id);
		return (tree.end() != object) ? object->second : nullptr;
	}

	class PoolRenderer
	{
	public:
		PoolRenderer(const BundledFontLookAndFeel &lookAndFeel, ErrorCollector &errorCollector) :
		  fonts(lookAndFeel),
		  background(lookAndFeel.findColour(ResizableWindow::backgroundColourId)),
		  collector(errorCollector)
		{
		}

		ExitCode render(const PoolInput &input, const File &outputDirectory)
		{
			images.clear();
			skippedObjects.clear();
			errors.clear();
			settings = resolve_settings(input.meta);
			directory = outputDirectory;

			String loadError = input.error;
			std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet;

			if (loadError.isEmpty())
			{
				workingSet = parse(input.data, loadError);
			}

			if (!directory.createDirectory())
			{
				std::cout << "FAIL  " << input.id << ": could not create " << directory.getFullPathName() << std::endl;
				return EXIT_RENDER_FAILED;
			}
			remove_previous_output(directory);

			if (nullptr != workingSet)
			{
				JuceManagedWorkingSetCache::set_data_mask_size(settings.dataMaskSize);
				JuceManagedWorkingSetCache::set_softkey_mask_dimension_info(settings.get_dimensions());
				render_working_set(workingSet);
			}

			std::sort(images.begin(), images.end(), [](const var &a, const var &b) { return a["file"].toString() < b["file"].toString(); });

			ExitCode result = EXIT_OK;
			if (loadError.isNotEmpty())
			{
				result = EXIT_LOAD_FAILED;
			}
			else if (!errors.isEmpty())
			{
				result = EXIT_RENDER_FAILED;
			}

			if (!write_manifest(input, loadError))
			{
				std::cout << "FAIL  " << input.id << ": could not write manifest.json" << std::endl;
				return (EXIT_OK == result) ? EXIT_RENDER_FAILED : result;
			}

			if (EXIT_LOAD_FAILED == result)
			{
				std::cout << "FAIL  " << input.id << ": " << loadError << std::endl;
			}
			else
			{
				std::cout << ((EXIT_OK == result) ? "OK    " : "ERROR ") << input.id << ": " << images.size() << " images, "
				          << errors.size() << " errors, " << skippedObjects.size() << " objects skipped" << std::endl;
			}
			return result;
		}

	private:
		std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parse(const MemoryBlock &data, String &error)
		{
			auto workingSet = std::make_shared<isobus::VirtualTerminalServerManagedWorkingSet>();
			auto bytes = static_cast<const std::uint8_t *>(data.getData());
			workingSet->add_iop_raw_data(std::vector<std::uint8_t>(bytes, bytes + data.getSize()));

			collector.messages.clear();
			auto &rawData = workingSet->get_iop_raw_data(0);
			if (workingSet->parse_iop_into_objects(rawData.data(), static_cast<std::uint32_t>(rawData.size())))
			{
				return workingSet;
			}

			error = "parsing failed";
			if (isobus::NULL_OBJECT_ID != workingSet->get_object_pool_faulting_object_id())
			{
				error << " at object " << String(workingSet->get_object_pool_faulting_object_id());
			}
			if (!collector.messages.isEmpty())
			{
				error << ": " << collector.messages.joinIntoString("; ");
			}
			return nullptr;
		}

		void render_working_set(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet)
		{
			for (const auto &entry : workingSet->get_object_tree())
			{
				const auto id = entry.first;
				const auto &object = entry.second;

				// get_object_by_id adds an empty entry for an id that is not in the pool, and the VT's
				// components call it while they are created, so empty entries turn up during the loop
				if (nullptr == object)
				{
					continue;
				}

				switch (object->get_object_type())
				{
					case isobus::VirtualTerminalObjectType::DataMask:
					{
						render_mask(workingSet, object, "dm_" + String(id) + ".png");
						render_screen(workingSet, object);
					}
					break;

					case isobus::VirtualTerminalObjectType::AlarmMask:
					{
						render_mask(workingSet, object, "am_" + String(id) + ".png");
					}
					break;

					case isobus::VirtualTerminalObjectType::SoftKeyMask:
					{
						render_mask(workingSet, object, "skm_" + String(id) + ".png");
						list_keys_without_position(workingSet, object);
					}
					break;

					default:
					{
						const auto status = get_skipped_object_status(object->get_object_type());
						if (nullptr != status)
						{
							add_skipped_object(id, object->get_object_type(), status, "the VT does not draw this object type");
						}
					}
					break;
				}
			}
			render_designator(workingSet, false);
			render_designator(workingSet, true);
		}

		void render_mask(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet,
		                 const std::shared_ptr<isobus::VTObject> &mask,
		                 const String &fileName)
		{
			auto component = JuceManagedWorkingSetCache::create_component(workingSet, mask);
			if ((nullptr == component) || component->getLocalBounds().isEmpty())
			{
				add_error(fileName, mask, "the VT created no component for it");
				return;
			}

			const auto image = paint_image(component->getWidth(), component->getHeight(), [&](Graphics &g) {
				component->paintEntireComponent(g, true);
			});
			add_image(fileName, mask, image);
		}

		/// The data mask next to its soft key mask, laid out as the data and soft key mask render areas of the VT
		void render_screen(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet,
		                   const std::shared_ptr<isobus::VTObject> &dataMask)
		{
			const auto fileName = "screen_" + String(dataMask->get_id()) + ".png";
			auto dataMaskComponent = JuceManagedWorkingSetCache::create_component(workingSet, dataMask);
			if (nullptr == dataMaskComponent)
			{
				add_error(fileName, dataMask, "the VT created no component for the data mask");
				return;
			}

			const auto softKeyMaskId = std::static_pointer_cast<isobus::DataMask>(dataMask)->get_soft_key_mask();
			auto softKeyMask = find_object(workingSet, softKeyMaskId);
			std::shared_ptr<Component> softKeyMaskComponent;
			if ((nullptr != softKeyMask) && (isobus::VirtualTerminalObjectType::SoftKeyMask == softKeyMask->get_object_type()))
			{
				softKeyMaskComponent = JuceManagedWorkingSetCache::create_component(workingSet, softKeyMask);
			}

			const int dataMaskSize = settings.dataMaskSize;
			const int softKeyAreaWidth = settings.get_soft_key_area_width();
			// In the VT each mask is a child of its render area and is clipped to its own bounds by JUCE;
			// painted directly, a mask's fillAll would cover whatever is left of the area as well
			const auto paintClipped = [](Graphics &g, Component &component) {
				Graphics::ScopedSaveState state(g);
				g.reduceClipRegion(component.getBounds());
				component.paintEntireComponent(g, true);
			};

			const auto image = paint_image(dataMaskSize + softKeyAreaWidth, dataMaskSize, [&](Graphics &g) {
				// DataMaskRenderAreaComponent::paint is covered by the data mask, so only the mask is painted
				g.fillAll(background);
				paintClipped(g, *dataMaskComponent);

				// SoftKeyMaskRenderAreaComponent::paint, then the soft key mask as its child
				Graphics::ScopedSaveState state(g);
				g.setOrigin(dataMaskSize, 0);
				g.reduceClipRegion(0, 0, softKeyAreaWidth, dataMaskSize);
				g.setColour(Colours::black);
				g.drawRect(0, 0, softKeyAreaWidth, dataMaskSize, 1);
				if (nullptr != softKeyMaskComponent)
				{
					paintClipped(g, *softKeyMaskComponent);
				}
			});

			auto record = add_image(fileName, dataMask, image);
			if (nullptr != record)
			{
				record->setProperty("soft_key_mask_id", (nullptr != softKeyMaskComponent) ? var(static_cast<int>(softKeyMaskId)) : var());
			}
		}

		/// The designator as the working set selector shows it: fitted to the selector button, on the
		/// working set's background colour, inside the padding of the selector column
		void render_designator(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, bool active)
		{
			const String fileName = active ? "ws_designator_active.png" : "ws_designator.png";
			auto workingSetObject = workingSet->get_working_set_object();
			if (nullptr == workingSetObject)
			{
				add_error(fileName, nullptr, "the pool has no working set object");
				return;
			}

			auto designator = JuceManagedWorkingSetCache::create_component(workingSet, workingSetObject);
			if (nullptr == designator)
			{
				add_error(fileName, workingSetObject, "the VT created no component for it");
				return;
			}

			constexpr int padding = (WorkingSetSelectorComponent::WIDTH - WorkingSetSelectorComponent::BUTTON_WIDTH) / 2;
			const juce::Rectangle<int> button(padding, padding, WorkingSetSelectorComponent::BUTTON_WIDTH, WorkingSetSelectorComponent::BUTTON_HEIGHT);
			Component column;
			column.setSize(WorkingSetSelectorComponent::WIDTH, button.getBottom() + padding);
			designator->setTopLeftPosition(button.getPosition());
			WorkingSetComponent::fit_designator_to_button(*designator, button);
			column.addAndMakeVisible(*designator);

			const auto vtColour = workingSet->get_colour(std::static_pointer_cast<isobus::WorkingSet>(workingSetObject)->get_background_color());
			const auto image = paint_image(column.getWidth(), column.getHeight(), [&](Graphics &g) {
				g.fillAll(background);
				g.setColour(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
				g.fillRect(button);
				column.paintEntireComponent(g, true);
				if (active)
				{
					WorkingSetComponent::paint_active_highlight(g, button);
				}
			});
			column.removeAllChildren();

			auto record = add_image(fileName, workingSetObject, image);
			if (nullptr != record)
			{
				record->setProperty("active", active);
			}
		}

		/// The VT has a fixed number of key positions and draws keys past them outside the soft key mask
		void list_keys_without_position(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet,
		                                const std::shared_ptr<isobus::VTObject> &softKeyMask)
		{
			const int positions = settings.get_dimensions().key_count();
			int position = 0;

			for (std::uint16_t i = 0; i < softKeyMask->get_number_children(); i++)
			{
				auto child = find_object(workingSet, softKeyMask->get_child_id(i));

				// SoftKeyMaskComponent gives a position to each child it gets a component for
				if ((nullptr == child) || (nullptr == JuceManagedWorkingSetCache::create_component(workingSet, child)))
				{
					continue;
				}

				position++;
				if (position > positions)
				{
					add_skipped_object(child->get_id(),
					                   child->get_object_type(),
					                   "ignored",
					                   "child " + String(position) + " of soft key mask " + String(softKeyMask->get_id()) + ", which has only " + String(positions) + " key positions");
				}
			}
		}

		DynamicObject *add_image(const String &fileName, const std::shared_ptr<isobus::VTObject> &object, const Image &image)
		{
			if (!write_png(image, directory.getChildFile(fileName)))
			{
				add_error(fileName, object, "could not write the PNG file");
				return nullptr;
			}

			DynamicObject::Ptr record = new DynamicObject();
			record->setProperty("file", fileName);
			record->setProperty("object_id", static_cast<int>(object->get_id()));
			record->setProperty("object_type", get_object_type_label(object->get_object_type()));
			record->setProperty("width", image.getWidth());
			record->setProperty("height", image.getHeight());
			record->setProperty("sha256", get_pixel_sha256(image));
			images.add(record.get());
			return record.get();
		}

		void add_error(const String &fileName, const std::shared_ptr<isobus::VTObject> &object, const String &message)
		{
			DynamicObject::Ptr record = new DynamicObject();
			record->setProperty("file", fileName);
			record->setProperty("object_id", (nullptr != object) ? var(static_cast<int>(object->get_id())) : var());
			record->setProperty("object_type", (nullptr != object) ? var(get_object_type_label(object->get_object_type())) : var());
			record->setProperty("error", message);
			errors.add(record.get());
		}

		void add_skipped_object(std::uint16_t id, isobus::VirtualTerminalObjectType type, const String &status, const String &reason)
		{
			DynamicObject::Ptr record = new DynamicObject();
			record->setProperty("object_id", static_cast<int>(id));
			record->setProperty("object_type", get_object_type_label(type));
			record->setProperty("status", status);
			record->setProperty("reason", reason);
			skippedObjects.add(record.get());
		}

		var describe_settings() const
		{
			DynamicObject::Ptr result = new DynamicObject();
#if JUCE_WINDOWS
			result->setProperty("platform", "windows");
#elif JUCE_LINUX
			result->setProperty("platform", "linux");
#elif JUCE_MAC
			result->setProperty("platform", "macos");
#else
			result->setProperty("platform", "other");
#endif
			result->setProperty("renderer", "JUCE software renderer");
			result->setProperty("juce_version", SystemStats::getJUCEVersion());
			result->setProperty("pixels_per_vt_pixel", 1);
			result->setProperty("data_mask_size", settings.dataMaskSize);
			result->setProperty("softkey_count", settings.softKeyCount);
			result->setProperty("softkey_width", settings.softKeyWidth);
			result->setProperty("softkey_height", settings.softKeyHeight);
			result->setProperty("softkey_rows", settings.softKeyRows);
			result->setProperty("softkey_columns", settings.softKeyColumns);
			result->setProperty("softkey_padding", static_cast<int>(SoftKeyMaskDimensions::PADDING));
			result->setProperty("working_set_button_size", WorkingSetSelectorComponent::BUTTON_WIDTH);

			Array<var> fromMeta;
			for (const auto &key : settings.fromMeta)
			{
				fromMeta.add(key);
			}
			result->setProperty("from_meta", fromMeta);
			result->setProperty("fonts", fonts.get_font_descriptions());
			result->setProperty("font_fallback", false);
			result->setProperty("pixel_hash", "sha256 of width and height as uint32 big endian, then the pixels as unpremultiplied RGBA8, row by row");
			return result.get();
		}

		bool write_manifest(const PoolInput &input, const String &loadError) const
		{
			DynamicObject::Ptr load = new DynamicObject();
			load->setProperty("ok", loadError.isEmpty());
			load->setProperty("error", loadError.isEmpty() ? var() : var(loadError));

			DynamicObject::Ptr manifest = new DynamicObject();
			manifest->setProperty("manifest_version", MANIFEST_VERSION);
			manifest->setProperty("pool_id", input.id);
			manifest->setProperty("pool_name", input.meta.name);
			manifest->setProperty("manufacturer", input.meta.manufacturer);
			manifest->setProperty("public", input.meta.isPublic);

			Array<var> knownIssues;
			for (const auto &issue : input.meta.knownIssues)
			{
				knownIssues.add(issue);
			}
			manifest->setProperty("known_issues", knownIssues);
			manifest->setProperty("pool_sha256", input.sha256.isEmpty() ? var() : var(input.sha256));
			manifest->setProperty("pool_size", static_cast<int64>(input.data.getSize()));
			manifest->setProperty("vt_commit", git_IsPopulated() ? String(git_CommitSHA1()) : String("unknown"));
			manifest->setProperty("vt_commit_dirty", git_IsPopulated() && git_AnyUncommittedChanges());
			manifest->setProperty("render_settings", describe_settings());
			manifest->setProperty("load", load.get());
			manifest->setProperty("images", images);
			manifest->setProperty("unsupported_objects", skippedObjects);
			manifest->setProperty("errors", errors);

			const auto text = JSON::toString(var(manifest.get()), false).replace("\r\n", "\n") + "\n";
			return directory.getChildFile("manifest.json").replaceWithText(text, false, false, "\n");
		}

		const BundledFontLookAndFeel &fonts;
		const Colour background;
		ErrorCollector &collector;
		RenderSettings settings;
		File directory;
		Array<var> images;
		Array<var> skippedObjects;
		Array<var> errors;
	};

	/// A load failure outranks a render failure
	ExitCode get_worse(ExitCode a, ExitCode b)
	{
		const auto rank = [](ExitCode code) { return (EXIT_LOAD_FAILED == code) ? 2 : ((EXIT_OK == code) ? 0 : 1); };
		return (rank(b) > rank(a)) ? b : a;
	}

	ExitCode render_collection(const File &root, const File &outputDirectory, PoolRenderer &renderer)
	{
		const auto pools = root.getChildFile("pools");
		Array<File> poolDirectories;
		for (const auto &manufacturer : pools.findChildFiles(File::findDirectories, false))
		{
			poolDirectories.addArray(manufacturer.findChildFiles(File::findDirectories, false));
		}
		poolDirectories.sort();

		if (poolDirectories.isEmpty())
		{
			std::cout << "No pool folders found in " << pools.getFullPathName() << std::endl;
			return EXIT_LOAD_FAILED;
		}

		ExitCode result = EXIT_OK;
		std::map<String, File> seenIds;
		for (const auto &directory : poolDirectories)
		{
			const auto input = read_pool_input(directory.getChildFile("pool.iop"), directory.getChildFile("meta.yaml"));
			const auto relativePath = directory.getRelativePathFrom(root).replaceCharacter('\\', '/');

			if (seenIds.count(input.id) > 0)
			{
				std::cout << "FAIL  " << relativePath << ": pool id " << input.id << " is also used by "
				          << seenIds[input.id].getRelativePathFrom(root).replaceCharacter('\\', '/') << "; not rendered" << std::endl;
				result = EXIT_LOAD_FAILED;
				continue;
			}
			seenIds[input.id] = directory;

			std::cout << relativePath << std::endl;
			result = get_worse(result, renderer.render(input, outputDirectory.getChildFile(input.id)));
		}
		return result;
	}

	void print_usage()
	{
		std::cout << "Usage:\n"
		             "  vt-render --pool <pool.iop> --meta <meta.yaml> --out <folder>\n"
		             "  vt-render --all <collection root> --out <folder>\n"
		             "\n"
		             "Renders the working set designator and every data mask, alarm mask and soft key mask\n"
		             "of an object pool to PNG, plus manifest.json with the hash of every image.\n"
		             "--all renders each <collection root>/pools/*/*/ into <folder>/<pool id>/.\n"
		             "\n"
		             "Exit codes: 0 everything rendered, 1 some image could not be rendered,\n"
		             "            2 a pool could not be loaded, 3 wrong arguments."
		          << std::endl;
	}
}

int main(int argc, char *argv[])
{
	ScopedJuceInitialiser_GUI juceInitialiser;

	std::map<String, String> arguments;
	for (int i = 1; i < argc; i++)
	{
		const auto name = String::fromUTF8(argv[i]);
		if (!StringArray({ "--pool", "--meta", "--out", "--all" }).contains(name) || (i + 1 >= argc) || (arguments.count(name) > 0))
		{
			print_usage();
			return EXIT_USAGE;
		}
		arguments[name] = String::fromUTF8(argv[++i]);
	}

	const bool singlePool = (arguments.count("--pool") > 0) && (arguments.count("--meta") > 0) && (0 == arguments.count("--all"));
	const bool collection = (arguments.count("--all") > 0) && (0 == arguments.count("--pool")) && (0 == arguments.count("--meta"));
	if ((0 == arguments.count("--out")) || !(singlePool || collection))
	{
		print_usage();
		return EXIT_USAGE;
	}

	const auto resolve = [](const String &path) { return File::getCurrentWorkingDirectory().getChildFile(path); };
	const auto outputDirectory = resolve(arguments["--out"]);

	if (collection && !resolve(arguments["--all"]).getChildFile("pools").isDirectory())
	{
		std::cout << "No pools folder in " << resolve(arguments["--all"]).getFullPathName() << std::endl;
		return EXIT_USAGE;
	}

	BundledFontLookAndFeel lookAndFeel;
	if (!lookAndFeel.is_loaded())
	{
		std::cout << "The bundled fonts could not be loaded" << std::endl;
		return EXIT_RENDER_FAILED;
	}
	LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
	TextDrawingComponent::set_font_fallback_enabled(false);
	PictureGraphicComponent::set_use_software_images(true);

	ErrorCollector errorCollector;
	isobus::CANStackLogger::set_can_stack_logger_sink(&errorCollector);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Error);

	ExitCode result;
	{
		PoolRenderer renderer(lookAndFeel, errorCollector);
		if (singlePool)
		{
			result = renderer.render(read_pool_input(resolve(arguments["--pool"]), resolve(arguments["--meta"])), outputDirectory);
		}
		else
		{
			result = render_collection(resolve(arguments["--all"]), outputDirectory, renderer);
		}
	}

	isobus::CANStackLogger::set_can_stack_logger_sink(nullptr);
	LookAndFeel::setDefaultLookAndFeel(nullptr);
	return result;
}
