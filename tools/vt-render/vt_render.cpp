/*******************************************************************************
** @file       vt_render.cpp
** @brief      vt-render: renders an object pool to PNG files and a manifest.json
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "JuceHeader.h"

#include "JuceManagedWorkingSetCache.hpp"
#include "KeyComponent.hpp"
#include "Main.hpp"
#include "PictureGraphicComponent.hpp"
#include "SoftKeyMaskComponent.hpp"
#include "SoftKeyMaskRenderAreaComponent.hpp"
#include "SoftKeyPageBar.hpp"
#include "WorkingSetComponent.hpp"
#include "WorkingSetSelectorComponent.hpp"

#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>

enum ExitCode
{
	EXIT_OK = 0, ///< Every image was written
	EXIT_RENDER_FAILED = 1, ///< An image could not be rendered or written
	EXIT_LOAD_FAILED = 2, ///< The pool could not be loaded
	EXIT_USAGE = 3
};

constexpr int MANIFEST_VERSION = 1;

/// Collects the CAN stack's error messages
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

/// A pool read from disk
struct PoolInput
{
	String id;
	MemoryBlock data;
	String sha256;
	String error; ///< Set if the pool could not be read
};

class PoolRenderer
{
public:
	explicit PoolRenderer(ErrorCollector &errorCollector) :
	  background(LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId)),
	  collector(errorCollector)
	{
	}

	ExitCode render(const PoolInput &input, const File &outputDirectory)
	{
		images.clear();
		skippedObjects.clear();
		errors.clear();
		drawableTypes.clear();
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

	ExitCode render_collection(const File &root, const File &outputDirectory)
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
			const auto input = read_pool_input(directory.getChildFile("pool.iop"));
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
			result = get_worse(result, render(input, outputDirectory.getChildFile(input.id)));
		}
		return result;
	}

	static PoolInput read_pool_input(const File &iopFile)
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

		input.id = input.sha256.isNotEmpty() ? input.sha256.substring(0, 8) : String("unknown");
		return input;
	}

	static void print_usage()
	{
		std::cout << "Usage:\n"
		             "  vt-render --pool <pool.iop> --out <folder>\n"
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

private:
	static const char *get_object_type_name(isobus::VirtualTerminalObjectType type)
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

	static String get_object_type_label(isobus::VirtualTerminalObjectType type)
	{
		const String name = get_object_type_name(type);
		return (name == "Unknown") ? ("Unknown(" + String(static_cast<int>(type)) + ")") : name;
	}

	static String get_auxiliary_function_type_label(isobus::AuxiliaryFunctionType2::FunctionType type)
	{
		using FunctionType = isobus::AuxiliaryFunctionType2::FunctionType;
		switch (type)
		{
			case FunctionType::BooleanLatchingOnOff:
				return "BooleanLatchingOnOff";
			case FunctionType::Analouge:
				return "Analogue";
			case FunctionType::BooleanNonLatchingIncreaseValue:
				return "BooleanNonLatchingIncreaseValue";
			case FunctionType::AnalougeReturnTo50Percent:
				return "AnalogueReturnTo50Percent";
			case FunctionType::AnalougeReturnTo0PercentIncreaseValue:
				return "AnalogueReturnTo0PercentIncreaseValue";
			case FunctionType::DualBooleanBothLatching:
				return "DualBooleanBothLatching";
			case FunctionType::DualBooleanBothNonLatching:
				return "DualBooleanBothNonLatching";
			case FunctionType::DualBooleanLatchingUp:
				return "DualBooleanLatchingUp";
			case FunctionType::DualBooleanLatchingDown:
				return "DualBooleanLatchingDown";
			case FunctionType::CombinedAnalougeReturnTo50PercentWithDualBooleanLatching:
				return "CombinedAnalogueReturnTo50PercentWithDualBooleanLatching";
			case FunctionType::CombinedAnalougeMaintainsPositionWithDualBooleanLatching:
				return "CombinedAnalogueMaintainsPositionWithDualBooleanLatching";
			case FunctionType::QuadratureBooleanNonLatching:
				return "QuadratureBooleanNonLatching";
			case FunctionType::QuadratureAnalouge:
				return "QuadratureAnalogue";
			case FunctionType::QuadratureAnalougeReturnTo50Percent:
				return "QuadratureAnalogueReturnTo50Percent";
			case FunctionType::BidirectionalEncoder:
				return "BidirectionalEncoder";
			default:
				return "Reserved(" + String(static_cast<int>(type)) + ")";
		}
	}

	/// Whether an object type is drawn, only referred to, or ignored
	enum class ObjectRole
	{
		Drawn,
		Referenced,
		Ignored
	};

	static ObjectRole get_object_role(isobus::VirtualTerminalObjectType type)
	{
		using Type = isobus::VirtualTerminalObjectType;
		switch (type)
		{
			case Type::ExternalObjectDefinition:
			case Type::ExternalReferenceNAME:
			case Type::AuxiliaryFunctionType1:
			case Type::AuxiliaryInputType1:
			case Type::AuxiliaryControlDesignatorType2:
				return ObjectRole::Ignored;

			case Type::NumberVariable:
			case Type::StringVariable:
			case Type::FontAttributes:
			case Type::LineAttributes:
			case Type::FillAttributes:
			case Type::InputAttributes:
			case Type::ExtendedInputAttributes:
			case Type::ColourMap:
			case Type::ObjectLabelRefrenceList:
			case Type::GraphicData:
			case Type::Macro:
				return ObjectRole::Referenced;

			default:
				return ObjectRole::Drawn;
		}
	}

	/// SHA-256 of the width, height and unpremultiplied RGBA8 pixels
	static String get_pixel_sha256(const Image &image)
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

	/// Paints into a software image, with an empty glyph cache
	static Image paint_image(int width, int height, const std::function<void(Graphics &)> &paint)
	{
		Typeface::clearTypefaceCache();
		Image image(Image::ARGB, width, height, true, SoftwareImageType());
		{
			Graphics g(image);
			paint(g);
		}
		return image;
	}

	static bool write_png(const Image &image, const File &file)
	{
		file.deleteFile();
		FileOutputStream stream(file);
		PNGImageFormat png;
		return stream.openedOk() && png.writeImageToStream(image, stream) && (stream.flush(), stream.getStatus().wasOk());
	}

	/// Removes an earlier run's output
	static void remove_previous_output(const File &directory)
	{
		const StringArray prefixes = { "ws_designator", "dm_", "am_", "skm_", "screen_", "aux_" };
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

	/// Finds an object without adding an entry for a missing id
	static std::shared_ptr<isobus::VTObject> find_object(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, std::uint16_t id)
	{
		const auto &tree = workingSet->get_object_tree();
		const auto object = tree.find(id);
		return (tree.end() != object) ? object->second : nullptr;
	}

	/// A load failure outranks a render failure
	static ExitCode get_worse(ExitCode a, ExitCode b)
	{
		const auto rank = [](ExitCode code) { return (EXIT_LOAD_FAILED == code) ? 2 : ((EXIT_OK == code) ? 0 : 1); };
		return (rank(b) > rank(a)) ? b : a;
	}

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

			// The VT's components add empty entries for missing ids
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
					render_screen(workingSet, object);
				}
				break;

				case isobus::VirtualTerminalObjectType::SoftKeyMask:
				{
					render_soft_key_mask(workingSet, object);
				}
				break;

				case isobus::VirtualTerminalObjectType::AuxiliaryFunctionType2:
				case isobus::VirtualTerminalObjectType::AuxiliaryInputType2:
				{
					render_auxiliary_designator(workingSet, object);
				}
				break;

				default:
				{
					// The component factory decides what the VT can draw
					const auto role = get_object_role(object->get_object_type());
					if (ObjectRole::Ignored == role)
					{
						add_skipped_object(id, object->get_object_type(), "ignored", "not part of any mask");
					}
					else if ((ObjectRole::Drawn == role) && !is_drawable(workingSet, object))
					{
						add_skipped_object(id, object->get_object_type(), "unsupported", "the VT does not draw this object type");
					}
				}
				break;
			}
		}
		render_designator(workingSet, false);
		render_designator(workingSet, true);
	}

	/// Asked once per type: a container creates its children
	bool is_drawable(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &object)
	{
		const auto type = object->get_object_type();
		if (0 == drawableTypes.count(type))
		{
			drawableTypes[type] = (nullptr != JuceManagedWorkingSetCache::create_component(workingSet, object));
		}
		return drawableTypes[type];
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

	/// A data or alarm mask next to its soft keys
	void render_screen(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet,
	                   const std::shared_ptr<isobus::VTObject> &mask)
	{
		const auto fileName = "screen_" + String(mask->get_id()) + ".png";
		auto maskComponent = JuceManagedWorkingSetCache::create_component(workingSet, mask);
		if (nullptr == maskComponent)
		{
			add_error(fileName, mask, "the VT created no component for the mask");
			return;
		}

		const auto softKeyMaskId = get_soft_key_mask_id(mask);
		auto softKeyMask = find_object(workingSet, softKeyMaskId);
		std::shared_ptr<Component> softKeyMaskComponent;
		int pageCount = 0;
		Colour softKeyBackground;
		if ((nullptr != softKeyMask) && (isobus::VirtualTerminalObjectType::SoftKeyMask == softKeyMask->get_object_type()))
		{
			softKeyMaskComponent = JuceManagedWorkingSetCache::create_component(workingSet, softKeyMask);
			pageCount = SoftKeyMaskComponent::get_page_count(softKeyMask, workingSet, dimensions);
			softKeyBackground = get_background(workingSet, softKeyMask);
		}

		const auto image = paint_image(dimensions.height + dimensions.total_width(), dimensions.height, [&](Graphics &g) {
			paint_screen(g, *maskComponent, softKeyMaskComponent.get(), softKeyBackground, pageCount);
		});

		auto record = add_image(fileName, mask, image);
		if (nullptr != record)
		{
			record->setProperty("soft_key_mask_id", (nullptr != softKeyMaskComponent) ? var(static_cast<int>(softKeyMaskId)) : var());
		}
	}

	static std::uint16_t get_soft_key_mask_id(const std::shared_ptr<isobus::VTObject> &mask)
	{
		if (isobus::VirtualTerminalObjectType::AlarmMask == mask->get_object_type())
		{
			return std::static_pointer_cast<isobus::AlarmMask>(mask)->get_soft_key_mask();
		}
		return std::static_pointer_cast<isobus::DataMask>(mask)->get_soft_key_mask();
	}

	/// Paints a mask clipped to its bounds
	static void paint_clipped(Graphics &g, Component &component)
	{
		Graphics::ScopedSaveState state(g);
		g.reduceClipRegion(component.getBounds());
		component.paintEntireComponent(g, true);
	}

	void paint_screen(Graphics &g, Component &mask, Component *softKeyMask, Colour softKeyBackground, int pageCount)
	{
		// The mask covers its render area
		g.fillAll(background);
		paint_clipped(g, mask);

		const juce::Rectangle<int> softKeyArea(0, 0, dimensions.total_width(), dimensions.height);
		Graphics::ScopedSaveState state(g);
		g.setOrigin(dimensions.height, 0);
		g.reduceClipRegion(softKeyArea);
		SoftKeyMaskRenderAreaComponent::paint_area(g, softKeyArea, background, true);
		if (nullptr != softKeyMask)
		{
			paint_clipped(g, *softKeyMask);
			paint_page_bar(g, softKeyBackground, 0, pageCount);
		}
	}

	/// The designator as the selector shows it
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

		const auto button = WorkingSetSelectorComponent::button_bounds(0);
		Component column;
		column.setSize(WorkingSetSelectorComponent::WIDTH, button.getBottom() + button.getY());
		WorkingSetSelectorComponent::place_designator(*designator, button);
		column.addAndMakeVisible(*designator);

		const auto image = paint_image(column.getWidth(), column.getHeight(), [&](Graphics &g) {
			paint_designator(g, column, button, workingSet, active);
		});
		column.removeAllChildren();

		auto record = add_image(fileName, workingSetObject, image);
		if (nullptr != record)
		{
			record->setProperty("active", active);
		}
	}

	void paint_designator(Graphics &g,
	                      Component &column,
	                      juce::Rectangle<int> button,
	                      const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet,
	                      bool active)
	{
		g.fillAll(background);
		WorkingSetSelectorComponent::paint_button_background(g, button, workingSet);
		column.paintEntireComponent(g, true);
		if (active)
		{
			WorkingSetComponent::paint_active_highlight(g, button);
		}
	}

	/// An auxiliary function or input, drawn as a key
	void render_auxiliary_designator(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet,
	                                 const std::shared_ptr<isobus::VTObject> &auxiliaryObject)
	{
		const auto fileName = "aux_" + String(auxiliaryObject->get_id()) + ".png";

		isobus::Key key;
		key.set_id(auxiliaryObject->get_id());
		key.set_background_color(auxiliaryObject->get_background_color());
		for (std::uint16_t i = 0; i < auxiliaryObject->get_number_children(); i++)
		{
			key.add_child(auxiliaryObject->get_child_id(i), auxiliaryObject->get_child_x(i), auxiliaryObject->get_child_y(i));
		}

		KeyComponent component(workingSet, key, dimensions.keyWidth, dimensions.keyHeight);
		const auto image = paint_image(component.getWidth(), component.getHeight(), [&](Graphics &g) {
			component.paintEntireComponent(g, true);
		});

		auto record = add_image(fileName, auxiliaryObject, image);
		if (nullptr == record)
		{
			return;
		}

		if (isobus::VirtualTerminalObjectType::AuxiliaryFunctionType2 == auxiliaryObject->get_object_type())
		{
			using Function = isobus::AuxiliaryFunctionType2;
			const auto function = std::static_pointer_cast<Function>(auxiliaryObject);
			record->setProperty("function_type", get_auxiliary_function_type_label(function->get_function_type()));
			record->setProperty("critical_control", function->get_function_attribute(Function::CriticalControl));
			record->setProperty("assignment_restriction", function->get_function_attribute(Function::AssignmentRestriction));
			record->setProperty("single_assignment", function->get_function_attribute(Function::SingleAssignment));
		}
		else
		{
			using Input = isobus::AuxiliaryInputType2;
			const auto input = std::static_pointer_cast<Input>(auxiliaryObject);
			record->setProperty("function_type", get_auxiliary_function_type_label(input->get_function_type()));
			record->setProperty("critical_control", input->get_function_attribute(Input::CriticalControl));
			record->setProperty("single_assignment", input->get_function_attribute(Input::SingleAssignment));
		}
	}

	/// Each page of a soft key mask
	void render_soft_key_mask(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet,
	                          const std::shared_ptr<isobus::VTObject> &softKeyMask)
	{
		const int pageCount = std::max(1, SoftKeyMaskComponent::get_page_count(softKeyMask, workingSet, dimensions));
		const auto background = get_background(workingSet, softKeyMask);

		for (int page = 0; page < pageCount; page++)
		{
			const String fileName = "skm_" + String(softKeyMask->get_id()) + ((0 == page) ? String() : "_page" + String(page + 1)) + ".png";
			SoftKeyMaskComponent component(workingSet, *std::static_pointer_cast<isobus::SoftKeyMask>(softKeyMask), dimensions, static_cast<std::uint16_t>(page * dimensions.key_count()));
			const auto image = paint_image(component.getWidth(), component.getHeight(), [&](Graphics &g) {
				paint_soft_key_page(g, component, background, page, pageCount);
			});

			auto record = add_image(fileName, softKeyMask, image);
			if (nullptr != record)
			{
				record->setProperty("page", page + 1);
				record->setProperty("page_count", pageCount);
			}
		}
	}

	void paint_soft_key_page(Graphics &g, Component &softKeyMask, Colour background, int page, int pageCount)
	{
		softKeyMask.paintEntireComponent(g, true);
		paint_page_bar(g, background, page, pageCount);
	}

	/// The VT's page bar, if there is more than one page
	void paint_page_bar(Graphics &g, Colour background, int page, int pageCount)
	{
		if (pageCount < 2)
		{
			return;
		}

		SoftKeyPageBar bar;
		bar.setBounds(SoftKeyPageBar::get_area(dimensions));
		bar.set_page(page, pageCount);
		bar.set_mask_background(background);

		Graphics::ScopedSaveState state(g);
		g.setOrigin(bar.getPosition());
		bar.paintEntireComponent(g, true);
	}

	static Colour get_background(const std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &workingSet, const std::shared_ptr<isobus::VTObject> &object)
	{
		const auto vtColour = workingSet->get_colour(object->get_background_color());
		return Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f);
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
		result->setProperty("data_mask_size", dimensions.height);
		result->setProperty("softkey_count", dimensions.key_count());
		result->setProperty("softkey_width", dimensions.keyWidth);
		result->setProperty("softkey_height", dimensions.keyHeight);
		result->setProperty("softkey_rows", dimensions.rowCount);
		result->setProperty("softkey_columns", dimensions.columnCount);
		result->setProperty("softkey_padding", static_cast<int>(SoftKeyMaskDimensions::PADDING));
		result->setProperty("working_set_button_size", WorkingSetSelectorComponent::BUTTON_WIDTH);

		result->setProperty("from_meta", Array<var>());
		result->setProperty("fonts", Array<var>());
		result->setProperty("font_fallback", true);
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
		manifest->setProperty("pool_name", String());
		manifest->setProperty("manufacturer", String());
		manifest->setProperty("public", false);

		manifest->setProperty("known_issues", Array<var>());
		manifest->setProperty("pool_sha256", input.sha256.isEmpty() ? var() : var(input.sha256));
		manifest->setProperty("pool_size", static_cast<int64>(input.data.getSize()));
		manifest->setProperty("vt_commit", String(AgISOVirtualTerminalApplication::getApplicationBuildInfo()));
		manifest->setProperty("render_settings", describe_settings());
		manifest->setProperty("load", load.get());
		manifest->setProperty("images", images);
		manifest->setProperty("unsupported_objects", skippedObjects);
		manifest->setProperty("errors", errors);

		const auto text = JSON::toString(var(manifest.get()), false).replace("\r\n", "\n") + "\n";
		return directory.getChildFile("manifest.json").replaceWithText(text, false, false, "\n");
	}

	const Colour background;
	ErrorCollector &collector;
	const SoftKeyMaskDimensions dimensions; // the VT's defaults
	File directory;
	Array<var> images;
	Array<var> skippedObjects;
	Array<var> errors;
	std::map<isobus::VirtualTerminalObjectType, bool> drawableTypes;
};

int main(int argc, char *argv[])
{
	ScopedJuceInitialiser_GUI juceInitialiser;

	std::map<String, String> arguments;
	for (int i = 1; i < argc; i++)
	{
		const auto name = String::fromUTF8(argv[i]);
		if (!StringArray({ "--pool", "--out", "--all" }).contains(name) || (i + 1 >= argc) || (arguments.count(name) > 0))
		{
			PoolRenderer::print_usage();
			return EXIT_USAGE;
		}
		arguments[name] = String::fromUTF8(argv[++i]);
	}

	const bool singlePool = (arguments.count("--pool") > 0) && (0 == arguments.count("--all"));
	const bool collection = (arguments.count("--all") > 0) && (0 == arguments.count("--pool"));
	if ((0 == arguments.count("--out")) || !(singlePool || collection))
	{
		PoolRenderer::print_usage();
		return EXIT_USAGE;
	}

	const auto resolve = [](const String &path) { return File::getCurrentWorkingDirectory().getChildFile(path); };
	const auto outputDirectory = resolve(arguments["--out"]);

	if (collection && !resolve(arguments["--all"]).getChildFile("pools").isDirectory())
	{
		std::cout << "No pools folder in " << resolve(arguments["--all"]).getFullPathName() << std::endl;
		return EXIT_USAGE;
	}

	PictureGraphicComponent::set_use_software_images(true);

	ErrorCollector errorCollector;
	isobus::CANStackLogger::set_can_stack_logger_sink(&errorCollector);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Error);

	ExitCode result;
	{
		PoolRenderer renderer(errorCollector);
		if (singlePool)
		{
			result = renderer.render(PoolRenderer::read_pool_input(resolve(arguments["--pool"])), outputDirectory);
		}
		else
		{
			result = renderer.render_collection(resolve(arguments["--all"]), outputDirectory);
		}
	}

	isobus::CANStackLogger::set_can_stack_logger_sink(nullptr);
	return result;
}