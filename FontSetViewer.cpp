// FontSet="http://dwayner-test/Fonts/Fonts.json" DWriteDLL="O:\fbl_grfx_dev_proto.obj.x86chk\windows\core\text\dll\win8\objchk\i386\DWrite.dll"

// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// Contents:    Main user interface window.
//
//----------------------------------------------------------------------------
#include "precomp.h"
#include "resources/resource.h"
#include "font/DWritEx.h"
#include "FontSetViewer.h"


////////////////////////////////////////
// Main entry.

// Shows an error message if the function returned a failing HRESULT,
// then returning that same error code.
HRESULT ShowMessageIfFailed(HRESULT functionResult, const wchar_t* message);

HRESULT CopyImageToClipboard(HWND hwnd, HDC hdc, bool isUpsideDown);


namespace
{
    std::wstring g_dwriteDllName = L"dwrite.dll"; // Point elsewhere to load a custom one.
    bool g_startBlankList = false;

    const static wchar_t* g_locales[][2] = {
        { L"English US", L"en-US"},
        { L"English UK", L"en-GB"},
        { L"الْعَرَبيّة Arabic Egypt", L"ar-EG"},
        { L"الْعَرَبيّة Arabic Iraq", L"ar-IQ"},
        { L"中文 Chinese PRC", L"zh-CN"},
        { L"中文 Chinese Taiwan", L"zh-TW"},
        { L"한글 Hangul Korea", L"ko-KR"},
        { L"עִבְרִית Hebrew Israel", L"he-IL"},
        { L"हिन्दी Hindi India", L"hi-IN"},
        { L"日本語 Japanese", L"ja-JP"},
        { L"Romania" , L"ro-RO"},
        { L"Русский язык Russian", L"ru-RU"},
        { L"ca-ES",        L"ca-ES"},
        { L"cs-CZ",        L"cs-CZ"},
        { L"da-DK",        L"da-DK"},
        { L"de-DE",        L"de-DE"},
        { L"el-GR",        L"el-GR"},
        { L"es-ES",        L"es-ES"},
        { L"es-ES_tradnl", L"es-ES_tradnl"},
        { L"es-MX",        L"es-MX"},
        { L"eu-ES",        L"eu-ES"},
        { L"fi-FI",        L"fi-FI"},
        { L"fr-CA",        L"fr-CA"},
        { L"fr-FR",        L"fr-FR"},
        { L"hu-HU",        L"hu-HU"},
        { L"it-IT",        L"it-IT"},
        { L"nb-NO",        L"nb-NO"},
        { L"nl-NL",        L"nl-NL"},
        { L"pl-PL",        L"pl-PL"},
        { L"pt-BR",        L"pt-BR"},
        { L"pt-PT",        L"pt-PT"},
        { L"ru-RU",        L"ru-RU"},
        { L"sk-SK",        L"sk-SK"},
        { L"sl-SI",        L"sl-SI"},
        { L"sv-SE",        L"sv-SE"},
        { L"tr-TR",        L"tr-TR"},
        };

    const static wchar_t* g_fontCollectionFilterModeNames[] = {
        L"Ungrouped",            // DWRITE_FONT_PROPERTY_ID_NONE
        L"WssFamilyName",        // DWRITE_FONT_PROPERTY_ID_WEIGHT_STRETCH_STYLE_FAMILY_NAME
        L"TypographicFamilyName",// DWRITE_FONT_PROPERTY_ID_TYPOGRAPHIC_FAMILY_NAME
        L"WssFaceName",          // DWRITE_FONT_PROPERTY_ID_WEIGHT_STRETCH_STYLE_FAMILY_FACE_NAME
        L"FullName",             // DWRITE_FONT_PROPERTY_ID_FULL_NAME
        L"Win32FamilyName",      // DWRITE_FONT_PROPERTY_ID_WIN32_FAMILY_NAME
        L"PostscriptName",       // DWRITE_FONT_PROPERTY_ID_POSTSCRIPT_NAME
        L"DesignedScriptTag",    // DWRITE_FONT_PROPERTY_ID_DESIGN_SCRIPT_LANGUAGE_TAG,
        L"SupportedScriptTag",   // DWRITE_FONT_PROPERTY_ID_SUPPORTED_SCRIPT_LANGUAGE_TAG
        L"SemanticTag",          // DWRITE_FONT_PROPERTY_ID_SEMANTIC_TAG
        L"Weight",               // DWRITE_FONT_PROPERTY_ID_WEIGHT
        L"Stretch",              // DWRITE_FONT_PROPERTY_ID_STRETCH
        L"Style",                // DWRITE_FONT_PROPERTY_ID_STYLE
        L"TypographicFaceName",  // DWRITE_FONT_PROPERTY_ID_TYPOGRAPHIC_FACE_NAME
    };

    static_assert(ARRAYSIZE(g_fontCollectionFilterModeNames) == int(MainWindow::FontCollectionFilterMode::Total), "Update the name list to match the actual count.");

    struct FamilyNameTags
    {
        wchar_t const* fontName;
        wchar_t const* tags;
        wchar_t const* scripts;
    };

    static FamilyNameTags const g_knownFamilyNameTags[] =
    {
        { L"Agency FB", L"Display;", L"Latn;" },
        { L"Aharoni", L"Text;", L"Hebr;" },
        { L"Aharoni Bold", L"Display;", L"Hebr;" },
        { L"Ahn B", L"Display;", L"Kore;" },
        { L"Ahn L", L"Text;", L"Kore;" },
        { L"Ahn M", L"Text;", L"Kore;" },
        { L"Aldhabi", L"Text;", L"Arab;" },
        { L"Algerian", L"Display;", L"Latn;" },
        { L"Ami R", L"Text;", L"Kore;" },
        { L"Andalus", L"Display;", L"Arab;" },
        { L"Angsana New", L"Text;", L"Thai;" },
        { L"AngsanaUPC", L"Text;", L"Thai;" },
        { L"Aparajita", L"Display;", L"Deva;" },
        { L"Arabic Typesetting", L"Text;", L"Arab;" },
        { L"Arial", L"Text;", L"Latn;Grek;Cyrl;Hebr;Arab;" },
        { L"Arial Rounded MT", L"Display;", L"Latn;" },
        { L"Arial Unicode MS", L"Text;", L"Latn;Grek;Cyrl;Armn;Geor;Arab;Hebr;" },
        { L"Baskerville Old Face", L"Text;", L"Latn;" },
        { L"Batang", L"Text;", L"Kore;" },
        { L"Batang Old Hangul", L"Text;", L"Kore;" },
        { L"Batang Old Koreul", L"Text;", L"Kore;" },
        { L"BatangChe", L"Text;", L"Kore;" },
        { L"Bauhaus 93", L"Display;", L"Latn;" },
        { L"Bell MT", L"Text;", L"Latn;" },
        { L"Berlin Sans FB", L"Display;", L"Latn;" },
        { L"Bernard MT", L"Display;", L"Latn;" },
        { L"Big Round R", L"Display;", L"Kore;" },
        { L"Big Sans R", L"Display;", L"Kore;" },
        { L"Blackadder ITC", L"Display;", L"Latn;" },
        { L"Bodoni MT", L"Text;", L"Latn;" },
        { L"Bodoni MT Poster", L"Display;", L"Latn;" },
        { L"Book Antiqua", L"Text;", L"Latn;" },
        { L"Bookman Old Style", L"Text;", L"Latn;" },
        { L"Bookshelf Symbol 7", L"Symbol;", L"Zsym;" },
        { L"Bradley Hand ITC", L"Informal;", L"Latn;" },
        { L"Britannic", L"Display;", L"Latn;" },
        { L"Broadway", L"Display;", L"Latn;" },
        { L"Browallia New", L"Text;", L"Thai;" },
        { L"BrowalliaUPC", L"Text;", L"Thai;" },
        { L"Brush Script MT", L"Informal;", L"Latn;" },
        { L"Calibri", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Californian FB", L"Text;", L"Latn;" },
        { L"Calisto MT", L"Text;", L"Latn;" },
        { L"Cambria", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Cambria Math", L"Symbol;", L"Zsym;Zmth;" },
        { L"Candara", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Castellar", L"Display;", L"Latn;" },
        { L"Centaur", L"Text;", L"Latn;" },
        { L"Century", L"Text;", L"Latn;" },
        { L"Century Gothic", L"Display;", L"Latn;" },
        { L"Century Schoolbook", L"Text;", L"Latn;" },
        { L"Chiller", L"Display;", L"Latn;" },
        { L"Colonna MT", L"Display;", L"Latn;" },
        { L"Comic Sans MS", L"Informal;", L"Latn;Grek;Cyrl;" },
        { L"Consolas", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Constantia", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Cooper", L"Display;", L"Latn;" },
        { L"Copperplate Gothic", L"Display;", L"Latn;" },
        { L"Corbel", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Cordia New", L"Text;", L"Thai;" },
        { L"CordiaUPC", L"Text;", L"Thai;" },
        { L"Courier", L"Text;", L"Latn;" },
        { L"Courier New", L"Text;", L"Latn;Grek;Cyrl;Hebr;" },
        { L"Curlz MT", L"Display;", L"Latn;" },
        { L"DFKai-SB", L"Text;", L"Hant;" },
        { L"DaunPenh", L"Text;", L"Khmr;" },
        { L"David", L"Text;", L"Hebr;" },
        { L"DejaVu Sans", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"DejaVu Sans Mono", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"DejaVu Serif", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"DilleniaUPC", L"Text;", L"Thai;" },
        { L"DokChampa", L"Text;", L"Laoo;" },
        { L"Dotum", L"Text;", L"Kore;" },
        { L"Dotum Old Hangul", L"Text;", L"Kore;" },
        { L"Dotum Old Koreul", L"Text;", L"Kore;" },
        { L"DotumChe", L"Text;", L"Kore;" },
        { L"Ebrima", L"Text;", L"Vaii;Nkoo;Tfng;Osma;Ethi;" },
        { L"Edwardian Script ITC", L"Display;", L"Latn;" },
        { L"Elephant", L"Display;", L"Latn;" },
        { L"Engravers MT", L"Display;", L"Latn;" },
        { L"Eras ITC", L"Text;", L"Latn;" },
        { L"Eras ITC Medium", L"Display;", L"Latn;" },
        { L"Estrangelo Edessa", L"Text;", L"Syrc;" },
        { L"EucrosiaUPC", L"Text;", L"Thai;" },
        { L"Euphemia", L"Text;", L"Cans;" },
        { L"Expo B", L"Display;", L"Kore;" },
        { L"Expo L", L"Text;", L"Kore;" },
        { L"Expo M", L"Display;", L"Kore;" },
        { L"FZShuTi", L"Text;", L"Hans;" },
        { L"FZYaoTi", L"Text;", L"Hans;" },
        { L"FangSong", L"Text;", L"Hans;" },
        { L"Felix Titling", L"Display;", L"Latn;" },
        { L"Fixedsys", L"Text;", L"Latn;" },
        { L"Footlight MT", L"Text;", L"Latn;" },
        { L"Forte", L"Informal;", L"Latn;" },
        { L"FrankRuehl", L"Text;", L"Hebr;" },
        { L"Franklin Gothic", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Franklin Gothic Book", L"Text;", L"Latn;" },
        { L"FreesiaUPC", L"Text;", L"Thai;" },
        { L"Freestyle Script", L"Informal;", L"Latn;" },
        { L"French Script MT", L"Informal;", L"Latn;" },
        { L"Gabriola", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Gadugi", L"Text;", L"Cher;" },
        { L"Garam B", L"Text;", L"Kore;" },
        { L"Garamond", L"Text;", L"Latn;" },
        { L"Gautami", L"Text;", L"Telu;" },
        { L"Georgia", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Gigi", L"Display;", L"Latn;" },
        { L"Gill Sans", L"Display;", L"Latn;" },
        { L"Gill Sans MT", L"Text;", L"Latn;" },
        { L"Gisha", L"Text;", L"Hebr;" },
        { L"Gloucester MT", L"Display;", L"Latn;" },
        { L"Gothic B", L"Text;", L"Kore;" },
        { L"Gothic L", L"Text;", L"Kore;" },
        { L"Gothic Newsletter", L"Text;", L"Kore;" },
        { L"Gothic R", L"Text;", L"Kore;" },
        { L"Gothic Round B", L"Text;", L"Kore;" },
        { L"Gothic Round L", L"Text;", L"Kore;" },
        { L"Gothic Round R", L"Text;", L"Kore;" },
        { L"Gothic Round XB", L"Display;", L"Kore;" },
        { L"Gothic XB", L"Display;", L"Kore;" },
        { L"Goudy Old Style", L"Text;", L"Latn;" },
        { L"Goudy Stout", L"Display;", L"Latn;" },
        { L"Graphic B", L"Display;", L"Kore;" },
        { L"Graphic New R", L"Display;", L"Kore;" },
        { L"Graphic R", L"Text;", L"Kore;" },
        { L"Graphic Sans B", L"Display;", L"Kore;" },
        { L"Graphic Sans R", L"Text;", L"Kore;" },
        { L"Gulim", L"Text;", L"Kore;" },
        { L"GulimChe", L"Text;", L"Kore;" },
        { L"Gungsuh", L"Text;", L"Kore;" },
        { L"Gungsuh Old Hangul", L"Text;", L"Kore;" },
        { L"Gungsuh Old Koreul", L"Text;", L"Kore;" },
        { L"Gungsuh R", L"Text;", L"Kore;" },
        { L"GungsuhChe", L"Text;", L"Kore;" },
        { L"HGGothicE", L"Text;", L"Jpan;" },
        { L"HGGothicM", L"Text;", L"Jpan;" },
        { L"HGGyoshotai", L"Text;", L"Jpan;" },
        { L"HGKyokashotai", L"Text;", L"Jpan;" },
        { L"HGMaruGothicMPRO", L"Text;", L"Jpan;" },
        { L"HGMinchoB", L"Text;", L"Jpan;" },
        { L"HGMinchoE", L"Text;", L"Jpan;" },
        { L"HGPGothicE", L"Text;", L"Jpan;" },
        { L"HGPGothicM", L"Text;", L"Jpan;" },
        { L"HGPGyoshotai", L"Text;", L"Jpan;" },
        { L"HGPKyokashotai", L"Text;", L"Jpan;" },
        { L"HGPMinchoB", L"Text;", L"Jpan;" },
        { L"HGPMinchoE", L"Text;", L"Jpan;" },
        { L"HGPSoeiKakugothicUB", L"Display;", L"Jpan;" },
        { L"HGPSoeiKakupoptai", L"Display;", L"Jpan;" },
        { L"HGPSoeiPresenceEB", L"Text;", L"Jpan;" },
        { L"HGSGothicE", L"Text;", L"Jpan;" },
        { L"HGSGothicM", L"Text;", L"Jpan;" },
        { L"HGSGyoshotai", L"Text;", L"Jpan;" },
        { L"HGSKyokashotai", L"Text;", L"Jpan;" },
        { L"HGSMinchoB", L"Text;", L"Jpan;" },
        { L"HGSMinchoE", L"Text;", L"Jpan;" },
        { L"HGSSoeiKakugothicUB", L"Display;", L"Jpan;" },
        { L"HGSSoeiKakupoptai", L"Display;", L"Jpan;" },
        { L"HGSSoeiPresenceEB", L"Text;", L"Jpan;" },
        { L"HGSeikaishotaiPRO", L"Text;", L"Jpan;" },
        { L"HGSoeiKakugothicUB", L"Display;", L"Jpan;" },
        { L"HGSoeiKakupoptai", L"Display;", L"Jpan;" },
        { L"HGSoeiPresenceEB", L"Text;", L"Jpan;" },
        { L"HYBackSong", L"Display;", L"Kore;" },
        { L"HYBudle", L"Text;", L"Kore;" },
        { L"HYGothic", L"Text;", L"Kore;" },
        { L"HYGothic-Extra", L"Display;", L"Kore;" },
        { L"HYGraphic", L"Text;", L"Kore;" },
        { L"HYGungSo", L"Text;", L"Kore;" },
        { L"HYHaeSo", L"Text;", L"Kore;" },
        { L"HYHeadLine", L"Display;", L"Kore;" },
        { L"HYKHeadLine", L"Display;", L"Kore;" },
        { L"HYLongSamul", L"Display;", L"Kore;" },
        { L"HYMokGak", L"Display;", L"Kore;" },
        { L"HYMokPan", L"Display;", L"Kore;" },
        { L"HYMyeongJo", L"Text;", L"Kore;" },
        { L"HYMyeongJo Extra Bold", L"Display;", L"Kore;" },
        { L"HYMyeongJo-Extra", L"Text;", L"Kore;" },
        { L"HYPMokGak", L"Display;", L"Kore;" },
        { L"HYPMokPan", L"Display;", L"Kore;" },
        { L"HYPillGi", L"Informal;", L"Kore;" },
        { L"HYPost", L"Informal;", L"Kore;" },
        { L"HYRGothic", L"Text;", L"Kore;" },
        { L"HYSeNse", L"Informal;", L"Kore;" },
        { L"HYShortSamul", L"Text;", L"Kore;" },
        { L"HYSinGraphic", L"Text;", L"Kore;" },
        { L"HYSinMun-MyeongJo", L"Text;", L"Kore;" },
        { L"HYSinMyeongJo", L"Text;", L"Kore;" },
        { L"HYSinMyeongJo-Medium-HanjaA", L"Symbol;", L"Zsym;" },
        { L"HYSinMyeongJo-Medium-HanjaB", L"Symbol;", L"Zsym;" },
        { L"HYSinMyeongJo-Medium-HanjaC", L"Symbol;", L"Zsym;" },
        { L"HYSooN-MyeongJo", L"Text;", L"Kore;" },
        { L"HYSymbolA", L"Symbol;", L"Zsym;" },
        { L"HYSymbolB", L"Symbol;", L"Zsym;" },
        { L"HYSymbolC", L"Symbol;", L"Zsym;" },
        { L"HYSymbolD", L"Symbol;", L"Zsym;" },
        { L"HYSymbolE", L"Symbol;", L"Zsym;" },
        { L"HYSymbolF", L"Symbol;", L"Zsym;" },
        { L"HYSymbolG", L"Symbol;", L"Zsym;" },
        { L"HYSymbolH", L"Symbol;", L"Zsym;" },
        { L"HYTaJa", L"Text;", L"Kore;" },
        { L"HYTaJa Bold", L"Display;", L"Kore;" },
        { L"HYTaJaFull", L"Text;", L"Kore;" },
        { L"HYTaJaFull Bold", L"Display;", L"Kore;" },
        { L"HYTeBack", L"Display;", L"Kore;" },
        { L"HYYeaSo", L"Display;", L"Kore;" },
        { L"HYYeasoL", L"Display;", L"Kore;" },
        { L"HYYeatGul", L"Display;", L"Kore;" },
        { L"Haettenschweiler", L"Display;", L"Latn;" },
        { L"Harlow Solid", L"Display;", L"Latn;" },
        { L"Harrington", L"Display;", L"Latn;" },
        { L"Headline R", L"Display;", L"Kore;" },
        { L"Headline Sans R", L"Display;", L"Kore;" },
        { L"High Tower Text", L"Text;", L"Latn;" },
        { L"Impact", L"Display;", L"Latn;Grek;Cyrl;" },
        { L"Imprint MT Shadow", L"Display;", L"Latn;" },
        { L"Informal Roman", L"Informal;", L"Latn;" },
        { L"IrisUPC", L"Display;", L"Thai;" },
        { L"Iskoola Pota", L"Text;", L"Sinh;" },
        { L"JasmineUPC", L"Display;", L"Thai;" },
        { L"Jasu B", L"Display;", L"Kore;" },
        { L"Jasu L", L"Display;", L"Kore;" },
        { L"Jasu R", L"Display;", L"Kore;" },
        { L"Jasu XB", L"Display;", L"Kore;" },
        { L"Javanese Text", L"Text;", L"Java;" },
        { L"Jokerman", L"Display;", L"Latn;" },
        { L"Juice ITC", L"Display;", L"Latn;" },
        { L"KaiTi", L"Text;", L"Hans;" },
        { L"Kalinga", L"Text;", L"Orya;" },
        { L"Kartika", L"Text;", L"Mlym;" },
        { L"Khmer UI", L"Text;", L"Khmr;" },
        { L"KodchiangUPC", L"Display;", L"Thai;" },
        { L"Kokila", L"Text;", L"Deva;" },
        { L"Kristen ITC", L"Informal;", L"Latn;" },
        { L"Kunstler Script", L"Display;", L"Latn;" },
        { L"Lao UI", L"Text;", L"Laoo;" },
        { L"Latha", L"Text;", L"Taml;" },
        { L"Latin", L"Display;", L"Latn;" },
        { L"Leelawadee", L"Text;", L"Thai;" },
        { L"Leelawadee UI", L"Text;", L"Thai;Laoo;Bugi;Khmr;" },
        { L"Levenim MT", L"Display;", L"Hebr;" },
        { L"LiSu", L"Text;", L"Hans;" },
        { L"LilyUPC", L"Display;", L"Thai;" },
        { L"Lucida Bright", L"Text;", L"Latn;" },
        { L"Lucida Calligraphy", L"Display;", L"Latn;" },
        { L"Lucida Console", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Lucida Fax", L"Text;", L"Latn;" },
        { L"Lucida Handwriting", L"Informal;", L"Latn;" },
        { L"Lucida Sans", L"Text;", L"Latn;" },
        { L"Lucida Sans Typewriter", L"Text;", L"Latn;" },
        { L"Lucida Sans Unicode", L"Text;", L"Latn;Grek;Cyrl;Hebr;" },
        { L"MS Gothic", L"Text;", L"Jpan;" },
        { L"MS Mincho", L"Text;", L"Jpan;" },
        { L"MS Outlook", L"Symbol;", L"Zsym;" },
        { L"MS PGothic", L"Text;", L"Jpan;" },
        { L"MS PMincho", L"Text;", L"Jpan;" },
        { L"MS Reference Sans Serif", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"MS Reference Specialty", L"Symbol;", L"Zsym;" },
        { L"MS Sans Serif", L"Text;", L"Latn;" },
        { L"MS Serif", L"Text;", L"Latn;" },
        { L"MS UI Gothic", L"Text;", L"Jpan;" },
        { L"MV Boli", L"Text;", L"Thaa;" },
        { L"Magic R", L"Display;", L"Kore;" },
        { L"Magneto", L"Display;", L"Latn;" },
        { L"Maiandra GD", L"Text;", L"Latn;" },
        { L"Malgun Gothic", L"Text;", L"Kore;" },
        { L"Mangal", L"Text;", L"Deva;" },
        { L"Marlett", L"Symbol", L"Zsym;" },
        { L"Matura MT Script Capitals", L"Display;", L"Latn;" },
        { L"Meiryo", L"Text;", L"Jpan;" },
        { L"Meiryo UI", L"Text;", L"Jpan;" },
        { L"Meorimyungjo B", L"Display;", L"Kore;" },
        { L"Meorimyungjo XB", L"Display;", L"Kore;" },
        { L"Microsoft Himalaya", L"Text;", L"Tibt;" },
        { L"Microsoft JhengHei", L"Text;", L"Hant;" },
        { L"Microsoft JhengHei Light", L"Text;", L"Hant;" },
        { L"Microsoft JhengHei UI", L"Text;", L"Hant;" },
        { L"Microsoft JhengHei UI Light", L"Text;", L"Hant;" },
        { L"Microsoft New Tai Lue", L"Text;", L"Talu;" },
        { L"Microsoft PhagsPa", L"Text;", L"Phag;" },
        { L"Microsoft Sans Serif", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Microsoft Tai Le", L"Text;", L"Tale;" },
        { L"Microsoft Uighur", L"Text;", L"ug-Arab;" },
        { L"Microsoft YaHei", L"Text;", L"Hans;" },
        { L"Microsoft YaHei Light", L"Text;", L"Hans;" },
        { L"Microsoft YaHei UI", L"Text;", L"Hans;" },
        { L"Microsoft YaHei UI Light", L"Text;", L"Hans;" },
        { L"Microsoft Yi Baiti", L"Text;", L"Yiii;" },
        { L"MingLiU", L"Text;", L"Hant;" },
        { L"MingLiU-ExtB", L"Text;", L"Hant;" },
        { L"MingLiU_HKSCS", L"Text;", L"Hant-HK;" },
        { L"MingLiU_HKSCS-ExtB", L"Text;", L"Hant-HK;" },
        { L"Miriam", L"Text;", L"Hebr;" },
        { L"Miriam Fixed", L"Text;", L"Hebr;" },
        { L"Mistral", L"Informal;", L"Latn;" },
        { L"Modak R", L"Display;", L"Kore;" },
        { L"Modern", L"Text;", L"Latn;" },
        { L"Modern No. 20", L"Text;", L"Latn;" },
        { L"MoeumT B", L"Display;", L"Kore;" },
        { L"MoeumT L", L"Text;", L"Kore;" },
        { L"MoeumT R", L"Text;", L"Kore;" },
        { L"MoeumT XB", L"Display;", L"Kore;" },
        { L"Mongolian Baiti", L"Text;", L"Mong;" },
        { L"Monotype Corsiva", L"Informal;", L"Latn;" },
        { L"MoolBoran", L"Display;", L"Khmr;" },
        { L"Myanmar Text", L"Text;", L"Mymr;" },
        { L"Myungjo B", L"Text;", L"Kore;" },
        { L"Myungjo L", L"Text;", L"Kore;" },
        { L"Myungjo Newsletter", L"Text;", L"Kore;" },
        { L"Myungjo R", L"Text;", L"Kore;" },
        { L"Myungjo SK B", L"Text;", L"Kore;" },
        { L"Myungjo XB", L"Display;", L"Kore;" },
        { L"NSimSun", L"Text;", L"Hans;" },
        { L"Namu B", L"Informal;", L"Kore;" },
        { L"Namu L", L"Text;", L"Kore;" },
        { L"Namu R", L"Text;", L"Kore;" },
        { L"Namu XB", L"Display;", L"Kore;" },
        { L"Narkisim", L"Text;", L"Hebr;" },
        { L"New Batang", L"Text;", L"Kore;" },
        { L"New Dotum", L"Text;", L"Kore;" },
        { L"New Gulim", L"Text;", L"Kore;" },
        { L"New Gungsuh", L"Text;", L"Kore;" },
        { L"NewGulim Old Hangul", L"Text;", L"Kore;" },
        { L"NewGulim Old Koreul", L"Text;", L"Kore;" },
        { L"Niagara Engraved", L"Display;", L"Latn;" },
        { L"Niagara Solid", L"Display;", L"Latn;" },
        { L"Nirmala UI", L"Text;", L"Taml;Beng;Deva;Gujr;Guru;Knda;Mlym;Orya;Sinh;Telu;Olck;Sora;" },
        { L"Nyala", L"Text;", L"Ethi;" },
        { L"OCR A", L"Display;", L"Latn;" },
        { L"OCRB", L"Text;", L"Latn;" },
        { L"Old English Text MT", L"Display;", L"Latn;" },
        { L"Onyx", L"Display;", L"Latn;" },
        { L"OpenSymbol", L"Symbol;", L"Zsym;" },
        { L"PMingLiU", L"Text;", L"Hant;" },
        { L"PMingLiU-ExtB", L"Text;", L"Hant;" },
        { L"Palace Script MT", L"Display;", L"Latn;" },
        { L"Palatino Linotype", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Pam B", L"Display;", L"Kore;" },
        { L"Pam L", L"Text;", L"Kore;" },
        { L"Pam M", L"Text;", L"Kore;" },
        { L"Pam New B", L"Display;", L"Kore;" },
        { L"Pam New L", L"Text;", L"Kore;" },
        { L"Pam New M", L"Text;", L"Kore;" },
        { L"Panhwa R", L"Display;", L"Kore;" },
        { L"Papyrus", L"Informal;", L"Latn;" },
        { L"Parchment", L"Display;", L"Latn;" },
        { L"Perpetua", L"Text;", L"Latn;" },
        { L"Perpetua Titling MT", L"Display;", L"Latn;" },
        { L"Plantagenet Cherokee", L"Text;", L"Cher;" },
        { L"Playbill", L"Display;", L"Latn;" },
        { L"Poor Richard", L"Display;", L"Latn;" },
        { L"Pristina", L"Informal;", L"Latn;" },
        { L"Pyunji R", L"Informal;", L"Kore;" },
        { L"Raavi", L"Text;", L"Guru;" },
        { L"Rage", L"Informal;", L"Latn;" },
        { L"Ravie", L"Display;", L"Latn;" },
        { L"Rockwell", L"Text;", L"Latn;" },
        { L"Rod", L"Text;", L"Hebr;" },
        { L"Roman", L"Text;", L"Latn;" },
        { L"STCaiyun", L"Display;", L"Hans;" },
        { L"STFangsong", L"Text;", L"Hans;" },
        { L"STHupo", L"Display;", L"Hans;" },
        { L"STKaiti", L"Text;", L"Hans;" },
        { L"STLiti", L"Text;", L"Hans;" },
        { L"STSong", L"Text;", L"Hans;" },
        { L"STXihei", L"Text;", L"Hans;" },
        { L"STXingkai", L"Text;", L"Hans;" },
        { L"STXinwei", L"Text;", L"Hans;" },
        { L"STZhongsong", L"Text;", L"Hans;" },
        { L"SWGamekeys MT", L"Symbol;", L"Zsym;" }, // instead of SWGamekeys MT Regular
        { L"SWMacro Regular", L"Symbol;", L"Zsym;" },
        { L"Saenaegi B", L"Text;", L"Kore;" },
        { L"Saenaegi L", L"Text;", L"Kore;" },
        { L"Saenaegi R", L"Text;", L"Kore;" },
        { L"Saenaegi XB", L"Display;", L"Kore;" },
        { L"Sakkal Majalla", L"Text;", L"Arab;" },
        { L"Sam B", L"Display;", L"Kore;" },
        { L"Sam L", L"Text;", L"Kore;" },
        { L"Sam M", L"Text;", L"Kore;" },
        { L"Sam New B", L"Display;", L"Kore;" },
        { L"Sam New L", L"Text;", L"Kore;" },
        { L"Sam New M", L"Text;", L"Kore;" },
        { L"Script", L"Informal;", L"Latn;" },
        { L"Script MT", L"Display;", L"Latn;" },
        { L"Segoe Print", L"Informal;", L"Latn;Grek;Cyrl;" },
        { L"Segoe Script", L"Informal;", L"Latn;Grek;Cyrl;" },
        { L"Segoe UI", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Segoe UI Black", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Segoe UI Black Italic", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Segoe UI Bold", L"Text;", L"Latn;Grek;Cyrl;Armn;Geor;Geok;Arab;Hebr;Lisu;" },
        { L"Segoe UI Bold Italic", L"Text;", L"Latn;Grek;Cyrl;Armn;Geor;" },
        { L"Segoe UI Emoji", L"Symbol;", L"Zsym;" },
        { L"Segoe UI Italic", L"Text;", L"Latn;Grek;Cyrl;Armn;Geor;" },
        { L"Segoe UI Light", L"Text;", L"Latn;Grek;Cyrl;Armn;Geor;Geok;Arab;Hebr;Lisu;" },
        { L"Segoe UI Light Italic", L"Text;", L"Latn;Grek;Cyrl;Armn;Geor;" },
        { L"Segoe UI Regular", L"Text;", L"Latn;Grek;Cyrl;Armn;Geor;Geok;Arab;Hebr;Lisu;" },
        { L"Segoe UI Semibold", L"Text;", L"Latn;Grek;Cyrl;Armn;Geor;Geok;Arab;Hebr;Lisu;" },
        { L"Segoe UI Semibold Italic", L"Text;", L"Latn;Grek;Cyrl;Armn;Geor;" },
        { L"Segoe UI Semilight", L"Text;", L"Latn;Grek;Cyrl;Armn;Geor;Geok;Arab;Hebr;Lisu;" },
        { L"Segoe UI Semilight Italic", L"Text;", L"Latn;Grek;Cyrl;Armn;Geor;" },
        { L"Segoe UI Symbol", L"Symbol;", L"Zsym;Brai;Dsrt;Glag;Goth;Ital;Ogam;Orkh;Runr;Copt;Merc;" },
        { L"Shonar Bangla", L"Text;", L"Beng;" },
        { L"Showcard Gothic", L"Display;", L"Latn;" },
        { L"Shruti", L"Text;", L"Gujr;" },
        { L"SimHei", L"Text;", L"Hans;" },
        { L"SimSun", L"Text;", L"Hans;" },
        { L"SimSun-ExtB", L"Text;", L"Hans;" },
        { L"Simplified Arabic", L"Text;", L"Arab;" },
        { L"Simplified Arabic Fixed", L"Text;", L"Arab;" },
        { L"Sitka Banner", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Sitka Display", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Sitka Heading", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Sitka Small", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Sitka Subheading", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Sitka Text", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Small Fonts", L"Text;", L"Latn;" },
        { L"Snap ITC", L"Display;", L"Latn;" },
        { L"Soha R", L"Display;", L"Kore;" },
        { L"Stencil", L"Display;", L"Latn;" },
        { L"Sylfaen", L"Text;", L"Grek;Cyrl;Armn;Geor;" },
        { L"Symbol", L"Symbol;", L"Zsym;" },
        { L"System", L"Text;", L"Latn;" },
        { L"Tahoma", L"Text;", L"Latn;Grek;Cyrl;Armn;Hebr;" },
        { L"Tempus Sans ITC", L"Informal;", L"Latn;" },
        { L"Terminal", L"Text;", L"Latn;" },
        { L"Times New Roman", L"Text;", L"Latn;Grek;Cyrl;Hebr;" },
        { L"Traditional Arabic", L"Text;", L"Arab;" },
        { L"Trebuchet MS", L"Display;", L"Latn;Grek;Cyrl;" },
        { L"Tunga", L"Text;", L"Knda;" },
        { L"Tw Cen MT", L"Text;", L"Latn;" },
        { L"Urdu Typesetting", L"Text;", L"Arab;" },
        { L"Utsaah", L"Text;", L"Deva;" },
        { L"Vani", L"Text;", L"Telu;" },
        { L"Verdana", L"Text;", L"Latn;Grek;Cyrl;" },
        { L"Vijaya", L"Text;", L"Taml;" },
        { L"Viner Hand ITC", L"Informal;", L"Latn;" },
        { L"Vivaldi", L"Display;", L"Latn;" },
        { L"Vladimir Script", L"Display;", L"Latn;" },
        { L"Vrinda", L"Text;", L"Beng;" },
        { L"Webdings", L"Symbol;", L"Zsym;" },
        { L"Wingdings", L"Symbol;", L"Zsym;" },
        { L"Wingdings 2", L"Symbol;", L"Zsym;" },
        { L"Wingdings 3", L"Symbol;", L"Zsym;" },
        { L"Woorin R", L"Text;", L"Kore;" },
        { L"Yeopseo R", L"Informal;", L"Kore;" },
        { L"Yet R", L"Display;", L"Kore;" },
        { L"Yet Sans XB", L"Display;", L"Kore;" },
        { L"Yet Sans B", L"Text;", L"Kore;" },
        { L"Yet Sans L", L"Text;", L"Kore;" },
        { L"Yet Sans R", L"Text;", L"Kore;" },
        { L"YouYuan", L"Text;", L"Hans;" },
        { L"Yu Gothic", L"Text;", L"Jpan;" },
        { L"Yu Gothic Light", L"Text;", L"Jpan;" },
        { L"Yu Mincho", L"Text;", L"Jpan;" },
        { L"Yu Mincho Light", L"Text;", L"Jpan;" },
    };

    bool FindTagsFromKnownFontName(
        wchar_t const* fullFontName,
        wchar_t const* familyName,
        OUT wchar_t const** tags,
        OUT wchar_t const** scripts
        ) // not WWS or GDI name
    {
        for (uint32_t i = 0; i < 2; ++i)
        {
            auto fontName = i == 0 ? fullFontName : familyName;
            if (fontName == nullptr || fontName[0] == '\0')
                continue;

            auto const* begin = g_knownFamilyNameTags;
            auto const* end = g_knownFamilyNameTags + ARRAYSIZE(g_knownFamilyNameTags);

            while (begin < end)
            {
                auto const* p = begin + (end - begin) / 2;

                auto cmp = _wcsicmp(fontName, p->fontName);
                if (cmp == 0)
                {
                    *tags = p->tags;
                    *scripts = p->scripts;
                    return true;
                }

                cmp = wcscmp(fontName, p->fontName);
                if (cmp < 0)
                {
                    end = p;
                }
                else // if (cmp > 0)
                {
                    begin = p + 1;
                }
            }
        }

        return false;
    }


    void AppendStringIfNotPresent(
        __in_z wchar_t* newString,
        __inout std::wstring& existingString
        )
    {
        if (existingString.find(newString) == std::wstring::npos)
        {
            existingString.append(newString);
        }
    }


    ////////////////////////////////////////
    // DWrite helper functions


    HRESULT GetLocalFileLoaderAndKey(
        IDWriteFontFile* fontFile,
        _Out_ void const** fontFileReferenceKey,
        _Out_ uint32_t& fontFileReferenceKeySize,
        _Out_ IDWriteLocalFontFileLoader** localFontFileLoader
    )
    {
        *localFontFileLoader = nullptr;
        *fontFileReferenceKey = nullptr;
        fontFileReferenceKeySize = 0;

        if (fontFile == nullptr)
            return E_INVALIDARG;

        ComPtr<IDWriteFontFileLoader> fontFileLoader;
        IFR(fontFile->GetLoader(OUT &fontFileLoader));
        IFR(fontFileLoader->QueryInterface(OUT localFontFileLoader));

        IFR(fontFile->GetReferenceKey(fontFileReferenceKey, OUT &fontFileReferenceKeySize));

        return S_OK;
    }


    HRESULT GetFilePath(
        IDWriteFontFile* fontFile,
        OUT std::wstring& filePath
    ) throw()
    {
        std::wstring tempFilePath;
        std::swap(tempFilePath, filePath);

        if (fontFile == nullptr)
            return E_INVALIDARG;

        ComPtr<IDWriteLocalFontFileLoader> localFileLoader;
        uint32_t fontFileReferenceKeySize = 0;
        const void* fontFileReferenceKey = nullptr;

        IFR(GetLocalFileLoaderAndKey(fontFile, OUT &fontFileReferenceKey, OUT fontFileReferenceKeySize, OUT &localFileLoader));

        uint32_t filePathLength = 0;
        IFR(localFileLoader->GetFilePathLengthFromKey(
            fontFileReferenceKey,
            fontFileReferenceKeySize,
            OUT &filePathLength
        ));

        try
        {
            tempFilePath.resize(filePathLength);
        }
        catch (std::bad_alloc const&)
        {
            return E_OUTOFMEMORY; // This is the only exception type we need to worry about.
        }

        IFR(localFileLoader->GetFilePathFromKey(
            fontFileReferenceKey,
            fontFileReferenceKeySize,
            OUT &tempFilePath[0],
            filePathLength + 1
        ));
        std::swap(tempFilePath, filePath);

        return S_OK;
    }


    HRESULT GetFontFile(
        IDWriteFontFace* fontFace,
        OUT IDWriteFontFile** fontFile
    )
    {
        *fontFile = nullptr;

        if (fontFace == nullptr)
            return E_INVALIDARG;

        ComPtr<IDWriteFontFile> fontFiles[8];
        uint32_t fontFileCount = 0;

        IFR(fontFace->GetFiles(OUT &fontFileCount, nullptr));
        if (fontFileCount > ARRAYSIZE(fontFiles))
            return E_NOT_SUFFICIENT_BUFFER;

        IFR(fontFace->GetFiles(IN OUT &fontFileCount, OUT &fontFiles[0]));

        if (fontFileCount == 0 || fontFiles[0] == nullptr)
            return E_FAIL;

        *fontFile = fontFiles[0].Detach();
        return S_OK;
    }


    HRESULT GetFilePath(
        IDWriteFontFace* fontFace,
        OUT std::wstring& filePath
    ) throw()
    {
        filePath.clear();

        if (fontFace == nullptr)
            return E_INVALIDARG;

        ComPtr<IDWriteFontFile> fontFile;
        IFR(GetFontFile(fontFace, OUT &fontFile));

        return GetFilePath(fontFile, OUT filePath);
    }


    HRESULT GetFilePath(
        IDWriteFontFaceReference* fontFaceReference,
        OUT std::wstring& filePath
    ) throw()
    {
        filePath.clear();

        if (fontFaceReference == nullptr)
            return E_INVALIDARG;

        ComPtr<IDWriteFontFile> fontFile;
        IFR(fontFaceReference->GetFontFile(OUT &fontFile));
        return GetFilePath(fontFile, OUT filePath);
    }


    HRESULT CreateFontSetFromFileNames(
        IDWriteFactory5* dwriteFactory,
        _In_opt_z_ wchar_t const* baseFilePath,
        _In_reads_bytes_(fileNamesCount) wchar_t const* fileNames,
        uint32_t fileNamesCount,
        _Out_ IDWriteFontSet** fontSet,
        std::wstring& failedFileNames
    )
    {
        *fontSet = nullptr;

        ComPtr<IDWriteFontSet> newFontSet;
        ComPtr<IDWriteFontSetBuilder1> fontSetBuilder;
        IFR(dwriteFactory->CreateFontSetBuilder(OUT &fontSetBuilder));
        auto fileNamesEnd = fileNames + fileNamesCount;

        if (baseFilePath == nullptr)
            baseFilePath = L"";

        // Create font set with a single file.
        for (wchar_t const* fileName = fileNames; fileName < fileNamesEnd && fileName[0] != '\0'; )
        {
            // Get the full path in case relative filenames were passed.
            wchar_t filePath[MAX_PATH + 1];
            PathCombine(OUT filePath, baseFilePath, fileName);
            ComPtr<IDWriteFontFile> fontFile;

            // Add the font file to the set, but don't fail the whole font set if an error happened.
            // Instead, return success, but keep track of the failure.
            if (FAILED(dwriteFactory->CreateFontFileReference(filePath, nullptr, OUT &fontFile))
            ||  FAILED(fontSetBuilder->AddFontFile(fontFile)))
            {
                failedFileNames.append(filePath);
                failedFileNames.push_back('\0');
            }

            fileName = std::find(fileName, fileNamesEnd, '\0') + 1;
        }
        IFR(fontSetBuilder->CreateFontSet(OUT &newFontSet));
        *fontSet = newFontSet.Detach();

        return S_OK;
    }


    HRESULT GetLocalizedString(
        _In_ IDWriteStringList* stringList,
        uint32_t stringIndex,
        _Out_ std::wstring& stringValue
        )
    {
        uint32_t length = 0;
        stringValue.clear();
        if (stringList == nullptr)
            return E_INVALIDARG;

        IFR(stringList->GetStringLength(stringIndex, OUT &length));
        if (length > 0 && length != UINT32_MAX)
        {
            stringValue.resize(length);
        }
        IFR(stringList->GetString(stringIndex, OUT &stringValue[0], length + 1));

        return S_OK;
    }
}


int APIENTRY wWinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPWSTR      commandLine,
    int         nCmdShow
    )
{
    // The Microsoft Security Development Lifecycle recommends that all
    // applications include the following call to ensure that heap corruptions
    // do not go unnoticed and therefore do not introduce opportunities
    // for security exploits.
    HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

    MainWindow::RegisterWindowClass();
    HWND mainHwnd = CreateDialog(HINST_THISCOMPONENT, MAKEINTRESOURCE(IddMainWindow), nullptr, &MainWindow::StaticDialogProc);

    if (mainHwnd == nullptr)
    {
        IFR(ShowMessageIfFailed(
                HRESULT_FROM_WIN32(GetLastError()),
                L"Could not create main demo window! CreateWindow()"  FAILURE_LOCATION
            ));
    }
    else
    {
        MainWindow::RunMessageLoop();
    }

    return 0;
}


MainWindow::MainWindow(HWND hwnd)
:   hwnd_(hwnd)
{
    fontColor_ = GetSysColor(COLOR_WINDOWTEXT) | 0xFF000000; // Ensure opaque
    backgroundColor_ = GetSysColor(COLOR_WINDOW) | 0xFF000000;

    highlightColor_ = GetSysColor(COLOR_HIGHLIGHTTEXT) | 0xFF000000; // Ensure opaque
    highlightBackgroundColor_ = GetSysColor(COLOR_HIGHLIGHT) | 0xFF000000;

    faintSelectionColor_ = GetSysColor(COLOR_BTNFACE) | 0xFF000000;
}


ATOM MainWindow::RegisterWindowClass()
{
    // Registers the main window class.

    // Ensure that the common control DLL is loaded.
    // (probably not needed, but do it anyway)
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_STANDARD_CLASSES|ICC_LISTVIEW_CLASSES|ICC_LINK_CLASS;
    InitCommonControlsEx(&icex); 

    return 0;
}


HRESULT MainWindow::Initialize()
{
    HRESULT hr = S_OK;

    std::wstring commandLine;
    GetCommandLineArguments(IN OUT commandLine);
    IFR(ParseCommandLine(commandLine.c_str()));

    //////////////////////////////
    // Create the DWrite factory.

    IFR(ShowMessageIfFailed(
            LoadDWrite(g_dwriteDllName.c_str(), DWRITE_FACTORY_TYPE_SHARED, OUT &dwriteFactory_, OUT dwriteModule_),
            L"Could not create DirectWrite factory! DWriteCreateFactory()" FAILURE_LOCATION
            ));

    //////////////////////////////
    // Create the main window

    if (hwnd_ == nullptr)
    {
        IFR(ShowMessageIfFailed(
                HRESULT_FROM_WIN32(GetLastError()),
                L"Could not create main demo window! CreateWindow()"  FAILURE_LOCATION
            ));
    }
    else
    {
        ShowWindow(hwnd_, SW_SHOWNORMAL);
        UpdateWindow(hwnd_);
    }

    //////////////////////////////
    // Initialize the render target.
    {
        ComPtr<IDWriteGdiInterop> gdiInterop;
        IFR(dwriteFactory_->GetGdiInterop(OUT &gdiInterop));


        uint32_t initialWidth = 256;
        uint32_t initialHeight = 256;

        HDC hdc = GetDC(hwnd_);
        hr = ShowMessageIfFailed(
                gdiInterop->CreateBitmapRenderTarget(hdc, initialWidth, initialHeight, OUT &renderTarget_),
                L"Could not create render target! CreateBitmapRenderTarget()" FAILURE_LOCATION
                );

        ReleaseDC(hwnd_, hdc);

        IFR(hr);
    }

    if (g_startBlankList)
    {
        InitializeBlankFontCollection();
    }

    OnMove();
    OnSize(); // update size and reflow

    // Check if it's an older version of DirectWrite, like Windows 7, displaying warning in log.
    ComPtr<IDWriteFactory3> factory3;
    dwriteFactory_->QueryInterface(OUT &factory3);
    if (factory3 == nullptr)
    {
        AppendLog(AppendLogModeImmediate, L"Windows version is older than Windows 10. Application will have very limited functionality.\r\n");
    }

    InitializeLanguageMenu();
    InitializeFontCollectionFilterUI();
    InitializeFontCollectionListUI();
    RebuildFontCollectionList();
    UpdateFontCollectionFilterUI();
    UpdateFontCollectionListUI();

    return hr;
}


MainWindow::~MainWindow()
{
    if (dwriteFactory_ != nullptr)
    {
        //-dwriteFactory_->UnregisterFontFileLoader(RemoteStreamFontFileLoader::GetInstance());
    }
}


WPARAM MainWindow::RunMessageLoop()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        bool messageHandled = false;

        const DWORD style = GetWindowStyle(msg.hwnd);
        HWND dialog = msg.hwnd;

        if (style & WS_CHILD)
            dialog = GetAncestor(msg.hwnd, GA_ROOT);

        // Send the Return key to the right control (one with focus) so that
        // we get a NM_RETURN from that control, not IDOK to the parent window.
        if (!messageHandled && msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN)
        {
            messageHandled = !SendMessage(dialog, msg.message, msg.wParam, msg.lParam);
        }
        if (!messageHandled)
        {
            // Let the default dialog processing check it.
            messageHandled = !!IsDialogMessage(dialog, &msg);
        }
        if (!messageHandled)
        {
            // Not any of the above, so just handle it.
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return msg.wParam;
}


INT_PTR CALLBACK MainWindow::StaticDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MainWindow* window = GetClassFromWindow<MainWindow>(hwnd);
    if (window == nullptr)
    {
        window = new(std::nothrow) MainWindow(hwnd);
        if (window == nullptr)
        {
            return -1; // failed creation
        }

        ::SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)window);
    }

    const DialogProcResult result = window->DialogProc(hwnd, message, wParam, lParam);
    if (result.handled)
    {
        ::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, result.value);
    }
    return result.handled;
}


DialogProcResult CALLBACK MainWindow::DialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Relays messages for the main window to the internal class.

    switch (message)
    {
    case WM_INITDIALOG:
        if (FAILED(Initialize()))
            DestroyWindow(hwnd);
        return DialogProcResult(true, 1); // Let focus be set to first control.

    case WM_COMMAND:
        return OnCommand(hwnd, wParam, lParam);

    case WM_NOTIFY:
        return OnNotification(hwnd, wParam, lParam);

    case WM_INITMENUPOPUP:
        OnMenuPopup(
            reinterpret_cast<HMENU>(wParam),
            static_cast<UINT>(LOWORD(lParam)),
            static_cast<BOOL>(HIWORD(lParam))
            );
        break;

    case WM_SIZE:
        OnSize();
        break;

    case WM_NCDESTROY:
        delete this; // do NOT reference class after this
        PostQuitMessage(0);
        return true;

    case WM_KEYDOWN:
        return DialogProcResult(true, SendMessage(GetFocus(), message, wParam, lParam));

    case WM_DROPFILES:
        return OnDragAndDrop(hwnd, message, wParam, lParam);

    default:
        return false; // unhandled.
    }

    return true;
}


DialogProcResult CALLBACK MainWindow::OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    uint32_t wmId    = LOWORD(wParam);
    uint32_t wmEvent = HIWORD(wParam);
    UNREFERENCED_PARAMETER(wmEvent);

    switch (wmId)
    {
    case IDCANCEL:
    case IDCLOSE:
        {
            DestroyWindow(hwnd); // Do not reference class after this
            PostQuitMessage(0);
        }
        break;

    case IdcSelectViaChooseFont:
        OnChooseFont();
        break;

    case IdcOpenFontFiles:
        OpenFontFiles();
        break;

    case IdcReloadSystemFontSet:
        ReloadSystemFontSet();
        break;

    case IdcDownloadRemoteFonts:
        break;

    case IdcClearRemoteFontCache:
        break;

    case IdcViewRemoteFontCache:
        break;

    case IdcIncludeRemoteFonts:
        includeRemoteFonts_ = !includeRemoteFonts_;
        ResetFontList();
        RebuildFontCollectionList();
        UpdateFontCollectionListUI();
        break;

    case IdcViewSortedFonts:
        wantSortedFontList_ = !wantSortedFontList_;
        RebuildFontCollectionList();
        UpdateFontCollectionListUI();
        break;

    case IdcViewFontPreview:
        showFontPreview_ = !showFontPreview_;
        InvalidateRect(GetDlgItem(hwnd_, IdcFontCollectionList), nullptr, false);
        break;

    case IDOK:
    case IdcActivateFont:
        {
            auto listViewHwnd = GetDlgItem(hwnd_, IdcFontCollectionList);
            int itemIndex = ListView_GetNextItem(listViewHwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
            PushFilter(uint32_t(itemIndex)); // -1 changes to UINT_MAX, but this case is fine.
        }
        break;

    case MenuIdListViewIcon:
    case MenuIdListViewSmallIcon:
    case MenuIdListViewList:
    case MenuIdListViewReport:
    case MenuIdListViewTile:
        {
            HWND listViewHwnd = GetDlgItem(hwnd_, IdcFontCollectionList);
            ListView_SetView(listViewHwnd, wmId - MenuIdListViewFirst);
        }
        break;

    case IdcCopyListNames:
        CopyListTextToClipboard(GetDlgItem(hwnd_, IdcFontCollectionList), L"\t");
        break;

    case MenuIdLanguageFirst + 0:
    case MenuIdLanguageFirst + 2:
    case MenuIdLanguageFirst + 3:
    case MenuIdLanguageFirst + 4:
    case MenuIdLanguageFirst + 5:
    case MenuIdLanguageFirst + 6:
    case MenuIdLanguageFirst + 7:
    case MenuIdLanguageFirst + 8:
    case MenuIdLanguageFirst + 9:
    case MenuIdLanguageFirst + 10:
    case MenuIdLanguageFirst + 11:
    case MenuIdLanguageFirst + 12:
    case MenuIdLanguageFirst + 13:
    case MenuIdLanguageFirst + 14:
    case MenuIdLanguageFirst + 15:
    case MenuIdLanguageFirst + 16:
    case MenuIdLanguageFirst + 17:
    case MenuIdLanguageFirst + 18:
        static_assert(MenuIdLanguageFirst + 18 == MenuIdLanguageLast, "The language count has changed. Update this code to add more cases.");
        currentLanguageIndex_ = wmId - MenuIdLanguageFirst;
        AppendLog(AppendLogModeImmediate, L"Language set to %s\r\n", g_locales[currentLanguageIndex_][0]);
        RebuildFontCollectionList();
        UpdateFontCollectionListUI();
        break;

    case IdcActivateFontCollectionFilter:
        {
            // Get the focused item to see which filter is being activated.
            auto listViewHwnd = GetDlgItem(hwnd_, IdcFontCollectionFilter);
            int itemIndex = ListView_GetNextItem(listViewHwnd, -1, LVNI_FOCUSED|LVNI_SELECTED);

            // If you click on one of the filters, just change the filter mode,
            // else if you click one of the previously pushed filters, pop back to that level.
            if (uint32_t(itemIndex) > fontCollectionFilters_.size())
            {
                auto newFilterMode = FontCollectionFilterMode(itemIndex - 1 - fontCollectionFilters_.size());
                SetCurrentFilter(newFilterMode);
            }
            else
            {
                PopFilter(itemIndex - 1);
            }
        }
        break;

    default:
        return DialogProcResult(false, -1); // unhandled
    }

    return DialogProcResult(true, 0); // handled
}


#ifndef LVN_GETEMPTYTEXTW
#define LVN_GETEMPTYTEXTW          (LVN_FIRST-61) 
#define LVM_RESETEMPTYTEXT         0x1054
#endif

namespace
{
    bool HandleListViewEmptyText(LPARAM lParam, const wchar_t* text)
    {
        LV_DISPINFO& di = *(LV_DISPINFO*)lParam;
        if (di.hdr.code != LVN_GETEMPTYTEXTW)
            return false;

        if (di.item.mask & LVIF_TEXT)
        {  
            StringCchCopyW(
                di.item.pszText,
                di.item.cchTextMax,
                text
                );
            return true;
        }
        return false;
    }


    struct ListViewWriter : LVITEM
    {
    public:
        HWND hwnd;

    public:
        ListViewWriter(HWND listHwnd)
        {
            ZeroStructure(*(LVITEM*)this);
            this->mask = LVIF_TEXT | LVIF_IMAGE; // LVIF_PARAM | LVIF_IMAGE | LVIF_STATE; 
            this->hwnd = listHwnd;
        }

        void InsertItem(wchar_t const* text = L"")
        {
            this->pszText = (LPWSTR)text;
            this->iSubItem = 0;
            ListView_InsertItem(this->hwnd, this);
        }

        void SetItemText(int j, wchar_t const* text)
        {
            this->pszText  = (LPWSTR)text;
            this->iSubItem = j;
            ListView_SetItem(this->hwnd, this);
        }

        void AdvanceItem()
        {
            ++this->iItem;
        }

        void SelectAndFocusItem(int i)
        {
            uint32_t state = (i == -1) ? LVIS_SELECTED : LVIS_FOCUSED|LVIS_SELECTED;
            ListView_SetItemState(this->hwnd, i, state, state);
        }

        void SelectItem(int i)
        {
            uint32_t state = LVIS_SELECTED;
            ListView_SetItemState(this->hwnd, i, state, state);
        }

        void SelectItem()
        {
            SelectItem(this->iItem);
        }

        void FocusItem(int i)
        {
            assert(i != -1);
            uint32_t state = LVIS_FOCUSED;
            ListView_SetItemState(this->hwnd, i, state, state);
        }

        void DisableDrawing()
        {
            SendMessage(this->hwnd, WM_SETREDRAW, 0, 0);
        }

        void EnableDrawing()
        {
            SendMessage(this->hwnd, WM_SETREDRAW, 1, 0);
        }

        void EnsureVisible()
        {
            ListView_EnsureVisible(this->hwnd, this->iItem, false); // scroll down if needed
        }
    };

    struct ColumnInfo
    {
        int attributeType;
        int width;
        const wchar_t* name;
    };

    void InitializeSysListView(
        HWND hwnd,
        __in_ecount(columnInfoCount) const ColumnInfo* columnInfo,
        size_t columnInfoCount
        )
    {
        // Columns
        LVCOLUMN lc;
        lc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        for (size_t i = 0; i < columnInfoCount; ++i)
        {
            lc.iSubItem = columnInfo[i].attributeType;
            lc.cx       = columnInfo[i].width;
            lc.pszText  = (LPWSTR)columnInfo[i].name;
            ListView_InsertColumn(hwnd, lc.iSubItem, &lc);
        }
    }
}


DialogProcResult CALLBACK MainWindow::OnNotification(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    int idCtrl = (int)wParam;
    NMHDR const& nmh = *(NMHDR*)lParam;

    switch (idCtrl)
    {
    case IdcFontCollectionList:
        switch (nmh.code)
        {
        case LVN_ODSTATECHANGED:
        case LVN_ITEMCHANGED:
            break;

        case NM_DBLCLK:
        case NM_RETURN:
            // Activate if Enter pressed.
            OnCommand(hwnd, IdcActivateFont, (LPARAM)nmh.hwndFrom);
            break;

        case LVN_KEYDOWN:
            // Pop one filter if backspace is pressed.
            if (((NMLVKEYDOWN*)lParam)->wVKey == VK_BACK)
            {
                PopFilter(static_cast<uint32_t>(fontCollectionFilters_.size() - 1));
            }
            break;

        case LVN_GETEMPTYTEXTW:
            HandleListViewEmptyText(lParam, L"No visible font properties. Choose a different property or load a new font set.");
            return DialogProcResult(true, 1);

        case NM_CUSTOMDRAW:
            {
                auto customDraw = (NMLVCUSTOMDRAW*) lParam;

                switch (customDraw->nmcd.dwDrawStage)
                {
                case CDDS_PREPAINT:
                    return DialogProcResult(true, CDRF_NOTIFYITEMDRAW);

                case CDDS_ITEMPOSTPAINT:
                    if (DrawFontCollectionIconPreview(customDraw) != S_OK)
                        return DialogProcResult(true, CDRF_DODEFAULT);

                    return DialogProcResult(true, CDRF_DODEFAULT);

                case CDDS_ITEMPREPAINT:
                    // Why does it pass empty rects?
                    if (customDraw->nmcd.rc.bottom <= 0)
                        return DialogProcResult(true, CDRF_DODEFAULT);
                    return DialogProcResult(true, CDRF_DODEFAULT | CDRF_NOTIFYPOSTPAINT);

                default:
                    return DialogProcResult(true, CDRF_DODEFAULT);
                }
            }
            break;

        default:
            return false;
        }
        break;

    case IdcFontCollectionFilter:
        switch (nmh.code)
        {
        case LVN_KEYDOWN:
        case NM_RETURN:
            // Activate if Return or Space pressed.
            if (nmh.code == NM_RETURN || ((NMLVKEYDOWN*)lParam)->wVKey == VK_SPACE)
            {
                OnCommand(hwnd, IdcActivateFontCollectionFilter, (LPARAM)nmh.hwndFrom);
                ListViewWriter lw(GetDlgItem(hwnd_, IdcFontCollectionList));
                lw.SelectAndFocusItem(0);
                if (nmh.code == NM_RETURN)
                    SetFocus(lw.hwnd);
            }
            break;

        case NM_CLICK:
            // Use NM_CLICK rather than LVN_ITEMACTIVATE because the control
            // delays an annoying 1/3 second before actually sending the message.
            PostMessage(hwnd, WM_COMMAND, IdcActivateFontCollectionFilter, (LPARAM)nmh.hwndFrom);
            break;

        case NM_CUSTOMDRAW:
            {
                auto customDraw = (NMLVCUSTOMDRAW*) lParam;

                switch (customDraw->nmcd.dwDrawStage)
                {
                case CDDS_PREPAINT:
                    return DialogProcResult(true, CDRF_NOTIFYITEMDRAW);

                case CDDS_ITEMPREPAINT:
                    // Why does it pass empty rects?
                    if (customDraw->nmcd.dwItemSpec == (fontCollectionFilters_.size() + 1 + int(filterMode_)))
                    {
                        customDraw->clrTextBk = faintSelectionColor_ & 0x00FFFFFF;
                    }
                    return DialogProcResult(true, CDRF_DODEFAULT);

                default:
                    return DialogProcResult(true, CDRF_DODEFAULT);
                }
            }
            break;

        //case LVN_ODSTATECHANGED:
        //case LVN_ITEMCHANGED:
        //    break;

        default:
            return false;
        }
        break;

    default:
        return false;

    } // switch idCtrl

    return true;
}


DialogProcResult CALLBACK MainWindow::OnDragAndDrop(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    std::wstring fileName, fileNames;
    HDROP hDrop = (HDROP)wParam;
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    // Get the number of files being dropped.
    const uint32_t fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);

    auto deferredCleanup = DeferCleanup([&]() { DragFinish(hDrop); });

    // Accumulate all the filenames into a list to load.
    for (uint32_t i = 0; i < fileCount; ++i)
    {
        const uint32_t fileNameSize = DragQueryFile(hDrop, i, nullptr, 0);
        if (fileNameSize == 0)
            continue;

        fileName.resize(fileNameSize);
        array_ref<wchar_t> fileNameRef(fileName);
        if (DragQueryFile(hDrop, i, fileNameRef.data(), fileNameSize + 1))
        {
            fileNames.append(fileName);
            fileNames.push_back('\0');
        }
    }

    PopFilter(0); // Confusing to leave filters which likely don't apply to the new fonts. So clear them.
    IFR(RebuildFontCollectionListFromFileNames(/*baseFilePath*/ nullptr, fileNames));
    UpdateFontCollectionListUI();

    return DialogProcResult(/*handled*/ true, /*value*/ 0);
}


void AppendTag(_Inout_ std::wstring& s, uint32_t tag)
{
    auto* tagBegin = reinterpret_cast<char const*>(&tag);
    for (char ch : std::initializer_list<char>(tagBegin, tagBegin + 4))
    {
        s += ch;
    }
}


HRESULT MainWindow::DrawFontCollectionIconPreview(const NMLVCUSTOMDRAW* customDraw)
{
    if (customDraw->nmcd.rc.bottom <= 0)
        return S_FALSE;

    const uint32_t itemIndex = static_cast<uint32_t>(customDraw->nmcd.dwItemSpec);

    // Get the icon area rect.
    RECT rect = { 0, 0, 256, 256 };
    int iconWidth = 32, iconHeight = 32;
    ListView_GetItemRect(customDraw->nmcd.hdr.hwndFrom, itemIndex, OUT &rect, LVIR_ICON);
    HIMAGELIST imageList = ListView_GetImageList(customDraw->nmcd.hdr.hwndFrom, LVSIL_NORMAL);
    ImageList_GetIconSize(imageList, OUT &iconWidth, OUT &iconHeight);

    const int iconAreaWidth = rect.right - rect.left, iconAreaHeight = rect.bottom - rect.top;
    const int minimumIconSize = 16;
    const int iconPadding = 6;
    const int layoutWidth  = std::min(std::max(minimumIconSize, iconWidth  - iconPadding * 2), int(rect.right - rect.left));
    const int layoutHeight = std::min(std::max(minimumIconSize, iconHeight - iconPadding * 2), int(rect.bottom - rect.top));

    if (layoutWidth <= 16 || layoutHeight <= 16)
        return S_FALSE;

    // Get the item text and icon index.
    wchar_t textBuffer[256]; textBuffer[0] = '\0';
    LVITEM listViewItem;
    ZeroMemory(OUT &listViewItem, sizeof(listViewItem));
    listViewItem.mask  = LVIF_TEXT | LVIF_IMAGE;
    listViewItem.pszText = textBuffer;
    listViewItem.cchTextMax = ARRAYSIZE(textBuffer);
    listViewItem.iItem = itemIndex;
    ListView_GetItem(customDraw->nmcd.hdr.hwndFrom, OUT &listViewItem);

    // Transfer bitmap into temporary bitmap.
    BitBlt(renderTarget_->GetMemoryDC(), 0,0, rect.right - rect.left, rect.bottom - rect.top, customDraw->nmcd.hdc, rect.left, rect.top, SRCCOPY|NOMIRRORBITMAP);

    // Append details to the main text.
    std::wstring text(textBuffer, wcslen(textBuffer));
    uint32_t const mainTextLength = static_cast<uint32_t>(text.size());

    if (itemIndex < fontCollectionList_.size())
    {
        FontCollectionEntry& f = fontCollectionList_[itemIndex];
        auto& fontAxisValues = f.fontAxisValues;
        auto& fontAxisRanges = f.fontAxisRanges;

        wchar_t numericBuffer[32];
        text += L"\r\n\r\n";
        for (auto& fontAxisValue : fontAxisValues)
        {
            AppendTag(IN OUT text, fontAxisValue.axisTag);
            swprintf_s(numericBuffer, L":%0.5g ", fontAxisValue.value);
            text += numericBuffer;
        }
        if (f.fontSimulations != DWRITE_FONT_SIMULATIONS_NONE)
        {
            swprintf_s(numericBuffer, L"sims:%d", f.fontSimulations);
            text += numericBuffer;
        }
        text += L"\r\n";
        for (auto& fontAxisRange : fontAxisRanges)
        {
            AppendTag(IN OUT text, fontAxisRange.axisTag);
            swprintf_s(numericBuffer, L":%0.5g", fontAxisRange.minValue);
            text += numericBuffer;
            if (fontAxisRange.maxValue != fontAxisRange.minValue)
            {
                swprintf_s(numericBuffer, L"..%0.5g", fontAxisRange.maxValue);
                text += numericBuffer;
            }
            text.push_back(' ');
        }

        text += L"\r\n";
        text += f.filePath;

        if (f.fontFaceIndex > 0)
        {
            text.append(L" #", 2);
            text += std::to_wstring(f.fontFaceIndex);
        }
    }

    ComPtr<IDWriteTextFormat> textFormat;
    IFR(dwriteFactory_->CreateTextFormat(
        L"Segoe UI",
        fontCollection_,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        20.0f,
        L"",
        OUT &textFormat
    ));
    textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create the text layout to draw.
    ComPtr<IDWriteTextLayout> textLayout;
    IFR(dwriteFactory_->CreateTextLayout(
        text.c_str(),
        static_cast<uint32_t>(text.size()),
        textFormat,
        float(layoutWidth),
        float(layoutHeight),
        OUT &textLayout
        ));

    auto const mainTextRange = DWRITE_TEXT_RANGE{ 0, mainTextLength };
    auto const entireTextRange = DWRITE_TEXT_RANGE{ 0, UINT32_MAX };

    // Set font preview parameters.
    if (showFontPreview_ && itemIndex < fontCollectionList_.size())
    {
        textLayout->SetFontCollection(fontCollection_, entireTextRange);

        FontCollectionEntry& f = fontCollectionList_[itemIndex];
        textLayout->SetFontFamilyName(f.familyName.c_str(), mainTextRange);
        textLayout->SetFontWeight(f.fontWeight, mainTextRange);
        textLayout->SetFontStyle(f.fontStyle, mainTextRange);
        textLayout->SetFontStretch(f.fontStretch, mainTextRange);

        ComPtr<IDWriteTextLayout4> textLayout4;
        textLayout->QueryInterface(OUT &textLayout4);
        if (textLayout4 != nullptr)
        {
            auto& fontAxisValues = f.fontAxisValues;
            textLayout4->SetFontAxisValues(fontAxisValues.data(), static_cast<uint32_t>(fontAxisValues.size()), mainTextRange);
        }
    }

    // Make details smaller than main text.
    textLayout->SetFontSize(9, { mainTextLength, UINT32_MAX - mainTextLength });

    const float folderXShift = float(iconAreaWidth / 24);
    const float folderYShift = float(iconAreaHeight / -16);
    const float layoutLeft = ((iconAreaWidth  - layoutWidth)  / 2.0f) + (listViewItem.iImage == 0 ? 0.0f : folderXShift);
    const float layoutTop  = ((iconAreaHeight - layoutHeight) / 2.0f) + (listViewItem.iImage == 0 ? 0.0f : folderYShift);
    DrawTextLayout(renderTarget_, renderingParams_, textLayout, layoutLeft, layoutTop);

    // Copy bitmap back to screen.
    BitBlt(customDraw->nmcd.hdc, rect.left, rect.top, iconAreaWidth, iconAreaHeight, renderTarget_->GetMemoryDC(), 0, 0, SRCCOPY|NOMIRRORBITMAP);

    return S_OK;
}


void MainWindow::OnMenuPopup(
    HMENU menu,
    UINT position,
    BOOL isSystemWindowMenu
    )
{
    if (isSystemWindowMenu)
        return;

    // Get the menu id. Use GetMenuItemInfo instead of GetMenuItemID,
    // since the latter doesn't work for submenus.
    HMENU mainMenu = GetMenu(hwnd_);
    MENUITEMINFO menuItemInfo;
    menuItemInfo.cbSize = sizeof(menuItemInfo);
    menuItemInfo.fMask = MIIM_ID | MIIM_STATE | MIIM_SUBMENU;
    if (!GetMenuItemInfo(mainMenu, position, true, OUT &menuItemInfo))
        return;

    switch (menuItemInfo.wID)
    {
    case MenuIdOptions:
        CheckMenuItem(
            menu,
            IdcIncludeRemoteFonts,
            MF_BYCOMMAND | (includeRemoteFonts_ ? MF_CHECKED : MF_UNCHECKED)
            );
        CheckMenuItem(
            menu,
            IdcViewSortedFonts,
            MF_BYCOMMAND | (wantSortedFontList_ ? MF_CHECKED : MF_UNCHECKED)
            );
        CheckMenuItem(
            menu,
            IdcViewFontPreview,
            MF_BYCOMMAND | (showFontPreview_ ? MF_CHECKED : MF_UNCHECKED)
        );
        
        break;

    case MenuIdListView:
        {
            auto listViewHwnd = GetDlgItem(hwnd_, IdcFontCollectionList);
            uint32_t view = ListView_GetView(listViewHwnd);
            CheckMenuRadioItem(
                menu,
                MenuIdListViewFirst,
                MenuIdListViewLast,
                MenuIdListViewFirst + view,
                MF_BYCOMMAND
                );
        }
        break;

    case MenuIdLanguage:
        {
            CheckMenuRadioItem(
                menu,
                MenuIdLanguageFirst,
                MenuIdLanguageLast,
                MenuIdLanguageFirst + currentLanguageIndex_,
                MF_BYCOMMAND
                );
        }
        break;
    }
}


void MainWindow::OnKeyDown(UINT keyCode)
{
    // Handles menu commands.

    bool const heldShift     = (GetKeyState(VK_SHIFT)   & 0x80) != 0;
    bool const heldControl   = (GetKeyState(VK_CONTROL) & 0x80) != 0;
    bool const heldAlternate = (GetKeyState(VK_MENU)    & 0x80) != 0;


    switch (keyCode)
    {
    case VK_BACK:
        break;

    case VK_HOME:
        break;

    case VK_PRIOR:
        break;

    case VK_NEXT:
        break;

    case VK_UP:
        break;

    case VK_DOWN:
        break;
    }
}


void MainWindow::MarkNeedsRepaint()
{
    InvalidateRect(hwnd_, nullptr, false);
}


void MainWindow::OnSize()
{
    // Resizes the render target and flow source.

    HWND hwnd = hwnd_;
    long const spacing = 4;
    RECT clientRect;

    GetClientRect(hwnd, OUT &clientRect);
    RECT paddedClientRect = clientRect;
    InflateRect(IN OUT &paddedClientRect, -spacing, -spacing);

    const size_t filterIndex = 0;
    const size_t logIndex = 2;
    const long mainWindowThirdWidth = paddedClientRect.right / 3;
    const long mainWindowQuarterHeight = paddedClientRect.bottom / 4;
    WindowPosition windowPositions[] = {
        //WindowPosition(GetDlgItem(hwnd, IdcTags), PositionOptionsFillHeight | PositionOptionsAlignTop),
        WindowPosition(GetDlgItem(hwnd, IdcFontCollectionFilter), PositionOptionsFillHeight),
        WindowPosition(GetDlgItem(hwnd, IdcFontCollectionList), PositionOptionsFillWidth | PositionOptionsFillHeight),
        WindowPosition(GetDlgItem(hwnd, IdcLog), PositionOptionsFillWidth | PositionOptionsAlignTop | PositionOptionsNewLine),
    };
    // Filter list
    windowPositions[filterIndex].rect.left = 0;
    windowPositions[filterIndex].rect.right = std::min(mainWindowThirdWidth, 200l);
    // Log
    windowPositions[logIndex].rect.top = 0;
    windowPositions[logIndex].rect.bottom = std::min(mainWindowQuarterHeight, 100l);

    WindowPosition::ReflowGrid(windowPositions, ARRAY_SIZE(windowPositions), paddedClientRect, spacing, 0, PositionOptionsFlowHorizontal | PositionOptionsUnwrapped);
    WindowPosition::Update(windowPositions, ARRAY_SIZE(windowPositions));
}


void MainWindow::OnMove()
{
    // Updates rendering parameters according to current monitor.

    if (dwriteFactory_ == nullptr)
        return; // Not initialized yet.

    HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
    if (monitor == hmonitor_)
        return; // Still on previous monitor.

    // Create rendering params to get the defaults for the various parameters.
    ComPtr<IDWriteRenderingParams> renderingParams;
    dwriteFactory_->CreateMonitorRenderingParams(
                    MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST),
                    &renderingParams
                    );

    // Now create our own with the desired rendering mode.
    if (renderingParams != nullptr)
    {
        renderingParams_.Clear();
        dwriteFactory_->CreateCustomRenderingParams(
            renderingParams->GetGamma(),
            renderingParams->GetEnhancedContrast(),
            renderingParams->GetClearTypeLevel(),
            renderingParams->GetPixelGeometry(),
            DWRITE_RENDERING_MODE_DEFAULT,
            &renderingParams_
            );
    }

    if (renderingParams_ == nullptr)
        return;

    hmonitor_ = monitor;
    MarkNeedsRepaint();
}


namespace
{
    float const g_defaultFontSize = 20;
    bool const g_defaultHasUnderline = false;
    bool const g_defaultHasStrikethrough = false;
    DWRITE_FONT_WEIGHT const g_defaultFontWeight = DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STRETCH const g_defaultFontStretch = DWRITE_FONT_STRETCH_NORMAL;
    DWRITE_FONT_STYLE const g_defaultFontSlope = DWRITE_FONT_STYLE_NORMAL;
    wchar_t g_ActualFaceName[LF_FACESIZE];
    float g_ActualFontSize = 0;
    HWND g_chooseFontParent = nullptr;
}


UINT_PTR CALLBACK ChooseFontHookProc(
    HWND dialogHandle,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    // This callback retrieves the actual parameters from the user.

    enum {
        FontNameComboBoxId = 0x0470,
        FontSizeComboBoxId = 0x0472,
    };

    wchar_t fontSizeStringBuffer[100];

    if (message == WM_INITDIALOG)
    {
        CHOOSEFONT* chooseFont = reinterpret_cast<CHOOSEFONT*>(lParam);

        g_chooseFontParent = chooseFont->hwndOwner;

        // Correct the font size.
        HWND editHandle = GetDlgItem(dialogHandle, FontSizeComboBoxId);
        swprintf_s(OUT fontSizeStringBuffer, ARRAYSIZE(fontSizeStringBuffer), L"%0.1f", float(chooseFont->iPointSize) / 10.0f);
        Edit_SetText(editHandle, fontSizeStringBuffer);

        return true;
    }

    if (message == WM_COMMAND && HIWORD(wParam) == BN_CLICKED)
    {
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == WM_CHOOSEFONT_GETLOGFONT + 1)
        {
            // Get the font name which the user actually typed, not a reprocessed
            // or substituted one. This is needed for typing font names that either
            // are unlisted (user hid them), don't match DWrite's naming model,
            // or have localized names (which do not appear in the box).
            HWND editHandle = GetDlgItem(dialogHandle, FontNameComboBoxId);
            if (editHandle != nullptr)
            {
                // Ignore the result from ChooseFont() and keep track of
                // the actual font name typed into the family name combo.
                Edit_GetText(editHandle, OUT g_ActualFaceName, ARRAYSIZE(g_ActualFaceName));
            }

            // Get the true font size, since we want the original value rather than
            // a premultiplied and truncated value.
            editHandle = GetDlgItem(dialogHandle, FontSizeComboBoxId);
            if (editHandle != nullptr)
            {
                Edit_GetText(editHandle, OUT fontSizeStringBuffer, ARRAYSIZE(fontSizeStringBuffer));
                g_ActualFontSize = float(_wtof(fontSizeStringBuffer));

                // Set a whole integer before returning, since the dialog otherwise
                // complains about decimal values.
                swprintf_s(OUT fontSizeStringBuffer, ARRAYSIZE(fontSizeStringBuffer), L"%d", int(g_ActualFontSize));
                Edit_SetText(editHandle, fontSizeStringBuffer);
            }

            LOGFONT logFont = {};
            SendMessage(dialogHandle, WM_CHOOSEFONT_GETLOGFONT, 0, (LPARAM)&logFont);
            MainWindow* window = reinterpret_cast<MainWindow*>(GetWindowLongPtr(g_chooseFontParent, GWLP_USERDATA));
            if (window != nullptr)
            {
                window->ApplyLogFontFilter(logFont);
            }
        }
    }

    return 0;
}


STDMETHODIMP MainWindow::OnChooseFont()
{
    LOGFONT logFont = {};
    logFont.lfHeight            = -static_cast<LONG>(floor(g_defaultFontSize));
    logFont.lfWidth             = 0;
    logFont.lfEscapement        = 0;
    logFont.lfOrientation       = 0;
    logFont.lfWeight            = g_defaultFontWeight;
    logFont.lfItalic            = (g_defaultFontSlope > DWRITE_FONT_STYLE_NORMAL);
    logFont.lfUnderline         = static_cast<BYTE>(g_defaultHasUnderline);
    logFont.lfStrikeOut         = static_cast<BYTE>(g_defaultHasStrikethrough);
    logFont.lfCharSet           = DEFAULT_CHARSET;
    logFont.lfOutPrecision      = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision     = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality           = DEFAULT_QUALITY;
    logFont.lfPitchAndFamily    = DEFAULT_PITCH;
    wcsncpy_s(logFont.lfFaceName, L"Segoe UI", ARRAYSIZE(logFont.lfFaceName));
    wcsncpy_s(g_ActualFaceName, L"Segoe UI", ARRAYSIZE(g_ActualFaceName));

    //////////////////////////////
    // Initialize CHOOSEFONT for the dialog.

    #ifndef CF_INACTIVEFONTS
    #define CF_INACTIVEFONTS 0x02000000L
    #endif
    CHOOSEFONT chooseFont   = {};
    chooseFont.lpfnHook     = &ChooseFontHookProc;
    chooseFont.lStructSize  = sizeof(chooseFont);
    chooseFont.hwndOwner    = hwnd_;
    chooseFont.lpLogFont    = &logFont;
    chooseFont.iPointSize   = INT(g_defaultFontSize * 10 + .5f);
    chooseFont.rgbColors    = 0;
    chooseFont.Flags        = CF_SCREENFONTS
                            | CF_PRINTERFONTS
                            | CF_INACTIVEFONTS  // Don't hide fonts
                            | CF_NOVERTFONTS
                            | CF_NOSCRIPTSEL
                            | CF_INITTOLOGFONTSTRUCT
                            | CF_ENABLEHOOK
                            | CF_APPLY
                            ;

    // Don't show bitmap fonts because DirectWrite doesn't support them.

    // Show the common font dialog box.
    if (!ChooseFont(IN OUT &chooseFont))
    {
        // Check whether error or the user canceled.
        return CommDlgExtendedError() == 0 ? S_FALSE : E_FAIL;
    }

    // We don't need to call ApplyLogFontFilter
    // because the ChooseFontHookProc callback already did.
    return S_OK;
}


STDMETHODIMP MainWindow::OpenFontFiles()
{
    // Allocate sizeable buffer for file names, since the user can multi-select.
    // This should be enough for some of those large directories of 3000 names
    // of typical filename length.
    std::wstring fileNames(65536, '\0');

    // Initialize OPENFILENAME
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFile = &fileNames[0];
    ofn.nMaxFile = fileNames.size() - 1;
    ofn.lpstrFilter =
        L"Fonts\0*.ttf;*.ttc;*.otf;*.otc;*.tte\0"
        L"TrueType (ttf)\0*.ttf\0"
        L"TrueType Collection (ttc)\0*.ttc\0"
        L"OpenType (ttf)\0*.otf\0"
        L"OpenType Collection (ttc)\0*.otc\0"
        L"TrueType EUDC (tte)\0*.tte\0"
        L"All\0*.*\0"
        ;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrTitle = L"Open font file(s) - can select multiple";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_NOTESTFILECREATE;

    if (!GetOpenFileName(&ofn))
    {
        return S_FALSE;
    }

    // Explicitly separate the preceding path from filename for consistency
    // since the dialog returns things differently for one filename vs multiple.
    if (ofn.nFileOffset > 0)
    {
        fileNames[ofn.nFileOffset - 1] = '\0';
    }

    PopFilter(0); // Confusing to leave filters which likely don't apply to the new fonts. So clear them.
    IFR(RebuildFontCollectionListFromFileNames(
        fileNames.data(),
        { &fileNames[ofn.nFileOffset], fileNames.size() - ofn.nFileOffset }
        ));
    UpdateFontCollectionListUI();

    return S_OK;
}


STDMETHODIMP MainWindow::RebuildFontCollectionListFromFileNames(_In_opt_z_ wchar_t const* baseFilePath, array_ref<wchar_t const> fileNames)
{
    ComPtr<IDWriteFactory5> factory5;
    dwriteFactory_->QueryInterface(OUT &factory5);
    if (factory5 == nullptr)
    {
        AppendLog(AppendLogModeMessageBox, L"This function requires the Creators Update Windows 10");
        return E_NOTIMPL;
    }

    ResetFontList();
    fontSet_.clear();
    fontCollection_.clear();
    std::wstring failedFileNames;

    IFR(CreateFontSetFromFileNames(
        factory5,
        baseFilePath,
        fileNames.data(), // fontFileNames
        static_cast<uint32_t>(fileNames.size()), // fontFileNamesCharCount,
        OUT &fontSet_,
        OUT failedFileNames
        ));
    IFR(factory5->CreateFontCollectionFromFontSet(fontSet_, OUT reinterpret_cast<IDWriteFontCollection1**>(&fontCollection_)));

    RebuildFontCollectionList();

    if (!failedFileNames.empty())
    {
        std::replace(failedFileNames.begin(), failedFileNames.end(), '\0', '\r');
        AppendLog(AppendLogModeMessageBox, L"Failed to load files:\r\n%s\r\n", failedFileNames.c_str());
    }

    if (fontSet_->GetFontCount() > 0)
    {
        std::wstring fullName, wwsFamilyName, wwsFaceName, typographicFamilyName, typographicFaceName, postscriptName, win32FamilyName, designScripts;

        GetLocalizedString(fontSet_, 0, DWRITE_FONT_PROPERTY_ID_FULL_NAME, L"en-us", fullName);
        GetLocalizedString(fontSet_, 0, DWRITE_FONT_PROPERTY_ID_TYPOGRAPHIC_FAMILY_NAME, L"en-us", typographicFamilyName);
        GetLocalizedString(fontSet_, 0, DWRITE_FONT_PROPERTY_ID_TYPOGRAPHIC_FACE_NAME, L"en-us", typographicFaceName);
        GetLocalizedString(fontSet_, 0, DWRITE_FONT_PROPERTY_ID_WEIGHT_STRETCH_STYLE_FAMILY_NAME, L"en-us", wwsFamilyName);
        GetLocalizedString(fontSet_, 0, DWRITE_FONT_PROPERTY_ID_WEIGHT_STRETCH_STYLE_FACE_NAME, L"en-us", wwsFaceName);
        GetLocalizedString(fontSet_, 0, DWRITE_FONT_PROPERTY_ID_POSTSCRIPT_NAME, L"en-us", postscriptName);
        GetLocalizedString(fontSet_, 0, DWRITE_FONT_PROPERTY_ID_WIN32_FAMILY_NAME, L"en-us", win32FamilyName);
        GetLocalizedString(fontSet_, 0, DWRITE_FONT_PROPERTY_ID_DESIGN_SCRIPT_LANGUAGE_TAG, L"en-us", designScripts);
        AppendLog(
            AppendLogModeImmediate,
            L"Full: %s\r\n"
            L"Typographic: %s / %s\r\n"
            L"Wws: %s / %s\r\n"
            L"Postscript: %s\r\n"
            L"Win32: %s\r\n"
            L"Script: %s\r\n",
            fullName.c_str(),
            typographicFamilyName.c_str(),
            typographicFaceName.c_str(),
            wwsFamilyName.c_str(),
            wwsFaceName.c_str(),
            postscriptName.c_str(),
            win32FamilyName.c_str(),
            designScripts.c_str()
        );
    }

    return S_OK;
}


STDMETHODIMP MainWindow::ReloadSystemFontSet()
{
    ResetFontList();
    RebuildFontCollectionList();
    UpdateFontCollectionListUI();

    return S_OK;
}


STDMETHODIMP MainWindow::ChooseColor(IN OUT uint32_t& color)
{
    static COLORREF customColors[16] = {};
    CHOOSECOLOR colorOptions = {};
    colorOptions.lStructSize = sizeof(colorOptions);
    colorOptions.hwndOwner = hwnd_;
    colorOptions.rgbResult = color & 0x00FFFFFF;
    colorOptions.lpCustColors = customColors;
    colorOptions.Flags = CC_ANYCOLOR|CC_FULLOPEN|CC_RGBINIT;

    if (!::ChooseColor(&colorOptions))
    {
        // Check whether error or the user canceled.
        return CommDlgExtendedError() == 0 ? S_FALSE : E_FAIL;
    }

    color = 0xFF000000 | colorOptions.rgbResult;
    return S_OK;
}


STDMETHODIMP MainWindow::ApplyLogFontFilter(const LOGFONT& logFont)
{
    HRESULT hr = S_OK;

    float fontSize = g_ActualFontSize;
    DWRITE_FONT_WEIGHT fontWeight = g_defaultFontWeight;
    DWRITE_FONT_STRETCH fontStretch = g_defaultFontStretch;
    DWRITE_FONT_STYLE fontSlope = g_defaultFontSlope;

    // Abort if the user didn't select a face name.
    if (g_ActualFaceName[0] == L'\0')
    {
        return S_FALSE;
    }

    // Get the actual family name and stretch.
    {
        ComPtr<IDWriteGdiInterop> gdiInterop;
        ComPtr<IDWriteFont> tempFont;

        dwriteFactory_->GetGdiInterop(OUT &gdiInterop);
        if (gdiInterop != nullptr)
        {
            gdiInterop->CreateFontFromLOGFONT(&logFont, OUT &tempFont);
            if (tempFont != nullptr)
            {
                fontStretch = tempFont->GetStretch();
            }
        }
    }

    fontWeight = static_cast<DWRITE_FONT_WEIGHT>(logFont.lfWeight);
    fontSlope = logFont.lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

    return hr;
}


HRESULT MainWindow::InitializeFontCollectionListUI()
{
    auto listViewHwnd = GetDlgItem(hwnd_, IdcFontCollectionList);

    // Add the single column. Otherwise nothing shows up in list view (nothing at all!).
    const static ColumnInfo columnInfo[] = {
        { 0, 400, L"Name" },
    };
    ListView_SetExtendedListViewStyle(listViewHwnd, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP);
    InitializeSysListView(listViewHwnd, columnInfo, ARRAYSIZE(columnInfo));

    // Set the image list to use, small and large.
    HIMAGELIST fontCollectionImageList = ImageList_Create(
        96 * 2, // cx
        96 * 2, // cy
        ILC_COLOR32, // flags
        2, // cInitial
        2  // cGrow
        );
    HIMAGELIST fontCollectionImageListSmall = ImageList_Create(
        16, // cx
        16, // cy
        ILC_COLOR32, // flags
        2, // cInitial
        2  // cGrow
        );

    HICON fontBlankIcon = nullptr;
    HICON folderBlankIcon = nullptr;
    LoadIconWithScaleDown(HINST_THISCOMPONENT, MAKEINTRESOURCE(IdiFontBlank), 96 * 2, 96 * 2, OUT &fontBlankIcon);
    LoadIconWithScaleDown(HINST_THISCOMPONENT, MAKEINTRESOURCE(IdiFamilyBlank), 96 * 2, 96 * 2, OUT &folderBlankIcon);
    ImageList_AddIcon(fontCollectionImageList, fontBlankIcon);
    ImageList_AddIcon(fontCollectionImageList, folderBlankIcon);
    ImageList_AddIcon(fontCollectionImageListSmall, fontBlankIcon);
    ImageList_AddIcon(fontCollectionImageListSmall, folderBlankIcon);

    ListView_SetImageList(listViewHwnd, fontCollectionImageList, LVSIL_NORMAL);
    ListView_SetImageList(listViewHwnd, fontCollectionImageListSmall, LVSIL_SMALL);

    return S_OK;
}


HRESULT MainWindow::InitializeFontCollectionFilterUI()
{
    // Add the single column. Otherwise nothing shows up in list view (nothing at all!).
    const static ColumnInfo columnInfo[] = {
        { 0, 196, L"Name" },
        };
    auto listViewHwnd = GetDlgItem(hwnd_, IdcFontCollectionFilter);
    ListView_SetExtendedListViewStyle(listViewHwnd, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_ONECLICKACTIVATE | LVS_EX_UNDERLINEHOT);
    InitializeSysListView(listViewHwnd, columnInfo, ARRAYSIZE(columnInfo));

    HIMAGELIST fontFilterImageListSmall = ImageList_Create(
        16, // cx
        16, // cy
        ILC_COLOR32, // flags
        2, // cInitial
        2  // cGrow
        );

    //HBITMAP bitmap = LoadBitmap(HINST_THISCOMPONENT, MAKEINTRESOURCE(IdbFontCollectionFilter));
    HBITMAP bitmap = (HBITMAP)LoadImage(HINST_THISCOMPONENT, MAKEINTRESOURCE(IdbFontCollectionFilter), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE|LR_LOADTRANSPARENT);//LR_CREATEDIBSECTION

    ImageList_Add(fontFilterImageListSmall, bitmap, nullptr);
    ListView_SetImageList(listViewHwnd, fontFilterImageListSmall, LVSIL_SMALL);

    return S_OK;
}


HRESULT MainWindow::UpdateFontCollectionListUI(uint32_t newSelectedItem)
{
    ListViewWriter lw(GetDlgItem(hwnd_, IdcFontCollectionList));

    lw.DisableDrawing();
    ListView_DeleteAllItems(lw.hwnd);

    for (auto const& f : fontCollectionList_)
    {
        lw.iImage = (f.fontCount > 1) ? 1 : 0;
        lw.InsertItem(f.name.c_str());
        lw.AdvanceItem();
    }

    // Select the first item, and rescroll things back into view after updating the list UI.
    ListView_SetItemState(lw.hwnd, newSelectedItem, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

    lw.EnableDrawing();

    ListView_EnsureVisible(lw.hwnd, newSelectedItem, /*partialOK*/false);

    return S_OK;
}


HRESULT MainWindow::UpdateFontCollectionFilterUI()
{
    ListViewWriter lw(GetDlgItem(hwnd_, IdcFontCollectionFilter));

    lw.DisableDrawing();
    ListView_DeleteAllItems(lw.hwnd);

    lw.mask |= LVIF_INDENT;
    lw.InsertItem(L"Font set root");
    lw.AdvanceItem();
    lw.iIndent++;

    std::wstring text;
    for (auto const& f : fontCollectionFilters_)
    {
        GetFormattedString(IN OUT text, L"%s: %s", g_fontCollectionFilterModeNames[int(f.mode)], f.parameter.c_str());
        lw.iImage = 1;
        lw.InsertItem(text.c_str());
        lw.AdvanceItem();
    }
    lw.iIndent++;
    for (auto const& f : g_fontCollectionFilterModeNames)
    {
        // lw.iIndent resumes from the previous loop and stays the same.
        lw.iImage = 2;
        lw.InsertItem(f);
        lw.AdvanceItem();
    }

    // Set focus to the restored filter mode.
    int newItemIndex = static_cast<uint32_t>(fontCollectionFilters_.size() + 1 + unsigned int(filterMode_));
    ListView_SetItemState(lw.hwnd, newItemIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

    lw.EnableDrawing();

    return S_OK;
}


bool IsLanguageAgnosticFilterMode(MainWindow::FontCollectionFilterMode filterMode)
{
    switch (filterMode)
    {
    case MainWindow::FontCollectionFilterMode::DesignedScriptTag:
    case MainWindow::FontCollectionFilterMode::SupportedScriptTag:
        return true;
    }
    return false;
}


DWRITE_FONT_PROPERTY_ID FilterModeToPropertyId(MainWindow::FontCollectionFilterMode filterMode)
{
    if (filterMode >= MainWindow::FontCollectionFilterMode::Total)
    {
        return DWRITE_FONT_PROPERTY_ID_TYPOGRAPHIC_FAMILY_NAME;
    }
    if (filterMode == MainWindow::FontCollectionFilterMode::None)
    {
        return DWRITE_FONT_PROPERTY_ID_FULL_NAME;
    }

    return DWRITE_FONT_PROPERTY_ID(static_cast<uint32_t>(filterMode));
}


void MainWindow::ResetFontList()
{
    fontSet_.clear();
    fontCollection_.clear();
}


HRESULT MainWindow::InitializeBlankFontCollection()
{
    ResetFontList();

    ComPtr<IDWriteFactory3> dwriteFactory3;
    dwriteFactory_->QueryInterface(OUT &dwriteFactory3);

    if (dwriteFactory3 != nullptr)
    {
        ComPtr<IDWriteFontSet> newFontSet;
        ComPtr<IDWriteFontSetBuilder> fontSetBuilder;
        IFR(dwriteFactory3->CreateFontSetBuilder(OUT &fontSetBuilder));
        IFR(fontSetBuilder->CreateFontSet(OUT &fontSet_));
        IFR(dwriteFactory3->CreateFontCollectionFromFontSet(fontSet_, OUT reinterpret_cast<IDWriteFontCollection1**>(&fontCollection_)));
    }
    else
    {
        IFR(CreateFontCollection(
            dwriteFactory_,
            L"",
            0,
            OUT &fontCollection_
            ));
    }

    return S_OK;
}


HRESULT MainWindow::RebuildFontCollectionList()
{
    if (fontCollection_ == nullptr)
    {
        ComPtr<IDWriteFactory3> dwriteFactory3;
        dwriteFactory_->QueryInterface(OUT &dwriteFactory3);
        if (dwriteFactory3 != nullptr)
        {
            IFR(dwriteFactory3->GetSystemFontCollection(includeRemoteFonts_, OUT reinterpret_cast<IDWriteFontCollection1**>(&fontCollection_)));
        }
        else // Downloadable fonts not supported.
        {
            IFR(dwriteFactory_->GetSystemFontCollection(OUT &fontCollection_));
        }
    }

    if (fontSet_ == nullptr)
    {
        ComPtr<IDWriteFactory6> dwriteFactory6;
        dwriteFactory_->QueryInterface(OUT &dwriteFactory6);
        if (dwriteFactory6 != nullptr)
        {
            // Get the font set directly in the true item order.
            IFR(dwriteFactory6->GetSystemFontSet(includeRemoteFonts_, OUT reinterpret_cast<IDWriteFontSet1**>(&fontSet_)));
        }
        else // Running on older version with at least IDWriteFactory3 hopefully. Get the font set from the collection instead.
        {
            ComPtr<IDWriteFontCollection1> fontCollection1;
            fontCollection_->QueryInterface(OUT &fontCollection1);
            if (fontCollection1 != nullptr)
            {
                fontCollection1->GetFontSet(OUT &fontSet_);
            }
        }
    }

    // Initialization common to either case, whether a font set is available or not.
    fontCollectionList_.clear();
    fontCollectionListStringMap_.clear();
    wchar_t const* languageName = g_locales[currentLanguageIndex_][1];

    if (fontSet_ != nullptr)
    {
        DWRITE_FONT_PROPERTY fontProperty = { DWRITE_FONT_PROPERTY_ID_WEIGHT_STRETCH_STYLE_FAMILY_NAME, L"WssFamilyName", L"en-us" };

        ////////////////////
        // Apply all the filters.

        ComPtr<IDWriteFontSet> fontSet = fontSet_;
        for (const auto& fontFilter : fontCollectionFilters_)
        {
            ComPtr<IDWriteFontSet> fontSubset;
            fontProperty.propertyId = FilterModeToPropertyId(fontFilter.mode);
            fontProperty.propertyValue = fontFilter.parameter.c_str();

            IFR(fontSet->GetMatchingFonts(&fontProperty, 1, OUT &fontSubset));

            std::swap(fontSet, fontSubset);
        }

        ////////////////////
        // Get the list of all distinct properties for the current property type.

        auto currentPropertyId = FilterModeToPropertyId(filterMode_);
        bool const isUngroupedList = (filterMode_ == FontCollectionFilterMode::None);
        bool const isLanguageAgnosticProperty = IsLanguageAgnosticFilterMode(filterMode_);
        ComPtr<IDWriteStringList> stringList;
        if (isLanguageAgnosticProperty)
            fontSet->GetPropertyValues(currentPropertyId, OUT &stringList);
        else
            fontSet->GetPropertyValues(currentPropertyId, languageName, OUT &stringList);

        uint32_t const stringCount = stringList->GetCount();
        uint32_t const fontCount = fontSet->GetFontCount();
        uint32_t const entryCount = isUngroupedList ? fontCount : stringCount;
        std::wstring stringValue, wssFamilyName, weightString, stretchString, slopeString;

        for (uint32_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
        {
            ComPtr<IDWriteFontSet> fontSubset;
            uint32_t fontSetItemIndex = 0;
            uint32_t subsetFontCount = 1;

            if (isUngroupedList)
            {
                fontSetItemIndex = entryIndex;
                fontSubset = fontSet; // Same list since 1:1.

                BOOL dummyExists;
                ComPtr<IDWriteLocalizedStrings> itemStringList;
                IFR(fontSet->GetPropertyValues(fontSetItemIndex, currentPropertyId, OUT &dummyExists, OUT &itemStringList));
                IFR(GetLocalizedString(itemStringList, languageName, OUT stringValue));
            }
            else // Group multiple font items together.
            {
                IFR(GetLocalizedString(stringList, entryIndex, OUT stringValue));

                // Get the subset of fonts matching the named property.
                fontProperty.propertyId = currentPropertyId;
                fontProperty.propertyValue = stringValue.c_str();

                IFR(fontSet->GetMatchingFonts(&fontProperty, 1, OUT &fontSubset));

                subsetFontCount = fontSubset->GetFontCount();
            }


            // Get the weight-style-stretch family name and WSS values.
            wssFamilyName.clear();
            long weightValue = 400;
            long stretchValue = 100;
            long slopeValue = 0;

            if (subsetFontCount > 0)
            {
                BOOL dummyExists;
                ComPtr<IDWriteLocalizedStrings> familyNameStringList;
                IFR(fontSubset->GetPropertyValues(fontSetItemIndex, DWRITE_FONT_PROPERTY_ID_WEIGHT_STRETCH_STYLE_FAMILY_NAME, OUT &dummyExists, OUT &familyNameStringList));
                IFR(GetLocalizedString(familyNameStringList, languageName, OUT wssFamilyName));

                ComPtr<IDWriteLocalizedStrings> weightStringList;
                ComPtr<IDWriteLocalizedStrings> stretchStringList;
                ComPtr<IDWriteLocalizedStrings> slopeStringList;
                IFR(fontSubset->GetPropertyValues(fontSetItemIndex, DWRITE_FONT_PROPERTY_ID_WEIGHT, OUT &dummyExists, OUT &weightStringList));
                IFR(fontSubset->GetPropertyValues(fontSetItemIndex, DWRITE_FONT_PROPERTY_ID_STRETCH, OUT &dummyExists, OUT &stretchStringList));
                IFR(fontSubset->GetPropertyValues(fontSetItemIndex, DWRITE_FONT_PROPERTY_ID_STYLE, OUT &dummyExists, OUT &slopeStringList));
                GetLocalizedString(weightStringList, languageName, OUT weightString);
                GetLocalizedString(stretchStringList, languageName, OUT stretchString);
                GetLocalizedString(slopeStringList, languageName, OUT slopeString);
                weightValue = wcstol(weightString.c_str(),  /*end*/nullptr, /*base*/10);
                stretchValue = wcstol(stretchString.c_str(), /*end*/nullptr, /*base*/10);
                slopeValue = wcstol(slopeString.c_str(),   /*end*/nullptr, /*base*/10);
            }

            FontCollectionEntry fontCollectionEntry = {
                stringValue,
                0, // dummy firstFontIndex
                subsetFontCount,
                wssFamilyName, 
                DWRITE_FONT_WEIGHT(weightValue), DWRITE_FONT_STRETCH(stretchValue), DWRITE_FONT_STYLE(slopeValue)
                };

            // Get the font axis values and ranges.
            ComPtr<IDWriteFontSet1> fontSubset1;
            fontSubset->QueryInterface(OUT &fontSubset1);
            if (fontSubset->GetFontCount() > 0 && fontSubset1 != nullptr)
            {
                auto& fontAxisValues = fontCollectionEntry.fontAxisValues;
                auto& fontAxisRanges = fontCollectionEntry.fontAxisRanges;

                // Get axis ranges.
                uint32_t actualFontAxisRangeCount = 0;
                if (isUngroupedList) // Get axis range of specific item.
                {
                    fontSubset1->GetFontAxisRanges(fontSetItemIndex, nullptr, 0, OUT &actualFontAxisRangeCount);
                    fontAxisRanges.resize(actualFontAxisRangeCount);
                    fontSubset1->GetFontAxisRanges(fontSetItemIndex, OUT fontAxisRanges.data(), actualFontAxisRangeCount, OUT &actualFontAxisRangeCount);
                }
                else // Get axis range of all items in the subset.
                {
                    fontSubset1->GetFontAxisRanges(nullptr, 0, OUT &actualFontAxisRangeCount);
                    fontAxisRanges.resize(actualFontAxisRangeCount);
                    fontSubset1->GetFontAxisRanges(OUT fontAxisRanges.data(), actualFontAxisRangeCount, OUT &actualFontAxisRangeCount);
                }

                // Get axis values.
                ComPtr<IDWriteFontFaceReference1> fontFaceReference;
                fontSubset1->GetFontFaceReference(fontSetItemIndex, OUT &fontFaceReference);
                uint32_t fontAxisValueCount = fontFaceReference->GetFontAxisValueCount();
                fontAxisValues.resize(fontAxisValueCount);
                fontFaceReference->GetFontAxisValues(OUT fontAxisValues.data(), fontAxisValueCount);

                GetFilePath(fontFaceReference, OUT fontCollectionEntry.filePath);
                fontCollectionEntry.fontFaceIndex = fontFaceReference->GetFontFaceIndex();
                fontCollectionEntry.fontSimulations = fontFaceReference->GetSimulations();
            }

            // Add the entry to the list.
            fontCollectionList_.push_back(std::move(fontCollectionEntry));
        }
    }
    else
    {
        ////////////////////
        // Add all the IDWriteFont's to a single easy-to-use array (basically
        // flatten the heirarchy into addressable indices).

        std::vector<ComPtr<IDWriteFont> > fontCollectionFonts;

        for (uint32_t i = 0, ci = fontCollection_->GetFontFamilyCount(); i < ci; ++i)
        {
            ComPtr<IDWriteFontFamily> fontFamily;
            fontCollection_->GetFontFamily(i, OUT &fontFamily);

            if (fontFamily == nullptr) continue;
            for (uint32_t j = 0, cj = fontFamily->GetFontCount(); j < cj; ++j)
            {
                ComPtr<IDWriteFont> font;
                fontFamily->GetFont(j, OUT &font);
                if (font == nullptr || font->GetSimulations() != DWRITE_FONT_SIMULATIONS_NONE)
                    continue;

                fontCollectionFonts.push_back(font);
            }
        }

        ////////////////////
        // Reinitialize the lists.
        std::vector<uint32_t> fontCollectionFontIndices;

        fontCollectionFontIndices.resize(fontCollectionFonts.size());
        uint32_t fontIndex = 0;
        std::generate(fontCollectionFontIndices.begin(), fontCollectionFontIndices.end(), [&] {return fontIndex++; });

        std::wstring fontPropertyValue;
        std::vector<std::pair<uint32_t, uint32_t> > fontPropertyValueTokens;

        ////////////////////
        // Apply all the filters.
        for (const auto& fontFilter : fontCollectionFilters_)
        {
            uint32_t indicesCount = static_cast<uint32_t>(fontCollectionFontIndices.size());
            uint32_t newIndicesCount = 0;

            // Check the subset of fonts that remain (not filtered out already in a previous pass),
            // and copy over any fonts for which the current filter applies, skipping the others.
            for (uint32_t i = 0; i < indicesCount; ++i)
            {
                auto fontIndex = fontCollectionFontIndices[i];
                assert(fontIndex < fontCollectionFonts.size());
                IDWriteFont* font = fontCollectionFonts[fontIndex];
                GetFontProperty(font, fontFilter.mode, languageName, OUT fontPropertyValue, OUT fontPropertyValueTokens);
                if (DoesFontFilterApply(fontFilter.mode, fontFilter.parameter, fontPropertyValue, fontPropertyValueTokens))
                {
                    fontCollectionFontIndices[newIndicesCount++] = fontIndex; // Keep this font for the next filter pass.
                }
            }
            // Reduce the filtered subset to the new size.
            fontCollectionFontIndices.resize(newIndicesCount);
        }

        //////////
        // Add the fonts to the collection list. fontCollectionList_ is still empty at this point.
        for (auto fontIndex : fontCollectionFontIndices)
        {
            assert(fontIndex < fontCollectionFonts.size());
            IDWriteFont* font = fontCollectionFonts[fontIndex];
            GetFontProperty(font, filterMode_, languageName, OUT fontPropertyValue, OUT fontPropertyValueTokens);

            // Add the font to the list.
            // If the property had a single value, just add it.
            // If multiple values, add each occurrence separately.
            // If empty string, still add it. Most properties will not have an
            // empty string, but semantic tags and script tags may, leaving any
            // unknown fonts to fall in those buckets.
            if (!fontPropertyValueTokens.empty())
            {
                for (auto const& token : fontPropertyValueTokens)
                {
                    AddFontToFontCollectionList(font, fontIndex, languageName, fontPropertyValue.substr(token.first, token.second));
                }
            }
            else
            {
                AddFontToFontCollectionList(font, fontIndex, languageName, fontPropertyValue);
            }
        }
    }

    if (wantSortedFontList_)
    {
        std::sort(fontCollectionList_.begin(), fontCollectionList_.end());
    }

    AppendLog(AppendLogModeImmediate, L"List contains %d entries\r\n", fontCollectionList_.size());

    return S_OK;
}


HRESULT MainWindow::GetFontProperty(
    IDWriteFont* font,
    FontCollectionFilterMode filterMode,
    _In_z_ wchar_t const* languageName,
    _Out_ std::wstring& fontPropertyValue,
    _Out_ std::vector<std::pair<uint32_t,uint32_t> >& fontPropertyValueTokens
    )
{
    fontPropertyValue.clear();
    fontPropertyValueTokens.clear();

    DWRITE_INFORMATIONAL_STRING_ID stringId = DWRITE_INFORMATIONAL_STRING_NONE;
    switch (filterMode)
    {
    case FontCollectionFilterMode::None:
        // Use the full name in this case, as it is the most unique.
        stringId = DWRITE_INFORMATIONAL_STRING_FULL_NAME;
        break;

    case FontCollectionFilterMode::WssFamilyName:
        GetFontFamilyNameWws(font, languageName, OUT fontPropertyValue);
        return S_OK;

    case FontCollectionFilterMode::TypographicFamilyName:
        stringId = DWRITE_INFORMATIONAL_STRING_PREFERRED_FAMILY_NAMES;
        break;

    case FontCollectionFilterMode::WssFaceName:
        GetFontFaceNameWws(font, languageName, OUT fontPropertyValue);
        return S_OK;

    case FontCollectionFilterMode::TypographicFaceName:
        stringId = DWRITE_INFORMATIONAL_STRING_PREFERRED_SUBFAMILY_NAMES;
        return S_OK;

    case FontCollectionFilterMode::FullName:
        stringId = DWRITE_INFORMATIONAL_STRING_FULL_NAME;
        break;

    case FontCollectionFilterMode::Win32FamilyName:
        stringId = DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES;
        break;
    case FontCollectionFilterMode::PostscriptName:
        stringId = DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME;
        break;

    case FontCollectionFilterMode::DesignedScriptTag:
    case FontCollectionFilterMode::SupportedScriptTag:
    case FontCollectionFilterMode::SemanticTag:
        {
            //////////
            // Find known tags that match this font if known.

            wchar_t const* tags = nullptr;
            wchar_t const* scripts = nullptr;
            std::wstring familyName;
            GetFontFamilyNameWws(font, languageName, OUT familyName);
            FindTagsFromKnownFontName(
                nullptr, // fullFontName
                familyName.c_str(),
                OUT &tags,
                OUT &scripts
                );

            switch (filterMode)
            {
            case FontCollectionFilterMode::DesignedScriptTag:
            case FontCollectionFilterMode::SupportedScriptTag:
                if (scripts != nullptr)
                    fontPropertyValue.assign(scripts);
                break;

            case FontCollectionFilterMode::SemanticTag:
                if (tags != nullptr)
                    fontPropertyValue.assign(tags);

                ComPtr<IDWriteFont2> font2;
                font->QueryInterface(OUT &font2);
                if (font2 == nullptr)
                    break; // Okay, just return what we have.

                // Always end with a semicolon.
                if (!fontPropertyValue.empty() && fontPropertyValue.back() != ';')
                    fontPropertyValue.push_back(';');

                //////////
                // Add derived tags from properties.

                if (font2->IsSymbolFont())
                {
                    AppendStringIfNotPresent(L"Symbol;", IN OUT fontPropertyValue);
                }
                if (font2->IsMonospacedFont())
                {
                    AppendStringIfNotPresent(L"Monospace;", IN OUT fontPropertyValue);
                }
                if (font2->IsColorFont())
                {
                    AppendStringIfNotPresent(L"Color;", IN OUT fontPropertyValue);
                }

                DWRITE_PANOSE panose = {};
                font2->GetPanose(OUT &panose);
                uint8_t serifStyle = 0;
                switch (panose.familyKind)
                {
                case DWRITE_PANOSE_FAMILY_TEXT_DISPLAY: serifStyle = panose.text.serifStyle;         break;
                case DWRITE_PANOSE_FAMILY_DECORATIVE:   serifStyle = panose.decorative.serifVariant; break;
                }

                switch (serifStyle)
                {
                case DWRITE_PANOSE_SERIF_STYLE_NORMAL_SANS:
                case DWRITE_PANOSE_SERIF_STYLE_OBTUSE_SANS:
                case DWRITE_PANOSE_SERIF_STYLE_PERPENDICULAR_SANS:
                case DWRITE_PANOSE_SERIF_STYLE_FLARED:
                case DWRITE_PANOSE_SERIF_STYLE_ROUNDED:
                    AppendStringIfNotPresent(L"Sans-Serif;", IN OUT fontPropertyValue);
                    break;

                case DWRITE_PANOSE_SERIF_STYLE_COVE:
                case DWRITE_PANOSE_SERIF_STYLE_OBTUSE_COVE:
                case DWRITE_PANOSE_SERIF_STYLE_SQUARE_COVE:
                case DWRITE_PANOSE_SERIF_STYLE_OBTUSE_SQUARE_COVE:
                case DWRITE_PANOSE_SERIF_STYLE_SQUARE:
                case DWRITE_PANOSE_SERIF_STYLE_THIN:
                case DWRITE_PANOSE_SERIF_STYLE_OVAL:
                case DWRITE_PANOSE_SERIF_STYLE_EXAGGERATED:
                case DWRITE_PANOSE_SERIF_STYLE_TRIANGLE:
                case DWRITE_PANOSE_SERIF_STYLE_SCRIPT:
                    AppendStringIfNotPresent(L"Serif;", IN OUT fontPropertyValue);
                    break;
                }
            } // switch (filterMode)

            //////////
            // Split the string into tokens.
            std::pair<uint32_t, uint32_t> token; // Starting position and length.
            uint32_t lastTokenEnd = 0;
            for (uint32_t i = 0, ci = static_cast<uint32_t>(fontPropertyValue.size()); i <= ci; ++i)
            {
                if ((i == ci && i != lastTokenEnd) || fontPropertyValue[i] == ';')
                {
                    token.first = lastTokenEnd;
                    token.second = i - lastTokenEnd;
                    lastTokenEnd = i + 1;
                    fontPropertyValueTokens.push_back(token);
                }
            }
        }
        return S_OK;

    case FontCollectionFilterMode::Weight:
    case FontCollectionFilterMode::Stretch:
    case FontCollectionFilterMode::Style:
        return E_NOTIMPL;

    default:
        return E_INVALIDARG;
    }

    // Get the string.
    ComPtr<IDWriteLocalizedStrings> strings;
    BOOL dummyExists;
    font->GetInformationalStrings(stringId, OUT &strings, OUT &dummyExists);
    GetLocalizedString(strings, languageName, OUT fontPropertyValue);

    // The preferred name is often empty, so use the normal family name in that case.
    if (fontPropertyValue.empty() && stringId == DWRITE_INFORMATIONAL_STRING_PREFERRED_FAMILY_NAMES)
    {
        GetFontFamilyNameWws(font, languageName, OUT fontPropertyValue);
    }

    // The full font name really shouldn't be empty, but if it is, concatenate the family and face name.
    if (fontPropertyValue.empty() && stringId == DWRITE_INFORMATIONAL_STRING_FULL_NAME)
    {
        std::wstring familyName, faceName;
        GetFontFamilyNameWws(font, languageName, OUT familyName);
        GetFontFaceNameWws(font, languageName, OUT faceName);
        familyName += L" ";
        familyName += faceName;
        fontPropertyValue = std::move(familyName);
    }

    return S_OK;
}


bool MainWindow::DoesFontFilterApply(
    FontCollectionFilterMode filterMode,
    const std::wstring& filterParameter,
    const std::wstring& fontPropertyValue,
    const std::vector<std::pair<uint32_t, uint32_t> > fontPropertyValueTokens
    )
{
    if (fontPropertyValueTokens.empty())
    {
        // Just compare strings directly.
        return fontPropertyValue == filterParameter;
    }
    else
    {
        // Check if the value is contained in any of the tokens.
        for (const auto& token : fontPropertyValueTokens)
        {
            if (fontPropertyValue.compare(token.first, token.second, filterParameter) == 0)
                return true;
        }
    }

    return false;
}


HRESULT MainWindow::AddFontToFontCollectionList(
    IDWriteFont* font,
    uint32_t firstFontIndex,
    _In_z_ wchar_t const* languageName, // Needed for the family name.
    const std::wstring& name
    )
{
    // See if this one has already been added. If so, just increment the count.
    // Otherwise append it.
    auto match = fontCollectionListStringMap_.find(name);
    if (match != fontCollectionListStringMap_.end())
    {
        fontCollectionList_[match->second].fontCount++;
        return S_OK;
    }

    fontCollectionListStringMap_.insert(std::pair<std::wstring, uint32_t>(name, static_cast<uint32_t>(fontCollectionList_.size())));

    std::wstring fontFamilyName;
    GetFontFamilyNameWws(font, languageName, OUT fontFamilyName);

    FontCollectionEntry entry = {
        name,
        firstFontIndex,
        /*fontCount*/1,
        fontFamilyName, 
        font->GetWeight(), font->GetStretch(), font->GetStyle()
        };
    fontCollectionList_.push_back(entry);

    return S_OK;
}


HRESULT MainWindow::SetCurrentFilter(
    FontCollectionFilterMode newFilterMode
    )
{
    if (uint32_t(newFilterMode) >= uint32_t(FontCollectionFilterMode::Total))
        return E_INVALIDARG;

    // Redraw previous filter item in ListView.
    auto listViewHwnd = GetDlgItem(hwnd_, IdcFontCollectionFilter);
    auto oldItemIndex = int(filterMode_) + 1 + fontCollectionFilters_.size();
    ListView_RedrawItems(listViewHwnd, oldItemIndex, oldItemIndex);

    // Set the new filter, and rebuild the collection list.
    filterMode_ = newFilterMode;
    AppendLog(AppendLogModeImmediate, L"Filter set to %s\r\n", g_fontCollectionFilterModeNames[int(filterMode_)]);
    RebuildFontCollectionList();
    UpdateFontCollectionListUI();

    return S_OK;
}


HRESULT MainWindow::PushFilter(
    uint32_t selectedFontIndex // Must be < fontCollectionList_.size()
    )
{
    if (selectedFontIndex >= fontCollectionList_.size())
        return E_INVALIDARG;

    if (filterMode_ == FontCollectionFilterMode::None)
        return S_FALSE;

    const FontCollectionEntry& entry = fontCollectionList_[selectedFontIndex];

#if 0 // Debug to show all properties to log window.
    if (filterMode_ == FontCollectionFilterMode::None)
    {
        std::wstring fontPropertyValue;
        std::vector<std::pair<uint32_t, uint32_t> > fontPropertyValueTokens;
        IDWriteFont* font = fontCollectionFonts_[entry.firstFontIndex];
        AppendLog(AppendLogModeImmediate, L"Font info\r\n", g_fontCollectionFilterModeNames[int(filterMode_)], entry.name.c_str());
        AppendLog(AppendLogModeImmediate, L"Family name: '%s'\r\n", fontPropertyValue.c_str());
        GetFontProperty(font, FontCollectionFilterMode::FamilyName,         /*languageName*/nullptr, OUT fontPropertyValue, OUT fontPropertyValueTokens);
        AppendLog(AppendLogModeImmediate, L"Family name: '%s'\r\n", fontPropertyValue.c_str());
        GetFontProperty(font, FontCollectionFilterMode::PreferredFamilyName,/*languageName*/nullptr, OUT fontPropertyValue, OUT fontPropertyValueTokens);
        AppendLog(AppendLogModeImmediate, L"Preferred family name: '%s'\r\n", fontPropertyValue.c_str());
        GetFontProperty(font, FontCollectionFilterMode::FaceName,           /*languageName*/nullptr, OUT fontPropertyValue, OUT fontPropertyValueTokens);
        AppendLog(AppendLogModeImmediate, L"Face name: '%s'\r\n", fontPropertyValue.c_str());
        GetFontProperty(font, FontCollectionFilterMode::FullName,           /*languageName*/nullptr, OUT fontPropertyValue, OUT fontPropertyValueTokens);
        AppendLog(AppendLogModeImmediate, L"Full name: '%s'\r\n", fontPropertyValue.c_str());
        GetFontProperty(font, FontCollectionFilterMode::Win32FamilyName,    /*languageName*/nullptr, OUT fontPropertyValue, OUT fontPropertyValueTokens);
        AppendLog(AppendLogModeImmediate, L"win32 family name: '%s'\r\n", fontPropertyValue.c_str());
        GetFontProperty(font, FontCollectionFilterMode::PostscriptName,     /*languageName*/nullptr, OUT fontPropertyValue, OUT fontPropertyValueTokens);
        AppendLog(AppendLogModeImmediate, L"Postscript name: '%s'\r\n", fontPropertyValue.c_str());
        GetFontProperty(font, FontCollectionFilterMode::ScriptTag,          /*languageName*/nullptr, OUT fontPropertyValue, OUT fontPropertyValueTokens);
        AppendLog(AppendLogModeImmediate, L"Scripts: '%s'\r\n", fontPropertyValue.c_str());
        GetFontProperty(font, FontCollectionFilterMode::SemanticTag,        /*languageName*/nullptr, OUT fontPropertyValue, OUT fontPropertyValueTokens);
        AppendLog(AppendLogModeImmediate, L"Tags: '%s'\r\n", fontPropertyValue.c_str());

        return S_OK;
    }
#endif

    fontCollectionFilters_.push_back(FontCollectionFilter{ filterMode_, selectedFontIndex, entry.name });
    filterMode_ = FontCollectionFilterMode::None;

    AppendLog(AppendLogModeImmediate, L"Filter added %s: '%s'\r\n", g_fontCollectionFilterModeNames[int(filterMode_)], entry.name.c_str());
    UpdateFontCollectionFilterUI();
    RebuildFontCollectionList();
    UpdateFontCollectionListUI();

    return S_OK;
}


HRESULT MainWindow::PopFilter(
    uint32_t newFilterLevel
    )
{
    uint32_t selectedFontIndex = 0;
    if (newFilterLevel > fontCollectionFilters_.size()  || fontCollectionFilters_.empty())
    {
        // Clicked on the root, so clear everything, and shows all fonts.
        filterMode_ = FontCollectionFilterMode::None;
        fontCollectionFilters_.clear();
    }
    else
    {
        // Clicked on a previous filter, so reset to that view.
        filterMode_ = fontCollectionFilters_.back().mode;
        selectedFontIndex = fontCollectionFilters_.back().selectedFontIndex;
        fontCollectionFilters_.resize(newFilterLevel);
    }
    AppendLog(AppendLogModeImmediate, L"Filter removed. Using %s\r\n", g_fontCollectionFilterModeNames[int(filterMode_)]);

    UpdateFontCollectionFilterUI();
    RebuildFontCollectionList();
    UpdateFontCollectionListUI(selectedFontIndex);

    return S_OK;
}


HRESULT MainWindow::InitializeLanguageMenu()
{
    HMENU mainMenu = GetMenu(hwnd_);
    HMENU languageMenu = GetSubMenu(mainMenu, MenuIdLanguage, /*byPosition*/false);
    RemoveMenu(languageMenu, 0, MF_BYPOSITION); // Remove the placeholder entry.

    std::wstring text;
    for (uint32_t i = 0; i < ARRAYSIZE(g_locales); ++i)
    {
        GetFormattedString(IN OUT text, L"%s (%s)", g_locales[i][0], g_locales[i][1]);
        AppendMenu(
            languageMenu,
            MF_STRING,
            MenuIdLanguageFirst + i,
            text.c_str()
            );
    }

    return S_OK;
}


void MainWindow::AppendLog(AppendLogMode appendLogMode, const wchar_t* logMessage, ...)
{
    HWND hwndLog = GetDlgItem(hwnd_, IdcLog);

    va_list argList;
    va_start(argList, logMessage);

    wchar_t buffer[1000];
    buffer[0] = 0;
    StringCchVPrintf(
        buffer,
        ARRAYSIZE(buffer),
        logMessage,
        argList
        );

    if (appendLogMode == AppendLogModeCached)
    {
        cachedLog_ += buffer;
    }
    else
    {
        uint32_t textLength = Edit_GetTextLength(hwndLog);
        if (!cachedLog_.empty())
        {
            Edit_SetSel(hwndLog, textLength, textLength);
            Edit_ReplaceSel(hwndLog, cachedLog_.c_str());
            cachedLog_.clear();
            textLength = Edit_GetTextLength(hwndLog);
        }
        Edit_SetSel(hwndLog, textLength, textLength);
        Edit_ReplaceSel(hwndLog, buffer);
    }

    if (appendLogMode == AppendLogModeMessageBox)
    {
        MessageBox(hwnd_, buffer, APPLICATION_TITLE, MB_OK | MB_ICONEXCLAMATION);
    }
}


HRESULT MainWindow::ParseCommandLine(_In_z_ const wchar_t* commandLine)
{
    // Parse all the parameters on the command line. However, do very little
    // work here, deferring all the processing to later, once the application
    // is more initialized.

    if (commandLine == nullptr || commandLine[0] == '\0')
        return S_OK;

    if ((commandLine[0] == '-' || commandLine[0] == '/') && (commandLine[1] == '?' || commandLine[1] == 'h') && (commandLine[2] == '\0'))
    {
        return S_FALSE;
    }

    // Read all commands into a text tree.
    TextTree commands;
    uint32_t textLength = static_cast<uint32_t>(wcslen(commandLine));
    JsonexParser parser(commandLine, textLength, TextTreeParser::OptionsNoEscapeSequence);
    parser.ReadNodes(IN OUT commands);

    // Display any parse errors.
    if (parser.GetErrorCount() > 0)
    {
        ShowHelp();
        return E_INVALIDARG;
    }

    // Skip any empty root node if present.
    if (!commands.GetKeyValue(0, L"DWriteDLL", OUT g_dwriteDllName))
    {
        g_dwriteDllName = L"DWrite.dll";
    }

    std::wstring value;
    if (commands.GetKeyValue(0, L"StartBlank", OUT value))
    {
        g_startBlankList = (value == L"true");
    }

    return S_OK;
}


void MainWindow::ShowHelp()
{
    MessageBox(
        nullptr,
        L"FontCollectionViewer usage:\r\n"
        L"DWriteDLL: \"c:\\alternatepath\\dwrite.dll\"",
        APPLICATION_TITLE,
        MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL
        );
}


HRESULT ShowMessageIfFailed(HRESULT functionResult, const wchar_t* message)
{
    // Displays an error message for API failures,
    // returning the very same error code that came in.

    if (FAILED(functionResult))
    {
        const wchar_t* format = L"%s\r\nError code = %X";

        wchar_t buffer[1000];
        buffer[0] = '\0';

        StringCchPrintf(buffer, ARRAYSIZE(buffer), format, message, functionResult);
        MessageBox(nullptr, buffer, APPLICATION_TITLE, MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL);
    }
    return functionResult;
}

#if 0
void MainWindow::ParseJsonFontSet(const wchar_t* filename)
{
    std::vector<uint8_t> rawJsonFile;

    // Read all commands into a text tree.
    std::wstring fontSetText;

    IFR(ReadTextFile(filename, OUT fontSetText));

    ConvertText(rawJsonFile, OUT fontSetText);
    TextTree nodes;
    uint32_t textLength = static_cast<uint32_t>(fontSetText.size());
    JsonexParser parser(fontSetText.data(), textLength, TextTreeParser::OptionsNoEscapeSequence);
    parser.ReadNodes(IN OUT nodes);

    uint32_t nodeIndex = 0;
    if (!nodes.AdvanceChildNode(IN OUT nodeIndex) // skip the root node.
        || nodes.GetNode(nodeIndex).type != TextTree::Node::TypeArray
        || !nodes.AdvanceChildNode(IN OUT nodeIndex))
    {
        return S_FALSE;
    }

    std::wstring filePath;
    std::wstring familyName;
    std::wstring fullName;
    std::wstring weight;
    std::wstring stretch;
    std::wstring slope;
    std::wstring value;
    uint32_t faceIndex = 0;

    DWRITE_FONT_PROPERTY properties[5] = {
        { DWRITE_FONT_PROPERTY_ID_FULL_NAME, L"FullName", L"en-us" },
        { DWRITE_FONT_PROPERTY_ID_WEIGHT_STRETCH_STYLE_FAMILY_NAME, L"WssFamilyName", L"en-us" },
        { DWRITE_FONT_PROPERTY_ID_WEIGHT, L"Weight", L"" },
        { DWRITE_FONT_PROPERTY_ID_STRETCH, L"Stretch", L"" },
        { DWRITE_FONT_PROPERTY_ID_STYLE, L"Slope", L"" },
    };

    while (nodeIndex < nodes.GetNodeCount())
    {
        if (nodes.GetNode(nodeIndex).type == TextTree::Node::TypeObject)
        {
            uint32_t subnodeIndex;
            nodes.GetKeyValue(nodeIndex, L"Path", OUT filePath);

            if (nodes.FindKey(nodeIndex, L"FullName", OUT subnodeIndex))
            {
                nodes.GetKeyValue(subnodeIndex, L"en-us", OUT fullName);
            }
            if (nodes.FindKey(nodeIndex, L"WssFamilyName", OUT subnodeIndex))
            {
                nodes.GetKeyValue(subnodeIndex, L"en-us", OUT familyName);
            }
            if (nodes.GetKeyValue(nodeIndex, L"FaceIndex", OUT value))
            {
                faceIndex = _wtoi(value.c_str());
            }
            nodes.GetKeyValue(nodeIndex, L"Weight", OUT weight);
            nodes.GetKeyValue(nodeIndex, L"Stretch", OUT stretch);
            nodes.GetKeyValue(nodeIndex, L"Slope", OUT slope);

            ComPtr<IDWriteFontFile> fontFile;
            ComPtr<IDWriteFontFaceReference> fontFaceReference;

            wchar_t const* fontFileReferenceKey = filePath.data();
            uint32_t fontFileReferenceKeySize = (static_cast<uint32_t>(filePath.size()) + 1) * sizeof(wchar_t);
            IFR(dwriteFactory3p->CreateCustomFontFileReference(
                fontFileReferenceKey,
                fontFileReferenceKeySize,
                fontFileLoader,
                OUT &fontFile
            ));

            IFR(dwriteFactory3p->CreateFontFaceReference(
                fontFile,
                faceIndex,
                DWRITE_FONT_SIMULATIONS_NONE,
                OUT &fontFaceReference
            ));

            static_assert(ARRAYSIZE(properties) == 5, "Update this code to match the size");
            properties[0].propertyValue = fullName.c_str();
            properties[1].propertyValue = familyName.c_str();
            properties[2].propertyValue = weight.c_str();
            properties[3].propertyValue = stretch.c_str();
            properties[4].propertyValue = slope.c_str();
            fontSetBuilder->AddFontFaceReference(
                fontFaceReference,
                &properties[0],
                ARRAYSIZE(properties) // propertiesCount
            );
        }

        if (!nodes.AdvanceNextNode(IN OUT nodeIndex))
            break;
    }

    // Create the font set.
    IFR(fontSetBuilder->CreateFontSet(OUT &fontSet_));
    uint32_t fontCount = fontSet_->GetFontCount();
    IDWriteFontCollection1* fontCollection1;
    IFR(dwriteFactory3->CreateFontCollectionFromFontSet(fontSet_, OUT &fontCollection1));
    fontCollection_ = fontCollection1;
}
#endif
