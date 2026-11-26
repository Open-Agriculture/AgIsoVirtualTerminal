/*******************************************************************************
** @file       TouchInputListMenu.cpp
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "TouchInputListMenu.hpp"

#include <algorithm>

namespace
{
	constexpr int DRAG_THRESHOLD = 8; ///< Pixels a finger may move and still count as a tap
	constexpr int ARROW_HEIGHT = 64; ///< Height of each scroll arrow
	constexpr int MINIMUM_THUMB_SIZE = 64; ///< The scroll bar thumb stays large enough to drag with a finger
	constexpr int MINIMUM_POPUP_WIDTH = 320; ///< Narrow fields still get rows wide enough to read
	constexpr int MINIMUM_POPUP_HEIGHT = 168; ///< Three rows, when there is little space next to the field
	constexpr int SCREEN_MARGIN = 16; ///< Space kept between the drop down and the edge of the screen

	const Colour KEY_COLOUR(0xffeee7b7);
	const Colour KEY_HOVER_COLOUR(0xfff5efcc);
	const Colour KEY_DOWN_COLOUR(0xffd5cc91);
	const Colour TRACK_COLOUR(0xffaaa581);
	const Colour HIGHLIGHT_COLOUR(0xff8278ff);
} // namespace

TouchInputListItem::TouchInputListItem(std::shared_ptr<Component> itemContent, int width, int height, float contentScale) :
  PopupMenu::CustomComponent(false),
  content(std::move(itemContent)),
  rowWidth(width),
  rowHeight(std::max(MINIMUM_HEIGHT, height)),
  scale(contentScale)
{
	// The row handles taps itself, so the rendered item must not take them
	setInterceptsMouseClicks(true, false);
	content->setInterceptsMouseClicks(false, false);
	addAndMakeVisible(*content);
}

void TouchInputListItem::getIdealSize(int &idealWidth, int &idealHeight)
{
	idealWidth = rowWidth;
	idealHeight = rowHeight;
}

void TouchInputListItem::resized()
{
	// Drawn like the field, but never larger than the row
	const float fittedScale = std::min({ scale,
	                                     static_cast<float>(std::max(1, getWidth() - 8)) / static_cast<float>(std::max(1, content->getWidth())),
	                                     static_cast<float>(getHeight()) / static_cast<float>(std::max(1, content->getHeight())) });
	content->setTransform(AffineTransform::scale(fittedScale));
	content->setTopLeftPosition(4, std::max(0, roundToInt((getHeight() - content->getHeight() * fittedScale) * 0.5f)));
}

void TouchInputListItem::paint(Graphics &graphics)
{
	graphics.fillAll(Colours::white);
}

void TouchInputListItem::paintOverChildren(Graphics &graphics)
{
	if (isItemHighlighted())
	{
		graphics.fillAll(HIGHLIGHT_COLOUR.withAlpha(0.5f));
	}
	graphics.setColour(Colours::grey.withAlpha(0.35f));
	graphics.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
}

void TouchInputListItem::mouseDown(const MouseEvent &)
{
	dragged = false;
}

void TouchInputListItem::mouseDrag(const MouseEvent &event)
{
	dragged = dragged || (event.getDistanceFromDragStart() > DRAG_THRESHOLD);
}

void TouchInputListItem::mouseUp(const MouseEvent &event)
{
	// A swipe scrolls the list, so lifting the finger after one must not choose a row
	if ((!dragged) && (!event.mouseWasDraggedSinceMouseDown()) && (nullptr != onTap))
	{
		onTap();
	}
}

std::unique_ptr<AccessibilityHandler> TouchInputListItem::createAccessibilityHandler()
{
	return std::make_unique<AccessibilityHandler>(*this,
	                                              AccessibilityRole::listItem,
	                                              AccessibilityActions().addAction(AccessibilityActionType::press, [this]() {
		                                              if (nullptr != onTap)
		                                              {
			                                              onTap();
		                                              }
	                                              }));
}

/// @brief The whole drop down, shown as the only item of a popup menu
/// @details JUCE still positions the popup, dismisses it on an outside tap or Escape and ends it.
/// The panel replaces the small hover scroll zones a popup menu puts around its items.
class TouchInputListMenu::Panel : public PopupMenu::CustomComponent
  , private ScrollBar::Listener
{
public:
	Panel(std::vector<Entry> rows, int selectedItemID, int width, int height, std::shared_ptr<int> chosenItemID) :
	  PopupMenu::CustomComponent(false),
	  entries(std::move(rows)),
	  idealWidth(width),
	  idealHeight(height),
	  result(std::move(chosenItemID)),
	  upArrow(true),
	  downArrow(false)
	{
		setWantsKeyboardFocus(true);
		viewport.setViewedComponent(&rowContainer, false);
		viewport.setScrollBarsShown(false, false);
		viewport.setScrollOnDragMode(Viewport::ScrollOnDragMode::all);
		viewport.onMove = [this]() { update_scroll_controls(); };
		addAndMakeVisible(viewport);

		scrollBar.setLookAndFeel(&scrollBarLookAndFeel);
		scrollBar.setAutoHide(false);
		scrollBar.addListener(this);
		addAndMakeVisible(scrollBar);
		addAndMakeVisible(upArrow);
		addAndMakeVisible(downArrow);
		upArrow.onClick = [this]() { scroll_by(-rowHeight); };
		downArrow.onClick = [this]() { scroll_by(rowHeight); };

		for (std::size_t i = 0; i < entries.size(); i++)
		{
			int itemWidth = 0;
			int itemHeight = 0;
			entries[i].component->getIdealSize(itemWidth, itemHeight);
			rowHeight = std::max(rowHeight, itemHeight);
			entries[i].component->onTap = [this, i]() { choose(static_cast<int>(i)); };
			rowContainer.addAndMakeVisible(*entries[i].component);

			if (selectedItemID == entries[i].itemID)
			{
				selectedRow = static_cast<int>(i);
			}
		}

		if ((selectedRow < 0) && (!entries.empty()))
		{
			selectedRow = 0;
		}
		update_highlight();
		setSize(idealWidth, idealHeight);
		scroll_to_selected_row();
	}

	~Panel() override
	{
		viewport.onMove = nullptr;
		viewport.setViewedComponent(nullptr, false);
		scrollBar.removeListener(this);
		scrollBar.setLookAndFeel(nullptr);
	}

	void getIdealSize(int &width, int &height) override
	{
		width = idealWidth;
		height = idealHeight;
	}

	void resized() override
	{
		const int contentHeight = static_cast<int>(entries.size()) * rowHeight;
		const bool needsScrolling = (contentHeight > getHeight());
		const int scrollColumnWidth = needsScrolling ? SCROLL_COLUMN_WIDTH : 0;

		viewport.setBounds(0, 0, std::max(1, getWidth() - scrollColumnWidth), getHeight());
		rowContainer.setSize(viewport.getWidth(), contentHeight);

		for (std::size_t i = 0; i < entries.size(); i++)
		{
			entries[i].component->setBounds(0, static_cast<int>(i) * rowHeight, rowContainer.getWidth(), rowHeight);
		}

		upArrow.setVisible(needsScrolling);
		downArrow.setVisible(needsScrolling);
		scrollBar.setVisible(needsScrolling);
		upArrow.setBounds(getWidth() - scrollColumnWidth, 0, scrollColumnWidth, ARROW_HEIGHT);
		downArrow.setBounds(getWidth() - scrollColumnWidth, getHeight() - ARROW_HEIGHT, scrollColumnWidth, ARROW_HEIGHT);
		scrollBar.setBounds(getWidth() - scrollColumnWidth, ARROW_HEIGHT, scrollColumnWidth, std::max(1, getHeight() - (2 * ARROW_HEIGHT)));
		update_scroll_controls();
	}

	void paint(Graphics &graphics) override
	{
		graphics.fillAll(Colours::white);
	}

	bool keyPressed(const KeyPress &key) override
	{
		bool retVal = true;

		if (key == KeyPress::escapeKey)
		{
			PopupMenu::dismissAllActiveMenus();
		}
		else if (key == KeyPress::returnKey)
		{
			choose(selectedRow);
		}
		else if ((key == KeyPress::upKey) || (key == KeyPress::downKey) || (key == KeyPress::homeKey) || (key == KeyPress::endKey))
		{
			const int lastRow = static_cast<int>(entries.size()) - 1;

			if (key == KeyPress::homeKey)
			{
				selectedRow = 0;
			}
			else if (key == KeyPress::endKey)
			{
				selectedRow = lastRow;
			}
			else
			{
				selectedRow = jlimit(0, lastRow, selectedRow + ((key == KeyPress::upKey) ? -1 : 1));
			}
			update_highlight();
			scroll_to_selected_row();
		}
		else
		{
			retVal = false;
		}
		return retVal;
	}

private:
	/// @brief A large scroll arrow for a finger
	class ScrollArrow : public Button
	{
	public:
		explicit ScrollArrow(bool upwards) :
		  Button(upwards ? "Scroll up" : "Scroll down"),
		  pointsUp(upwards)
		{
			setWantsKeyboardFocus(false);
			setRepeatSpeed(400, 140, 80);
		}

		void paintButton(Graphics &graphics, bool isHighlighted, bool isDown) override
		{
			graphics.fillAll(isDown ? KEY_DOWN_COLOUR : (isHighlighted ? KEY_HOVER_COLOUR : KEY_COLOUR));
			graphics.setColour(Colours::grey);
			graphics.drawRect(getLocalBounds(), 1);
			graphics.setColour(Colours::black.withAlpha(isEnabled() ? 1.0f : 0.25f));

			const float centreX = getWidth() * 0.5f;
			const float centreY = getHeight() * 0.5f;
			const float baseY = centreY + (pointsUp ? 8.0f : -8.0f);
			const float tipY = centreY + (pointsUp ? -14.0f : 14.0f);
			Path arrow;
			arrow.addTriangle(centreX - 18.0f, baseY, centreX + 18.0f, baseY, centreX, tipY);
			graphics.fillPath(arrow);
		}

	private:
		bool pointsUp;
	};

	/// @brief Reports every scroll, including one made by dragging the rows
	class ListViewport : public Viewport
	{
	public:
		void visibleAreaChanged(const Rectangle<int> &) override
		{
			if (nullptr != onMove)
			{
				onMove();
			}
		}

		std::function<void()> onMove;
	};

	/// @brief Draws a wide scroll bar without its own small buttons
	class ScrollBarLookAndFeel : public LookAndFeel_V4
	{
	public:
		bool areScrollbarButtonsVisible() override
		{
			return false;
		}

		int getMinimumScrollbarThumbSize(ScrollBar &) override
		{
			return MINIMUM_THUMB_SIZE;
		}

		void drawScrollbar(Graphics &graphics, ScrollBar &, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool, bool) override
		{
			graphics.setColour(TRACK_COLOUR);
			graphics.fillRect(x, y, width, height);

			const auto thumb = isScrollbarVertical ? Rectangle<int>(x + 2, thumbStartPosition, width - 4, thumbSize) : Rectangle<int>(thumbStartPosition, y + 2, thumbSize, height - 4);
			graphics.setColour(KEY_COLOUR);
			graphics.fillRect(thumb);
			graphics.setColour(Colours::grey);
			graphics.drawRect(thumb, 1);
		}
	};

	void choose(int row)
	{
		// The end of a drag is not a choice
		if ((row >= 0) && (row < static_cast<int>(entries.size())) && (!viewport.isCurrentlyScrollingOnDrag()))
		{
			*result = entries[static_cast<std::size_t>(row)].itemID;
			triggerMenuItem();
		}
	}

	void update_highlight()
	{
		for (std::size_t i = 0; i < entries.size(); i++)
		{
			entries[i].component->setHighlighted(static_cast<int>(i) == selectedRow);
		}
	}

	void scroll_by(int pixels)
	{
		viewport.setViewPosition(0, viewport.getViewPositionY() + pixels);
	}

	void scroll_to_selected_row()
	{
		const int rowTop = selectedRow * rowHeight;
		const int rowBottom = rowTop + rowHeight;

		if (rowTop < viewport.getViewPositionY())
		{
			viewport.setViewPosition(0, rowTop);
		}
		else if (rowBottom > (viewport.getViewPositionY() + viewport.getHeight()))
		{
			viewport.setViewPosition(0, rowBottom - viewport.getHeight());
		}
	}

	void update_scroll_controls()
	{
		scrollBar.setRangeLimits(0, std::max(rowContainer.getHeight(), viewport.getHeight()), dontSendNotification);
		scrollBar.setCurrentRange(viewport.getViewPositionY(), viewport.getHeight(), dontSendNotification);
		upArrow.setEnabled(viewport.getViewPositionY() > 0);
		downArrow.setEnabled((viewport.getViewPositionY() + viewport.getHeight()) < rowContainer.getHeight());
	}

	void scrollBarMoved(ScrollBar *, double newRangeStart) override
	{
		viewport.setViewPosition(0, roundToInt(newRangeStart));
	}

	std::vector<Entry> entries;
	int idealWidth;
	int idealHeight;
	int selectedRow = -1;
	int rowHeight = TouchInputListItem::MINIMUM_HEIGHT;
	std::shared_ptr<int> result; ///< Where the chosen row's ID is written, read by the menu's callback
	ScrollBarLookAndFeel scrollBarLookAndFeel; ///< Declared before the scroll bar, so it outlives it
	Component rowContainer;
	ListViewport viewport;
	ScrollBar scrollBar{ true };
	ScrollArrow upArrow;
	ScrollArrow downArrow;
};

void TouchInputListMenu::add_item(int itemID, std::unique_ptr<TouchInputListItem> row, const String &title)
{
	row->setTitle(title);
	entries.push_back({ itemID, std::move(row) });
}

int TouchInputListMenu::get_number_of_items() const
{
	return static_cast<int>(entries.size());
}

void TouchInputListMenu::show(Rectangle<int> fieldScreenBounds, Component *owner, int selectedItemID, std::function<void(int)> callback)
{
	if (entries.empty())
	{
		return;
	}

	const auto *display = Desktop::getInstance().getDisplays().getDisplayForRect(fieldScreenBounds);
	const auto screenArea = (nullptr != display) ? display->userArea : Rectangle<int>(0, 0, 1024, 768);

	// The popup opens above or below the field, whichever has more room
	const int spaceAbove = fieldScreenBounds.getY() - screenArea.getY();
	const int spaceBelow = screenArea.getBottom() - fieldScreenBounds.getBottom();
	const int availableHeight = std::max(MINIMUM_POPUP_HEIGHT, std::max(spaceAbove, spaceBelow) - SCREEN_MARGIN);

	int rowHeight = TouchInputListItem::MINIMUM_HEIGHT;
	for (auto &entry : entries)
	{
		int itemWidth = 0;
		int itemHeight = 0;
		entry.component->getIdealSize(itemWidth, itemHeight);
		rowHeight = std::max(rowHeight, itemHeight);
	}

	const int contentHeight = static_cast<int>(entries.size()) * rowHeight;
	const int height = std::min(contentHeight, std::min(availableHeight, screenArea.getHeight() - (2 * SCREEN_MARGIN)));
	const bool needsScrolling = (contentHeight > height);
	const int width = std::min(std::max(MINIMUM_POPUP_WIDTH, fieldScreenBounds.getWidth() + (needsScrolling ? SCROLL_COLUMN_WIDTH : 0)), screenArea.getWidth() - (2 * SCREEN_MARGIN));

	auto chosenItemID = std::make_shared<int>(0);
	auto panel = std::make_unique<Panel>(std::move(entries), selectedItemID, width, height, chosenItemID);
	entries.clear();
	const Component::SafePointer<Panel> safePanel(panel.get());

	PopupMenu popup;
	popup.addCustomItem(1, std::move(panel), nullptr, "Choose a list item");

	const auto options = PopupMenu::Options()
	                       .withTargetComponent(owner)
	                       .withTargetScreenArea(fieldScreenBounds)
	                       .withMinimumWidth(width)
	                       .withMaximumNumColumns(1)
	                       .withPreferredPopupDirection(PopupMenu::Options::PopupDirection::downwards)
	                       .withInitiallySelectedItem(1)
	                       .withItemThatMustBeVisible(1);

	popup.showMenuAsync(options, [chosenItemID, callback = std::move(callback)](int menuResult) {
		if (nullptr != callback)
		{
			callback((menuResult > 0) ? *chosenItemID : 0);
		}
	});

	// Keyboard navigation works once the panel has focus
	MessageManager::callAsync([safePanel]() {
		if (nullptr != safePanel)
		{
			safePanel->grabKeyboardFocus();
		}
	});
}
