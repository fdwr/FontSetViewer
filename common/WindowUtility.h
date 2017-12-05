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

#pragma once


enum PositionOptions
{
    PositionOptionsUseSlackWidth    = 1,        // use any remaining width of parent's client rect (use FillWidth unless you want specific alignment)
    PositionOptionsUseSlackHeight   = 2,        // use any remaining height of parent's client rect (use FillHeight unless you want specific alignment)
    PositionOptionsAlignHCenter     = 0,        // center horizontally within allocated space
    PositionOptionsAlignLeft        = 16,       // align left within allocated space
    PositionOptionsAlignRight       = 32,       // align right within allocated space
    PositionOptionsAlignHStretch    = 16|32,    // stretch left within allocated space
    PositionOptionsAlignHMask       = 16|32,    // mask for horizontal alignment
    PositionOptionsAlignVCenter     = 0,        // center vertically within allocated space
    PositionOptionsAlignTop         = 64,       // align top within allocated space
    PositionOptionsAlignBottom      = 128,      // align bottom within allocated space
    PositionOptionsAlignVStretch    = 64|128,   // stretch within allocated space
    PositionOptionsAlignVMask       = 64|128,   // mask for vertical alignment

    PositionOptionsNewLine          = 256,      // start this control on a new line

    // Control options
    PositionOptionsUnwrapped        = 512,      // do not wrap / allow controls to flow off the edge even if larger than containing parent width/height
    PositionOptionsReverseFlowDu    = 1024,     // reverse the primary direction (ltr->rtl in horz, ttb->btt in vert)
    PositionOptionsReverseFlowDv    = 2048,     // reverse the secondary direction (ttb->btt in horz, ltr->rtl in vert)
    PositionOptionsFlowHorizontal   = 0,        // flow primarily horizontal, then vertical
    PositionOptionsFlowVertical     = 4096,     // flow primarily vertical, then horizontal
    PositionOptionsIgnored          = 8192,     // window will not be adjusted (either hidden or floating or out-of-band)

    PositionOptionsDefault          = PositionOptionsAlignLeft|PositionOptionsAlignTop,
    PositionOptionsFillWidth        = PositionOptionsUseSlackWidth|PositionOptionsAlignHStretch,
    PositionOptionsFillHeight       = PositionOptionsUseSlackHeight|PositionOptionsAlignVStretch,
};
DEFINE_ENUM_FLAG_OPERATORS(PositionOptions);


struct WindowPosition
{
    HWND hwnd;
    PositionOptions options;
    RECT rect;


    WindowPosition();
    WindowPosition(HWND newHwnd, PositionOptions newOptions = PositionOptionsDefault);
    WindowPosition(HWND newHwnd, PositionOptions newOptions, const RECT& newRect);

    inline long GetWidth();
    inline long GetHeight();

    // Updates the real system window to match the rectangle in the structure.
    // If a deferred window position handle is passed, it will apply it to that,
    // otherwise the effect is immediate.
    void Update(HDWP hdwp = nullptr);

    static void Update(
        __inout_ecount(windowPositionsCount) WindowPosition* windowPositions,
        __inout uint32_t windowPositionsCount
        );

    // Align the window to the given rectangle, using the window
    // rectangle and positioning flags.
    void AlignToRect(
        RECT rect
        );

    static RECT GetBounds(
        __inout_ecount(windowPositionsCount) WindowPosition* windowPositions,
        uint32_t windowPositionsCount
        );

    static void Offset(
        __inout_ecount(windowPositionsCount) WindowPosition* windowPositions,
        uint32_t windowPositionsCount,
        long dx,
        long dy
        );

    static RECT ReflowGrid(
        __inout_ecount(windowPositionsCount) WindowPosition* windowPositions,
        uint32_t windowPositionsCount,
        const RECT& clientRect,
        long spacing,
        uint32_t maxCols,
        PositionOptions parentOptions,
        bool queryOnly = false
        );

private:
    struct GridDimension
    {
        PositionOptions options;    // combined options for this grid cell (they come from the respective windows)
        long offset;                // x in horizontal, y in vertical
        long size;                  // width in horizontal, height in vertical
    };

    static void DistributeGridDimensions(
        __inout_ecount(gridDimensionsCount) GridDimension* gridDimensions,
        uint32_t gridDimensionsCount,
        long clientSize,
        long spacing,
        PositionOptions optionsMask // can be PositionOptionsUseSlackHeight or PositionOptionsUseSlackWidth
        );
};


template<typename T>
T* GetClassFromWindow(HWND hwnd)
{
    return reinterpret_cast<T*>(::GetWindowLongPtr(hwnd, DWLP_USER));
}

struct DialogProcResult
{
    INT_PTR handled; // whether it was handled or not
    LRESULT value;   // the return value

    DialogProcResult(INT_PTR newHandled, LRESULT newValue)
    :   handled(newHandled),
        value(newValue)
    { }

    DialogProcResult(bool newHandled)
    :   handled(newHandled),
        value()
    { }
};


HMENU WINAPI GetSubMenu(
    __in HMENU parentMenu,
    __in UINT item,
    __in BOOL byPosition
);

bool CopyTextToClipboard(
    HWND hwnd,
    _In_reads_(textLength) wchar_t const* text,
    size_t textLength
);


std::wstring GetListViewText(
    HWND listViewHwnd,
    __in_z const wchar_t* separator
);

void CopyListTextToClipboard(
    HWND listViewHwnd,
    _In_z_ wchar_t const* separator
);
