//+---------------------------------------------------------------------------
//
//  Copyright (c) Microsoft, 2008. All rights reserved.
//
//  Contents:   Window layout functions.
//
//  Author:     Dwayne Robinson (dwayner@microsoft.com)
//
//  History:    2008-02-11   dwayner    Created
//
//----------------------------------------------------------------------------
#include "precomp.h"
#include "WindowUtility.h"


WindowPosition::WindowPosition()
:   hwnd(),
    options(PositionOptionsDefault),
    rect()
{
}


WindowPosition::WindowPosition(HWND newHwnd, PositionOptions newOptions)
:   hwnd(newHwnd),
    options(newOptions)
{
    POINT pt = {};
    ScreenToClient(newHwnd, &pt);
    GetWindowRect(newHwnd, &rect);
    OffsetRect(&rect, pt.x, pt.y);
}


WindowPosition::WindowPosition(HWND newHwnd, PositionOptions newOptions, const RECT& newRect)
:   hwnd(newHwnd),
    options(newOptions),
    rect(newRect)
{
}


long WindowPosition::GetWidth()
{
    return rect.right - rect.left;
}


long WindowPosition::GetHeight()
{
    return rect.bottom - rect.top;
}



void WindowPosition::Update(HDWP hdwp)
{
    if (hwnd == nullptr)
        return;

    int width  = GetWidth();
    int height = GetHeight();

    if (width < 0)  width  = 0;
    if (height < 0) height = 0;

    if (hdwp != nullptr)
    {
        DeferWindowPos(
            hdwp,
            hwnd,
            nullptr,
            rect.left,
            rect.top,
            width,
            height,
            SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER
            );
    }
    else
    {
        SetWindowPos(
            hwnd,
            nullptr,
            rect.left,
            rect.top,
            width,
            height,
            SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER
            );
    }
}


void WindowPosition::Update(
    __inout_ecount(windowPositionsCount) WindowPosition* windowPositions,
    __inout uint32_t windowPositionsCount
    )
{
    HDWP hdwp = BeginDeferWindowPos(windowPositionsCount);
    for (uint32_t i = 0; i < windowPositionsCount; ++i)
    {
        windowPositions[i].Update(hdwp);
    }
    EndDeferWindowPos(hdwp);
}


RECT WindowPosition::GetBounds(
    __inout_ecount(windowPositionsCount) WindowPosition* windowPositions,
    uint32_t windowPositionsCount
    )
{
    RECT bounds;
    bounds.left   = INT_MAX;
    bounds.right  = -INT_MAX;
    bounds.top    = INT_MAX;
    bounds.bottom = -INT_MAX;

    for (uint32_t i = 0; i < windowPositionsCount; ++i)
    {
        UnionRect(OUT &bounds, &bounds, &windowPositions[i].rect);
    }

    return bounds;
}


void WindowPosition::Offset(
    __inout_ecount(windowPositionsCount) WindowPosition* windowPositions,
    uint32_t windowPositionsCount,
    long dx,
    long dy
    )
{
    for (uint32_t i = 0; i < windowPositionsCount; ++i)
    {
        OffsetRect(IN OUT &windowPositions[i].rect, dx, dy);
    }
}


// Align the window to the given rectangle, using the window
// rectangle and positioning flags.
void WindowPosition::AlignToRect(
    RECT rect
    )
{
    const PositionOptions horizontalAlignment = this->options & PositionOptionsAlignHMask;
    const PositionOptions verticalAlignment   = this->options & PositionOptionsAlignVMask;

    long windowWidth  = this->rect.right - this->rect.left;
    long windowHeight = this->rect.bottom - this->rect.top;
    long rectWidth    = rect.right - rect.left;
    long rectHeight   = rect.bottom - rect.top;

    long x = 0;
    switch (horizontalAlignment)
    {
    case PositionOptionsAlignHStretch:
        windowWidth = rectWidth;
        // fall through
    case PositionOptionsAlignLeft:
    default:
        x = 0;
        break;
    case PositionOptionsAlignRight:
        x = rectWidth - windowWidth;
        break;
    case PositionOptionsAlignHCenter:
        x = (rectWidth - windowWidth) / 2;
        break;
    }

    long y = 0;
    switch (verticalAlignment)
    {
    case PositionOptionsAlignVStretch:
        windowHeight = rectHeight;
        // fall through
    case PositionOptionsAlignTop:
    default:
        y = 0;
        break;
    case PositionOptionsAlignBottom:
        y = rectHeight - windowHeight;
        break;
    case PositionOptionsAlignVCenter:
        y = (rectHeight - windowHeight) / 2;
        break;
    }

    this->rect.left   = x + rect.left;
    this->rect.top    = y + rect.top;
    this->rect.right  = this->rect.left + windowWidth;
    this->rect.bottom = this->rect.top  + windowHeight;
}


struct GridDimension
{
    PositionOptions options;    // combined options for this grid cell (they come from the respective windows)
    long offset;                // x in horizontal, y in vertical
    long size;                  // width in horizontal, height in vertical
};


void WindowPosition::DistributeGridDimensions(
    __inout_ecount(gridDimensionsCount) GridDimension* gridDimensions,
    uint32_t gridDimensionsCount,
    long clientSize,
    long spacing,
    PositionOptions optionsMask // can be PositionOptionsUseSlackHeight or PositionOptionsUseSlackWidth
    )
{
    if (gridDimensionsCount <= 0)
    {
        return; // nothing to do
    }

    // Determine the maximum size along a single dimension,
    // and assign all offsets.

    long accumulatedSize = 0;
    uint32_t gridFillCount = 0;

    for (uint32_t i = 0; i < gridDimensionsCount; ++i)
    {
        if (gridDimensions[i].options & PositionOptionsIgnored)
            continue;

        // Add spacing between all controls, but not before the first control,
        // not after zero width controls.
        if (i > 0)
            accumulatedSize += spacing;
        gridDimensions[i].offset = accumulatedSize;
        accumulatedSize += gridDimensions[i].size;

        if (gridDimensions[i].options & optionsMask)
            ++gridFillCount; // found another grid cell to fill
    }

    // Check if we still have leftover space inside the parent's client rect.

    const long slackSizeToRedistribute = clientSize - accumulatedSize;
    if (gridFillCount <= 0 || slackSizeToRedistribute <= 0)
    {
        return; // nothing more to do
    }

    // Distribute any slack space remaining throughout windows that want it.

    uint32_t gridFillIndex = 0;
    long offsetAdjustment = 0;

    for (uint32_t i = 0; i < gridDimensionsCount; ++i)
    {
        if (gridDimensions[i].options & PositionOptionsIgnored)
            continue;

        // Shift the offset over from any earlier slack space contribution.
        gridDimensions[i].offset += offsetAdjustment;

        // Contribute the slack space into this row/col.
        if (gridDimensions[i].options & optionsMask)
        {
            long sizeForGridCell = (slackSizeToRedistribute * (gridFillIndex + 1) / gridFillCount)
                                 - (slackSizeToRedistribute *  gridFillIndex      / gridFillCount);

            // Increase the size of subsequent offsets.
            gridDimensions[i].size += sizeForGridCell;
            offsetAdjustment += sizeForGridCell;
            ++gridFillIndex;
        }
    }
}

RECT WindowPosition::ReflowGrid(
    __inout_ecount(windowPositionsCount) WindowPosition* windowPositions,
    uint32_t windowPositionsCount,
    const RECT& clientRect,
    long spacing,
    uint32_t maxCols,
    PositionOptions parentOptions,
    bool queryOnly
    )
{
    // There can be up to n rows and n columns, so allocate up to 2n.
    // Rather than allocate two separate arrays, use the first half
    // for columns and second half for rows. Note the interpretation
    // of rows and columns flips if the layout is vertical.
    std::vector<GridDimension> gridDimensions(windowPositionsCount * 2);

    bool isVertical = (parentOptions & PositionOptionsFlowVertical)  != 0;
    bool isRtl      = (parentOptions & PositionOptionsReverseFlowDu) != 0;
    bool isWrapped  = (parentOptions & PositionOptionsUnwrapped)     == 0;

    if (maxCols < windowPositionsCount)
    {
        // Set the flag to make logic easier in the later loop.
        gridDimensions[maxCols].options |= PositionOptionsNewLine;
    }
    else
    {
        maxCols = windowPositionsCount;
    }

    ////////////////////////////////////////
    // Arrange all the children, flowing left-to-right for horizontal,
    // top-to-bottom for vertical (flipping at the end if RTL). Note
    // that all the coordinates are rotated 90 degrees when vertical
    // (so x is really y).

    long colOffset = 0;    // current column offset
    long rowOffset = 0;    // current column offset
    long totalColSize = 0; // total accumulated column size
    long totalRowSize = 0; // total accumulated row size
    long maxColSize = clientRect.right - clientRect.left;
    long maxRowSize = clientRect.bottom - clientRect.top;
    if (isVertical)
        std::swap(maxColSize, maxRowSize);

    uint32_t colCount = 0; // number of columns (reset per row)
    uint32_t rowCount = 0; // number of rows
    uint32_t gridStartingColIndex = 0; // starting column index on current row
    uint32_t gridColIndex = 0; // array index, not column offset
    uint32_t gridRowIndex = windowPositionsCount; // array index, not row offset

    for (size_t windowIndex = 0; windowIndex < windowPositionsCount; ++windowIndex)
    {
        const WindowPosition& window = windowPositions[windowIndex];
        PositionOptions options = window.options;

        // Skip any windows that invisible, floating, or resized by the caller.
        if (options & PositionOptionsIgnored)
        {
            continue;
        }

        // Expand the grid if the window is larger than any previous.
        long colSize = window.rect.right - window.rect.left;
        long rowSize = window.rect.bottom - window.rect.top;

        // If the row or column should be stretched, zero the size so that it
        // does not prematurely increase the size. Otherwise resizing the same
        // control repeatedly would end up only ever growing the control.
        if ((options & PositionOptionsAlignHMask) == PositionOptionsAlignHStretch)
        {
            colSize = 0;
        }
        if ((options & PositionOptionsAlignVMask) == PositionOptionsAlignVStretch)
        {
            rowSize = 0;
        }

        if (isVertical)
            std::swap(colSize, rowSize);

        // Flow down to a new line if wider than maximum column count or column size,
        // or if windows hints says it should be on a new line.
        if (colCount > 0)
        {
            if ((options & PositionOptionsNewLine)
            ||  (maxCols > 0 && colCount >= maxCols)
            ||  (isWrapped && colOffset + colSize > maxColSize))
            {
                // Distribute the columns. For a regular grid, we'll distribute
                // later once we've looked at all the rows. For irregular flow,
                // distribute now while we still know the range of windows on
                // the current row.
                if (maxCols == 0)
                {
                    DistributeGridDimensions(
                        &gridDimensions[gridStartingColIndex],
                        colCount,
                        maxColSize,
                        spacing,
                        isVertical ? PositionOptionsUseSlackHeight : PositionOptionsUseSlackWidth
                    );
                    gridStartingColIndex += colCount;
                }
                else
                {
                    gridColIndex = 0;
                }

                // Reset and advance.
                ++rowCount;
                ++gridRowIndex;
                colCount = 0;
                colOffset = 0;
                rowOffset = totalRowSize + spacing;

                // Set this flag to make later logic easier so that it doesn't
                // need to recheck a number of wrapping conditions.
                options |= PositionOptionsNewLine;
            }
        }

        // Combine the positioning options of the window into the grid column/row.
        gridDimensions[gridColIndex].options |= options;
        gridDimensions[gridRowIndex].options |= options;

        // If this window is larger than any previous, increase the grid row/column size,
        // and add to the accumulated total rect.
        if (colSize > gridDimensions[gridColIndex].size)
        {
            gridDimensions[gridColIndex].size = std::min(colSize, maxColSize);
        }
        if (rowSize > gridDimensions[gridRowIndex].size)
        {
            gridDimensions[gridRowIndex].size = std::min(rowSize, maxRowSize);
        }

        totalColSize = std::max(colOffset + colSize, totalColSize);
        totalRowSize = std::max(rowOffset + rowSize, totalRowSize);

        // Next column...
        colOffset = totalColSize + spacing;
        ++colCount;
        ++gridColIndex;
    }
    if (colCount > 0)
    {
        ++rowCount; // count the last row
    }

    ////////////////////////////////////////
    // Return the bounding rect if wanted.

    RECT resultRect = {0,0,totalColSize,totalRowSize};
    if (isVertical)
        std::swap(resultRect.right, resultRect.bottom);

    if (queryOnly)
        return resultRect;

    ////////////////////////////////////////
    // Distribute the any remaining slack space into rows and columns.

    DistributeGridDimensions(
        &gridDimensions.data()[gridStartingColIndex],
        (maxCols > 0) ? maxCols : colCount,
        maxColSize,
        spacing,
        isVertical ? PositionOptionsUseSlackHeight : PositionOptionsUseSlackWidth
        );

    gridRowIndex = windowPositionsCount; // array index, not row offset
    DistributeGridDimensions(
        &gridDimensions.data()[gridRowIndex],
        rowCount,
        maxRowSize,
        spacing,
        !isVertical ? PositionOptionsUseSlackHeight : PositionOptionsUseSlackWidth
        );

    ////////////////////////////////////////
    // Update window rects with the new positions from the grid.

    // Reset for second loop.
    colCount = 0;
    gridColIndex = 0;
    gridRowIndex = windowPositionsCount;

    for (size_t windowIndex = 0; windowIndex < windowPositionsCount; ++windowIndex)
    {
        WindowPosition& window = windowPositions[windowIndex];

        // Skip any invisible/floating windows.
        if (window.options & PositionOptionsIgnored)
        {
            continue;
        }

        const bool isNewLine = !!(gridDimensions[gridColIndex].options & PositionOptionsNewLine);
        if (isNewLine && colCount > 0)
        {
            ++gridRowIndex;
            if (maxCols > 0)
            {
                gridColIndex = 0;
            }
        }

        RECT gridRect = {
            clientRect.left + gridDimensions[gridColIndex].offset,
            clientRect.top  + gridDimensions[gridRowIndex].offset,
            gridRect.left   + gridDimensions[gridColIndex].size,
            gridRect.top    + gridDimensions[gridRowIndex].size,
        };
        if (isVertical)
        {
            std::swap(gridRect.left,  gridRect.top);
            std::swap(gridRect.right, gridRect.bottom);
        }

        window.AlignToRect(gridRect);

        ++colCount;
        ++gridColIndex;
    }

    if (isRtl)
    {
        for (size_t windowIndex = 0; windowIndex < windowPositionsCount; ++windowIndex)
        {
            WindowPosition& window = windowPositions[windowIndex];
            long adjustment = clientRect.right - window.rect.right + clientRect.left - window.rect.left;
            window.rect.left  += adjustment;
            window.rect.right += adjustment;
        }
    }

    return resultRect;
}


// Seriously, why does GetSubMenu not support either by-position or by-id like GetMenuItemInfo?
HMENU WINAPI GetSubMenu(
    __in HMENU parentMenu,
    __in UINT item,
    __in BOOL byPosition
    )
{
    MENUITEMINFO menuItemInfo;
    menuItemInfo.cbSize = sizeof(menuItemInfo);
    menuItemInfo.fMask = MIIM_ID | MIIM_SUBMENU;
    if (!GetMenuItemInfo(parentMenu, item, byPosition, OUT &menuItemInfo))
        return nullptr;

    return menuItemInfo.hSubMenu;
}


bool CopyTextToClipboard(
    HWND hwnd,
    _In_reads_(textLength) wchar_t const* text,
    size_t textLength
    )
{
    bool succeeded = false;

    if (OpenClipboard(hwnd))
    {
        if (EmptyClipboard())
        {
            const size_t bufferLength = textLength * sizeof(text[0]);
            HGLOBAL bufferHandle = GlobalAlloc(GMEM_MOVEABLE, bufferLength);
            if (bufferHandle != nullptr)
            {
                wchar_t* buffer = reinterpret_cast<wchar_t*>(GlobalLock(bufferHandle));

                memcpy(buffer, text, bufferLength);

                GlobalUnlock(bufferHandle);

                if (SetClipboardData(CF_UNICODETEXT, bufferHandle) != nullptr)
                {
                    succeeded = true;
                }
                else
                {
                    GlobalFree(bufferHandle);
                }
            }
        }
        CloseClipboard();
    }

    return succeeded;
}


void CopyListTextToClipboard(
    HWND listViewHwnd,
    _In_z_ wchar_t const* separator
    )
{
    std::wstring text = GetListViewText(listViewHwnd, separator);
    CopyTextToClipboard(listViewHwnd, text.c_str(), text.length() + 1);
}


std::wstring GetListViewText(
    HWND listViewHwnd,
    __in_z const wchar_t* separator
    )
{
    std::wstring text;
    wchar_t itemTextBuffer[256];
    wchar_t groupTextBuffer[256];

    LVITEMINDEX listViewIndex = {-1, -1};

    // Setup the item to retrieve text.
    LVITEM listViewItem;
    listViewItem.mask       = LVIF_TEXT | LVIF_GROUPID;
    listViewItem.iSubItem   = 0;

    // Setup the group to retrieve text.
    LVGROUP listViewGroup;
    listViewGroup.cbSize    = sizeof(LVGROUP);
    listViewGroup.mask      = LVGF_HEADER | LVGF_GROUPID;
    listViewGroup.iGroupId  = 0;
    int previousGroupId     = -1;

    const uint32_t columnCount = Header_GetItemCount(ListView_GetHeader(listViewHwnd));
    std::vector<int32_t> columnOrdering(columnCount);
    ListView_GetColumnOrderArray(listViewHwnd, columnCount, OUT columnOrdering.data());

    // Find the first selected row. If nothing is selected, then we'll copy the entire text.
    uint32_t searchMask = LVNI_SELECTED;
    ListView_GetNextItemIndex(listViewHwnd, &listViewIndex, searchMask);
    if (listViewIndex.iItem < 0)
    {
        searchMask = LVNI_ALL;
        ListView_GetNextItemIndex(listViewHwnd, &listViewIndex, searchMask);
    }

    // Build up a concatenated string.
    // Append next item's column to the text. (either the next selected or all)
    while (listViewIndex.iItem >= 0)
    {
        for (uint32_t column = 0; column < columnCount; ++column)
        {
            int32_t columnIndex = columnOrdering[column];

            if (columnCount > 1 && ListView_GetColumnWidth(listViewHwnd, columnIndex) <= 0)
                continue; // Skip zero-width columns.

            // Set up fields for call. We must reset the string pointer
            // and count for virtual listviews because while it copies the
            // text out to our buffer, it also messes with the pointer.
            // Otherwise the next call will crash.
            itemTextBuffer[0]       = '\0';
            listViewItem.iItem      = listViewIndex.iItem;
            listViewItem.iSubItem   = columnIndex;
            listViewItem.pszText    = (LPWSTR)itemTextBuffer;
            listViewItem.cchTextMax = ARRAYSIZE(itemTextBuffer);
            ListView_GetItem(listViewHwnd, &listViewItem);

            // Append the group name if we changed to a different group.
            if (listViewItem.iGroupId != previousGroupId)
            {
                // Add an extra blank line between the previous group.
                if (previousGroupId >= 0)
                {
                    text += L"\r\n";
                }
                previousGroupId = listViewItem.iGroupId;

                // Add group header (assuming it belongs to a group).
                if (listViewItem.iGroupId >= 0)
                {
                    groupTextBuffer[0]      = '\0';
                    listViewGroup.pszHeader = (LPWSTR)groupTextBuffer;
                    listViewGroup.cchHeader = ARRAYSIZE(groupTextBuffer);
                    ListView_GetGroupInfo(listViewHwnd, listViewItem.iGroupId, &listViewGroup);

                    text += L"― ";
                    text += groupTextBuffer;
                    text += L" ―\r\n";
                }
            }

            text += itemTextBuffer;

            // Append a new line if last column, or the separator if between columns.
            if (column + 1 >= columnCount)
            {
                text += L"\r\n";
            }
            else
            {
                text += separator; // may be a tab, or other separator such as comma or field/value pair ": "
            }
        } // for column
        
        if (!ListView_GetNextItemIndex(listViewHwnd, &listViewIndex, searchMask))
            break;
    } // while iItem

    text.shrink_to_fit();

    return text;
}
