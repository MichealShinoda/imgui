
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <ctype.h>       
#include <stdio.h>         
#if defined(_MSC_VER) && _MSC_VER <= 1500     
#include <stddef.h>      
#else
#include <stdint.h>      
#endif

#if defined(_WIN32) && defined(IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS) && defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS) && defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#define IMGUI_DISABLE_WIN32_FUNCTIONS
#endif
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef __MINGW32__
#include <Windows.h>          
#else
#include <windows.h>
#endif
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)       
#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
#endif
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4127)                 
#pragma warning (disable: 4996)                          
#if defined(_MSC_VER) && _MSC_VER >= 1922        
#pragma warning (disable: 5054)                     
#endif
#endif

#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"                                                             
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                     
#pragma clang diagnostic ignored "-Wold-style-cast"                                                       
#pragma clang diagnostic ignored "-Wfloat-equal"                                         
#pragma clang diagnostic ignored "-Wformat-nonliteral"                                                
#pragma clang diagnostic ignored "-Wexit-time-destructors"                                               
#pragma clang diagnostic ignored "-Wglobal-constructors"                                     
#pragma clang diagnostic ignored "-Wsign-conversion"                     
#pragma clang diagnostic ignored "-Wformat-pedantic"                                            
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"                 
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"                                    
#pragma clang diagnostic ignored "-Wdouble-promotion"                                            
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"            
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                          
#pragma GCC diagnostic ignored "-Wunused-function"                
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"               
#pragma GCC diagnostic ignored "-Wformat"                                 
#pragma GCC diagnostic ignored "-Wdouble-promotion"                     
#pragma GCC diagnostic ignored "-Wconversion"                         
#pragma GCC diagnostic ignored "-Wformat-nonliteral"                  
#pragma GCC diagnostic ignored "-Wstrict-overflow"                            
#pragma GCC diagnostic ignored "-Wclass-memaccess"                              
#endif

#define IMGUI_DEBUG_NAV_SCORING     0                  
#define IMGUI_DEBUG_NAV_RECTS       0           
#define IMGUI_DEBUG_INI_SETTINGS    0                 

static const float NAV_WINDOWING_HIGHLIGHT_DELAY            = 0.20f;              
static const float NAV_WINDOWING_LIST_APPEAR_DELAY          = 0.15f;            

static const float WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS = 4.0f;            
static const float WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER = 0.04f;                
static const float WINDOWS_MOUSE_WHEEL_SCROLL_LOCK_TIMER    = 2.00f;                        

static const float DOCKING_TRANSPARENT_PAYLOAD_ALPHA        = 0.50f;                
static const float DOCKING_SPLITTER_SIZE                    = 2.0f;

static void             SetCurrentWindow(ImGuiWindow* window);
static void             FindHoveredWindow();
static ImGuiWindow*     CreateNewWindow(const char* name, ImGuiWindowFlags flags);
static ImVec2           CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window);

static void             AddDrawListToDrawData(ImVector<ImDrawList*>* out_list, ImDrawList* draw_list);
static void             AddWindowToSortBuffer(ImVector<ImGuiWindow*>* out_sorted_windows, ImGuiWindow* window);

static void             WindowSettingsHandler_ClearAll(ImGuiContext*, ImGuiSettingsHandler*);
static void*            WindowSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
static void             WindowSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
static void             WindowSettingsHandler_ApplyAll(ImGuiContext*, ImGuiSettingsHandler*);
static void             WindowSettingsHandler_WriteAll(ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer* buf);

static const char*      GetClipboardTextFn_DefaultImpl(void* user_data);
static void             SetClipboardTextFn_DefaultImpl(void* user_data, const char* text);

namespace ImGui
{
static void             NavUpdate();
static void             NavUpdateWindowing();
static void             NavUpdateWindowingOverlay();
static void             NavUpdateMoveResult();
static void             NavUpdateInitResult();
static float            NavUpdatePageUpPageDown();
static inline void      NavUpdateAnyRequestFlag();
static void             NavEndFrame();
static bool             NavScoreItem(ImGuiNavMoveResult* result, ImRect cand);
static void             NavApplyItemToResult(ImGuiNavMoveResult* result, ImGuiWindow* window, ImGuiID id, const ImRect& nav_bb_rel);
static void             NavProcessItem(ImGuiWindow* window, const ImRect& nav_bb, ImGuiID id);
static ImVec2           NavCalcPreferredRefPos();
static void             NavSaveLastChildNavWindowIntoParent(ImGuiWindow* nav_window);
static ImGuiWindow*     NavRestoreLastChildNavWindow(ImGuiWindow* window);
static int              FindWindowFocusIndex(ImGuiWindow* window);

static void             ErrorCheckNewFrameSanityChecks();
static void             ErrorCheckEndFrameSanityChecks();

static void             UpdateSettings();
static void             UpdateMouseInputs();
static void             UpdateMouseWheel();
static void             UpdateTabFocus();
static void             UpdateDebugToolItemPicker();
static bool             UpdateWindowManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4], const ImRect& visibility_rect);
static void             RenderWindowOuterBorders(ImGuiWindow* window);
static void             RenderWindowDecorations(ImGuiWindow* window, const ImRect& title_bar_rect, bool title_bar_is_highlight, bool handle_borders_and_resize_grips, int resize_grip_count, const ImU32 resize_grip_col[4], float resize_grip_draw_size);
static void             RenderWindowTitleBarContents(ImGuiWindow* window, const ImRect& title_bar_rect, const char* name, bool* p_open);
static void             EndFrameDrawDimmedBackgrounds();

const ImGuiID           IMGUI_VIEWPORT_DEFAULT_ID = 0x11111111;                       
static ImGuiViewportP*  AddUpdateViewport(ImGuiWindow* window, ImGuiID id, const ImVec2& platform_pos, const ImVec2& size, ImGuiViewportFlags flags);
static void             UpdateViewportsNewFrame();
static void             UpdateViewportsEndFrame();
static void             UpdateSelectWindowViewport(ImGuiWindow* window);
static bool             UpdateTryMergeWindowIntoHostViewport(ImGuiWindow* window, ImGuiViewportP* host_viewport);
static bool             UpdateTryMergeWindowIntoHostViewports(ImGuiWindow* window);
static void             SetCurrentViewport(ImGuiWindow* window, ImGuiViewportP* viewport);
static bool             GetWindowAlwaysWantOwnViewport(ImGuiWindow* window);
static int              FindPlatformMonitorForPos(const ImVec2& pos);
static int              FindPlatformMonitorForRect(const ImRect& r);
static void             UpdateViewportPlatformMonitor(ImGuiViewportP* viewport);

}

#ifndef GImGui
ImGuiContext*   GImGui = NULL;
#endif

#ifndef IMGUI_DISABLE_DEFAULT_ALLOCATORS
static void*   MallocWrapper(size_t size, void* user_data)    { IM_UNUSED(user_data); return malloc(size); }
static void    FreeWrapper(void* ptr, void* user_data)        { IM_UNUSED(user_data); free(ptr); }
#else
static void*   MallocWrapper(size_t size, void* user_data)    { IM_UNUSED(user_data); IM_UNUSED(size); IM_ASSERT(0); return NULL; }
static void    FreeWrapper(void* ptr, void* user_data)        { IM_UNUSED(user_data); IM_UNUSED(ptr); IM_ASSERT(0); }
#endif

static void*  (*GImAllocatorAllocFunc)(size_t size, void* user_data) = MallocWrapper;
static void   (*GImAllocatorFreeFunc)(void* ptr, void* user_data) = FreeWrapper;
static void*    GImAllocatorUserData = NULL;

ImGuiStyle::ImGuiStyle()
{
    Alpha                   = 1.0f;                    
    WindowPadding           = ImVec2(8,8);          
    WindowRounding          = 7.0f;                                      
    WindowBorderSize        = 1.0f;                             
    WindowMinSize           = ImVec2(32,32);       
    WindowTitleAlign        = ImVec2(0.0f,0.5f);     
    WindowMenuButtonPosition= ImGuiDir_Left;                 
    ChildRounding           = 0.0f;                           
    ChildBorderSize         = 1.0f;                              
    PopupRounding           = 0.0f;                           
    PopupBorderSize         = 1.0f;                                
    FramePadding            = ImVec2(4,3);               
    FrameRounding           = 0.0f;                             
    FrameBorderSize         = 0.0f;                             
    ItemSpacing             = ImVec2(8,4);            
    ItemInnerSpacing        = ImVec2(4,4);                       
    CellPadding             = ImVec2(4,2);           
    TouchExtraPadding       = ImVec2(0,0);                                           
    IndentSpacing           = 21.0f;                         
    ColumnsMinSpacing       = 6.0f;                        
    ScrollbarSize           = 14.0f;                      
    ScrollbarRounding       = 9.0f;                    
    GrabMinSize             = 10.0f;                    
    GrabRounding            = 0.0f;                          
    LogSliderDeadzone       = 4.0f;                            
    TabRounding             = 4.0f;                           
    TabBorderSize           = 0.0f;                  
    TabMinWidthForCloseButton = 0.0f;                                          
    ColorButtonPosition     = ImGuiDir_Right;                
    ButtonTextAlign         = ImVec2(0.5f,0.5f);          
    SelectableTextAlign     = ImVec2(0.0f,0.0f);                            
    DisplayWindowPadding    = ImVec2(19,19);                           
    DisplaySafeAreaPadding  = ImVec2(3,3);                              
    MouseCursorScale        = 1.0f;                          
    AntiAliasedLines        = true;                        
    AntiAliasedLinesUseTex  = true;                           
    AntiAliasedFill         = true;                     
    CurveTessellationTol    = 1.25f;                                    
    CircleSegmentMaxError   = 1.60f;                                      

    ImGui::StyleColorsDark(this);
}

void ImGuiStyle::ScaleAllSizes(float scale_factor)
{
    WindowPadding = ImFloor(WindowPadding * scale_factor);
    WindowRounding = ImFloor(WindowRounding * scale_factor);
    WindowMinSize = ImFloor(WindowMinSize * scale_factor);
    ChildRounding = ImFloor(ChildRounding * scale_factor);
    PopupRounding = ImFloor(PopupRounding * scale_factor);
    FramePadding = ImFloor(FramePadding * scale_factor);
    FrameRounding = ImFloor(FrameRounding * scale_factor);
    ItemSpacing = ImFloor(ItemSpacing * scale_factor);
    ItemInnerSpacing = ImFloor(ItemInnerSpacing * scale_factor);
    CellPadding = ImFloor(CellPadding * scale_factor);
    TouchExtraPadding = ImFloor(TouchExtraPadding * scale_factor);
    IndentSpacing = ImFloor(IndentSpacing * scale_factor);
    ColumnsMinSpacing = ImFloor(ColumnsMinSpacing * scale_factor);
    ScrollbarSize = ImFloor(ScrollbarSize * scale_factor);
    ScrollbarRounding = ImFloor(ScrollbarRounding * scale_factor);
    GrabMinSize = ImFloor(GrabMinSize * scale_factor);
    GrabRounding = ImFloor(GrabRounding * scale_factor);
    LogSliderDeadzone = ImFloor(LogSliderDeadzone * scale_factor);
    TabRounding = ImFloor(TabRounding * scale_factor);
    TabMinWidthForCloseButton = (TabMinWidthForCloseButton != FLT_MAX) ? ImFloor(TabMinWidthForCloseButton * scale_factor) : FLT_MAX;
    DisplayWindowPadding = ImFloor(DisplayWindowPadding * scale_factor);
    DisplaySafeAreaPadding = ImFloor(DisplaySafeAreaPadding * scale_factor);
    MouseCursorScale = ImFloor(MouseCursorScale * scale_factor);
}

ImGuiIO::ImGuiIO()
{
    memset(this, 0, sizeof(*this));
    IM_ASSERT(IM_ARRAYSIZE(ImGuiIO::MouseDown) == ImGuiMouseButton_COUNT && IM_ARRAYSIZE(ImGuiIO::MouseClicked) == ImGuiMouseButton_COUNT);                

    ConfigFlags = ImGuiConfigFlags_None;
    BackendFlags = ImGuiBackendFlags_None;
    DisplaySize = ImVec2(-1.0f, -1.0f);
    DeltaTime = 1.0f / 60.0f;
    IniSavingRate = 5.0f;
    IniFilename = "imgui.ini";
    LogFilename = "imgui_log.txt";
    MouseDoubleClickTime = 0.30f;
    MouseDoubleClickMaxDist = 6.0f;
    for (int i = 0; i < ImGuiKey_COUNT; i++)
        KeyMap[i] = -1;
    KeyRepeatDelay = 0.275f;
    KeyRepeatRate = 0.050f;
    UserData = NULL;

    Fonts = NULL;
    FontGlobalScale = 1.0f;
    FontDefault = NULL;
    FontAllowUserScaling = false;
    DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    ConfigDockingNoSplit = false;
    ConfigDockingWithShift = false;
    ConfigDockingAlwaysTabBar = false;
    ConfigDockingTransparentPayload = false;

    ConfigViewportsNoAutoMerge = false;
    ConfigViewportsNoTaskBarIcon = false;
    ConfigViewportsNoDecoration = true;
    ConfigViewportsNoDefaultParent = false;

    MouseDrawCursor = false;
#ifdef __APPLE__
    ConfigMacOSXBehaviors = true;              
#else
    ConfigMacOSXBehaviors = false;
#endif
    ConfigInputTextCursorBlink = true;
    ConfigWindowsResizeFromEdges = true;
    ConfigWindowsMoveFromTitleBarOnly = false;
    ConfigMemoryCompactTimer = 60.0f;

    BackendPlatformName = BackendRendererName = NULL;
    BackendPlatformUserData = BackendRendererUserData = BackendLanguageUserData = NULL;
    GetClipboardTextFn = GetClipboardTextFn_DefaultImpl;       
    SetClipboardTextFn = SetClipboardTextFn_DefaultImpl;
    ClipboardUserData = NULL;

    MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    MousePosPrev = ImVec2(-FLT_MAX, -FLT_MAX);
    MouseDragThreshold = 6.0f;
    for (int i = 0; i < IM_ARRAYSIZE(MouseDownDuration); i++) MouseDownDuration[i] = MouseDownDurationPrev[i] = -1.0f;
    for (int i = 0; i < IM_ARRAYSIZE(KeysDownDuration); i++) KeysDownDuration[i]  = KeysDownDurationPrev[i] = -1.0f;
    for (int i = 0; i < IM_ARRAYSIZE(NavInputsDownDuration); i++) NavInputsDownDuration[i] = -1.0f;
}

void ImGuiIO::AddInputCharacter(unsigned int c)
{
    if (c != 0)
        InputQueueCharacters.push_back(c <= IM_UNICODE_CODEPOINT_MAX ? (ImWchar)c : IM_UNICODE_CODEPOINT_INVALID);
}

void ImGuiIO::AddInputCharacterUTF16(ImWchar16 c)
{
    if (c == 0 && InputQueueSurrogate == 0)
        return;

    if ((c & 0xFC00) == 0xD800)     
    {
        if (InputQueueSurrogate != 0)
            InputQueueCharacters.push_back(IM_UNICODE_CODEPOINT_INVALID);
        InputQueueSurrogate = c;
        return;
    }

    ImWchar cp = c;
    if (InputQueueSurrogate != 0)
    {
        if ((c & 0xFC00) != 0xDC00)    
            InputQueueCharacters.push_back(IM_UNICODE_CODEPOINT_INVALID);
        else if (IM_UNICODE_CODEPOINT_MAX == (0xFFFF))                
            cp = IM_UNICODE_CODEPOINT_INVALID;
        else
            cp = (ImWchar)(((InputQueueSurrogate - 0xD800) << 10) + (c - 0xDC00) + 0x10000);
        InputQueueSurrogate = 0;
    }
    InputQueueCharacters.push_back(cp);
}

void ImGuiIO::AddInputCharactersUTF8(const char* utf8_chars)
{
    while (*utf8_chars != 0)
    {
        unsigned int c = 0;
        utf8_chars += ImTextCharFromUtf8(&c, utf8_chars, NULL);
        if (c != 0)
            InputQueueCharacters.push_back((ImWchar)c);
    }
}

void ImGuiIO::ClearInputCharacters()
{
    InputQueueCharacters.resize(0);
}

ImVec2 ImBezierClosestPoint(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& p, int num_segments)
{
    IM_ASSERT(num_segments > 0);   
    ImVec2 p_last = p1;
    ImVec2 p_closest;
    float p_closest_dist2 = FLT_MAX;
    float t_step = 1.0f / (float)num_segments;
    for (int i_step = 1; i_step <= num_segments; i_step++)
    {
        ImVec2 p_current = ImBezierCalc(p1, p2, p3, p4, t_step * i_step);
        ImVec2 p_line = ImLineClosestPoint(p_last, p_current, p);
        float dist2 = ImLengthSqr(p - p_line);
        if (dist2 < p_closest_dist2)
        {
            p_closest = p_line;
            p_closest_dist2 = dist2;
        }
        p_last = p_current;
    }
    return p_closest;
}

static void BezierClosestPointCasteljauStep(const ImVec2& p, ImVec2& p_closest, ImVec2& p_last, float& p_closest_dist2, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level)
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = ((x2 - x4) * dy - (y2 - y4) * dx);
    float d3 = ((x3 - x4) * dy - (y3 - y4) * dx);
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy))
    {
        ImVec2 p_current(x4, y4);
        ImVec2 p_line = ImLineClosestPoint(p_last, p_current, p);
        float dist2 = ImLengthSqr(p - p_line);
        if (dist2 < p_closest_dist2)
        {
            p_closest = p_line;
            p_closest_dist2 = dist2;
        }
        p_last = p_current;
    }
    else if (level < 10)
    {
        float x12 = (x1 + x2)*0.5f,       y12 = (y1 + y2)*0.5f;
        float x23 = (x2 + x3)*0.5f,       y23 = (y2 + y3)*0.5f;
        float x34 = (x3 + x4)*0.5f,       y34 = (y3 + y4)*0.5f;
        float x123 = (x12 + x23)*0.5f,    y123 = (y12 + y23)*0.5f;
        float x234 = (x23 + x34)*0.5f,    y234 = (y23 + y34)*0.5f;
        float x1234 = (x123 + x234)*0.5f, y1234 = (y123 + y234)*0.5f;
        BezierClosestPointCasteljauStep(p, p_closest, p_last, p_closest_dist2, x1, y1, x12, y12, x123, y123, x1234, y1234, tess_tol, level + 1);
        BezierClosestPointCasteljauStep(p, p_closest, p_last, p_closest_dist2, x1234, y1234, x234, y234, x34, y34, x4, y4, tess_tol, level + 1);
    }
}

ImVec2 ImBezierClosestPointCasteljau(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& p, float tess_tol)
{
    IM_ASSERT(tess_tol > 0.0f);
    ImVec2 p_last = p1;
    ImVec2 p_closest;
    float p_closest_dist2 = FLT_MAX;
    BezierClosestPointCasteljauStep(p, p_closest, p_last, p_closest_dist2, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, tess_tol, 0);
    return p_closest;
}

ImVec2 ImLineClosestPoint(const ImVec2& a, const ImVec2& b, const ImVec2& p)
{
    ImVec2 ap = p - a;
    ImVec2 ab_dir = b - a;
    float dot = ap.x * ab_dir.x + ap.y * ab_dir.y;
    if (dot < 0.0f)
        return a;
    float ab_len_sqr = ab_dir.x * ab_dir.x + ab_dir.y * ab_dir.y;
    if (dot > ab_len_sqr)
        return b;
    return a + ab_dir * dot / ab_len_sqr;
}

bool ImTriangleContainsPoint(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p)
{
    bool b1 = ((p.x - b.x) * (a.y - b.y) - (p.y - b.y) * (a.x - b.x)) < 0.0f;
    bool b2 = ((p.x - c.x) * (b.y - c.y) - (p.y - c.y) * (b.x - c.x)) < 0.0f;
    bool b3 = ((p.x - a.x) * (c.y - a.y) - (p.y - a.y) * (c.x - a.x)) < 0.0f;
    return ((b1 == b2) && (b2 == b3));
}

void ImTriangleBarycentricCoords(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p, float& out_u, float& out_v, float& out_w)
{
    ImVec2 v0 = b - a;
    ImVec2 v1 = c - a;
    ImVec2 v2 = p - a;
    const float denom = v0.x * v1.y - v1.x * v0.y;
    out_v = (v2.x * v1.y - v1.x * v2.y) / denom;
    out_w = (v0.x * v2.y - v2.x * v0.y) / denom;
    out_u = 1.0f - out_v - out_w;
}

ImVec2 ImTriangleClosestPoint(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p)
{
    ImVec2 proj_ab = ImLineClosestPoint(a, b, p);
    ImVec2 proj_bc = ImLineClosestPoint(b, c, p);
    ImVec2 proj_ca = ImLineClosestPoint(c, a, p);
    float dist2_ab = ImLengthSqr(p - proj_ab);
    float dist2_bc = ImLengthSqr(p - proj_bc);
    float dist2_ca = ImLengthSqr(p - proj_ca);
    float m = ImMin(dist2_ab, ImMin(dist2_bc, dist2_ca));
    if (m == dist2_ab)
        return proj_ab;
    if (m == dist2_bc)
        return proj_bc;
    return proj_ca;
}

int ImStricmp(const char* str1, const char* str2)
{
    int d;
    while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; }
    return d;
}

int ImStrnicmp(const char* str1, const char* str2, size_t count)
{
    int d = 0;
    while (count > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; count--; }
    return d;
}

void ImStrncpy(char* dst, const char* src, size_t count)
{
    if (count < 1)
        return;
    if (count > 1)
        strncpy(dst, src, count - 1);
    dst[count - 1] = 0;
}

char* ImStrdup(const char* str)
{
    size_t len = strlen(str);
    void* buf = IM_ALLOC(len + 1);
    return (char*)memcpy(buf, (const void*)str, len + 1);
}

char* ImStrdupcpy(char* dst, size_t* p_dst_size, const char* src)
{
    size_t dst_buf_size = p_dst_size ? *p_dst_size : strlen(dst) + 1;
    size_t src_size = strlen(src) + 1;
    if (dst_buf_size < src_size)
    {
        IM_FREE(dst);
        dst = (char*)IM_ALLOC(src_size);
        if (p_dst_size)
            *p_dst_size = src_size;
    }
    return (char*)memcpy(dst, (const void*)src, src_size);
}

const char* ImStrchrRange(const char* str, const char* str_end, char c)
{
    const char* p = (const char*)memchr(str, (int)c, str_end - str);
    return p;
}

int ImStrlenW(const ImWchar* str)
{
    int n = 0;
    while (*str++) n++;
    return n;
}

const char* ImStreolRange(const char* str, const char* str_end)
{
    const char* p = (const char*)memchr(str, '\n', str_end - str);
    return p ? p : str_end;
}

const ImWchar* ImStrbolW(const ImWchar* buf_mid_line, const ImWchar* buf_begin)   
{
    while (buf_mid_line > buf_begin && buf_mid_line[-1] != '\n')
        buf_mid_line--;
    return buf_mid_line;
}

const char* ImStristr(const char* haystack, const char* haystack_end, const char* needle, const char* needle_end)
{
    if (!needle_end)
        needle_end = needle + strlen(needle);

    const char un0 = (char)toupper(*needle);
    while ((!haystack_end && *haystack) || (haystack_end && haystack < haystack_end))
    {
        if (toupper(*haystack) == un0)
        {
            const char* b = needle + 1;
            for (const char* a = haystack + 1; b < needle_end; a++, b++)
                if (toupper(*a) != toupper(*b))
                    break;
            if (b == needle_end)
                return haystack;
        }
        haystack++;
    }
    return NULL;
}

void ImStrTrimBlanks(char* buf)
{
    char* p = buf;
    while (p[0] == ' ' || p[0] == '\t')       
        p++;
    char* p_start = p;
    while (*p != 0)                             
        p++;
    while (p > p_start && (p[-1] == ' ' || p[-1] == '\t'))    
        p--;
    if (p_start != buf)                            
        memmove(buf, p_start, p - p_start);
    buf[p - p_start] = 0;                     
}

const char* ImStrSkipBlank(const char* str)
{
    while (str[0] == ' ' || str[0] == '\t')
        str++;
    return str;
}

#ifndef IMGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS

#ifdef IMGUI_USE_STB_SPRINTF
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#endif

#if defined(_MSC_VER) && !defined(vsnprintf)
#define vsnprintf _vsnprintf
#endif

int ImFormatString(char* buf, size_t buf_size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef IMGUI_USE_STB_SPRINTF
    int w = stbsp_vsnprintf(buf, (int)buf_size, fmt, args);
#else
    int w = vsnprintf(buf, buf_size, fmt, args);
#endif
    va_end(args);
    if (buf == NULL)
        return w;
    if (w == -1 || w >= (int)buf_size)
        w = (int)buf_size - 1;
    buf[w] = 0;
    return w;
}

int ImFormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args)
{
#ifdef IMGUI_USE_STB_SPRINTF
    int w = stbsp_vsnprintf(buf, (int)buf_size, fmt, args);
#else
    int w = vsnprintf(buf, buf_size, fmt, args);
#endif
    if (buf == NULL)
        return w;
    if (w == -1 || w >= (int)buf_size)
        w = (int)buf_size - 1;
    buf[w] = 0;
    return w;
}
#endif   

static const ImU32 GCrc32LookupTable[256] =
{
    0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
    0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
    0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
    0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,
    0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
    0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
    0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
    0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
    0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
    0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
    0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
    0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
    0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
    0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
    0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
    0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D,
};

ImGuiID ImHashData(const void* data_p, size_t data_size, ImU32 seed)
{
    ImU32 crc = ~seed;
    const unsigned char* data = (const unsigned char*)data_p;
    const ImU32* crc32_lut = GCrc32LookupTable;
    while (data_size-- != 0)
        crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ *data++];
    return ~crc;
}

ImGuiID ImHashStr(const char* data_p, size_t data_size, ImU32 seed)
{
    seed = ~seed;
    ImU32 crc = seed;
    const unsigned char* data = (const unsigned char*)data_p;
    const ImU32* crc32_lut = GCrc32LookupTable;
    if (data_size != 0)
    {
        while (data_size-- != 0)
        {
            unsigned char c = *data++;
            if (c == '#' && data_size >= 2 && data[0] == '#' && data[1] == '#')
                crc = seed;
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        }
    }
    else
    {
        while (unsigned char c = *data++)
        {
            if (c == '#' && data[0] == '#' && data[1] == '#')
                crc = seed;
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        }
    }
    return ~crc;
}

#ifndef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

ImFileHandle ImFileOpen(const char* filename, const char* mode)
{
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && !defined(__CYGWIN__) && !defined(__GNUC__)
    const int filename_wsize = ::MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
    const int mode_wsize = ::MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0);
    ImVector<ImWchar> buf;
    buf.resize(filename_wsize + mode_wsize);
    ::MultiByteToWideChar(CP_UTF8, 0, filename, -1, (wchar_t*)&buf[0], filename_wsize);
    ::MultiByteToWideChar(CP_UTF8, 0, mode, -1, (wchar_t*)&buf[filename_wsize], mode_wsize);
    return ::_wfopen((const wchar_t*)&buf[0], (const wchar_t*)&buf[filename_wsize]);
#else
    return fopen(filename, mode);
#endif
}

bool    ImFileClose(ImFileHandle f)     { return fclose(f) == 0; }
ImU64   ImFileGetSize(ImFileHandle f)   { long off = 0, sz = 0; return ((off = ftell(f)) != -1 && !fseek(f, 0, SEEK_END) && (sz = ftell(f)) != -1 && !fseek(f, off, SEEK_SET)) ? (ImU64)sz : (ImU64)-1; }
ImU64   ImFileRead(void* data, ImU64 sz, ImU64 count, ImFileHandle f)           { return fread(data, (size_t)sz, (size_t)count, f); }
ImU64   ImFileWrite(const void* data, ImU64 sz, ImU64 count, ImFileHandle f)    { return fwrite(data, (size_t)sz, (size_t)count, f); }
#endif   

void*   ImFileLoadToMemory(const char* filename, const char* mode, size_t* out_file_size, int padding_bytes)
{
    IM_ASSERT(filename && mode);
    if (out_file_size)
        *out_file_size = 0;

    ImFileHandle f;
    if ((f = ImFileOpen(filename, mode)) == NULL)
        return NULL;

    size_t file_size = (size_t)ImFileGetSize(f);
    if (file_size == (size_t)-1)
    {
        ImFileClose(f);
        return NULL;
    }

    void* file_data = IM_ALLOC(file_size + padding_bytes);
    if (file_data == NULL)
    {
        ImFileClose(f);
        return NULL;
    }
    if (ImFileRead(file_data, 1, file_size, f) != file_size)
    {
        ImFileClose(f);
        IM_FREE(file_data);
        return NULL;
    }
    if (padding_bytes > 0)
        memset((void*)(((char*)file_data) + file_size), 0, (size_t)padding_bytes);

    ImFileClose(f);
    if (out_file_size)
        *out_file_size = file_size;

    return file_data;
}

int ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end)
{
    static const char lengths[32] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0 };
    static const int masks[]  = { 0x00, 0x7f, 0x1f, 0x0f, 0x07 };
    static const uint32_t mins[] = { 0x400000, 0, 0x80, 0x800, 0x10000 };
    static const int shiftc[] = { 0, 18, 12, 6, 0 };
    static const int shifte[] = { 0, 6, 4, 2, 0 };
    int len = lengths[*(const unsigned char*)in_text >> 3];
    int wanted = len + !len;

    if (in_text_end == NULL)
        in_text_end = in_text + wanted;         

    unsigned char s[4];
    s[0] = in_text + 0 < in_text_end ? in_text[0] : 0;
    s[1] = in_text + 1 < in_text_end ? in_text[1] : 0;
    s[2] = in_text + 2 < in_text_end ? in_text[2] : 0;
    s[3] = in_text + 3 < in_text_end ? in_text[3] : 0;

    *out_char  = (uint32_t)(s[0] & masks[len]) << 18;
    *out_char |= (uint32_t)(s[1] & 0x3f) << 12;
    *out_char |= (uint32_t)(s[2] & 0x3f) <<  6;
    *out_char |= (uint32_t)(s[3] & 0x3f) <<  0;
    *out_char >>= shiftc[len];

    int e = 0;
    e  = (*out_char < mins[len]) << 6;   
    e |= ((*out_char >> 11) == 0x1b) << 7;    
    e |= (*out_char > IM_UNICODE_CODEPOINT_MAX) << 8;     
    e |= (s[1] & 0xc0) >> 2;
    e |= (s[2] & 0xc0) >> 4;
    e |= (s[3]       ) >> 6;
    e ^= 0x2a;         
    e >>= shifte[len];

    if (e)
    {
        wanted = ImMin(wanted, !!s[0] + !!s[1] + !!s[2] + !!s[3]);
        *out_char = IM_UNICODE_CODEPOINT_INVALID;
    }

    return wanted;
}

int ImTextStrFromUtf8(ImWchar* buf, int buf_size, const char* in_text, const char* in_text_end, const char** in_text_remaining)
{
    ImWchar* buf_out = buf;
    ImWchar* buf_end = buf + buf_size;
    while (buf_out < buf_end - 1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        if (c == 0)
            break;
        *buf_out++ = (ImWchar)c;
    }
    *buf_out = 0;
    if (in_text_remaining)
        *in_text_remaining = in_text;
    return (int)(buf_out - buf);
}

int ImTextCountCharsFromUtf8(const char* in_text, const char* in_text_end)
{
    int char_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c;
        in_text += ImTextCharFromUtf8(&c, in_text, in_text_end);
        if (c == 0)
            break;
        char_count++;
    }
    return char_count;
}

static inline int ImTextCharToUtf8(char* buf, int buf_size, unsigned int c)
{
    if (c < 0x80)
    {
        buf[0] = (char)c;
        return 1;
    }
    if (c < 0x800)
    {
        if (buf_size < 2) return 0;
        buf[0] = (char)(0xc0 + (c >> 6));
        buf[1] = (char)(0x80 + (c & 0x3f));
        return 2;
    }
    if (c < 0x10000)
    {
        if (buf_size < 3) return 0;
        buf[0] = (char)(0xe0 + (c >> 12));
        buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[2] = (char)(0x80 + ((c ) & 0x3f));
        return 3;
    }
    if (c <= 0x10FFFF)
    {
        if (buf_size < 4) return 0;
        buf[0] = (char)(0xf0 + (c >> 18));
        buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
        buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
        buf[3] = (char)(0x80 + ((c ) & 0x3f));
        return 4;
    }
    return 0;
}

int ImTextCountUtf8BytesFromChar(const char* in_text, const char* in_text_end)
{
    unsigned int unused = 0;
    return ImTextCharFromUtf8(&unused, in_text, in_text_end);
}

static inline int ImTextCountUtf8BytesFromChar(unsigned int c)
{
    if (c < 0x80) return 1;
    if (c < 0x800) return 2;
    if (c < 0x10000) return 3;
    if (c <= 0x10FFFF) return 4;
    return 3;
}

int ImTextStrToUtf8(char* buf, int buf_size, const ImWchar* in_text, const ImWchar* in_text_end)
{
    char* buf_out = buf;
    const char* buf_end = buf + buf_size;
    while (buf_out < buf_end - 1 && (!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c = (unsigned int)(*in_text++);
        if (c < 0x80)
            *buf_out++ = (char)c;
        else
            buf_out += ImTextCharToUtf8(buf_out, (int)(buf_end - buf_out - 1), c);
    }
    *buf_out = 0;
    return (int)(buf_out - buf);
}

int ImTextCountUtf8BytesFromStr(const ImWchar* in_text, const ImWchar* in_text_end)
{
    int bytes_count = 0;
    while ((!in_text_end || in_text < in_text_end) && *in_text)
    {
        unsigned int c = (unsigned int)(*in_text++);
        if (c < 0x80)
            bytes_count++;
        else
            bytes_count += ImTextCountUtf8BytesFromChar(c);
    }
    return bytes_count;
}

IMGUI_API ImU32 ImAlphaBlendColors(ImU32 col_a, ImU32 col_b)
{
    float t = ((col_b >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;
    int r = ImLerp((int)(col_a >> IM_COL32_R_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_R_SHIFT) & 0xFF, t);
    int g = ImLerp((int)(col_a >> IM_COL32_G_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_G_SHIFT) & 0xFF, t);
    int b = ImLerp((int)(col_a >> IM_COL32_B_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_B_SHIFT) & 0xFF, t);
    return IM_COL32(r, g, b, 0xFF);
}

ImVec4 ImGui::ColorConvertU32ToFloat4(ImU32 in)
{
    float s = 1.0f / 255.0f;
    return ImVec4(
        ((in >> IM_COL32_R_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_G_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_B_SHIFT) & 0xFF) * s,
        ((in >> IM_COL32_A_SHIFT) & 0xFF) * s);
}

ImU32 ImGui::ColorConvertFloat4ToU32(const ImVec4& in)
{
    ImU32 out;
    out  = ((ImU32)IM_F32_TO_INT8_SAT(in.x)) << IM_COL32_R_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.y)) << IM_COL32_G_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.z)) << IM_COL32_B_SHIFT;
    out |= ((ImU32)IM_F32_TO_INT8_SAT(in.w)) << IM_COL32_A_SHIFT;
    return out;
}

void ImGui::ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v)
{
    float K = 0.f;
    if (g < b)
    {
        ImSwap(g, b);
        K = -1.f;
    }
    if (r < g)
    {
        ImSwap(r, g);
        K = -2.f / 6.f - K;
    }

    const float chroma = r - (g < b ? g : b);
    out_h = ImFabs(K + (g - b) / (6.f * chroma + 1e-20f));
    out_s = chroma / (r + 1e-20f);
    out_v = r;
}

void ImGui::ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b)
{
    if (s == 0.0f)
    {
        out_r = out_g = out_b = v;
        return;
    }

    h = ImFmod(h, 1.0f) / (60.0f / 360.0f);
    int   i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i)
    {
    case 0: out_r = v; out_g = t; out_b = p; break;
    case 1: out_r = q; out_g = v; out_b = p; break;
    case 2: out_r = p; out_g = v; out_b = t; break;
    case 3: out_r = p; out_g = q; out_b = v; break;
    case 4: out_r = t; out_g = p; out_b = v; break;
    case 5: default: out_r = v; out_g = p; out_b = q; break;
    }
}

static ImGuiStorage::ImGuiStoragePair* LowerBound(ImVector<ImGuiStorage::ImGuiStoragePair>& data, ImGuiID key)
{
    ImGuiStorage::ImGuiStoragePair* first = data.Data;
    ImGuiStorage::ImGuiStoragePair* last = data.Data + data.Size;
    size_t count = (size_t)(last - first);
    while (count > 0)
    {
        size_t count2 = count >> 1;
        ImGuiStorage::ImGuiStoragePair* mid = first + count2;
        if (mid->key < key)
        {
            first = ++mid;
            count -= count2 + 1;
        }
        else
        {
            count = count2;
        }
    }
    return first;
}

void ImGuiStorage::BuildSortByKey()
{
    struct StaticFunc
    {
        static int IMGUI_CDECL PairCompareByID(const void* lhs, const void* rhs)
        {
            if (((const ImGuiStoragePair*)lhs)->key > ((const ImGuiStoragePair*)rhs)->key) return +1;
            if (((const ImGuiStoragePair*)lhs)->key < ((const ImGuiStoragePair*)rhs)->key) return -1;
            return 0;
        }
    };
    if (Data.Size > 1)
        ImQsort(Data.Data, (size_t)Data.Size, sizeof(ImGuiStoragePair), StaticFunc::PairCompareByID);
}

int ImGuiStorage::GetInt(ImGuiID key, int default_val) const
{
    ImGuiStoragePair* it = LowerBound(const_cast<ImVector<ImGuiStoragePair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_i;
}

bool ImGuiStorage::GetBool(ImGuiID key, bool default_val) const
{
    return GetInt(key, default_val ? 1 : 0) != 0;
}

float ImGuiStorage::GetFloat(ImGuiID key, float default_val) const
{
    ImGuiStoragePair* it = LowerBound(const_cast<ImVector<ImGuiStoragePair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return default_val;
    return it->val_f;
}

void* ImGuiStorage::GetVoidPtr(ImGuiID key) const
{
    ImGuiStoragePair* it = LowerBound(const_cast<ImVector<ImGuiStoragePair>&>(Data), key);
    if (it == Data.end() || it->key != key)
        return NULL;
    return it->val_p;
}

int* ImGuiStorage::GetIntRef(ImGuiID key, int default_val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_i;
}

bool* ImGuiStorage::GetBoolRef(ImGuiID key, bool default_val)
{
    return (bool*)GetIntRef(key, default_val ? 1 : 0);
}

float* ImGuiStorage::GetFloatRef(ImGuiID key, float default_val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_f;
}

void** ImGuiStorage::GetVoidPtrRef(ImGuiID key, void* default_val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
        it = Data.insert(it, ImGuiStoragePair(key, default_val));
    return &it->val_p;
}

void ImGuiStorage::SetInt(ImGuiID key, int val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, ImGuiStoragePair(key, val));
        return;
    }
    it->val_i = val;
}

void ImGuiStorage::SetBool(ImGuiID key, bool val)
{
    SetInt(key, val ? 1 : 0);
}

void ImGuiStorage::SetFloat(ImGuiID key, float val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, ImGuiStoragePair(key, val));
        return;
    }
    it->val_f = val;
}

void ImGuiStorage::SetVoidPtr(ImGuiID key, void* val)
{
    ImGuiStoragePair* it = LowerBound(Data, key);
    if (it == Data.end() || it->key != key)
    {
        Data.insert(it, ImGuiStoragePair(key, val));
        return;
    }
    it->val_p = val;
}

void ImGuiStorage::SetAllInt(int v)
{
    for (int i = 0; i < Data.Size; i++)
        Data[i].val_i = v;
}

ImGuiTextFilter::ImGuiTextFilter(const char* default_filter)
{
    if (default_filter)
    {
        ImStrncpy(InputBuf, default_filter, IM_ARRAYSIZE(InputBuf));
        Build();
    }
    else
    {
        InputBuf[0] = 0;
        CountGrep = 0;
    }
}

bool ImGuiTextFilter::Draw(const char* label, float width)
{
    if (width != 0.0f)
        ImGui::SetNextItemWidth(width);
    bool value_changed = ImGui::InputText(label, InputBuf, IM_ARRAYSIZE(InputBuf));
    if (value_changed)
        Build();
    return value_changed;
}

void ImGuiTextFilter::ImGuiTextRange::split(char separator, ImVector<ImGuiTextRange>* out) const
{
    out->resize(0);
    const char* wb = b;
    const char* we = wb;
    while (we < e)
    {
        if (*we == separator)
        {
            out->push_back(ImGuiTextRange(wb, we));
            wb = we + 1;
        }
        we++;
    }
    if (wb != we)
        out->push_back(ImGuiTextRange(wb, we));
}

void ImGuiTextFilter::Build()
{
    Filters.resize(0);
    ImGuiTextRange input_range(InputBuf, InputBuf + strlen(InputBuf));
    input_range.split(',', &Filters);

    CountGrep = 0;
    for (int i = 0; i != Filters.Size; i++)
    {
        ImGuiTextRange& f = Filters[i];
        while (f.b < f.e && ImCharIsBlankA(f.b[0]))
            f.b++;
        while (f.e > f.b && ImCharIsBlankA(f.e[-1]))
            f.e--;
        if (f.empty())
            continue;
        if (Filters[i].b[0] != '-')
            CountGrep += 1;
    }
}

bool ImGuiTextFilter::PassFilter(const char* text, const char* text_end) const
{
    if (Filters.empty())
        return true;

    if (text == NULL)
        text = "";

    for (int i = 0; i != Filters.Size; i++)
    {
        const ImGuiTextRange& f = Filters[i];
        if (f.empty())
            continue;
        if (f.b[0] == '-')
        {
            if (ImStristr(text, text_end, f.b + 1, f.e) != NULL)
                return false;
        }
        else
        {
            if (ImStristr(text, text_end, f.b, f.e) != NULL)
                return true;
        }
    }

    if (CountGrep == 0)
        return true;

    return false;
}

#ifndef va_copy
#if defined(__GNUC__) || defined(__clang__)
#define va_copy(dest, src) __builtin_va_copy(dest, src)
#else
#define va_copy(dest, src) (dest = src)
#endif
#endif

char ImGuiTextBuffer::EmptyString[1] = { 0 };

void ImGuiTextBuffer::append(const char* str, const char* str_end)
{
    int len = str_end ? (int)(str_end - str) : (int)strlen(str);

    const int write_off = (Buf.Size != 0) ? Buf.Size : 1;
    const int needed_sz = write_off + len;
    if (write_off + len >= Buf.Capacity)
    {
        int new_capacity = Buf.Capacity * 2;
        Buf.reserve(needed_sz > new_capacity ? needed_sz : new_capacity);
    }

    Buf.resize(needed_sz);
    memcpy(&Buf[write_off - 1], str, (size_t)len);
    Buf[write_off - 1 + len] = 0;
}

void ImGuiTextBuffer::appendf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    appendfv(fmt, args);
    va_end(args);
}

void ImGuiTextBuffer::appendfv(const char* fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    int len = ImFormatStringV(NULL, 0, fmt, args);                      
    if (len <= 0)
    {
        va_end(args_copy);
        return;
    }

    const int write_off = (Buf.Size != 0) ? Buf.Size : 1;
    const int needed_sz = write_off + len;
    if (write_off + len >= Buf.Capacity)
    {
        int new_capacity = Buf.Capacity * 2;
        Buf.reserve(needed_sz > new_capacity ? needed_sz : new_capacity);
    }

    Buf.resize(needed_sz);
    ImFormatStringV(&Buf[write_off - 1], (size_t)len + 1, fmt, args_copy);
    va_end(args_copy);
}

static bool GetSkipItemForListClipping()
{
    ImGuiContext& g = *GImGui;
    return (g.CurrentTable ? g.CurrentTable->HostSkipItems : g.CurrentWindow->SkipItems);
}

void ImGui::CalcListClipping(int items_count, float items_height, int* out_items_display_start, int* out_items_display_end)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.LogEnabled)
    {
        *out_items_display_start = 0;
        *out_items_display_end = items_count;
        return;
    }
    if (GetSkipItemForListClipping())
    {
        *out_items_display_start = *out_items_display_end = 0;
        return;
    }

    ImRect unclipped_rect = window->ClipRect;
    if (g.NavMoveRequest)
        unclipped_rect.Add(g.NavScoringRect);
    if (g.NavJustMovedToId && window->NavLastIds[0] == g.NavJustMovedToId)
        unclipped_rect.Add(ImRect(window->Pos + window->NavRectRel[0].Min, window->Pos + window->NavRectRel[0].Max));

    const ImVec2 pos = window->DC.CursorPos;
    int start = (int)((unclipped_rect.Min.y - pos.y) / items_height);
    int end = (int)((unclipped_rect.Max.y - pos.y) / items_height);

    if (g.NavMoveRequest && g.NavMoveClipDir == ImGuiDir_Up)
        start--;
    if (g.NavMoveRequest && g.NavMoveClipDir == ImGuiDir_Down)
        end++;

    start = ImClamp(start, 0, items_count);
    end = ImClamp(end + 1, start, items_count);
    *out_items_display_start = start;
    *out_items_display_end = end;
}

static void SetCursorPosYAndSetupForPrevLine(float pos_y, float line_height)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    float off_y = pos_y - window->DC.CursorPos.y;
    window->DC.CursorPos.y = pos_y;
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, pos_y);
    window->DC.CursorPosPrevLine.y = window->DC.CursorPos.y - line_height;                  
    window->DC.PrevLineSize.y = (line_height - g.Style.ItemSpacing.y);                                         
    if (ImGuiOldColumns* columns = window->DC.CurrentColumns)
        columns->LineMinY = window->DC.CursorPos.y;                                   
    if (ImGuiTable* table = g.CurrentTable)
    {
        if (table->IsInsideRow)
            ImGui::TableEndRow(table);
        table->RowPosY2 = window->DC.CursorPos.y;
        const int row_increase = (int)((off_y / line_height) + 0.5f);
        table->RowBgColorCounter += row_increase;
    }
}

ImGuiListClipper::ImGuiListClipper()
{
    memset(this, 0, sizeof(*this));
    ItemsCount = -1;
}

ImGuiListClipper::~ImGuiListClipper()
{
    IM_ASSERT(ItemsCount == -1 && "Forgot to call End(), or to Step() until false?");
}

void ImGuiListClipper::Begin(int items_count, float items_height)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (ImGuiTable* table = g.CurrentTable)
        if (table->IsInsideRow)
            ImGui::TableEndRow(table);

    StartPosY = window->DC.CursorPos.y;
    ItemsHeight = items_height;
    ItemsCount = items_count;
    ItemsFrozen = 0;
    StepNo = 0;
    DisplayStart = -1;
    DisplayEnd = 0;
}

void ImGuiListClipper::End()
{
    if (ItemsCount < 0)   
        return;

    if (ItemsCount < INT_MAX && DisplayStart >= 0)
        SetCursorPosYAndSetupForPrevLine(StartPosY + (ItemsCount - ItemsFrozen) * ItemsHeight, ItemsHeight);
    ItemsCount = -1;
    StepNo = 3;
}

bool ImGuiListClipper::Step()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImGuiTable* table = g.CurrentTable;
    if (table && table->IsInsideRow)
        ImGui::TableEndRow(table);

    if (DisplayEnd >= ItemsCount || GetSkipItemForListClipping())
    {
        End();
        return false;
    }

    if (StepNo == 0)
    {
        if (table != NULL && !table->IsUnfrozen)
        {
            DisplayStart = ItemsFrozen;
            DisplayEnd = ItemsFrozen + 1;
            ItemsFrozen++;
            return true;
        }

        StartPosY = window->DC.CursorPos.y;
        if (ItemsHeight <= 0.0f)
        {
            DisplayStart = ItemsFrozen;
            DisplayEnd = ItemsFrozen + 1;
            StepNo = 1;
            return true;
        }

        DisplayStart = DisplayEnd;
        StepNo = 2;
    }

    if (StepNo == 1)
    {
        IM_ASSERT(ItemsHeight <= 0.0f);
        if (table)
        {
            const float pos_y1 = table->RowPosY1;               
            const float pos_y2 = table->RowPosY2;              
            ItemsHeight = pos_y2 - pos_y1;
            window->DC.CursorPos.y = pos_y2;
        }
        else
        {
            ItemsHeight = window->DC.CursorPos.y - StartPosY;
        }
        IM_ASSERT(ItemsHeight > 0.0f && "Unable to calculate item height! First item hasn't moved the cursor vertically!");
        StepNo = 2;
    }

    if (StepNo == 2)
    {
        IM_ASSERT(ItemsHeight > 0.0f);

        int already_submitted = DisplayEnd;
        ImGui::CalcListClipping(ItemsCount - already_submitted, ItemsHeight, &DisplayStart, &DisplayEnd);
        DisplayStart += already_submitted;
        DisplayEnd += already_submitted;

        if (DisplayStart > already_submitted)
            SetCursorPosYAndSetupForPrevLine(StartPosY + (DisplayStart - ItemsFrozen) * ItemsHeight, ItemsHeight);

        StepNo = 3;
        return true;
    }

    if (StepNo == 3)
    {
        if (ItemsCount < INT_MAX)
            SetCursorPosYAndSetupForPrevLine(StartPosY + (ItemsCount - ItemsFrozen) * ItemsHeight, ItemsHeight);   
        ItemsCount = -1;
        return false;
    }

    IM_ASSERT(0);
    return false;
}

ImGuiStyle& ImGui::GetStyle()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext() ?");
    return GImGui->Style;
}

ImU32 ImGui::GetColorU32(ImGuiCol idx, float alpha_mul)
{
    ImGuiStyle& style = GImGui->Style;
    ImVec4 c = style.Colors[idx];
    c.w *= style.Alpha * alpha_mul;
    return ColorConvertFloat4ToU32(c);
}

ImU32 ImGui::GetColorU32(const ImVec4& col)
{
    ImGuiStyle& style = GImGui->Style;
    ImVec4 c = col;
    c.w *= style.Alpha;
    return ColorConvertFloat4ToU32(c);
}

const ImVec4& ImGui::GetStyleColorVec4(ImGuiCol idx)
{
    ImGuiStyle& style = GImGui->Style;
    return style.Colors[idx];
}

ImU32 ImGui::GetColorU32(ImU32 col)
{
    ImGuiStyle& style = GImGui->Style;
    if (style.Alpha >= 1.0f)
        return col;
    ImU32 a = (col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT;
    a = (ImU32)(a * style.Alpha);             
    return (col & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

void ImGui::PushStyleColor(ImGuiCol idx, ImU32 col)
{
    ImGuiContext& g = *GImGui;
    ImGuiColorMod backup;
    backup.Col = idx;
    backup.BackupValue = g.Style.Colors[idx];
    g.ColorStack.push_back(backup);
    g.Style.Colors[idx] = ColorConvertU32ToFloat4(col);
}

void ImGui::PushStyleColor(ImGuiCol idx, const ImVec4& col)
{
    ImGuiContext& g = *GImGui;
    ImGuiColorMod backup;
    backup.Col = idx;
    backup.BackupValue = g.Style.Colors[idx];
    g.ColorStack.push_back(backup);
    g.Style.Colors[idx] = col;
}

void ImGui::PopStyleColor(int count)
{
    ImGuiContext& g = *GImGui;
    while (count > 0)
    {
        ImGuiColorMod& backup = g.ColorStack.back();
        g.Style.Colors[backup.Col] = backup.BackupValue;
        g.ColorStack.pop_back();
        count--;
    }
}

struct ImGuiStyleVarInfo
{
    ImGuiDataType   Type;
    ImU32           Count;
    ImU32           Offset;
    void*           GetVarPtr(ImGuiStyle* style) const { return (void*)((unsigned char*)style + Offset); }
};

static const ImGuiStyleVarInfo GStyleVarInfo[] =
{
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, Alpha) },                
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowPadding) },        
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowRounding) },       
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowBorderSize) },     
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowMinSize) },        
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowTitleAlign) },     
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ChildRounding) },        
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ChildBorderSize) },      
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, PopupRounding) },        
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, PopupBorderSize) },      
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, FramePadding) },         
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, FrameRounding) },        
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, FrameBorderSize) },      
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ItemSpacing) },          
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ItemInnerSpacing) },     
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, IndentSpacing) },        
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, CellPadding) },          
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ScrollbarSize) },        
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ScrollbarRounding) },    
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, GrabMinSize) },          
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, GrabRounding) },         
    { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, TabRounding) },          
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ButtonTextAlign) },      
    { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, SelectableTextAlign) },  
};

static const ImGuiStyleVarInfo* GetStyleVarInfo(ImGuiStyleVar idx)
{
    IM_ASSERT(idx >= 0 && idx < ImGuiStyleVar_COUNT);
    IM_ASSERT(IM_ARRAYSIZE(GStyleVarInfo) == ImGuiStyleVar_COUNT);
    return &GStyleVarInfo[idx];
}

void ImGui::PushStyleVar(ImGuiStyleVar idx, float val)
{
    const ImGuiStyleVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 1)
    {
        ImGuiContext& g = *GImGui;
        float* pvar = (float*)var_info->GetVarPtr(&g.Style);
        g.StyleVarStack.push_back(ImGuiStyleMod(idx, *pvar));
        *pvar = val;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() float variant but variable is not a float!");
}

void ImGui::PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
{
    const ImGuiStyleVarInfo* var_info = GetStyleVarInfo(idx);
    if (var_info->Type == ImGuiDataType_Float && var_info->Count == 2)
    {
        ImGuiContext& g = *GImGui;
        ImVec2* pvar = (ImVec2*)var_info->GetVarPtr(&g.Style);
        g.StyleVarStack.push_back(ImGuiStyleMod(idx, *pvar));
        *pvar = val;
        return;
    }
    IM_ASSERT(0 && "Called PushStyleVar() ImVec2 variant but variable is not a ImVec2!");
}

void ImGui::PopStyleVar(int count)
{
    ImGuiContext& g = *GImGui;
    while (count > 0)
    {
        ImGuiStyleMod& backup = g.StyleVarStack.back();
        const ImGuiStyleVarInfo* info = GetStyleVarInfo(backup.VarIdx);
        void* data = info->GetVarPtr(&g.Style);
        if (info->Type == ImGuiDataType_Float && info->Count == 1)      { ((float*)data)[0] = backup.BackupFloat[0]; }
        else if (info->Type == ImGuiDataType_Float && info->Count == 2) { ((float*)data)[0] = backup.BackupFloat[0]; ((float*)data)[1] = backup.BackupFloat[1]; }
        g.StyleVarStack.pop_back();
        count--;
    }
}

const char* ImGui::GetStyleColorName(ImGuiCol idx)
{
    switch (idx)
    {
    case ImGuiCol_Text: return "Text";
    case ImGuiCol_TextDisabled: return "TextDisabled";
    case ImGuiCol_WindowBg: return "WindowBg";
    case ImGuiCol_ChildBg: return "ChildBg";
    case ImGuiCol_PopupBg: return "PopupBg";
    case ImGuiCol_Border: return "Border";
    case ImGuiCol_BorderShadow: return "BorderShadow";
    case ImGuiCol_FrameBg: return "FrameBg";
    case ImGuiCol_FrameBgHovered: return "FrameBgHovered";
    case ImGuiCol_FrameBgActive: return "FrameBgActive";
    case ImGuiCol_TitleBg: return "TitleBg";
    case ImGuiCol_TitleBgActive: return "TitleBgActive";
    case ImGuiCol_TitleBgCollapsed: return "TitleBgCollapsed";
    case ImGuiCol_MenuBarBg: return "MenuBarBg";
    case ImGuiCol_ScrollbarBg: return "ScrollbarBg";
    case ImGuiCol_ScrollbarGrab: return "ScrollbarGrab";
    case ImGuiCol_ScrollbarGrabHovered: return "ScrollbarGrabHovered";
    case ImGuiCol_ScrollbarGrabActive: return "ScrollbarGrabActive";
    case ImGuiCol_CheckMark: return "CheckMark";
    case ImGuiCol_SliderGrab: return "SliderGrab";
    case ImGuiCol_SliderGrabActive: return "SliderGrabActive";
    case ImGuiCol_Button: return "Button";
    case ImGuiCol_ButtonHovered: return "ButtonHovered";
    case ImGuiCol_ButtonActive: return "ButtonActive";
    case ImGuiCol_Header: return "Header";
    case ImGuiCol_HeaderHovered: return "HeaderHovered";
    case ImGuiCol_HeaderActive: return "HeaderActive";
    case ImGuiCol_Separator: return "Separator";
    case ImGuiCol_SeparatorHovered: return "SeparatorHovered";
    case ImGuiCol_SeparatorActive: return "SeparatorActive";
    case ImGuiCol_ResizeGrip: return "ResizeGrip";
    case ImGuiCol_ResizeGripHovered: return "ResizeGripHovered";
    case ImGuiCol_ResizeGripActive: return "ResizeGripActive";
    case ImGuiCol_Tab: return "Tab";
    case ImGuiCol_TabHovered: return "TabHovered";
    case ImGuiCol_TabActive: return "TabActive";
    case ImGuiCol_TabUnfocused: return "TabUnfocused";
    case ImGuiCol_TabUnfocusedActive: return "TabUnfocusedActive";
    case ImGuiCol_DockingPreview: return "DockingPreview";
    case ImGuiCol_DockingEmptyBg: return "DockingEmptyBg";
    case ImGuiCol_PlotLines: return "PlotLines";
    case ImGuiCol_PlotLinesHovered: return "PlotLinesHovered";
    case ImGuiCol_PlotHistogram: return "PlotHistogram";
    case ImGuiCol_PlotHistogramHovered: return "PlotHistogramHovered";
    case ImGuiCol_TableHeaderBg: return "TableHeaderBg";
    case ImGuiCol_TableBorderStrong: return "TableBorderStrong";
    case ImGuiCol_TableBorderLight: return "TableBorderLight";
    case ImGuiCol_TableRowBg: return "TableRowBg";
    case ImGuiCol_TableRowBgAlt: return "TableRowBgAlt";
    case ImGuiCol_TextSelectedBg: return "TextSelectedBg";
    case ImGuiCol_DragDropTarget: return "DragDropTarget";
    case ImGuiCol_NavHighlight: return "NavHighlight";
    case ImGuiCol_NavWindowingHighlight: return "NavWindowingHighlight";
    case ImGuiCol_NavWindowingDimBg: return "NavWindowingDimBg";
    case ImGuiCol_ModalWindowDimBg: return "ModalWindowDimBg";
    }
    IM_ASSERT(0);
    return "Unknown";
}


const char* ImGui::FindRenderedTextEnd(const char* text, const char* text_end)
{
    const char* text_display_end = text;
    if (!text_end)
        text_end = (const char*)-1;

    while (text_display_end < text_end && *text_display_end != '\0' && (text_display_end[0] != '#' || text_display_end[1] != '#'))
        text_display_end++;
    return text_display_end;
}

void ImGui::RenderText(ImVec2 pos, const char* text, const char* text_end, bool hide_text_after_hash)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    const char* text_display_end;
    if (hide_text_after_hash)
    {
        text_display_end = FindRenderedTextEnd(text, text_end);
    }
    else
    {
        if (!text_end)
            text_end = text + strlen(text);  
        text_display_end = text_end;
    }

    if (text != text_display_end)
    {
        window->DrawList->AddText(g.Font, g.FontSize, pos, GetColorU32(ImGuiCol_Text), text, text_display_end);
        if (g.LogEnabled)
            LogRenderedText(&pos, text, text_display_end);
    }
}

void ImGui::RenderTextWrapped(ImVec2 pos, const char* text, const char* text_end, float wrap_width)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (!text_end)
        text_end = text + strlen(text);  

    if (text != text_end)
    {
        window->DrawList->AddText(g.Font, g.FontSize, pos, GetColorU32(ImGuiCol_Text), text, text_end, wrap_width);
        if (g.LogEnabled)
            LogRenderedText(&pos, text, text_end);
    }
}

void ImGui::RenderTextClippedEx(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_display_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect)
{
    ImVec2 pos = pos_min;
    const ImVec2 text_size = text_size_if_known ? *text_size_if_known : CalcTextSize(text, text_display_end, false, 0.0f);

    const ImVec2* clip_min = clip_rect ? &clip_rect->Min : &pos_min;
    const ImVec2* clip_max = clip_rect ? &clip_rect->Max : &pos_max;
    bool need_clipping = (pos.x + text_size.x >= clip_max->x) || (pos.y + text_size.y >= clip_max->y);
    if (clip_rect)          
        need_clipping |= (pos.x < clip_min->x) || (pos.y < clip_min->y);

    if (align.x > 0.0f) pos.x = ImMax(pos.x, pos.x + (pos_max.x - pos.x - text_size.x) * align.x);
    if (align.y > 0.0f) pos.y = ImMax(pos.y, pos.y + (pos_max.y - pos.y - text_size.y) * align.y);

    if (need_clipping)
    {
        ImVec4 fine_clip_rect(clip_min->x, clip_min->y, clip_max->x, clip_max->y);
        draw_list->AddText(NULL, 0.0f, pos, GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, &fine_clip_rect);
    }
    else
    {
        draw_list->AddText(NULL, 0.0f, pos, GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, NULL);
    }
}

void ImGui::RenderTextClipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect)
{
    const char* text_display_end = FindRenderedTextEnd(text, text_end);
    const int text_len = (int)(text_display_end - text);
    if (text_len == 0)
        return;

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    RenderTextClippedEx(window->DrawList, pos_min, pos_max, text, text_display_end, text_size_if_known, align, clip_rect);
    if (g.LogEnabled)
        LogRenderedText(&pos_min, text, text_display_end);
}


void ImGui::RenderTextEllipsis(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, float clip_max_x, float ellipsis_max_x, const char* text, const char* text_end_full, const ImVec2* text_size_if_known)
{
    ImGuiContext& g = *GImGui;
    if (text_end_full == NULL)
        text_end_full = FindRenderedTextEnd(text);
    const ImVec2 text_size = text_size_if_known ? *text_size_if_known : CalcTextSize(text, text_end_full, false, 0.0f);

    if (text_size.x > pos_max.x - pos_min.x)
    {
        const ImFont* font = draw_list->_Data->Font;
        const float font_size = draw_list->_Data->FontSize;
        const char* text_end_ellipsis = NULL;

        ImWchar ellipsis_char = font->EllipsisChar;
        int ellipsis_char_count = 1;
        if (ellipsis_char == (ImWchar)-1)
        {
            ellipsis_char = (ImWchar)'.';
            ellipsis_char_count = 3;
        }
        const ImFontGlyph* glyph = font->FindGlyph(ellipsis_char);

        float ellipsis_glyph_width = glyph->X1;                           
        float ellipsis_total_width = ellipsis_glyph_width;           

        if (ellipsis_char_count > 1)
        {
            const float spacing_between_dots = 1.0f * (draw_list->_Data->FontSize / font->FontSize);
            ellipsis_glyph_width = glyph->X1 - glyph->X0 + spacing_between_dots;
            ellipsis_total_width = ellipsis_glyph_width * (float)ellipsis_char_count - spacing_between_dots;
        }

        const float text_avail_width = ImMax((ImMax(pos_max.x, ellipsis_max_x) - ellipsis_total_width) - pos_min.x, 1.0f);
        float text_size_clipped_x = font->CalcTextSizeA(font_size, text_avail_width, 0.0f, text, text_end_full, &text_end_ellipsis).x;
        if (text == text_end_ellipsis && text_end_ellipsis < text_end_full)
        {
            text_end_ellipsis = text + ImTextCountUtf8BytesFromChar(text, text_end_full);
            text_size_clipped_x = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text, text_end_ellipsis).x;
        }
        while (text_end_ellipsis > text && ImCharIsBlankA(text_end_ellipsis[-1]))
        {
            text_end_ellipsis--;
            text_size_clipped_x -= font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text_end_ellipsis, text_end_ellipsis + 1).x;       
        }

        RenderTextClippedEx(draw_list, pos_min, ImVec2(clip_max_x, pos_max.y), text, text_end_ellipsis, &text_size, ImVec2(0.0f, 0.0f));
        float ellipsis_x = pos_min.x + text_size_clipped_x;
        if (ellipsis_x + ellipsis_total_width <= ellipsis_max_x)
            for (int i = 0; i < ellipsis_char_count; i++)
            {
                font->RenderChar(draw_list, font_size, ImVec2(ellipsis_x, pos_min.y), GetColorU32(ImGuiCol_Text), ellipsis_char);
                ellipsis_x += ellipsis_glyph_width;
            }
    }
    else
    {
        RenderTextClippedEx(draw_list, pos_min, ImVec2(clip_max_x, pos_max.y), text, text_end_full, &text_size, ImVec2(0.0f, 0.0f));
    }

    if (g.LogEnabled)
        LogRenderedText(&pos_min, text, text_end_full);
}

void ImGui::RenderFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border, float rounding)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DrawList->AddRectFilled(p_min, p_max, fill_col, rounding);
    const float border_size = g.Style.FrameBorderSize;
    if (border && border_size > 0.0f)
    {
        window->DrawList->AddRect(p_min + ImVec2(1, 1), p_max + ImVec2(1, 1), GetColorU32(ImGuiCol_BorderShadow), rounding, ImDrawCornerFlags_All, border_size);
        window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, ImDrawCornerFlags_All, border_size);
    }
}

void ImGui::RenderFrameBorder(ImVec2 p_min, ImVec2 p_max, float rounding)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const float border_size = g.Style.FrameBorderSize;
    if (border_size > 0.0f)
    {
        window->DrawList->AddRect(p_min + ImVec2(1, 1), p_max + ImVec2(1, 1), GetColorU32(ImGuiCol_BorderShadow), rounding, ImDrawCornerFlags_All, border_size);
        window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, ImDrawCornerFlags_All, border_size);
    }
}

void ImGui::RenderNavHighlight(const ImRect& bb, ImGuiID id, ImGuiNavHighlightFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (id != g.NavId)
        return;
    if (g.NavDisableHighlight && !(flags & ImGuiNavHighlightFlags_AlwaysDraw))
        return;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->DC.NavHideHighlightOneFrame)
        return;

    float rounding = (flags & ImGuiNavHighlightFlags_NoRounding) ? 0.0f : g.Style.FrameRounding;
    ImRect display_rect = bb;
    display_rect.ClipWith(window->ClipRect);
    if (flags & ImGuiNavHighlightFlags_TypeDefault)
    {
        const float THICKNESS = 2.0f;
        const float DISTANCE = 3.0f + THICKNESS * 0.5f;
        display_rect.Expand(ImVec2(DISTANCE, DISTANCE));
        bool fully_visible = window->ClipRect.Contains(display_rect);
        if (!fully_visible)
            window->DrawList->PushClipRect(display_rect.Min, display_rect.Max);
        window->DrawList->AddRect(display_rect.Min + ImVec2(THICKNESS * 0.5f, THICKNESS * 0.5f), display_rect.Max - ImVec2(THICKNESS * 0.5f, THICKNESS * 0.5f), GetColorU32(ImGuiCol_NavHighlight), rounding, ImDrawCornerFlags_All, THICKNESS);
        if (!fully_visible)
            window->DrawList->PopClipRect();
    }
    if (flags & ImGuiNavHighlightFlags_TypeThin)
    {
        window->DrawList->AddRect(display_rect.Min, display_rect.Max, GetColorU32(ImGuiCol_NavHighlight), rounding, ~0, 1.0f);
    }
}

ImGuiWindow::ImGuiWindow(ImGuiContext* context, const char* name) : DrawListInst(NULL)
{
    memset(this, 0, sizeof(*this));
    Name = ImStrdup(name);
    NameBufLen = (int)strlen(name) + 1;
    ID = ImHashStr(name);
    IDStack.push_back(ID);
    ViewportAllowPlatformMonitorExtend = -1;
    ViewportPos = ImVec2(FLT_MAX, FLT_MAX);
    MoveId = GetID("#MOVE");
    ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);
    ScrollTargetCenterRatio = ImVec2(0.5f, 0.5f);
    AutoFitFramesX = AutoFitFramesY = -1;
    AutoPosLastDirection = ImGuiDir_None;
    SetWindowPosAllowFlags = SetWindowSizeAllowFlags = SetWindowCollapsedAllowFlags = SetWindowDockAllowFlags = ImGuiCond_Always | ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing;
    SetWindowPosVal = SetWindowPosPivot = ImVec2(FLT_MAX, FLT_MAX);
    LastFrameActive = -1;
    LastFrameJustFocused = -1;
    LastTimeActive = -1.0f;
    FontWindowScale = FontDpiScale = 1.0f;
    SettingsOffset = -1;
    DockOrder = -1;
    DrawList = &DrawListInst;
    DrawList->_Data = &context->DrawListSharedData;
    DrawList->_OwnerName = Name;
}

ImGuiWindow::~ImGuiWindow()
{
    IM_ASSERT(DrawList == &DrawListInst);
    IM_DELETE(Name);
    for (int i = 0; i != ColumnsStorage.Size; i++)
        ColumnsStorage[i].~ImGuiOldColumns();
}

ImGuiID ImGuiWindow::GetID(const char* str, const char* str_end)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashStr(str, str_end ? (str_end - str) : 0, seed);
    ImGui::KeepAliveID(id);
#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiContext& g = *GImGui;
    IMGUI_TEST_ENGINE_ID_INFO2(id, ImGuiDataType_String, str, str_end);
#endif
    return id;
}

ImGuiID ImGuiWindow::GetID(const void* ptr)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashData(&ptr, sizeof(void*), seed);
    ImGui::KeepAliveID(id);
#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiContext& g = *GImGui;
    IMGUI_TEST_ENGINE_ID_INFO(id, ImGuiDataType_Pointer, ptr);
#endif
    return id;
}

ImGuiID ImGuiWindow::GetID(int n)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashData(&n, sizeof(n), seed);
    ImGui::KeepAliveID(id);
#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiContext& g = *GImGui;
    IMGUI_TEST_ENGINE_ID_INFO(id, ImGuiDataType_S32, (intptr_t)n);
#endif
    return id;
}

ImGuiID ImGuiWindow::GetIDNoKeepAlive(const char* str, const char* str_end)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashStr(str, str_end ? (str_end - str) : 0, seed);
#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiContext& g = *GImGui;
    IMGUI_TEST_ENGINE_ID_INFO2(id, ImGuiDataType_String, str, str_end);
#endif
    return id;
}

ImGuiID ImGuiWindow::GetIDNoKeepAlive(const void* ptr)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashData(&ptr, sizeof(void*), seed);
#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiContext& g = *GImGui;
    IMGUI_TEST_ENGINE_ID_INFO(id, ImGuiDataType_Pointer, ptr);
#endif
    return id;
}

ImGuiID ImGuiWindow::GetIDNoKeepAlive(int n)
{
    ImGuiID seed = IDStack.back();
    ImGuiID id = ImHashData(&n, sizeof(n), seed);
#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiContext& g = *GImGui;
    IMGUI_TEST_ENGINE_ID_INFO(id, ImGuiDataType_S32, (intptr_t)n);
#endif
    return id;
}

ImGuiID ImGuiWindow::GetIDFromRectangle(const ImRect& r_abs)
{
    ImGuiID seed = IDStack.back();
    const int r_rel[4] = { (int)(r_abs.Min.x - Pos.x), (int)(r_abs.Min.y - Pos.y), (int)(r_abs.Max.x - Pos.x), (int)(r_abs.Max.y - Pos.y) };
    ImGuiID id = ImHashData(&r_rel, sizeof(r_rel), seed);
    ImGui::KeepAliveID(id);
    return id;
}

static void SetCurrentWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.CurrentWindow = window;
    g.CurrentTable = window && window->DC.CurrentTableIdx != -1 ? g.Tables.GetByIndex(window->DC.CurrentTableIdx) : NULL;
    if (window)
        g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize();
}

void ImGui::GcCompactTransientMiscBuffers()
{
    ImGuiContext& g = *GImGui;
    g.ItemFlagsStack.clear();
    g.GroupStack.clear();
    TableGcCompactSettings();
}

void ImGui::GcCompactTransientWindowBuffers(ImGuiWindow* window)
{
    window->MemoryCompacted = true;
    window->MemoryDrawListIdxCapacity = window->DrawList->IdxBuffer.Capacity;
    window->MemoryDrawListVtxCapacity = window->DrawList->VtxBuffer.Capacity;
    window->IDStack.clear();
    window->DrawList->_ClearFreeMemory();
    window->DC.ChildWindows.clear();
    window->DC.ItemWidthStack.clear();
    window->DC.TextWrapPosStack.clear();
}

void ImGui::GcAwakeTransientWindowBuffers(ImGuiWindow* window)
{
    window->MemoryCompacted = false;
    window->DrawList->IdxBuffer.reserve(window->MemoryDrawListIdxCapacity);
    window->DrawList->VtxBuffer.reserve(window->MemoryDrawListVtxCapacity);
    window->MemoryDrawListIdxCapacity = window->MemoryDrawListVtxCapacity = 0;
}

void ImGui::SetActiveID(ImGuiID id, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.ActiveIdIsJustActivated = (g.ActiveId != id);
    if (g.ActiveIdIsJustActivated)
    {
        g.ActiveIdTimer = 0.0f;
        g.ActiveIdHasBeenPressedBefore = false;
        g.ActiveIdHasBeenEditedBefore = false;
        if (id != 0)
        {
            g.LastActiveId = id;
            g.LastActiveIdTimer = 0.0f;
        }
    }
    g.ActiveId = id;
    g.ActiveIdAllowOverlap = false;
    g.ActiveIdNoClearOnFocusLoss = false;
    g.ActiveIdWindow = window;
    g.ActiveIdHasBeenEditedThisFrame = false;
    if (id)
    {
        g.ActiveIdIsAlive = id;
        g.ActiveIdSource = (g.NavActivateId == id || g.NavInputId == id || g.NavJustTabbedId == id || g.NavJustMovedToId == id) ? ImGuiInputSource_Nav : ImGuiInputSource_Mouse;
    }

    g.ActiveIdUsingNavDirMask = 0x00;
    g.ActiveIdUsingNavInputMask = 0x00;
    g.ActiveIdUsingKeyInputMask = 0x00;
}

void ImGui::ClearActiveID()
{
    SetActiveID(0, NULL);    
}

void ImGui::SetHoveredID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    g.HoveredId = id;
    g.HoveredIdAllowOverlap = false;
    if (id != 0 && g.HoveredIdPreviousFrame != id)
        g.HoveredIdTimer = g.HoveredIdNotActiveTimer = 0.0f;
}

ImGuiID ImGui::GetHoveredID()
{
    ImGuiContext& g = *GImGui;
    return g.HoveredId ? g.HoveredId : g.HoveredIdPreviousFrame;
}

void ImGui::KeepAliveID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId == id)
        g.ActiveIdIsAlive = id;
    if (g.ActiveIdPreviousFrame == id)
        g.ActiveIdPreviousFrameIsAlive = true;
}

void ImGui::MarkItemEdited(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ActiveId == id || g.ActiveId == 0 || g.DragDropActive);
    IM_UNUSED(id);          
    g.ActiveIdHasBeenEditedThisFrame = true;
    g.ActiveIdHasBeenEditedBefore = true;
    g.CurrentWindow->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_Edited;
}

static inline bool IsWindowContentHoverable(ImGuiWindow* window, ImGuiHoveredFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (g.NavWindow)
        if (ImGuiWindow* focused_root_window = g.NavWindow->RootWindow)
            if (focused_root_window->WasActive && focused_root_window != window->RootWindow)
            {
                if (focused_root_window->Flags & ImGuiWindowFlags_Modal)
                    return false;
                if ((focused_root_window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                    return false;
            }

    if (window->Viewport != g.MouseViewport)
        if (g.MovingWindow == NULL || window->RootWindow != g.MovingWindow->RootWindow)
            return false;

    return true;
}

bool ImGui::IsItemHovered(ImGuiHoveredFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.NavDisableMouseHover && !g.NavDisableHighlight)
        return IsItemFocused();

    if (!(window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect))
        return false;
    IM_ASSERT((flags & (ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows)) == 0);         

    if (g.HoveredRootWindow != window->RootWindow && !(flags & ImGuiHoveredFlags_AllowWhenOverlapped))
        return false;

    if (!(flags & ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        if (g.ActiveId != 0 && g.ActiveId != window->DC.LastItemId && !g.ActiveIdAllowOverlap && g.ActiveId != window->MoveId)
            return false;

    if (!IsWindowContentHoverable(window, flags))
        return false;

    if ((window->DC.ItemFlags & ImGuiItemFlags_Disabled) && !(flags & ImGuiHoveredFlags_AllowWhenDisabled))
        return false;

    if ((window->DC.LastItemId == window->ID || window->DC.LastItemId == window->MoveId) && window->WriteAccessed)
        return false;
    return true;
}

bool ImGui::ItemHoverable(const ImRect& bb, ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (g.HoveredId != 0 && g.HoveredId != id && !g.HoveredIdAllowOverlap)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    if (g.HoveredWindow != window)
        return false;
    if (g.ActiveId != 0 && g.ActiveId != id && !g.ActiveIdAllowOverlap)
        return false;
    if (!IsMouseHoveringRect(bb.Min, bb.Max))
        return false;
    if (g.NavDisableMouseHover)
        return false;
    if (!IsWindowContentHoverable(window, ImGuiHoveredFlags_None) || (window->DC.ItemFlags & ImGuiItemFlags_Disabled))
    {
        g.HoveredIdDisabled = true;
        return false;
    }

    if (id != 0)
    {
        SetHoveredID(id);

        if (g.DebugItemPickerActive && g.HoveredIdPreviousFrame == id)
            GetForegroundDrawList()->AddRect(bb.Min, bb.Max, IM_COL32(255, 255, 0, 255));
        if (g.DebugItemPickerBreakId == id)
            IM_DEBUG_BREAK();
    }

    return true;
}

bool ImGui::IsClippedEx(const ImRect& bb, ImGuiID id, bool clip_even_when_logged)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!bb.Overlaps(window->ClipRect))
        if (id == 0 || (id != g.ActiveId && id != g.NavId))
            if (clip_even_when_logged || !g.LogEnabled)
                return true;
    return false;
}

void ImGui::SetLastItemData(ImGuiWindow* window, ImGuiID item_id, ImGuiItemStatusFlags item_flags, const ImRect& item_rect)
{
    window->DC.LastItemId = item_id;
    window->DC.LastItemStatusFlags = item_flags;
    window->DC.LastItemRect = item_rect;
}

bool ImGui::FocusableItemRegister(ImGuiWindow* window, ImGuiID id)
{
    ImGuiContext& g = *GImGui;

    const bool is_tab_stop = (window->DC.ItemFlags & (ImGuiItemFlags_NoTabStop | ImGuiItemFlags_Disabled)) == 0;
    window->DC.FocusCounterRegular++;
    if (is_tab_stop)
        window->DC.FocusCounterTabStop++;

    if (g.ActiveId == id && g.FocusTabPressed && !IsActiveIdUsingKey(ImGuiKey_Tab) && g.FocusRequestNextWindow == NULL)
    {
        g.FocusRequestNextWindow = window;
        g.FocusRequestNextCounterTabStop = window->DC.FocusCounterTabStop + (g.IO.KeyShift ? (is_tab_stop ? -1 : 0) : +1);                    
    }

    if (g.FocusRequestCurrWindow == window)
    {
        if (window->DC.FocusCounterRegular == g.FocusRequestCurrCounterRegular)
            return true;
        if (is_tab_stop && window->DC.FocusCounterTabStop == g.FocusRequestCurrCounterTabStop)
        {
            g.NavJustTabbedId = id;
            return true;
        }

        if (g.ActiveId == id)
            ClearActiveID();
    }

    return false;
}

void ImGui::FocusableItemUnregister(ImGuiWindow* window)
{
    window->DC.FocusCounterRegular--;
    window->DC.FocusCounterTabStop--;
}

float ImGui::CalcWrapWidthForPos(const ImVec2& pos, float wrap_pos_x)
{
    if (wrap_pos_x < 0.0f)
        return 0.0f;

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (wrap_pos_x == 0.0f)
    {
        wrap_pos_x = window->WorkRect.Max.x;
    }
    else if (wrap_pos_x > 0.0f)
    {
        wrap_pos_x += window->Pos.x - window->Scroll.x;        
    }

    return ImMax(wrap_pos_x - pos.x, 1.0f);
}

void* ImGui::MemAlloc(size_t size)
{
    if (ImGuiContext* ctx = GImGui)
        ctx->IO.MetricsActiveAllocations++;
    return GImAllocatorAllocFunc(size, GImAllocatorUserData);
}

void ImGui::MemFree(void* ptr)
{
    if (ptr)
        if (ImGuiContext* ctx = GImGui)
            ctx->IO.MetricsActiveAllocations--;
    return GImAllocatorFreeFunc(ptr, GImAllocatorUserData);
}

const char* ImGui::GetClipboardText()
{
    ImGuiContext& g = *GImGui;
    return g.IO.GetClipboardTextFn ? g.IO.GetClipboardTextFn(g.IO.ClipboardUserData) : "";
}

void ImGui::SetClipboardText(const char* text)
{
    ImGuiContext& g = *GImGui;
    if (g.IO.SetClipboardTextFn)
        g.IO.SetClipboardTextFn(g.IO.ClipboardUserData, text);
}

const char* ImGui::GetVersion()
{
    return IMGUI_VERSION;
}

ImGuiContext* ImGui::GetCurrentContext()
{
    return GImGui;
}

void ImGui::SetCurrentContext(ImGuiContext* ctx)
{
#ifdef IMGUI_SET_CURRENT_CONTEXT_FUNC
    IMGUI_SET_CURRENT_CONTEXT_FUNC(ctx);             
#else
    GImGui = ctx;
#endif
}

void ImGui::SetAllocatorFunctions(void* (*alloc_func)(size_t sz, void* user_data), void (*free_func)(void* ptr, void* user_data), void* user_data)
{
    GImAllocatorAllocFunc = alloc_func;
    GImAllocatorFreeFunc = free_func;
    GImAllocatorUserData = user_data;
}

ImGuiContext* ImGui::CreateContext(ImFontAtlas* shared_font_atlas)
{
    ImGuiContext* ctx = IM_NEW(ImGuiContext)(shared_font_atlas);
    if (GImGui == NULL)
        SetCurrentContext(ctx);
    Initialize(ctx);
    return ctx;
}

void ImGui::DestroyContext(ImGuiContext* ctx)
{
    if (ctx == NULL)
        ctx = GImGui;
    Shutdown(ctx);
    if (GImGui == ctx)
        SetCurrentContext(NULL);
    IM_DELETE(ctx);
}

void ImGui::AddContextHook(ImGuiContext* ctx, const ImGuiContextHook* hook)
{
    ImGuiContext& g = *ctx;
    IM_ASSERT(hook->Callback != NULL);
    g.Hooks.push_back(*hook);
}

void ImGui::CallContextHooks(ImGuiContext* ctx, ImGuiContextHookType hook_type)
{
    ImGuiContext& g = *ctx;
    for (int n = 0; n < g.Hooks.Size; n++)
        if (g.Hooks[n].Type == hook_type)
            g.Hooks[n].Callback(&g, &g.Hooks[n]);
}

ImGuiIO& ImGui::GetIO()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext() ?");
    return GImGui->IO;
}

ImGuiPlatformIO& ImGui::GetPlatformIO()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() or ImGui::SetCurrentContext()?");
    return GImGui->PlatformIO;
}

ImDrawData* ImGui::GetDrawData()
{
    ImGuiContext& g = *GImGui;
    return g.Viewports[0]->DrawDataP.Valid ? &g.Viewports[0]->DrawDataP : NULL;
}

double ImGui::GetTime()
{
    return GImGui->Time;
}

int ImGui::GetFrameCount()
{
    return GImGui->FrameCount;
}

static ImDrawList* GetViewportDrawList(ImGuiViewportP* viewport, size_t drawlist_no, const char* drawlist_name)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(drawlist_no < IM_ARRAYSIZE(viewport->DrawLists));
    ImDrawList* draw_list = viewport->DrawLists[drawlist_no];
    if (draw_list == NULL)
    {
        draw_list = IM_NEW(ImDrawList)(&g.DrawListSharedData);
        draw_list->_OwnerName = drawlist_name;
        viewport->DrawLists[drawlist_no] = draw_list;
    }

    if (viewport->LastFrameDrawLists[drawlist_no] != g.FrameCount)
    {
        draw_list->_ResetForNewFrame();
        draw_list->PushTextureID(g.IO.Fonts->TexID);
        draw_list->PushClipRect(viewport->Pos, viewport->Pos + viewport->Size, false);
        viewport->LastFrameDrawLists[drawlist_no] = g.FrameCount;
    }
    return draw_list;
}

ImDrawList* ImGui::GetBackgroundDrawList(ImGuiViewport* viewport)
{
    return GetViewportDrawList((ImGuiViewportP*)viewport, 0, "##Background");
}

ImDrawList* ImGui::GetBackgroundDrawList()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return GetBackgroundDrawList(window->Viewport);
}

ImDrawList* ImGui::GetForegroundDrawList(ImGuiViewport* viewport)
{
    return GetViewportDrawList((ImGuiViewportP*)viewport, 1, "##Foreground");
}

ImDrawList* ImGui::GetForegroundDrawList()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return GetForegroundDrawList(window->Viewport);
}

ImDrawListSharedData* ImGui::GetDrawListSharedData()
{
    return &GImGui->DrawListSharedData;
}

void ImGui::StartMouseMovingWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    FocusWindow(window);
    SetActiveID(window->MoveId, window);
    g.NavDisableHighlight = true;
    g.ActiveIdNoClearOnFocusLoss = true;
    g.ActiveIdClickOffset = g.IO.MouseClickedPos[0] - window->RootWindow->Pos;

    bool can_move_window = true;
    if ((window->Flags & ImGuiWindowFlags_NoMove) || (window->RootWindow->Flags & ImGuiWindowFlags_NoMove))
        can_move_window = false;
    if (ImGuiDockNode* node = window->DockNodeAsHost)
        if (node->VisibleWindow && (node->VisibleWindow->Flags & ImGuiWindowFlags_NoMove))
            can_move_window = false;
    if (can_move_window)
        g.MovingWindow = window;
}

void ImGui::StartMouseMovingWindowOrNode(ImGuiWindow* window, ImGuiDockNode* node, bool undock_floating_node)
{
    ImGuiContext& g = *GImGui;
    bool can_undock_node = false;
    if (node != NULL && node->VisibleWindow && (node->VisibleWindow->Flags & ImGuiWindowFlags_NoMove) == 0)
    {
        ImGuiDockNode* root_node = DockNodeGetRootNode(node);
        if (root_node->OnlyNodeWithWindows != node || root_node->CentralNode != NULL)               
            if (undock_floating_node || root_node->IsDockSpace())
                can_undock_node = true;
    }

    const bool clicked = IsMouseClicked(0);
    const bool dragging = IsMouseDragging(0, g.IO.MouseDragThreshold * 1.70f);
    if (can_undock_node && dragging)
        DockContextQueueUndockNode(&g, node);           
    else if (!can_undock_node && (clicked || dragging) && g.MovingWindow != window)
        StartMouseMovingWindow(window);
}

void ImGui::UpdateMouseMovingWindowNewFrame()
{
    ImGuiContext& g = *GImGui;
    if (g.MovingWindow != NULL)
    {
        KeepAliveID(g.ActiveId);
        IM_ASSERT(g.MovingWindow && g.MovingWindow->RootWindow);
        ImGuiWindow* moving_window = g.MovingWindow->RootWindow;
        if (g.IO.MouseDown[0] && IsMousePosValid(&g.IO.MousePos))
        {
            ImVec2 pos = g.IO.MousePos - g.ActiveIdClickOffset;
            if (moving_window->Pos.x != pos.x || moving_window->Pos.y != pos.y)
            {
                MarkIniSettingsDirty(moving_window);
                SetWindowPos(moving_window, pos, ImGuiCond_Always);
                if (moving_window->ViewportOwned)                  
                    moving_window->Viewport->Pos = pos;
            }
            FocusWindow(g.MovingWindow);
        }
        else
        {
            if (g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable)
                UpdateTryMergeWindowIntoHostViewport(moving_window, g.MouseViewport);

            if (!IsDragDropPayloadBeingAccepted())
                g.MouseViewport = moving_window->Viewport;

            moving_window->Viewport->Flags &= ~ImGuiViewportFlags_NoInputs;            

            ClearActiveID();
            g.MovingWindow = NULL;
        }
    }
    else
    {
        if (g.ActiveIdWindow && g.ActiveIdWindow->MoveId == g.ActiveId)
        {
            KeepAliveID(g.ActiveId);
            if (!g.IO.MouseDown[0])
                ClearActiveID();
        }
    }
}

void ImGui::UpdateMouseMovingWindowEndFrame()
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId != 0 || g.HoveredId != 0)
        return;

    if (g.NavWindow && g.NavWindow->Appearing)
        return;

    if (g.IO.MouseClicked[0])
    {
        ImGuiWindow* root_window = g.HoveredWindow ? g.HoveredWindow->RootWindowDockStop : NULL;
        const bool is_closed_popup = root_window && (root_window->Flags & ImGuiWindowFlags_Popup) && !IsPopupOpen(root_window->PopupId, ImGuiPopupFlags_AnyPopupLevel);

        if (root_window != NULL && !is_closed_popup)
        {
            StartMouseMovingWindow(g.HoveredWindow); 

            if (g.IO.ConfigWindowsMoveFromTitleBarOnly)
                if (!(root_window->Flags & ImGuiWindowFlags_NoTitleBar) || root_window->DockIsActive)
                    if (!root_window->TitleBarRect().Contains(g.IO.MouseClickedPos[0]))
                        g.MovingWindow = NULL;

            if (g.HoveredIdDisabled)
                g.MovingWindow = NULL;
        }
        else if (root_window == NULL && g.NavWindow != NULL && GetTopMostPopupModal() == NULL)
        {
            FocusWindow(NULL);
        }
    }

    if (g.IO.MouseClicked[1])
    {
        ImGuiWindow* modal = GetTopMostPopupModal();
        bool hovered_window_above_modal = g.HoveredWindow && IsWindowAbove(g.HoveredWindow, modal);
        ClosePopupsOverWindow(hovered_window_above_modal ? g.HoveredWindow : modal, true);
    }
}

static void TranslateWindow(ImGuiWindow* window, const ImVec2& delta)
{
    window->Pos += delta;
    window->ClipRect.Translate(delta);
    window->OuterRectClipped.Translate(delta);
    window->InnerRect.Translate(delta);
    window->DC.CursorPos += delta;
    window->DC.CursorStartPos += delta;
    window->DC.CursorMaxPos += delta;
    window->DC.LastItemRect.Translate(delta);
    window->DC.LastItemDisplayRect.Translate(delta);
}

static void ScaleWindow(ImGuiWindow* window, float scale)
{
    ImVec2 origin = window->Viewport->Pos;
    window->Pos = ImFloor((window->Pos - origin) * scale + origin);
    window->Size = ImFloor(window->Size * scale);
    window->SizeFull = ImFloor(window->SizeFull * scale);
    window->ContentSize = ImFloor(window->ContentSize * scale);
}

static bool IsWindowActiveAndVisible(ImGuiWindow* window)
{
    return (window->Active) && (!window->Hidden);
}

static void ImGui::UpdateMouseInputs()
{
    ImGuiContext& g = *GImGui;

    if (IsMousePosValid(&g.IO.MousePos))
        g.IO.MousePos = g.LastValidMousePos = ImFloor(g.IO.MousePos);

    if (IsMousePosValid(&g.IO.MousePos) && IsMousePosValid(&g.IO.MousePosPrev))
        g.IO.MouseDelta = g.IO.MousePos - g.IO.MousePosPrev;
    else
        g.IO.MouseDelta = ImVec2(0.0f, 0.0f);
    if (g.IO.MouseDelta.x != 0.0f || g.IO.MouseDelta.y != 0.0f)
        g.NavDisableMouseHover = false;

    g.IO.MousePosPrev = g.IO.MousePos;
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.MouseDown); i++)
    {
        g.IO.MouseClicked[i] = g.IO.MouseDown[i] && g.IO.MouseDownDuration[i] < 0.0f;
        g.IO.MouseReleased[i] = !g.IO.MouseDown[i] && g.IO.MouseDownDuration[i] >= 0.0f;
        g.IO.MouseDownDurationPrev[i] = g.IO.MouseDownDuration[i];
        g.IO.MouseDownDuration[i] = g.IO.MouseDown[i] ? (g.IO.MouseDownDuration[i] < 0.0f ? 0.0f : g.IO.MouseDownDuration[i] + g.IO.DeltaTime) : -1.0f;
        g.IO.MouseDoubleClicked[i] = false;
        if (g.IO.MouseClicked[i])
        {
            if ((float)(g.Time - g.IO.MouseClickedTime[i]) < g.IO.MouseDoubleClickTime)
            {
                ImVec2 delta_from_click_pos = IsMousePosValid(&g.IO.MousePos) ? (g.IO.MousePos - g.IO.MouseClickedPos[i]) : ImVec2(0.0f, 0.0f);
                if (ImLengthSqr(delta_from_click_pos) < g.IO.MouseDoubleClickMaxDist * g.IO.MouseDoubleClickMaxDist)
                    g.IO.MouseDoubleClicked[i] = true;
                g.IO.MouseClickedTime[i] = -g.IO.MouseDoubleClickTime * 2.0f;              
            }
            else
            {
                g.IO.MouseClickedTime[i] = g.Time;
            }
            g.IO.MouseClickedPos[i] = g.IO.MousePos;
            g.IO.MouseDownWasDoubleClick[i] = g.IO.MouseDoubleClicked[i];
            g.IO.MouseDragMaxDistanceAbs[i] = ImVec2(0.0f, 0.0f);
            g.IO.MouseDragMaxDistanceSqr[i] = 0.0f;
        }
        else if (g.IO.MouseDown[i])
        {
            ImVec2 delta_from_click_pos = IsMousePosValid(&g.IO.MousePos) ? (g.IO.MousePos - g.IO.MouseClickedPos[i]) : ImVec2(0.0f, 0.0f);
            g.IO.MouseDragMaxDistanceSqr[i] = ImMax(g.IO.MouseDragMaxDistanceSqr[i], ImLengthSqr(delta_from_click_pos));
            g.IO.MouseDragMaxDistanceAbs[i].x = ImMax(g.IO.MouseDragMaxDistanceAbs[i].x, delta_from_click_pos.x < 0.0f ? -delta_from_click_pos.x : delta_from_click_pos.x);
            g.IO.MouseDragMaxDistanceAbs[i].y = ImMax(g.IO.MouseDragMaxDistanceAbs[i].y, delta_from_click_pos.y < 0.0f ? -delta_from_click_pos.y : delta_from_click_pos.y);
        }
        if (!g.IO.MouseDown[i] && !g.IO.MouseReleased[i])
            g.IO.MouseDownWasDoubleClick[i] = false;
        if (g.IO.MouseClicked[i])                
            g.NavDisableMouseHover = false;
    }
}

static void StartLockWheelingWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.WheelingWindow == window)
        return;
    g.WheelingWindow = window;
    g.WheelingWindowRefMousePos = g.IO.MousePos;
    g.WheelingWindowTimer = WINDOWS_MOUSE_WHEEL_SCROLL_LOCK_TIMER;
}

void ImGui::UpdateMouseWheel()
{
    ImGuiContext& g = *GImGui;

    if (g.WheelingWindow != NULL)
    {
        g.WheelingWindowTimer -= g.IO.DeltaTime;
        if (IsMousePosValid() && ImLengthSqr(g.IO.MousePos - g.WheelingWindowRefMousePos) > g.IO.MouseDragThreshold * g.IO.MouseDragThreshold)
            g.WheelingWindowTimer = 0.0f;
        if (g.WheelingWindowTimer <= 0.0f)
        {
            g.WheelingWindow = NULL;
            g.WheelingWindowTimer = 0.0f;
        }
    }

    if (g.IO.MouseWheel == 0.0f && g.IO.MouseWheelH == 0.0f)
        return;

    ImGuiWindow* window = g.WheelingWindow ? g.WheelingWindow : g.HoveredWindow;
    if (!window || window->Collapsed)
        return;

    if (g.IO.MouseWheel != 0.0f && g.IO.KeyCtrl && g.IO.FontAllowUserScaling)
    {
        StartLockWheelingWindow(window);
        const float new_font_scale = ImClamp(window->FontWindowScale + g.IO.MouseWheel * 0.10f, 0.50f, 2.50f);
        const float scale = new_font_scale / window->FontWindowScale;
        window->FontWindowScale = new_font_scale;
        if (!(window->Flags & ImGuiWindowFlags_ChildWindow))
        {
            const ImVec2 offset = window->Size * (1.0f - scale) * (g.IO.MousePos - window->Pos) / window->Size;
            SetWindowPos(window, window->Pos + offset, 0);
            window->Size = ImFloor(window->Size * scale);
            window->SizeFull = ImFloor(window->SizeFull * scale);
        }
        return;
    }

    const float wheel_y = (g.IO.MouseWheel != 0.0f && !g.IO.KeyShift) ? g.IO.MouseWheel : 0.0f;
    if (wheel_y != 0.0f && !g.IO.KeyCtrl)
    {
        StartLockWheelingWindow(window);
        while ((window->Flags & ImGuiWindowFlags_ChildWindow) && ((window->ScrollMax.y == 0.0f) || ((window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))))
            window = window->ParentWindow;
        if (!(window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))
        {
            float max_step = window->InnerRect.GetHeight() * 0.67f;
            float scroll_step = ImFloor(ImMin(5 * window->CalcFontSize(), max_step));
            SetScrollY(window, window->Scroll.y - wheel_y * scroll_step);
        }
    }

    const float wheel_x = (g.IO.MouseWheelH != 0.0f && !g.IO.KeyShift) ? g.IO.MouseWheelH : (g.IO.MouseWheel != 0.0f && g.IO.KeyShift) ? g.IO.MouseWheel : 0.0f;
    if (wheel_x != 0.0f && !g.IO.KeyCtrl)
    {
        StartLockWheelingWindow(window);
        while ((window->Flags & ImGuiWindowFlags_ChildWindow) && ((window->ScrollMax.x == 0.0f) || ((window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))))
            window = window->ParentWindow;
        if (!(window->Flags & ImGuiWindowFlags_NoScrollWithMouse) && !(window->Flags & ImGuiWindowFlags_NoMouseInputs))
        {
            float max_step = window->InnerRect.GetWidth() * 0.67f;
            float scroll_step = ImFloor(ImMin(2 * window->CalcFontSize(), max_step));
            SetScrollX(window, window->Scroll.x - wheel_x * scroll_step);
        }
    }
}

void ImGui::UpdateTabFocus()
{
    ImGuiContext& g = *GImGui;

    g.FocusTabPressed = (g.NavWindow && g.NavWindow->Active && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs) && !g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_Tab));
    if (g.ActiveId == 0 && g.FocusTabPressed)
    {
        g.FocusRequestNextWindow = g.NavWindow;
        g.FocusRequestNextCounterRegular = INT_MAX;
        if (g.NavId != 0 && g.NavIdTabCounter != INT_MAX)
            g.FocusRequestNextCounterTabStop = g.NavIdTabCounter + 1 + (g.IO.KeyShift ? -1 : 1);
        else
            g.FocusRequestNextCounterTabStop = g.IO.KeyShift ? -1 : 0;
    }

    g.FocusRequestCurrWindow = NULL;
    g.FocusRequestCurrCounterRegular = g.FocusRequestCurrCounterTabStop = INT_MAX;
    if (g.FocusRequestNextWindow != NULL)
    {
        ImGuiWindow* window = g.FocusRequestNextWindow;
        g.FocusRequestCurrWindow = window;
        if (g.FocusRequestNextCounterRegular != INT_MAX && window->DC.FocusCounterRegular != -1)
            g.FocusRequestCurrCounterRegular = ImModPositive(g.FocusRequestNextCounterRegular, window->DC.FocusCounterRegular + 1);
        if (g.FocusRequestNextCounterTabStop != INT_MAX && window->DC.FocusCounterTabStop != -1)
            g.FocusRequestCurrCounterTabStop = ImModPositive(g.FocusRequestNextCounterTabStop, window->DC.FocusCounterTabStop + 1);
        g.FocusRequestNextWindow = NULL;
        g.FocusRequestNextCounterRegular = g.FocusRequestNextCounterTabStop = INT_MAX;
    }

    g.NavIdTabCounter = INT_MAX;
}

void ImGui::UpdateHoveredWindowAndCaptureFlags()
{
    ImGuiContext& g = *GImGui;

    bool clear_hovered_windows = false;
    FindHoveredWindow();
    IM_ASSERT(g.HoveredWindow == NULL || g.HoveredWindow == g.MovingWindow || g.HoveredWindow->Viewport == g.MouseViewport);

    ImGuiWindow* modal_window = GetTopMostPopupModal();
    if (modal_window && g.HoveredRootWindow && !IsWindowChildOf(g.HoveredRootWindow, modal_window))
        clear_hovered_windows = true;

    if (g.IO.ConfigFlags & ImGuiConfigFlags_NoMouse)
        clear_hovered_windows = true;

    int mouse_earliest_button_down = -1;
    bool mouse_any_down = false;
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.MouseDown); i++)
    {
        if (g.IO.MouseClicked[i])
            g.IO.MouseDownOwned[i] = (g.HoveredWindow != NULL) || (g.OpenPopupStack.Size > 0);
        mouse_any_down |= g.IO.MouseDown[i];
        if (g.IO.MouseDown[i])
            if (mouse_earliest_button_down == -1 || g.IO.MouseClickedTime[i] < g.IO.MouseClickedTime[mouse_earliest_button_down])
                mouse_earliest_button_down = i;
    }
    const bool mouse_avail_to_imgui = (mouse_earliest_button_down == -1) || g.IO.MouseDownOwned[mouse_earliest_button_down];

    const bool mouse_dragging_extern_payload = g.DragDropActive && (g.DragDropSourceFlags & ImGuiDragDropFlags_SourceExtern) != 0;
    if (!mouse_avail_to_imgui && !mouse_dragging_extern_payload)
        clear_hovered_windows = true;

    if (clear_hovered_windows)
        g.HoveredWindow = g.HoveredRootWindow = g.HoveredWindowUnderMovingWindow = NULL;

    if (g.WantCaptureMouseNextFrame != -1)
        g.IO.WantCaptureMouse = (g.WantCaptureMouseNextFrame != 0);
    else
        g.IO.WantCaptureMouse = (mouse_avail_to_imgui && (g.HoveredWindow != NULL || mouse_any_down)) || (g.OpenPopupStack.Size > 0);

    if (g.WantCaptureKeyboardNextFrame != -1)
        g.IO.WantCaptureKeyboard = (g.WantCaptureKeyboardNextFrame != 0);
    else
        g.IO.WantCaptureKeyboard = (g.ActiveId != 0) || (modal_window != NULL);
    if (g.IO.NavActive && (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) && !(g.IO.ConfigFlags & ImGuiConfigFlags_NavNoCaptureKeyboard))
        g.IO.WantCaptureKeyboard = true;

    g.IO.WantTextInput = (g.WantTextInputNextFrame != -1) ? (g.WantTextInputNextFrame != 0) : false;
}

ImGuiKeyModFlags ImGui::GetMergedKeyModFlags()
{
    ImGuiContext& g = *GImGui;
    ImGuiKeyModFlags key_mod_flags = ImGuiKeyModFlags_None;
    if (g.IO.KeyCtrl)   { key_mod_flags |= ImGuiKeyModFlags_Ctrl; }
    if (g.IO.KeyShift)  { key_mod_flags |= ImGuiKeyModFlags_Shift; }
    if (g.IO.KeyAlt)    { key_mod_flags |= ImGuiKeyModFlags_Alt; }
    if (g.IO.KeySuper)  { key_mod_flags |= ImGuiKeyModFlags_Super; }
    return key_mod_flags;
}

void ImGui::NewFrame()
{
    IM_ASSERT(GImGui != NULL && "No current context. Did you call ImGui::CreateContext() and ImGui::SetCurrentContext() ?");
    ImGuiContext& g = *GImGui;

    CallContextHooks(&g, ImGuiContextHookType_NewFramePre);

    g.ConfigFlagsLastFrame = g.ConfigFlagsCurrFrame;
    ErrorCheckNewFrameSanityChecks();
    g.ConfigFlagsCurrFrame = g.IO.ConfigFlags;

    UpdateSettings();

    g.Time += g.IO.DeltaTime;
    g.WithinFrameScope = true;
    g.FrameCount += 1;
    g.TooltipOverrideCount = 0;
    g.WindowsActiveCount = 0;
    g.MenusIdSubmittedThisFrame.resize(0);

    g.FramerateSecPerFrameAccum += g.IO.DeltaTime - g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx];
    g.FramerateSecPerFrame[g.FramerateSecPerFrameIdx] = g.IO.DeltaTime;
    g.FramerateSecPerFrameIdx = (g.FramerateSecPerFrameIdx + 1) % IM_ARRAYSIZE(g.FramerateSecPerFrame);
    g.IO.Framerate = (g.FramerateSecPerFrameAccum > 0.0f) ? (1.0f / (g.FramerateSecPerFrameAccum / (float)IM_ARRAYSIZE(g.FramerateSecPerFrame))) : FLT_MAX;

    UpdateViewportsNewFrame();

    g.IO.Fonts->Locked = true;
    SetCurrentFont(GetDefaultFont());
    IM_ASSERT(g.Font->IsLoaded());
    ImRect virtual_space(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (int n = 0; n < g.Viewports.Size; n++)
        virtual_space.Add(g.Viewports[n]->GetMainRect());
    g.DrawListSharedData.ClipRectFullscreen = ImVec4(virtual_space.Min.x, virtual_space.Min.y, virtual_space.Max.x, virtual_space.Max.y);
    g.DrawListSharedData.CurveTessellationTol = g.Style.CurveTessellationTol;
    g.DrawListSharedData.SetCircleSegmentMaxError(g.Style.CircleSegmentMaxError);
    g.DrawListSharedData.InitialFlags = ImDrawListFlags_None;
    if (g.Style.AntiAliasedLines)
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AntiAliasedLines;
    if (g.Style.AntiAliasedLinesUseTex && !(g.Font->ContainerAtlas->Flags & ImFontAtlasFlags_NoBakedLines))
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AntiAliasedLinesUseTex;
    if (g.Style.AntiAliasedFill)
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AntiAliasedFill;
    if (g.IO.BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset)
        g.DrawListSharedData.InitialFlags |= ImDrawListFlags_AllowVtxOffset;

    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        viewport->DrawData = NULL;
        viewport->DrawDataP.Clear();
    }

    if (g.DragDropActive && g.DragDropPayload.SourceId == g.ActiveId)
        KeepAliveID(g.DragDropPayload.SourceId);

    if (!g.HoveredIdPreviousFrame)
        g.HoveredIdTimer = 0.0f;
    if (!g.HoveredIdPreviousFrame || (g.HoveredId && g.ActiveId == g.HoveredId))
        g.HoveredIdNotActiveTimer = 0.0f;
    if (g.HoveredId)
        g.HoveredIdTimer += g.IO.DeltaTime;
    if (g.HoveredId && g.ActiveId != g.HoveredId)
        g.HoveredIdNotActiveTimer += g.IO.DeltaTime;
    g.HoveredIdPreviousFrame = g.HoveredId;
    g.HoveredId = 0;
    g.HoveredIdAllowOverlap = false;
    g.HoveredIdDisabled = false;

    if (g.ActiveIdIsAlive != g.ActiveId && g.ActiveIdPreviousFrame == g.ActiveId && g.ActiveId != 0)
        ClearActiveID();
    if (g.ActiveId)
        g.ActiveIdTimer += g.IO.DeltaTime;
    g.LastActiveIdTimer += g.IO.DeltaTime;
    g.ActiveIdPreviousFrame = g.ActiveId;
    g.ActiveIdPreviousFrameWindow = g.ActiveIdWindow;
    g.ActiveIdPreviousFrameHasBeenEditedBefore = g.ActiveIdHasBeenEditedBefore;
    g.ActiveIdIsAlive = 0;
    g.ActiveIdHasBeenEditedThisFrame = false;
    g.ActiveIdPreviousFrameIsAlive = false;
    g.ActiveIdIsJustActivated = false;
    if (g.TempInputId != 0 && g.ActiveId != g.TempInputId)
        g.TempInputId = 0;
    if (g.ActiveId == 0)
    {
        g.ActiveIdUsingNavDirMask = 0x00;
        g.ActiveIdUsingNavInputMask = 0x00;
        g.ActiveIdUsingKeyInputMask = 0x00;
    }

    g.DragDropAcceptIdPrev = g.DragDropAcceptIdCurr;
    g.DragDropAcceptIdCurr = 0;
    g.DragDropAcceptIdCurrRectSurface = FLT_MAX;
    g.DragDropWithinSource = false;
    g.DragDropWithinTarget = false;
    g.DragDropHoldJustPressedId = 0;

    g.IO.KeyMods = GetMergedKeyModFlags();
    memcpy(g.IO.KeysDownDurationPrev, g.IO.KeysDownDuration, sizeof(g.IO.KeysDownDuration));
    for (int i = 0; i < IM_ARRAYSIZE(g.IO.KeysDown); i++)
        g.IO.KeysDownDuration[i] = g.IO.KeysDown[i] ? (g.IO.KeysDownDuration[i] < 0.0f ? 0.0f : g.IO.KeysDownDuration[i] + g.IO.DeltaTime) : -1.0f;

    NavUpdate();

    UpdateMouseInputs();

    DockContextUpdateUndocking(&g);

    UpdateHoveredWindowAndCaptureFlags();

    UpdateMouseMovingWindowNewFrame();

    if (GetTopMostPopupModal() != NULL || (g.NavWindowingTarget != NULL && g.NavWindowingHighlightAlpha > 0.0f))
        g.DimBgRatio = ImMin(g.DimBgRatio + g.IO.DeltaTime * 6.0f, 1.0f);
    else
        g.DimBgRatio = ImMax(g.DimBgRatio - g.IO.DeltaTime * 10.0f, 0.0f);

    g.MouseCursor = ImGuiMouseCursor_Arrow;
    g.WantCaptureMouseNextFrame = g.WantCaptureKeyboardNextFrame = g.WantTextInputNextFrame = -1;
    g.PlatformImePos = ImVec2(1.0f, 1.0f);             
    g.PlatformImePosViewport = NULL;

    UpdateMouseWheel();

    UpdateTabFocus();

    IM_ASSERT(g.WindowsFocusOrder.Size == g.Windows.Size);
    const float memory_compact_start_time = (g.GcCompactAll || g.IO.ConfigMemoryCompactTimer < 0.0f) ? FLT_MAX : (float)g.Time - g.IO.ConfigMemoryCompactTimer;
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        window->WasActive = window->Active;
        window->BeginCount = 0;
        window->Active = false;
        window->WriteAccessed = false;

        if (!window->WasActive && !window->MemoryCompacted && window->LastTimeActive < memory_compact_start_time)
            GcCompactTransientWindowBuffers(window);
    }

    for (int i = 0; i < g.TablesLastTimeActive.Size; i++)
        if (g.TablesLastTimeActive[i] >= 0.0f && g.TablesLastTimeActive[i] < memory_compact_start_time)
            TableGcCompactTransientBuffers(g.Tables.GetByIndex(i));
    if (g.GcCompactAll)
        GcCompactTransientMiscBuffers();
    g.GcCompactAll = false;

    if (g.NavWindow && !g.NavWindow->WasActive)
        FocusTopMostWindowUnderOne(NULL, NULL);

    g.CurrentWindowStack.resize(0);
    g.BeginPopupStack.resize(0);
    g.ItemFlagsStack.resize(0);
    g.ItemFlagsStack.push_back(ImGuiItemFlags_Default_);
    g.GroupStack.resize(0);
    ClosePopupsOverWindow(g.NavWindow, false);

    DockContextUpdateDocking(&g);

    UpdateDebugToolItemPicker();

    g.WithinFrameScopeWithImplicitWindow = true;
    SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    Begin("Debug##Default");
    IM_ASSERT(g.CurrentWindow->IsFallbackWindow == true);

    CallContextHooks(&g, ImGuiContextHookType_NewFramePost);
}

void ImGui::UpdateDebugToolItemPicker()
{
    ImGuiContext& g = *GImGui;
    g.DebugItemPickerBreakId = 0;
    if (g.DebugItemPickerActive)
    {
        const ImGuiID hovered_id = g.HoveredIdPreviousFrame;
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsKeyPressedMap(ImGuiKey_Escape))
            g.DebugItemPickerActive = false;
        if (ImGui::IsMouseClicked(0) && hovered_id)
        {
            g.DebugItemPickerBreakId = hovered_id;
            g.DebugItemPickerActive = false;
        }
        ImGui::SetNextWindowBgAlpha(0.60f);
        ImGui::BeginTooltip();
        ImGui::Text("HoveredId: 0x%08X", hovered_id);
        ImGui::Text("Press ESC to abort picking.");
        ImGui::TextColored(GetStyleColorVec4(hovered_id ? ImGuiCol_Text : ImGuiCol_TextDisabled), "Click to break in debugger!");
        ImGui::EndTooltip();
    }
}

void ImGui::Initialize(ImGuiContext* context)
{
    ImGuiContext& g = *context;
    IM_ASSERT(!g.Initialized && !g.SettingsLoaded);

    {
        ImGuiSettingsHandler ini_handler;
        ini_handler.TypeName = "Window";
        ini_handler.TypeHash = ImHashStr("Window");
        ini_handler.ClearAllFn = WindowSettingsHandler_ClearAll;
        ini_handler.ReadOpenFn = WindowSettingsHandler_ReadOpen;
        ini_handler.ReadLineFn = WindowSettingsHandler_ReadLine;
        ini_handler.ApplyAllFn = WindowSettingsHandler_ApplyAll;
        ini_handler.WriteAllFn = WindowSettingsHandler_WriteAll;
        g.SettingsHandlers.push_back(ini_handler);
    }

#ifdef IMGUI_HAS_TABLE
    TableSettingsInstallHandler(context);
#endif   

#ifdef IMGUI_HAS_DOCK
    ImGuiViewportP* viewport = IM_NEW(ImGuiViewportP)();
    viewport->ID = IMGUI_VIEWPORT_DEFAULT_ID;
    viewport->Idx = 0;
    viewport->PlatformWindowCreated = true;
    g.Viewports.push_back(viewport);
    g.PlatformIO.MainViewport = g.Viewports[0];              
    g.PlatformIO.Viewports.push_back(g.Viewports[0]);

    DockContextInitialize(&g);
#endif   

    g.Initialized = true;
}

void ImGui::Shutdown(ImGuiContext* context)
{
    ImGuiContext& g = *context;
    if (g.IO.Fonts && g.FontAtlasOwnedByContext)
    {
        g.IO.Fonts->Locked = false;
        IM_DELETE(g.IO.Fonts);
    }
    g.IO.Fonts = NULL;

    if (!g.Initialized)
        return;

    if (g.SettingsLoaded && g.IO.IniFilename != NULL)
    {
        ImGuiContext* backup_context = GImGui;
        SetCurrentContext(&g);
        SaveIniSettingsToDisk(g.IO.IniFilename);
        SetCurrentContext(backup_context);
    }

    ImGuiContext* backup_context = ImGui::GetCurrentContext();
    SetCurrentContext(context);
    DestroyPlatformWindows();
    SetCurrentContext(backup_context);

    DockContextShutdown(&g);

    CallContextHooks(&g, ImGuiContextHookType_Shutdown);

    for (int i = 0; i < g.Windows.Size; i++)
        IM_DELETE(g.Windows[i]);
    g.Windows.clear();
    g.WindowsFocusOrder.clear();
    g.WindowsTempSortBuffer.clear();
    g.CurrentWindow = NULL;
    g.CurrentWindowStack.clear();
    g.WindowsById.Clear();
    g.NavWindow = NULL;
    g.HoveredWindow = g.HoveredRootWindow = g.HoveredWindowUnderMovingWindow = NULL;
    g.ActiveIdWindow = g.ActiveIdPreviousFrameWindow = NULL;
    g.MovingWindow = NULL;
    g.ColorStack.clear();
    g.StyleVarStack.clear();
    g.FontStack.clear();
    g.OpenPopupStack.clear();
    g.BeginPopupStack.clear();

    g.CurrentViewport = g.MouseViewport = g.MouseLastHoveredViewport = NULL;
    for (int i = 0; i < g.Viewports.Size; i++)
        IM_DELETE(g.Viewports[i]);
    g.Viewports.clear();

    g.TabBars.Clear();
    g.CurrentTabBarStack.clear();
    g.ShrinkWidthBuffer.clear();

    g.Tables.Clear();
    g.CurrentTableStack.clear();
    g.DrawChannelsTempMergeBuffer.clear();

    g.ClipboardHandlerData.clear();
    g.MenusIdSubmittedThisFrame.clear();
    g.InputTextState.ClearFreeMemory();

    g.SettingsWindows.clear();
    g.SettingsHandlers.clear();

    if (g.LogFile)
    {
#ifndef IMGUI_DISABLE_TTY_FUNCTIONS
        if (g.LogFile != stdout)
#endif
            ImFileClose(g.LogFile);
        g.LogFile = NULL;
    }
    g.LogBuffer.clear();

    g.Initialized = false;
}

static int IMGUI_CDECL ChildWindowComparer(const void* lhs, const void* rhs)
{
    const ImGuiWindow* const a = *(const ImGuiWindow* const *)lhs;
    const ImGuiWindow* const b = *(const ImGuiWindow* const *)rhs;
    if (int d = (a->Flags & ImGuiWindowFlags_Popup) - (b->Flags & ImGuiWindowFlags_Popup))
        return d;
    if (int d = (a->Flags & ImGuiWindowFlags_Tooltip) - (b->Flags & ImGuiWindowFlags_Tooltip))
        return d;
    return (a->BeginOrderWithinParent - b->BeginOrderWithinParent);
}

static void AddWindowToSortBuffer(ImVector<ImGuiWindow*>* out_sorted_windows, ImGuiWindow* window)
{
    out_sorted_windows->push_back(window);
    if (window->Active)
    {
        int count = window->DC.ChildWindows.Size;
        if (count > 1)
            ImQsort(window->DC.ChildWindows.Data, (size_t)count, sizeof(ImGuiWindow*), ChildWindowComparer);
        for (int i = 0; i < count; i++)
        {
            ImGuiWindow* child = window->DC.ChildWindows[i];
            if (child->Active)
                AddWindowToSortBuffer(out_sorted_windows, child);
        }
    }
}

static void AddDrawListToDrawData(ImVector<ImDrawList*>* out_list, ImDrawList* draw_list)
{
    draw_list->_PopUnusedDrawCmd();
    if (draw_list->CmdBuffer.Size == 0)
        return;

    IM_ASSERT(draw_list->VtxBuffer.Size == 0 || draw_list->_VtxWritePtr == draw_list->VtxBuffer.Data + draw_list->VtxBuffer.Size);
    IM_ASSERT(draw_list->IdxBuffer.Size == 0 || draw_list->_IdxWritePtr == draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size);
    if (!(draw_list->Flags & ImDrawListFlags_AllowVtxOffset))
        IM_ASSERT((int)draw_list->_VtxCurrentIdx == draw_list->VtxBuffer.Size);

    if (sizeof(ImDrawIdx) == 2)
        IM_ASSERT(draw_list->_VtxCurrentIdx < (1 << 16) && "Too many vertices in ImDrawList using 16-bit indices. Read comment above");

    out_list->push_back(draw_list);
}

static void AddWindowToDrawData(ImGuiWindow* window, int layer)
{
    ImGuiContext& g = *GImGui;
    g.IO.MetricsRenderWindows++;
    AddDrawListToDrawData(&window->Viewport->DrawDataBuilder.Layers[layer], window->DrawList);
    for (int i = 0; i < window->DC.ChildWindows.Size; i++)
    {
        ImGuiWindow* child = window->DC.ChildWindows[i];
        if (IsWindowActiveAndVisible(child))         
            AddWindowToDrawData(child, layer);
    }
}

static void AddRootWindowToDrawData(ImGuiWindow* window)
{
    int layer = (window->Flags & ImGuiWindowFlags_Tooltip) ? 1 : 0;
    AddWindowToDrawData(window, layer);
}

void ImDrawDataBuilder::FlattenIntoSingleLayer()
{
    int n = Layers[0].Size;
    int size = n;
    for (int i = 1; i < IM_ARRAYSIZE(Layers); i++)
        size += Layers[i].Size;
    Layers[0].resize(size);
    for (int layer_n = 1; layer_n < IM_ARRAYSIZE(Layers); layer_n++)
    {
        ImVector<ImDrawList*>& layer = Layers[layer_n];
        if (layer.empty())
            continue;
        memcpy(&Layers[0][n], &layer[0], layer.Size * sizeof(ImDrawList*));
        n += layer.Size;
        layer.resize(0);
    }
}

static void SetupViewportDrawData(ImGuiViewportP* viewport, ImVector<ImDrawList*>* draw_lists)
{
    const bool is_minimized = (viewport->Flags & ImGuiViewportFlags_Minimized) != 0;

    ImDrawData* draw_data = &viewport->DrawDataP;
    viewport->DrawData = draw_data;    
    draw_data->Valid = true;
    draw_data->CmdLists = (draw_lists->Size > 0) ? draw_lists->Data : NULL;
    draw_data->CmdListsCount = draw_lists->Size;
    draw_data->TotalVtxCount = draw_data->TotalIdxCount = 0;
    draw_data->DisplayPos = viewport->Pos;
    draw_data->DisplaySize = is_minimized ? ImVec2(0.0f, 0.0f) : viewport->Size;
    draw_data->FramebufferScale = ImGui::GetIO().DisplayFramebufferScale;         
    draw_data->OwnerViewport = viewport;
    for (int n = 0; n < draw_lists->Size; n++)
    {
        draw_data->TotalVtxCount += draw_lists->Data[n]->VtxBuffer.Size;
        draw_data->TotalIdxCount += draw_lists->Data[n]->IdxBuffer.Size;
    }
}

void ImGui::PushClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DrawList->PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect);
    window->ClipRect = window->DrawList->_ClipRectStack.back();
}

void ImGui::PopClipRect()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DrawList->PopClipRect();
    window->ClipRect = window->DrawList->_ClipRectStack.back();
}

static ImGuiWindow* FindFrontMostVisibleChildWindow(ImGuiWindow* window)
{
    for (int n = window->DC.ChildWindows.Size - 1; n >= 0; n--)
        if (IsWindowActiveAndVisible(window->DC.ChildWindows[n]))
            return FindFrontMostVisibleChildWindow(window->DC.ChildWindows[n]);
    return window;
}

static void ImGui::EndFrameDrawDimmedBackgrounds()
{
    ImGuiContext& g = *GImGui;

    ImGuiWindow* modal_window = GetTopMostPopupModal();
    const bool dim_bg_for_modal = (modal_window != NULL);
    const bool dim_bg_for_window_list = (g.NavWindowingTargetAnim != NULL);
    if (dim_bg_for_modal || dim_bg_for_window_list)
        for (int viewport_n = 0; viewport_n < g.Viewports.Size; viewport_n++)
        {
            ImGuiViewportP* viewport = g.Viewports[viewport_n];
            if (modal_window && viewport == modal_window->Viewport)
                continue;
            if (g.NavWindowingListWindow && viewport == g.NavWindowingListWindow->Viewport)
                continue;
            if (g.NavWindowingTargetAnim && viewport == g.NavWindowingTargetAnim->Viewport)
                continue;
            if (viewport->Window && modal_window && IsWindowAbove(viewport->Window, modal_window))
                continue;
            ImDrawList* draw_list = GetForegroundDrawList(viewport);
            const ImU32 dim_bg_col = GetColorU32(dim_bg_for_modal ? ImGuiCol_ModalWindowDimBg : ImGuiCol_NavWindowingDimBg, g.DimBgRatio);
            draw_list->AddRectFilled(viewport->Pos, viewport->Pos + viewport->Size, dim_bg_col);
        }

    if (dim_bg_for_window_list && g.NavWindowingTargetAnim->Active)
    {
        ImGuiWindow* window = g.NavWindowingTargetAnim;
        ImDrawList* draw_list = FindFrontMostVisibleChildWindow(window->RootWindow)->DrawList;
        draw_list->PushClipRectFullScreen();

        if (window->RootWindowDockStop->DockIsActive)
            if (window->RootWindow != window->RootWindowDockStop)
                RenderRectFilledWithHole(draw_list, window->RootWindow->Rect(), window->RootWindowDockStop->Rect(), GetColorU32(ImGuiCol_NavWindowingDimBg, g.DimBgRatio), g.Style.WindowRounding);

        float rounding = ImMax(window->WindowRounding, g.Style.WindowRounding);
        ImRect bb = window->Rect();
        bb.Expand(g.FontSize);
        if (bb.Contains(window->Viewport->GetMainRect()))            
        {
            bb.Expand(-g.FontSize - 1.0f);
            rounding = window->WindowRounding;
        }
        draw_list->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha), rounding, ~0, 3.0f);
        draw_list->PopClipRect();
    }
}

void ImGui::EndFrame()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);

    if (g.FrameCountEnded == g.FrameCount)
        return;
    IM_ASSERT(g.WithinFrameScope && "Forgot to call ImGui::NewFrame()?");

    CallContextHooks(&g, ImGuiContextHookType_EndFramePre);

    ErrorCheckEndFrameSanityChecks();

    if (g.PlatformIO.Platform_SetImeInputPos && (g.PlatformImeLastPos.x == FLT_MAX || ImLengthSqr(g.PlatformImePos - g.PlatformImeLastPos) > 0.0001f))
        if (g.PlatformImePosViewport && g.PlatformImePosViewport->PlatformWindowCreated)
        {
            g.PlatformIO.Platform_SetImeInputPos(g.PlatformImePosViewport, g.PlatformImePos);
            g.PlatformImeLastPos = g.PlatformImePos;
            g.PlatformImePosViewport = NULL;
        }

    g.WithinFrameScopeWithImplicitWindow = false;
    if (g.CurrentWindow && !g.CurrentWindow->WriteAccessed)
        g.CurrentWindow->Active = false;
    End();

    EndFrameDrawDimmedBackgrounds();

    NavEndFrame();

    SetCurrentViewport(NULL, NULL);

    if (g.DragDropActive)
    {
        bool is_delivered = g.DragDropPayload.Delivery;
        bool is_elapsed = (g.DragDropPayload.DataFrameCount + 1 < g.FrameCount) && ((g.DragDropSourceFlags & ImGuiDragDropFlags_SourceAutoExpirePayload) || !IsMouseDown(g.DragDropMouseButton));
        if (is_delivered || is_elapsed)
            ClearDragDrop();
    }

    if (g.DragDropActive && g.DragDropSourceFrameCount < g.FrameCount && !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
    {
        g.DragDropWithinSource = true;
        SetTooltip("...");
        g.DragDropWithinSource = false;
    }

    g.WithinFrameScope = false;
    g.FrameCountEnded = g.FrameCount;

    UpdateMouseMovingWindowEndFrame();

    UpdateViewportsEndFrame();

    g.WindowsTempSortBuffer.resize(0);
    g.WindowsTempSortBuffer.reserve(g.Windows.Size);
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Active && (window->Flags & ImGuiWindowFlags_ChildWindow))                 
            continue;
        AddWindowToSortBuffer(&g.WindowsTempSortBuffer, window);
    }

    IM_ASSERT(g.Windows.Size == g.WindowsTempSortBuffer.Size);
    g.Windows.swap(g.WindowsTempSortBuffer);
    g.IO.MetricsActiveWindows = g.WindowsActiveCount;

    g.IO.Fonts->Locked = false;

    g.IO.MouseWheel = g.IO.MouseWheelH = 0.0f;
    g.IO.InputQueueCharacters.resize(0);
    memset(g.IO.NavInputs, 0, sizeof(g.IO.NavInputs));

    CallContextHooks(&g, ImGuiContextHookType_EndFramePost);
}

void ImGui::Render()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);

    if (g.FrameCountEnded != g.FrameCount)
        EndFrame();
    g.FrameCountRendered = g.FrameCount;
    g.IO.MetricsRenderWindows = 0;

    CallContextHooks(&g, ImGuiContextHookType_RenderPre);

    for (int n = 0; n != g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        viewport->DrawDataBuilder.Clear();
        if (viewport->DrawLists[0] != NULL)
            AddDrawListToDrawData(&viewport->DrawDataBuilder.Layers[0], GetBackgroundDrawList(viewport));
    }

    ImGuiWindow* windows_to_render_top_most[2];
    windows_to_render_top_most[0] = (g.NavWindowingTarget && !(g.NavWindowingTarget->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus)) ? g.NavWindowingTarget->RootWindow : NULL;
    windows_to_render_top_most[1] = (g.NavWindowingTarget ? g.NavWindowingListWindow : NULL);
    for (int n = 0; n != g.Windows.Size; n++)
    {
        ImGuiWindow* window = g.Windows[n];
        if (IsWindowActiveAndVisible(window) && (window->Flags & ImGuiWindowFlags_ChildWindow) == 0 && window != windows_to_render_top_most[0] && window != windows_to_render_top_most[1])
            AddRootWindowToDrawData(window);
    }
    for (int n = 0; n < IM_ARRAYSIZE(windows_to_render_top_most); n++)
        if (windows_to_render_top_most[n] && IsWindowActiveAndVisible(windows_to_render_top_most[n]))          
            AddRootWindowToDrawData(windows_to_render_top_most[n]);

    ImVec2 mouse_cursor_offset, mouse_cursor_size, mouse_cursor_uv[4];
    if (g.IO.MouseDrawCursor && g.MouseCursor != ImGuiMouseCursor_None)
        g.IO.Fonts->GetMouseCursorTexData(g.MouseCursor, &mouse_cursor_offset, &mouse_cursor_size, &mouse_cursor_uv[0], &mouse_cursor_uv[2]);

    g.IO.MetricsRenderVertices = g.IO.MetricsRenderIndices = 0;
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        viewport->DrawDataBuilder.FlattenIntoSingleLayer();

        if (mouse_cursor_size.x > 0.0f && mouse_cursor_size.y > 0.0f)
        {
            float scale = g.Style.MouseCursorScale * viewport->DpiScale;
            if (viewport->GetMainRect().Overlaps(ImRect(g.IO.MousePos, g.IO.MousePos + ImVec2(mouse_cursor_size.x + 2, mouse_cursor_size.y + 2) * scale)))
                RenderMouseCursor(GetForegroundDrawList(viewport), g.IO.MousePos, scale, g.MouseCursor, IM_COL32_WHITE, IM_COL32_BLACK, IM_COL32(0, 0, 0, 48));
        }

        if (viewport->DrawLists[1] != NULL)
            AddDrawListToDrawData(&viewport->DrawDataBuilder.Layers[0], GetForegroundDrawList(viewport));

        SetupViewportDrawData(viewport, &viewport->DrawDataBuilder.Layers[0]);
        g.IO.MetricsRenderVertices += viewport->DrawData->TotalVtxCount;
        g.IO.MetricsRenderIndices += viewport->DrawData->TotalIdxCount;
    }

    CallContextHooks(&g, ImGuiContextHookType_RenderPost);
}

ImVec2 ImGui::CalcTextSize(const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width)
{
    ImGuiContext& g = *GImGui;

    const char* text_display_end;
    if (hide_text_after_double_hash)
        text_display_end = FindRenderedTextEnd(text, text_end);            
    else
        text_display_end = text_end;

    ImFont* font = g.Font;
    const float font_size = g.FontSize;
    if (text == text_display_end)
        return ImVec2(0.0f, font_size);
    ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, wrap_width, text, text_display_end, NULL);

    text_size.x = IM_FLOOR(text_size.x + 0.95f);

    return text_size;
}

static void FindHoveredWindow()
{
    ImGuiContext& g = *GImGui;

    ImGuiViewportP* moving_window_viewport = g.MovingWindow ? g.MovingWindow->Viewport : NULL;
    if (g.MovingWindow)
        g.MovingWindow->Viewport = g.MouseViewport;

    ImGuiWindow* hovered_window = NULL;
    ImGuiWindow* hovered_window_ignoring_moving_window = NULL;
    if (g.MovingWindow && !(g.MovingWindow->Flags & ImGuiWindowFlags_NoMouseInputs))
        hovered_window = g.MovingWindow;

    ImVec2 padding_regular = g.Style.TouchExtraPadding;
    ImVec2 padding_for_resize_from_edges = g.IO.ConfigWindowsResizeFromEdges ? ImMax(g.Style.TouchExtraPadding, ImVec2(WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS, WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS)) : padding_regular;
    for (int i = g.Windows.Size - 1; i >= 0; i--)
    {
        ImGuiWindow* window = g.Windows[i];
        if (!window->Active || window->Hidden)
            continue;
        if (window->Flags & ImGuiWindowFlags_NoMouseInputs)
            continue;
        IM_ASSERT(window->Viewport);
        if (window->Viewport != g.MouseViewport)
            continue;

        ImRect bb(window->OuterRectClipped);
        if (window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
            bb.Expand(padding_regular);
        else
            bb.Expand(padding_for_resize_from_edges);
        if (!bb.Contains(g.IO.MousePos))
            continue;

        if (window->HitTestHoleSize.x != 0)
        {
            ImVec2 hole_pos(window->Pos.x + (float)window->HitTestHoleOffset.x, window->Pos.y + (float)window->HitTestHoleOffset.y);
            ImVec2 hole_size((float)window->HitTestHoleSize.x, (float)window->HitTestHoleSize.y);
            if (ImRect(hole_pos, hole_pos + hole_size).Contains(g.IO.MousePos))
                continue;
        }

        if (hovered_window == NULL)
            hovered_window = window;
        if (hovered_window_ignoring_moving_window == NULL && (!g.MovingWindow || window->RootWindow != g.MovingWindow->RootWindow))
            hovered_window_ignoring_moving_window = window;
        if (hovered_window && hovered_window_ignoring_moving_window)
            break;
    }

    g.HoveredWindow = hovered_window;
    g.HoveredRootWindow = g.HoveredWindow ? g.HoveredWindow->RootWindow : NULL;
    g.HoveredWindowUnderMovingWindow = hovered_window_ignoring_moving_window;

    if (g.MovingWindow)
        g.MovingWindow->Viewport = moving_window_viewport;
}

bool ImGui::IsMouseHoveringRect(const ImVec2& r_min, const ImVec2& r_max, bool clip)
{
    ImGuiContext& g = *GImGui;

    ImRect rect_clipped(r_min, r_max);
    if (clip)
        rect_clipped.ClipWith(g.CurrentWindow->ClipRect);

    const ImRect rect_for_touch(rect_clipped.Min - g.Style.TouchExtraPadding, rect_clipped.Max + g.Style.TouchExtraPadding);
    if (!rect_for_touch.Contains(g.IO.MousePos))
        return false;
    if (!g.MouseViewport->GetMainRect().Overlaps(rect_clipped))
        return false;
    return true;
}

int ImGui::GetKeyIndex(ImGuiKey imgui_key)
{
    IM_ASSERT(imgui_key >= 0 && imgui_key < ImGuiKey_COUNT);
    ImGuiContext& g = *GImGui;
    return g.IO.KeyMap[imgui_key];
}

bool ImGui::IsKeyDown(int user_key_index)
{
    if (user_key_index < 0)
        return false;
    ImGuiContext& g = *GImGui;
    IM_ASSERT(user_key_index >= 0 && user_key_index < IM_ARRAYSIZE(g.IO.KeysDown));
    return g.IO.KeysDown[user_key_index];
}

int ImGui::CalcTypematicRepeatAmount(float t0, float t1, float repeat_delay, float repeat_rate)
{
    if (t1 == 0.0f)
        return 1;
    if (t0 >= t1)
        return 0;
    if (repeat_rate <= 0.0f)
        return (t0 < repeat_delay) && (t1 >= repeat_delay);
    const int count_t0 = (t0 < repeat_delay) ? -1 : (int)((t0 - repeat_delay) / repeat_rate);
    const int count_t1 = (t1 < repeat_delay) ? -1 : (int)((t1 - repeat_delay) / repeat_rate);
    const int count = count_t1 - count_t0;
    return count;
}

int ImGui::GetKeyPressedAmount(int key_index, float repeat_delay, float repeat_rate)
{
    ImGuiContext& g = *GImGui;
    if (key_index < 0)
        return 0;
    IM_ASSERT(key_index >= 0 && key_index < IM_ARRAYSIZE(g.IO.KeysDown));
    const float t = g.IO.KeysDownDuration[key_index];
    return CalcTypematicRepeatAmount(t - g.IO.DeltaTime, t, repeat_delay, repeat_rate);
}

bool ImGui::IsKeyPressed(int user_key_index, bool repeat)
{
    ImGuiContext& g = *GImGui;
    if (user_key_index < 0)
        return false;
    IM_ASSERT(user_key_index >= 0 && user_key_index < IM_ARRAYSIZE(g.IO.KeysDown));
    const float t = g.IO.KeysDownDuration[user_key_index];
    if (t == 0.0f)
        return true;
    if (repeat && t > g.IO.KeyRepeatDelay)
        return GetKeyPressedAmount(user_key_index, g.IO.KeyRepeatDelay, g.IO.KeyRepeatRate) > 0;
    return false;
}

bool ImGui::IsKeyReleased(int user_key_index)
{
    ImGuiContext& g = *GImGui;
    if (user_key_index < 0) return false;
    IM_ASSERT(user_key_index >= 0 && user_key_index < IM_ARRAYSIZE(g.IO.KeysDown));
    return g.IO.KeysDownDurationPrev[user_key_index] >= 0.0f && !g.IO.KeysDown[user_key_index];
}

bool ImGui::IsMouseDown(ImGuiMouseButton button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseDown[button];
}

bool ImGui::IsMouseClicked(ImGuiMouseButton button, bool repeat)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    const float t = g.IO.MouseDownDuration[button];
    if (t == 0.0f)
        return true;

    if (repeat && t > g.IO.KeyRepeatDelay)
    {
        int amount = CalcTypematicRepeatAmount(t - g.IO.DeltaTime, t, g.IO.KeyRepeatDelay, g.IO.KeyRepeatRate * 0.50f);
        if (amount > 0)
            return true;
    }
    return false;
}

bool ImGui::IsMouseReleased(ImGuiMouseButton button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseReleased[button];
}

bool ImGui::IsMouseDoubleClicked(ImGuiMouseButton button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    return g.IO.MouseDoubleClicked[button];
}

bool ImGui::IsMouseDragPastThreshold(ImGuiMouseButton button, float lock_threshold)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (lock_threshold < 0.0f)
        lock_threshold = g.IO.MouseDragThreshold;
    return g.IO.MouseDragMaxDistanceSqr[button] >= lock_threshold * lock_threshold;
}

bool ImGui::IsMouseDragging(ImGuiMouseButton button, float lock_threshold)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (!g.IO.MouseDown[button])
        return false;
    return IsMouseDragPastThreshold(button, lock_threshold);
}

ImVec2 ImGui::GetMousePos()
{
    ImGuiContext& g = *GImGui;
    return g.IO.MousePos;
}

ImVec2 ImGui::GetMousePosOnOpeningCurrentPopup()
{
    ImGuiContext& g = *GImGui;
    if (g.BeginPopupStack.Size > 0)
        return g.OpenPopupStack[g.BeginPopupStack.Size - 1].OpenMousePos;
    return g.IO.MousePos;
}

bool ImGui::IsMousePosValid(const ImVec2* mouse_pos)
{
    IM_ASSERT(GImGui != NULL);
    const float MOUSE_INVALID = -256000.0f;
    ImVec2 p = mouse_pos ? *mouse_pos : GImGui->IO.MousePos;
    return p.x >= MOUSE_INVALID && p.y >= MOUSE_INVALID;
}

bool ImGui::IsAnyMouseDown()
{
    ImGuiContext& g = *GImGui;
    for (int n = 0; n < IM_ARRAYSIZE(g.IO.MouseDown); n++)
        if (g.IO.MouseDown[n])
            return true;
    return false;
}

ImVec2 ImGui::GetMouseDragDelta(ImGuiMouseButton button, float lock_threshold)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    if (lock_threshold < 0.0f)
        lock_threshold = g.IO.MouseDragThreshold;
    if (g.IO.MouseDown[button] || g.IO.MouseReleased[button])
        if (g.IO.MouseDragMaxDistanceSqr[button] >= lock_threshold * lock_threshold)
            if (IsMousePosValid(&g.IO.MousePos) && IsMousePosValid(&g.IO.MouseClickedPos[button]))
                return g.IO.MousePos - g.IO.MouseClickedPos[button];
    return ImVec2(0.0f, 0.0f);
}

void ImGui::ResetMouseDragDelta(ImGuiMouseButton button)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(button >= 0 && button < IM_ARRAYSIZE(g.IO.MouseDown));
    g.IO.MouseClickedPos[button] = g.IO.MousePos;
}

ImGuiMouseCursor ImGui::GetMouseCursor()
{
    return GImGui->MouseCursor;
}

void ImGui::SetMouseCursor(ImGuiMouseCursor cursor_type)
{
    GImGui->MouseCursor = cursor_type;
}

void ImGui::CaptureKeyboardFromApp(bool capture)
{
    GImGui->WantCaptureKeyboardNextFrame = capture ? 1 : 0;
}

void ImGui::CaptureMouseFromApp(bool capture)
{
    GImGui->WantCaptureMouseNextFrame = capture ? 1 : 0;
}

bool ImGui::IsItemActive()
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId)
    {
        ImGuiWindow* window = g.CurrentWindow;
        return g.ActiveId == window->DC.LastItemId;
    }
    return false;
}

bool ImGui::IsItemActivated()
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId)
    {
        ImGuiWindow* window = g.CurrentWindow;
        if (g.ActiveId == window->DC.LastItemId && g.ActiveIdPreviousFrame != window->DC.LastItemId)
            return true;
    }
    return false;
}

bool ImGui::IsItemDeactivated()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HasDeactivated)
        return (window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_Deactivated) != 0;
    return (g.ActiveIdPreviousFrame == window->DC.LastItemId && g.ActiveIdPreviousFrame != 0 && g.ActiveId != window->DC.LastItemId);
}

bool ImGui::IsItemDeactivatedAfterEdit()
{
    ImGuiContext& g = *GImGui;
    return IsItemDeactivated() && (g.ActiveIdPreviousFrameHasBeenEditedBefore || (g.ActiveId == 0 && g.ActiveIdHasBeenEditedBefore));
}

bool ImGui::IsItemFocused()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (g.NavId != window->DC.LastItemId || g.NavId == 0)
        return false;

    if (window->DC.LastItemId == window->ID && window->WriteAccessed)
        return false;

    return true;
}

bool ImGui::IsItemClicked(ImGuiMouseButton mouse_button)
{
    return IsMouseClicked(mouse_button) && IsItemHovered(ImGuiHoveredFlags_None);
}

bool ImGui::IsItemToggledOpen()
{
    ImGuiContext& g = *GImGui;
    return (g.CurrentWindow->DC.LastItemStatusFlags & ImGuiItemStatusFlags_ToggledOpen) ? true : false;
}

bool ImGui::IsItemToggledSelection()
{
    ImGuiContext& g = *GImGui;
    return (g.CurrentWindow->DC.LastItemStatusFlags & ImGuiItemStatusFlags_ToggledSelection) ? true : false;
}

bool ImGui::IsAnyItemHovered()
{
    ImGuiContext& g = *GImGui;
    return g.HoveredId != 0 || g.HoveredIdPreviousFrame != 0;
}

bool ImGui::IsAnyItemActive()
{
    ImGuiContext& g = *GImGui;
    return g.ActiveId != 0;
}

bool ImGui::IsAnyItemFocused()
{
    ImGuiContext& g = *GImGui;
    return g.NavId != 0 && !g.NavDisableHighlight;
}

bool ImGui::IsItemVisible()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->ClipRect.Overlaps(window->DC.LastItemRect);
}

bool ImGui::IsItemEdited()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return (window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_Edited) != 0;
}

void ImGui::SetItemAllowOverlap()
{
    ImGuiContext& g = *GImGui;
    if (g.HoveredId == g.CurrentWindow->DC.LastItemId)
        g.HoveredIdAllowOverlap = true;
    if (g.ActiveId == g.CurrentWindow->DC.LastItemId)
        g.ActiveIdAllowOverlap = true;
}

ImVec2 ImGui::GetItemRectMin()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.LastItemRect.Min;
}

ImVec2 ImGui::GetItemRectMax()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.LastItemRect.Max;
}

ImVec2 ImGui::GetItemRectSize()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.LastItemRect.GetSize();
}

bool ImGui::BeginChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;

    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoDocking;
    flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);      

    const ImVec2 content_avail = GetContentRegionAvail();
    ImVec2 size = ImFloor(size_arg);
    const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
    if (size.x <= 0.0f)
        size.x = ImMax(content_avail.x + size.x, 4.0f);          
    if (size.y <= 0.0f)
        size.y = ImMax(content_avail.y + size.y, 4.0f);
    SetNextWindowSize(size);

    char title[256];
    if (name)
        ImFormatString(title, IM_ARRAYSIZE(title), "%s/%s_%08X", parent_window->Name, name, id);
    else
        ImFormatString(title, IM_ARRAYSIZE(title), "%s/%08X", parent_window->Name, id);

    const float backup_border_size = g.Style.ChildBorderSize;
    if (!border)
        g.Style.ChildBorderSize = 0.0f;
    bool ret = Begin(title, NULL, flags);
    g.Style.ChildBorderSize = backup_border_size;

    ImGuiWindow* child_window = g.CurrentWindow;
    child_window->ChildId = id;
    child_window->AutoFitChildAxises = (ImS8)auto_fit_axises;

    if (child_window->BeginCount == 1)
        parent_window->DC.CursorPos = child_window->Pos;

    if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayerActiveMask != 0 || child_window->DC.NavHasScroll))
    {
        FocusWindow(child_window);
        NavInitWindow(child_window, false);
        SetActiveID(id + 1, child_window);              
        g.ActiveIdSource = ImGuiInputSource_Nav;
    }
    return ret;
}

bool ImGui::BeginChild(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    return BeginChildEx(str_id, window->GetID(str_id), size_arg, border, extra_flags);
}

bool ImGui::BeginChild(ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
    IM_ASSERT(id != 0);
    return BeginChildEx(NULL, id, size_arg, border, extra_flags);
}

void ImGui::EndChild()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    IM_ASSERT(g.WithinEndChild == false);
    IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);      

    g.WithinEndChild = true;
    if (window->BeginCount > 1)
    {
        End();
    }
    else
    {
        ImVec2 sz = window->Size;
        if (window->AutoFitChildAxises & (1 << ImGuiAxis_X))              
            sz.x = ImMax(4.0f, sz.x);
        if (window->AutoFitChildAxises & (1 << ImGuiAxis_Y))
            sz.y = ImMax(4.0f, sz.y);
        End();

        ImGuiWindow* parent_window = g.CurrentWindow;
        ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + sz);
        ItemSize(sz);
        if ((window->DC.NavLayerActiveMask != 0 || window->DC.NavHasScroll) && !(window->Flags & ImGuiWindowFlags_NavFlattened))
        {
            ItemAdd(bb, window->ChildId);
            RenderNavHighlight(bb, window->ChildId);

            if (window->DC.NavLayerActiveMask == 0 && window == g.NavWindow)
                RenderNavHighlight(ImRect(bb.Min - ImVec2(2, 2), bb.Max + ImVec2(2, 2)), g.NavId, ImGuiNavHighlightFlags_TypeThin);
        }
        else
        {
            ItemAdd(bb, 0);
        }
    }
    g.WithinEndChild = false;
}

bool ImGui::BeginChildFrame(ImGuiID id, const ImVec2& size, ImGuiWindowFlags extra_flags)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
    PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
    PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
    PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);
    bool ret = BeginChild(id, size, true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | extra_flags);
    PopStyleVar(3);
    PopStyleColor();
    return ret;
}

void ImGui::EndChildFrame()
{
    EndChild();
}

static void SetWindowConditionAllowFlags(ImGuiWindow* window, ImGuiCond flags, bool enabled)
{
    window->SetWindowPosAllowFlags       = enabled ? (window->SetWindowPosAllowFlags       | flags) : (window->SetWindowPosAllowFlags       & ~flags);
    window->SetWindowSizeAllowFlags      = enabled ? (window->SetWindowSizeAllowFlags      | flags) : (window->SetWindowSizeAllowFlags      & ~flags);
    window->SetWindowCollapsedAllowFlags = enabled ? (window->SetWindowCollapsedAllowFlags | flags) : (window->SetWindowCollapsedAllowFlags & ~flags);
    window->SetWindowDockAllowFlags      = enabled ? (window->SetWindowDockAllowFlags      | flags) : (window->SetWindowDockAllowFlags      & ~flags);
}

ImGuiWindow* ImGui::FindWindowByID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    return (ImGuiWindow*)g.WindowsById.GetVoidPtr(id);
}

ImGuiWindow* ImGui::FindWindowByName(const char* name)
{
    ImGuiID id = ImHashStr(name);
    return FindWindowByID(id);
}

static void ApplyWindowSettings(ImGuiWindow* window, ImGuiWindowSettings* settings)
{
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    window->ViewportPos = main_viewport->Pos;
    if (settings->ViewportId)
    {
        window->ViewportId = settings->ViewportId;
        window->ViewportPos = ImVec2(settings->ViewportPos.x, settings->ViewportPos.y);
    }
    window->Pos = ImFloor(ImVec2(settings->Pos.x + window->ViewportPos.x, settings->Pos.y + window->ViewportPos.y));
    if (settings->Size.x > 0 && settings->Size.y > 0)
        window->Size = window->SizeFull = ImFloor(ImVec2(settings->Size.x, settings->Size.y));
    window->Collapsed = settings->Collapsed;
    window->DockId = settings->DockId;
    window->DockOrder = settings->DockOrder;
}

static ImGuiWindow* CreateNewWindow(const char* name, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = IM_NEW(ImGuiWindow)(&g, name);
    window->Flags = flags;
    g.WindowsById.SetVoidPtr(window->ID, window);

    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    window->Pos = main_viewport->Pos + ImVec2(0, 0);
    window->ViewportPos = main_viewport->Pos;

    if (!(flags & ImGuiWindowFlags_NoSavedSettings))
        if (ImGuiWindowSettings* settings = ImGui::FindWindowSettings(window->ID))
        {
            window->SettingsOffset = g.SettingsWindows.offset_from_ptr(settings);
            SetWindowConditionAllowFlags(window, ImGuiCond_FirstUseEver, false);
            ApplyWindowSettings(window, settings);
        }
    window->DC.CursorStartPos = window->DC.CursorMaxPos = window->Pos;          

    if ((flags & ImGuiWindowFlags_AlwaysAutoResize) != 0)
    {
        window->AutoFitFramesX = window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = false;
    }
    else
    {
        if (window->Size.x <= 0.0f)
            window->AutoFitFramesX = 2;
        if (window->Size.y <= 0.0f)
            window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = (window->AutoFitFramesX > 0) || (window->AutoFitFramesY > 0);
    }

    g.WindowsFocusOrder.push_back(window);
    if (flags & ImGuiWindowFlags_NoBringToFrontOnFocus)
        g.Windows.push_front(window);        
    else
        g.Windows.push_back(window);
    return window;
}

static ImGuiWindow* GetWindowForTitleDisplay(ImGuiWindow* window)
{
    return window->DockNodeAsHost ? window->DockNodeAsHost->VisibleWindow : window;
}

static ImGuiWindow* GetWindowForTitleAndMenuHeight(ImGuiWindow* window)
{
    return (window->DockNodeAsHost && window->DockNodeAsHost->VisibleWindow) ? window->DockNodeAsHost->VisibleWindow : window;
}

static ImVec2 CalcWindowSizeAfterConstraint(ImGuiWindow* window, ImVec2 new_size)
{
    ImGuiContext& g = *GImGui;
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)
    {
        ImRect cr = g.NextWindowData.SizeConstraintRect;
        new_size.x = (cr.Min.x >= 0 && cr.Max.x >= 0) ? ImClamp(new_size.x, cr.Min.x, cr.Max.x) : window->SizeFull.x;
        new_size.y = (cr.Min.y >= 0 && cr.Max.y >= 0) ? ImClamp(new_size.y, cr.Min.y, cr.Max.y) : window->SizeFull.y;
        if (g.NextWindowData.SizeCallback)
        {
            ImGuiSizeCallbackData data;
            data.UserData = g.NextWindowData.SizeCallbackUserData;
            data.Pos = window->Pos;
            data.CurrentSize = window->SizeFull;
            data.DesiredSize = new_size;
            g.NextWindowData.SizeCallback(&data);
            new_size = data.DesiredSize;
        }
        new_size.x = IM_FLOOR(new_size.x);
        new_size.y = IM_FLOOR(new_size.y);
    }

    if (!(window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_AlwaysAutoResize)))
    {
        ImGuiWindow* window_for_height = GetWindowForTitleAndMenuHeight(window);
        new_size = ImMax(new_size, g.Style.WindowMinSize);
        new_size.y = ImMax(new_size.y, window_for_height->TitleBarHeight() + window_for_height->MenuBarHeight() + ImMax(0.0f, g.Style.WindowRounding - 1.0f));       
    }
    return new_size;
}

static ImVec2 CalcWindowContentSize(ImGuiWindow* window)
{
    if (window->Collapsed)
        if (window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
            return window->ContentSize;
    if (window->Hidden && window->HiddenFramesCannotSkipItems == 0 && window->HiddenFramesCanSkipItems > 0)
        return window->ContentSize;

    ImVec2 sz;
    sz.x = IM_FLOOR((window->ContentSizeExplicit.x != 0.0f) ? window->ContentSizeExplicit.x : window->DC.CursorMaxPos.x - window->DC.CursorStartPos.x);
    sz.y = IM_FLOOR((window->ContentSizeExplicit.y != 0.0f) ? window->ContentSizeExplicit.y : window->DC.CursorMaxPos.y - window->DC.CursorStartPos.y);
    return sz;
}

static ImVec2 CalcWindowAutoFitSize(ImGuiWindow* window, const ImVec2& size_contents)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImVec2 size_decorations = ImVec2(0.0f, window->TitleBarHeight() + window->MenuBarHeight());
    ImVec2 size_pad = window->WindowPadding * 2.0f;
    ImVec2 size_desired = size_contents + size_pad + size_decorations;
    if (window->Flags & ImGuiWindowFlags_Tooltip)
    {
        return size_desired;
    }
    else
    {
        const bool is_popup = (window->Flags & ImGuiWindowFlags_Popup) != 0;
        const bool is_menu = (window->Flags & ImGuiWindowFlags_ChildMenu) != 0;
        ImVec2 size_min = style.WindowMinSize;
        if (is_popup || is_menu)                        
            size_min = ImMin(size_min, ImVec2(4.0f, 4.0f));

        ImVec2 avail_size = window->Viewport->Size;
        if (window->ViewportOwned)
            avail_size = ImVec2(FLT_MAX, FLT_MAX);
        const int monitor_idx = window->ViewportAllowPlatformMonitorExtend;
        if (monitor_idx >= 0 && monitor_idx < g.PlatformIO.Monitors.Size)
            avail_size = g.PlatformIO.Monitors[monitor_idx].WorkSize;
        ImVec2 size_auto_fit = ImClamp(size_desired, size_min, ImMax(size_min, avail_size - g.Style.DisplaySafeAreaPadding * 2.0f));

        ImVec2 size_auto_fit_after_constraint = CalcWindowSizeAfterConstraint(window, size_auto_fit);
        bool will_have_scrollbar_x = (size_auto_fit_after_constraint.x - size_pad.x - size_decorations.x < size_contents.x && !(window->Flags & ImGuiWindowFlags_NoScrollbar) && (window->Flags & ImGuiWindowFlags_HorizontalScrollbar)) || (window->Flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar);
        bool will_have_scrollbar_y = (size_auto_fit_after_constraint.y - size_pad.y - size_decorations.y < size_contents.y && !(window->Flags & ImGuiWindowFlags_NoScrollbar)) || (window->Flags & ImGuiWindowFlags_AlwaysVerticalScrollbar);
        if (will_have_scrollbar_x)
            size_auto_fit.y += style.ScrollbarSize;
        if (will_have_scrollbar_y)
            size_auto_fit.x += style.ScrollbarSize;
        return size_auto_fit;
    }
}

ImVec2 ImGui::CalcWindowExpectedSize(ImGuiWindow* window)
{
    ImVec2 size_contents = CalcWindowContentSize(window);
    ImVec2 size_auto_fit = CalcWindowAutoFitSize(window, size_contents);
    ImVec2 size_final = CalcWindowSizeAfterConstraint(window, size_auto_fit);
    return size_final;
}

static ImGuiCol GetWindowBgColorIdxFromFlags(ImGuiWindowFlags flags)
{
    if (flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup))
        return ImGuiCol_PopupBg;
    if (flags & ImGuiWindowFlags_ChildWindow)
        return ImGuiCol_ChildBg;
    return ImGuiCol_WindowBg;
}

static void CalcResizePosSizeFromAnyCorner(ImGuiWindow* window, const ImVec2& corner_target, const ImVec2& corner_norm, ImVec2* out_pos, ImVec2* out_size)
{
    ImVec2 pos_min = ImLerp(corner_target, window->Pos, corner_norm);                   
    ImVec2 pos_max = ImLerp(window->Pos + window->Size, corner_target, corner_norm);    
    ImVec2 size_expected = pos_max - pos_min;
    ImVec2 size_constrained = CalcWindowSizeAfterConstraint(window, size_expected);
    *out_pos = pos_min;
    if (corner_norm.x == 0.0f)
        out_pos->x -= (size_constrained.x - size_expected.x);
    if (corner_norm.y == 0.0f)
        out_pos->y -= (size_constrained.y - size_expected.y);
    *out_size = size_constrained;
}

struct ImGuiResizeGripDef
{
    ImVec2  CornerPosN;
    ImVec2  InnerDir;
    int     AngleMin12, AngleMax12;
};

static const ImGuiResizeGripDef resize_grip_def[4] =
{
    { ImVec2(1, 1), ImVec2(-1, -1), 0, 3 },  
    { ImVec2(0, 1), ImVec2(+1, -1), 3, 6 },  
    { ImVec2(0, 0), ImVec2(+1, +1), 6, 9 },   
    { ImVec2(1, 0), ImVec2(-1, +1), 9, 12 },   
};

struct ImGuiResizeBorderDef
{
    ImVec2 InnerDir;
    ImVec2 CornerPosN1, CornerPosN2;
    float  OuterAngle;
};

static const ImGuiResizeBorderDef resize_border_def[4] =
{
    { ImVec2(0, +1), ImVec2(0, 0), ImVec2(1, 0), IM_PI * 1.50f },  
    { ImVec2(-1, 0), ImVec2(1, 0), ImVec2(1, 1), IM_PI * 0.00f },  
    { ImVec2(0, -1), ImVec2(1, 1), ImVec2(0, 1), IM_PI * 0.50f },  
    { ImVec2(+1, 0), ImVec2(0, 1), ImVec2(0, 0), IM_PI * 1.00f }  
};

static ImRect GetResizeBorderRect(ImGuiWindow* window, int border_n, float perp_padding, float thickness)
{
    ImRect rect = window->Rect();
    if (thickness == 0.0f) rect.Max -= ImVec2(1, 1);
    if (border_n == 0) { return ImRect(rect.Min.x + perp_padding, rect.Min.y - thickness,    rect.Max.x - perp_padding, rect.Min.y + thickness);    }  
    if (border_n == 1) { return ImRect(rect.Max.x - thickness,    rect.Min.y + perp_padding, rect.Max.x + thickness,    rect.Max.y - perp_padding); }  
    if (border_n == 2) { return ImRect(rect.Min.x + perp_padding, rect.Max.y - thickness,    rect.Max.x - perp_padding, rect.Max.y + thickness);    }  
    if (border_n == 3) { return ImRect(rect.Min.x - thickness,    rect.Min.y + perp_padding, rect.Min.x + thickness,    rect.Max.y - perp_padding); }  
    IM_ASSERT(0);
    return ImRect();
}

ImGuiID ImGui::GetWindowResizeID(ImGuiWindow* window, int n)
{
    IM_ASSERT(n >= 0 && n <= 7);
    ImGuiID id = window->DockIsActive ? window->DockNode->HostWindow->ID : window->ID;
    id = ImHashStr("#RESIZE", 0, id);
    id = ImHashData(&n, sizeof(int), id);
    return id;
}

static bool ImGui::UpdateWindowManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4], const ImRect& visibility_rect)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindowFlags flags = window->Flags;

    if ((flags & ImGuiWindowFlags_NoResize) || (flags & ImGuiWindowFlags_AlwaysAutoResize) || window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
        return false;
    if (window->WasActive == false)               
        return false;

    bool ret_auto_fit = false;
    const int resize_border_count = g.IO.ConfigWindowsResizeFromEdges ? 4 : 0;
    const float grip_draw_size = IM_FLOOR(ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f));
    const float grip_hover_inner_size = IM_FLOOR(grip_draw_size * 0.75f);
    const float grip_hover_outer_size = g.IO.ConfigWindowsResizeFromEdges ? WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS : 0.0f;

    ImVec2 pos_target(FLT_MAX, FLT_MAX);
    ImVec2 size_target(FLT_MAX, FLT_MAX);

    const bool clip_with_viewport_rect = !(g.IO.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport) || (g.IO.MouseHoveredViewport != window->ViewportId) || !(window->Viewport->Flags & ImGuiViewportFlags_NoDecoration);
    if (clip_with_viewport_rect)
        window->ClipRect = window->Viewport->GetMainRect();

    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

    PushID("#RESIZE");
    for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
    {
        const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
        const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPosN);

        ImRect resize_rect(corner - grip.InnerDir * grip_hover_outer_size, corner + grip.InnerDir * grip_hover_inner_size);
        if (resize_rect.Min.x > resize_rect.Max.x) ImSwap(resize_rect.Min.x, resize_rect.Max.x);
        if (resize_rect.Min.y > resize_rect.Max.y) ImSwap(resize_rect.Min.y, resize_rect.Max.y);
        bool hovered, held;
        ButtonBehavior(resize_rect, window->GetID(resize_grip_n), &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
        if (hovered || held)
            g.MouseCursor = (resize_grip_n & 1) ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE;

        if (held && g.IO.MouseDoubleClicked[0] && resize_grip_n == 0)
        {
            size_target = CalcWindowSizeAfterConstraint(window, size_auto_fit);
            ret_auto_fit = true;
            ClearActiveID();
        }
        else if (held)
        {
            ImVec2 corner_target = g.IO.MousePos - g.ActiveIdClickOffset + ImLerp(grip.InnerDir * grip_hover_outer_size, grip.InnerDir * -grip_hover_inner_size, grip.CornerPosN);          
            ImVec2 clamp_min = ImVec2(grip.CornerPosN.x == 1.0f ? visibility_rect.Min.x : -FLT_MAX, grip.CornerPosN.y == 1.0f ? visibility_rect.Min.y : -FLT_MAX);
            ImVec2 clamp_max = ImVec2(grip.CornerPosN.x == 0.0f ? visibility_rect.Max.x : +FLT_MAX, grip.CornerPosN.y == 0.0f ? visibility_rect.Max.y : +FLT_MAX);
            corner_target = ImClamp(corner_target, clamp_min, clamp_max);
            CalcResizePosSizeFromAnyCorner(window, corner_target, grip.CornerPosN, &pos_target, &size_target);
        }
        if (resize_grip_n == 0 || held || hovered)
            resize_grip_col[resize_grip_n] = GetColorU32(held ? ImGuiCol_ResizeGripActive : hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip);
    }
    for (int border_n = 0; border_n < resize_border_count; border_n++)
    {
        bool hovered, held;
        ImRect border_rect = GetResizeBorderRect(window, border_n, grip_hover_inner_size, WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS);
        ButtonBehavior(border_rect, window->GetID(border_n + 4), &hovered, &held, ImGuiButtonFlags_FlattenChildren);
        if ((hovered && g.HoveredIdTimer > WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER) || held)
        {
            g.MouseCursor = (border_n & 1) ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS;
            if (held)
                *border_held = border_n;
        }
        if (held)
        {
            ImVec2 border_target = window->Pos;
            ImVec2 border_posn;
            if (border_n == 0) { border_posn = ImVec2(0, 0); border_target.y = (g.IO.MousePos.y - g.ActiveIdClickOffset.y + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); }  
            if (border_n == 1) { border_posn = ImVec2(1, 0); border_target.x = (g.IO.MousePos.x - g.ActiveIdClickOffset.x + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); }  
            if (border_n == 2) { border_posn = ImVec2(0, 1); border_target.y = (g.IO.MousePos.y - g.ActiveIdClickOffset.y + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); }  
            if (border_n == 3) { border_posn = ImVec2(0, 0); border_target.x = (g.IO.MousePos.x - g.ActiveIdClickOffset.x + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); }  
            ImVec2 clamp_min = ImVec2(border_n == 1 ? visibility_rect.Min.x : -FLT_MAX, border_n == 2 ? visibility_rect.Min.y : -FLT_MAX);
            ImVec2 clamp_max = ImVec2(border_n == 3 ? visibility_rect.Max.x : +FLT_MAX, border_n == 0 ? visibility_rect.Max.y : +FLT_MAX);
            border_target = ImClamp(border_target, clamp_min, clamp_max);
            CalcResizePosSizeFromAnyCorner(window, border_target, border_posn, &pos_target, &size_target);
        }
    }
    PopID();

    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;

    if (g.NavWindowingTarget && g.NavWindowingTarget->RootWindow == window)
    {
        ImVec2 nav_resize_delta;
        if (g.NavInputSource == ImGuiInputSource_NavKeyboard && g.IO.KeyShift)
            nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard, ImGuiInputReadMode_Down);
        if (g.NavInputSource == ImGuiInputSource_NavGamepad)
            nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_Down);
        if (nav_resize_delta.x != 0.0f || nav_resize_delta.y != 0.0f)
        {
            const float NAV_RESIZE_SPEED = 600.0f;
            nav_resize_delta *= ImFloor(NAV_RESIZE_SPEED * g.IO.DeltaTime * ImMin(g.IO.DisplayFramebufferScale.x, g.IO.DisplayFramebufferScale.y));
            nav_resize_delta = ImMax(nav_resize_delta, visibility_rect.Min - window->Pos - window->Size);
            g.NavWindowingToggleLayer = false;
            g.NavDisableMouseHover = true;
            resize_grip_col[0] = GetColorU32(ImGuiCol_ResizeGripActive);
            size_target = CalcWindowSizeAfterConstraint(window, window->SizeFull + nav_resize_delta);
        }
    }

    if (size_target.x != FLT_MAX)
    {
        window->SizeFull = size_target;
        MarkIniSettingsDirty(window);
    }
    if (pos_target.x != FLT_MAX)
    {
        window->Pos = ImFloor(pos_target);
        MarkIniSettingsDirty(window);
    }

    window->Size = window->SizeFull;
    return ret_auto_fit;
}

static inline void ClampWindowRect(ImGuiWindow* window, const ImRect& visibility_rect)
{
    ImGuiContext& g = *GImGui;
    ImVec2 size_for_clamping = window->Size;
    if (g.IO.ConfigWindowsMoveFromTitleBarOnly && !(window->Flags & ImGuiWindowFlags_NoTitleBar))
        size_for_clamping.y = window->TitleBarHeight();
    window->Pos = ImClamp(window->Pos, visibility_rect.Min - size_for_clamping, visibility_rect.Max);
}

static void ImGui::RenderWindowOuterBorders(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    float rounding = window->WindowRounding;
    float border_size = window->WindowBorderSize;
    if (border_size > 0.0f && !(window->Flags & ImGuiWindowFlags_NoBackground))
        window->DrawList->AddRect(window->Pos, window->Pos + window->Size, GetColorU32(ImGuiCol_Border), rounding, ImDrawCornerFlags_All, border_size);

    int border_held = window->ResizeBorderHeld;
    if (border_held != -1)
    {
        const ImGuiResizeBorderDef& def = resize_border_def[border_held];
        ImRect border_r = GetResizeBorderRect(window, border_held, rounding, 0.0f);
        window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.CornerPosN1) + ImVec2(0.5f, 0.5f) + def.InnerDir * rounding, rounding, def.OuterAngle - IM_PI * 0.25f, def.OuterAngle);
        window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.CornerPosN2) + ImVec2(0.5f, 0.5f) + def.InnerDir * rounding, rounding, def.OuterAngle, def.OuterAngle + IM_PI * 0.25f);
        window->DrawList->PathStroke(GetColorU32(ImGuiCol_SeparatorActive), false, ImMax(2.0f, border_size));    
    }
    if (g.Style.FrameBorderSize > 0 && !(window->Flags & ImGuiWindowFlags_NoTitleBar) && !window->DockIsActive)
    {
        float y = window->Pos.y + window->TitleBarHeight() - 1;
        window->DrawList->AddLine(ImVec2(window->Pos.x + border_size, y), ImVec2(window->Pos.x + window->Size.x - border_size, y), GetColorU32(ImGuiCol_Border), g.Style.FrameBorderSize);
    }
}

void ImGui::RenderWindowDecorations(ImGuiWindow* window, const ImRect& title_bar_rect, bool title_bar_is_highlight, bool handle_borders_and_resize_grips, int resize_grip_count, const ImU32 resize_grip_col[4], float resize_grip_draw_size)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiWindowFlags flags = window->Flags;

    IM_ASSERT(window->BeginCount == 0);
    window->SkipItems = false;

    const float window_rounding = window->WindowRounding;
    const float window_border_size = window->WindowBorderSize;
    if (window->Collapsed)
    {
        float backup_border_size = style.FrameBorderSize;
        g.Style.FrameBorderSize = window->WindowBorderSize;
        ImU32 title_bar_col = GetColorU32((title_bar_is_highlight && !g.NavDisableHighlight) ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBgCollapsed);
        RenderFrame(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, true, window_rounding);
        g.Style.FrameBorderSize = backup_border_size;
    }
    else
    {
        if (!(flags & ImGuiWindowFlags_NoBackground))
        {
            bool is_docking_transparent_payload = false;
            if (g.DragDropActive && (g.FrameCount - g.DragDropAcceptFrameCount) <= 1 && g.IO.ConfigDockingTransparentPayload)
                if (g.DragDropPayload.IsDataType(IMGUI_PAYLOAD_TYPE_WINDOW) && *(ImGuiWindow**)g.DragDropPayload.Data == window)
                    is_docking_transparent_payload = true;

            ImU32 bg_col = GetColorU32(GetWindowBgColorIdxFromFlags(flags));
            if (window->ViewportOwned)
            {
                bg_col = (bg_col | IM_COL32_A_MASK);
                if (is_docking_transparent_payload)
                    window->Viewport->Alpha *= DOCKING_TRANSPARENT_PAYLOAD_ALPHA;
            }
            else
            {
                bool override_alpha = false;
                float alpha = 1.0f;
                if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasBgAlpha)
                {
                    alpha = g.NextWindowData.BgAlphaVal;
                    override_alpha = true;
                }
                if (is_docking_transparent_payload)
                {
                    alpha *= DOCKING_TRANSPARENT_PAYLOAD_ALPHA;       
                    override_alpha = true;
                }
                if (override_alpha)
                    bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
            }
            window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Bot);
        }

        if (!(flags & ImGuiWindowFlags_NoTitleBar) && !window->DockIsActive)
        {
            ImU32 title_bar_col = GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
            window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, window_rounding, ImDrawCornerFlags_Top);
        }

        if (flags & ImGuiWindowFlags_MenuBar)
        {
            ImRect menu_bar_rect = window->MenuBarRect();
            menu_bar_rect.ClipWith(window->Rect());                      
            window->DrawList->AddRectFilled(menu_bar_rect.Min + ImVec2(window_border_size, 0), menu_bar_rect.Max - ImVec2(window_border_size, 0), GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImDrawCornerFlags_Top);
            if (style.FrameBorderSize > 0.0f && menu_bar_rect.Max.y < window->Pos.y + window->Size.y)
                window->DrawList->AddLine(menu_bar_rect.GetBL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
        }

        ImGuiDockNode* node = window->DockNode;
        if (window->DockIsActive && node->IsHiddenTabBar() && !node->IsNoTabBar())
        {
            float unhide_sz_draw = ImFloor(g.FontSize * 0.70f);
            float unhide_sz_hit = ImFloor(g.FontSize * 0.55f);
            ImVec2 p = node->Pos;
            ImRect r(p, p + ImVec2(unhide_sz_hit, unhide_sz_hit));
            bool hovered, held;
            if (ButtonBehavior(r, window->GetID("#UNHIDE"), &hovered, &held, ImGuiButtonFlags_FlattenChildren))
                node->WantHiddenTabBarToggle = true;
            else if (held && IsMouseDragging(0))
                StartMouseMovingWindowOrNode(window, node, true);

            ImU32 col = GetColorU32(((held && hovered) || (node->IsFocused && !hovered)) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
            window->DrawList->AddTriangleFilled(p, p + ImVec2(unhide_sz_draw, 0.0f), p + ImVec2(0.0f, unhide_sz_draw), col);
        }

        if (window->ScrollbarX)
            Scrollbar(ImGuiAxis_X);
        if (window->ScrollbarY)
            Scrollbar(ImGuiAxis_Y);

        if (handle_borders_and_resize_grips && !(flags & ImGuiWindowFlags_NoResize))
        {
            for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
            {
                const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
                const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPosN);
                window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(window_border_size, resize_grip_draw_size) : ImVec2(resize_grip_draw_size, window_border_size)));
                window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(resize_grip_draw_size, window_border_size) : ImVec2(window_border_size, resize_grip_draw_size)));
                window->DrawList->PathArcToFast(ImVec2(corner.x + grip.InnerDir.x * (window_rounding + window_border_size), corner.y + grip.InnerDir.y * (window_rounding + window_border_size)), window_rounding, grip.AngleMin12, grip.AngleMax12);
                window->DrawList->PathFillConvex(resize_grip_col[resize_grip_n]);
            }
        }

        if (handle_borders_and_resize_grips && !window->DockNodeAsHost)
            RenderWindowOuterBorders(window);
    }
}

void ImGui::RenderWindowTitleBarContents(ImGuiWindow* window, const ImRect& title_bar_rect, const char* name, bool* p_open)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiWindowFlags flags = window->Flags;

    const bool has_close_button = (p_open != NULL);
    const bool has_collapse_button = !(flags & ImGuiWindowFlags_NoCollapse) && (style.WindowMenuButtonPosition != ImGuiDir_None);

    const ImGuiItemFlags item_flags_backup = window->DC.ItemFlags;
    window->DC.ItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;

    float pad_l = style.FramePadding.x;
    float pad_r = style.FramePadding.x;
    float button_sz = g.FontSize;
    ImVec2 close_button_pos;
    ImVec2 collapse_button_pos;
    if (has_close_button)
    {
        pad_r += button_sz;
        close_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - style.FramePadding.x, title_bar_rect.Min.y);
    }
    if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Right)
    {
        pad_r += button_sz;
        collapse_button_pos = ImVec2(title_bar_rect.Max.x - pad_r - style.FramePadding.x, title_bar_rect.Min.y);
    }
    if (has_collapse_button && style.WindowMenuButtonPosition == ImGuiDir_Left)
    {
        collapse_button_pos = ImVec2(title_bar_rect.Min.x + pad_l - style.FramePadding.x, title_bar_rect.Min.y);
        pad_l += button_sz;
    }

    if (has_collapse_button)
        if (CollapseButton(window->GetID("#COLLAPSE"), collapse_button_pos, NULL))
            window->WantCollapseToggle = true;                

    if (has_close_button)
        if (CloseButton(window->GetID("#CLOSE"), close_button_pos))
            *p_open = false;

    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
    window->DC.ItemFlags = item_flags_backup;

    const char* UNSAVED_DOCUMENT_MARKER = "*";
    const float marker_size_x = (flags & ImGuiWindowFlags_UnsavedDocument) ? CalcTextSize(UNSAVED_DOCUMENT_MARKER, NULL, false).x : 0.0f;
    const ImVec2 text_size = CalcTextSize(name, NULL, true) + ImVec2(marker_size_x, 0.0f);

    if (pad_l > style.FramePadding.x)
        pad_l += g.Style.ItemInnerSpacing.x;
    if (pad_r > style.FramePadding.x)
        pad_r += g.Style.ItemInnerSpacing.x;
    if (style.WindowTitleAlign.x > 0.0f && style.WindowTitleAlign.x < 1.0f)
    {
        float centerness = ImSaturate(1.0f - ImFabs(style.WindowTitleAlign.x - 0.5f) * 2.0f);        
        float pad_extend = ImMin(ImMax(pad_l, pad_r), title_bar_rect.GetWidth() - pad_l - pad_r - text_size.x);
        pad_l = ImMax(pad_l, pad_extend * centerness);
        pad_r = ImMax(pad_r, pad_extend * centerness);
    }

    ImRect layout_r(title_bar_rect.Min.x + pad_l, title_bar_rect.Min.y, title_bar_rect.Max.x - pad_r, title_bar_rect.Max.y);
    ImRect clip_r(layout_r.Min.x, layout_r.Min.y, layout_r.Max.x + g.Style.ItemInnerSpacing.x, layout_r.Max.y);
    RenderTextClipped(layout_r.Min, layout_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_r);
    if (flags & ImGuiWindowFlags_UnsavedDocument)
    {
        ImVec2 marker_pos = ImVec2(ImMax(layout_r.Min.x, layout_r.Min.x + (layout_r.GetWidth() - text_size.x) * style.WindowTitleAlign.x) + text_size.x, layout_r.Min.y) + ImVec2(2 - marker_size_x, 0.0f);
        ImVec2 off = ImVec2(0.0f, IM_FLOOR(-g.FontSize * 0.25f));
        RenderTextClipped(marker_pos + off, layout_r.Max + off, UNSAVED_DOCUMENT_MARKER, NULL, NULL, ImVec2(0, style.WindowTitleAlign.y), &clip_r);
    }
}

void ImGui::UpdateWindowParentAndRootLinks(ImGuiWindow* window, ImGuiWindowFlags flags, ImGuiWindow* parent_window)
{
    window->ParentWindow = parent_window;
    window->RootWindow = window->RootWindowDockStop = window->RootWindowForTitleBarHighlight = window->RootWindowForNav = window;
    if (parent_window && (flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Tooltip))
    {
        window->RootWindow = parent_window->RootWindow;
        if (!window->DockIsActive && !(parent_window->Flags & ImGuiWindowFlags_DockNodeHost))
            window->RootWindowDockStop = parent_window->RootWindowDockStop;
    }
    if (parent_window && !(flags & ImGuiWindowFlags_Modal) && (flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)))
        window->RootWindowForTitleBarHighlight = parent_window->RootWindowForTitleBarHighlight;
    while (window->RootWindowForNav->Flags & ImGuiWindowFlags_NavFlattened)
    {
        IM_ASSERT(window->RootWindowForNav->ParentWindow != NULL);
        window->RootWindowForNav = window->RootWindowForNav->ParentWindow;
    }
}

bool ImGui::Begin(const char* name, bool* p_open, ImGuiWindowFlags flags, void* parent_hwnd)  //##EDIT##//
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    IM_ASSERT(name != NULL && name[0] != '\0');        
    IM_ASSERT(g.WithinFrameScope);                      
    IM_ASSERT(g.FrameCountEnded != g.FrameCount);             

    ImGuiWindow* window = FindWindowByName(name);
    const bool window_just_created = (window == NULL);
    if (window_just_created)
        window = CreateNewWindow(name, flags);

    if ((flags & ImGuiWindowFlags_NoInputs) == ImGuiWindowFlags_NoInputs)
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    if (flags & ImGuiWindowFlags_NavFlattened)
        IM_ASSERT(flags & ImGuiWindowFlags_ChildWindow);

    const int current_frame = g.FrameCount;
    const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);
    window->IsFallbackWindow = (g.CurrentWindowStack.Size == 0 && g.WithinFrameScopeWithImplicitWindow);

    bool window_just_activated_by_user = (window->LastFrameActive < current_frame - 1);               
    const bool window_just_appearing_after_hidden_for_resize = (window->HiddenFramesCannotSkipItems > 0);
    if (flags & ImGuiWindowFlags_Popup)
    {
        ImGuiPopupData& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
        window_just_activated_by_user |= (window->PopupId != popup_ref.PopupId);             
        window_just_activated_by_user |= (window != popup_ref.Window);
    }
    window->Appearing = (window_just_activated_by_user || window_just_appearing_after_hidden_for_resize);
    if (window->Appearing)
        SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, true);

    if (first_begin_of_the_frame)
    {
        window->FlagsPreviousFrame = window->Flags;
        window->Flags = (ImGuiWindowFlags)flags;
        window->LastFrameActive = current_frame;
        window->LastTimeActive = (float)g.Time;
        window->BeginOrderWithinParent = 0;
        window->BeginOrderWithinContext = (short)(g.WindowsActiveCount++);
    }
    else
    {
        flags = window->Flags;
    }

    IM_ASSERT(window->DockNode == NULL || window->DockNodeAsHost == NULL);    
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasDock)
        SetWindowDock(window, g.NextWindowData.DockId, g.NextWindowData.DockCond);
    if (first_begin_of_the_frame)
    {
        bool has_dock_node = (window->DockId != 0 || window->DockNode != NULL);
        bool new_auto_dock_node = !has_dock_node && GetWindowAlwaysWantOwnTabBar(window);
        if (has_dock_node || new_auto_dock_node)
        {
            BeginDocked(window, p_open);
            flags = window->Flags;
            if (window->DockIsActive)
                IM_ASSERT(window->DockNode != NULL);

            g.NextWindowData.Flags &= ~ImGuiNextWindowDataFlags_HasSizeConstraint;
        }
    }

    ImGuiWindow* parent_window_in_stack = window->DockIsActive ? window->DockNode->HostWindow : g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back();
    ImGuiWindow* parent_window = first_begin_of_the_frame ? ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)) ? parent_window_in_stack : NULL) : window->ParentWindow;
    IM_ASSERT(parent_window != NULL || !(flags & ImGuiWindowFlags_ChildWindow));

    if (window->IDStack.Size == 0)
        window->IDStack.push_back(window->ID);

    g.CurrentWindowStack.push_back(window);
    g.CurrentWindow = window;
    window->DC.StackSizesOnBegin.SetToCurrentState();
    g.CurrentWindow = NULL;

    if (flags & ImGuiWindowFlags_Popup)
    {
        ImGuiPopupData& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
        popup_ref.Window = window;
        g.BeginPopupStack.push_back(popup_ref);
        window->PopupId = popup_ref.PopupId;
    }

    if (window_just_appearing_after_hidden_for_resize && !(flags & ImGuiWindowFlags_ChildWindow))
        window->NavLastIds[0] = 0;

    if (first_begin_of_the_frame)
        UpdateWindowParentAndRootLinks(window, flags, parent_window);

    bool window_pos_set_by_api = false;
    bool window_size_x_set_by_api = false, window_size_y_set_by_api = false;
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos)
    {
        window_pos_set_by_api = (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) != 0;
        if (window_pos_set_by_api && ImLengthSqr(g.NextWindowData.PosPivotVal) > 0.00001f)
        {
            window->SetWindowPosVal = g.NextWindowData.PosVal;
            window->SetWindowPosPivot = g.NextWindowData.PosPivotVal;
            window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
        }
        else
        {
            SetWindowPos(window, g.NextWindowData.PosVal, g.NextWindowData.PosCond);
        }
    }
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSize)
    {
        window_size_x_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.x > 0.0f);
        window_size_y_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.y > 0.0f);
        SetWindowSize(window, g.NextWindowData.SizeVal, g.NextWindowData.SizeCond);
    }
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasScroll)
    {
        if (g.NextWindowData.ScrollVal.x >= 0.0f)
        {
            window->ScrollTarget.x = g.NextWindowData.ScrollVal.x;
            window->ScrollTargetCenterRatio.x = 0.0f;
        }
        if (g.NextWindowData.ScrollVal.y >= 0.0f)
        {
            window->ScrollTarget.y = g.NextWindowData.ScrollVal.y;
            window->ScrollTargetCenterRatio.y = 0.0f;
        }
    }
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasContentSize)
        window->ContentSizeExplicit = g.NextWindowData.ContentSizeVal;
    else if (first_begin_of_the_frame)
        window->ContentSizeExplicit = ImVec2(0.0f, 0.0f);
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasWindowClass)
        window->WindowClass = g.NextWindowData.WindowClass;
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasCollapsed)
        SetWindowCollapsed(window, g.NextWindowData.CollapsedVal, g.NextWindowData.CollapsedCond);
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasFocus)
        FocusWindow(window);
    if (window->Appearing)
        SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, false);

    if (first_begin_of_the_frame)
    {
        const bool window_is_child_tooltip = (flags & ImGuiWindowFlags_ChildWindow) && (flags & ImGuiWindowFlags_Tooltip);          
        window->Active = true;
        window->HasCloseButton = (p_open != NULL);
        window->ClipRect = ImVec4(-FLT_MAX, -FLT_MAX, +FLT_MAX, +FLT_MAX);
        window->IDStack.resize(1);
        window->DrawList->_ResetForNewFrame();
        window->DC.CurrentTableIdx = -1;

        if (window->MemoryCompacted)
            GcAwakeTransientWindowBuffers(window);

        bool window_title_visible_elsewhere = false;
        if ((window->Viewport && window->Viewport->Window == window) || (window->DockIsActive))
            window_title_visible_elsewhere = true;
        else if (g.NavWindowingListWindow != NULL && (window->Flags & ImGuiWindowFlags_NoNavFocus) == 0)         
            window_title_visible_elsewhere = true;
        if (window_title_visible_elsewhere && !window_just_created && strcmp(name, window->Name) != 0)
        {
            size_t buf_len = (size_t)window->NameBufLen;
            window->Name = ImStrdupcpy(window->Name, &buf_len, name);
            window->NameBufLen = (int)buf_len;
        }

        window->ContentSize = CalcWindowContentSize(window);
        if (window->HiddenFramesCanSkipItems > 0)
            window->HiddenFramesCanSkipItems--;
        if (window->HiddenFramesCannotSkipItems > 0)
            window->HiddenFramesCannotSkipItems--;

        if (window_just_created && (!window_size_x_set_by_api || !window_size_y_set_by_api))
            window->HiddenFramesCannotSkipItems = 1;

        if (window_just_activated_by_user && (flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0)
        {
            window->HiddenFramesCannotSkipItems = 1;
            if (flags & ImGuiWindowFlags_AlwaysAutoResize)
            {
                if (!window_size_x_set_by_api)
                    window->Size.x = window->SizeFull.x = 0.f;
                if (!window_size_y_set_by_api)
                    window->Size.y = window->SizeFull.y = 0.f;
                window->ContentSize = ImVec2(0.f, 0.f);
            }
        }

        UpdateSelectWindowViewport(window);
        SetCurrentViewport(window, window->Viewport);
        window->FontDpiScale = (g.IO.ConfigFlags & ImGuiConfigFlags_DpiEnableScaleFonts) ? window->Viewport->DpiScale : 1.0f;
        SetCurrentWindow(window);
        flags = window->Flags;

        if (flags & ImGuiWindowFlags_ChildWindow)
            window->WindowBorderSize = style.ChildBorderSize;
        else
            window->WindowBorderSize = ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupBorderSize : style.WindowBorderSize;
        if (!window->DockIsActive && (flags & ImGuiWindowFlags_ChildWindow) && !(flags & (ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_Popup)) && window->WindowBorderSize == 0.0f)
            window->WindowPadding = ImVec2(0.0f, (flags & ImGuiWindowFlags_MenuBar) ? style.WindowPadding.y : 0.0f);
        else
            window->WindowPadding = style.WindowPadding;

        window->DC.MenuBarOffset.x = ImMax(ImMax(window->WindowPadding.x, style.ItemSpacing.x), g.NextWindowData.MenuBarOffsetMinVal.x);
        window->DC.MenuBarOffset.y = g.NextWindowData.MenuBarOffsetMinVal.y;

        if (!(flags & ImGuiWindowFlags_NoTitleBar) && !(flags & ImGuiWindowFlags_NoCollapse) && !window->DockIsActive)
        {
            ImRect title_bar_rect = window->TitleBarRect();
            if (g.HoveredWindow == window && g.HoveredId == 0 && g.HoveredIdPreviousFrame == 0 && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max) && g.IO.MouseDoubleClicked[0])
                window->WantCollapseToggle = true;
            if (window->WantCollapseToggle)
            {
                window->Collapsed = !window->Collapsed;
                MarkIniSettingsDirty(window);
                FocusWindow(window);
            }
        }
        else
        {
            window->Collapsed = false;
        }
        window->WantCollapseToggle = false;

        const ImVec2 size_auto_fit = CalcWindowAutoFitSize(window, window->ContentSize);
        bool use_current_size_for_scrollbar_x = window_just_created;
        bool use_current_size_for_scrollbar_y = window_just_created;
        if ((flags & ImGuiWindowFlags_AlwaysAutoResize) && !window->Collapsed)
        {
            if (!window_size_x_set_by_api)
            {
                window->SizeFull.x = size_auto_fit.x;
                use_current_size_for_scrollbar_x = true;
            }
            if (!window_size_y_set_by_api)
            {
                window->SizeFull.y = size_auto_fit.y;
                use_current_size_for_scrollbar_y = true;
            }
        }
        else if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
        {
            if (!window_size_x_set_by_api && window->AutoFitFramesX > 0)
            {
                window->SizeFull.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
                use_current_size_for_scrollbar_x = true;
            }
            if (!window_size_y_set_by_api && window->AutoFitFramesY > 0)
            {
                window->SizeFull.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
                use_current_size_for_scrollbar_y = true;
            }
            if (!window->Collapsed)
                MarkIniSettingsDirty(window);
        }

        window->SizeFull = CalcWindowSizeAfterConstraint(window, window->SizeFull);
        window->Size = window->Collapsed && !(flags & ImGuiWindowFlags_ChildWindow) ? window->TitleBarRect().GetSize() : window->SizeFull;

        const float decoration_up_height = window->TitleBarHeight() + window->MenuBarHeight();

        if (window_just_activated_by_user)
        {
            window->AutoPosLastDirection = ImGuiDir_None;
            if ((flags & ImGuiWindowFlags_Popup) != 0 && !(flags & ImGuiWindowFlags_Modal) && !window_pos_set_by_api)      
                window->Pos = g.BeginPopupStack.back().OpenPopupPos;
        }

        if (flags & ImGuiWindowFlags_ChildWindow)
        {
            IM_ASSERT(parent_window && parent_window->Active);
            window->BeginOrderWithinParent = (short)parent_window->DC.ChildWindows.Size;
            parent_window->DC.ChildWindows.push_back(window);
            if (!(flags & ImGuiWindowFlags_Popup) && !window_pos_set_by_api && !window_is_child_tooltip)
                window->Pos = parent_window->DC.CursorPos;
        }

        const bool window_pos_with_pivot = (window->SetWindowPosVal.x != FLT_MAX && window->HiddenFramesCannotSkipItems == 0);
        if (window_pos_with_pivot)
            SetWindowPos(window, window->SetWindowPosVal - window->Size * window->SetWindowPosPivot, 0);        
        else if ((flags & ImGuiWindowFlags_ChildMenu) != 0)
            window->Pos = FindBestWindowPosForPopup(window);
        else if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api && window_just_appearing_after_hidden_for_resize)
            window->Pos = FindBestWindowPosForPopup(window);
        else if ((flags & ImGuiWindowFlags_Tooltip) != 0 && !window_pos_set_by_api && !window_is_child_tooltip)
            window->Pos = FindBestWindowPosForPopup(window);

        if (window->ViewportAllowPlatformMonitorExtend >= 0 && !window->ViewportOwned && !(window->Viewport->Flags & ImGuiViewportFlags_Minimized))
            if (!window->Viewport->GetMainRect().Contains(window->Rect()))
            {
                window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_NoFocusOnAppearing);
                window->Viewport->Parent_Hwnd = parent_hwnd;  //##EDIT##//
                SetCurrentViewport(window, window->Viewport);
                window->FontDpiScale = (g.IO.ConfigFlags & ImGuiConfigFlags_DpiEnableScaleFonts) ? window->Viewport->DpiScale : 1.0f;
                SetCurrentWindow(window);
            }

        bool viewport_rect_changed = false;
        if (window->ViewportOwned)
        {
            if (window->Viewport->PlatformRequestMove)
            {
                window->Pos = window->Viewport->Pos;
                MarkIniSettingsDirty(window);
            }
            else if (memcmp(&window->Viewport->Pos, &window->Pos, sizeof(window->Pos)) != 0)
            {
                viewport_rect_changed = true;
                window->Viewport->Pos = window->Pos;
            }

            if (window->Viewport->PlatformRequestResize)
            {
                window->Size = window->SizeFull = window->Viewport->Size;
                MarkIniSettingsDirty(window);
            }
            else if (memcmp(&window->Viewport->Size, &window->Size, sizeof(window->Size)) != 0)
            {
                viewport_rect_changed = true;
                window->Viewport->Size = window->Size;
            }

            if (viewport_rect_changed)
                UpdateViewportPlatformMonitor(window->Viewport);

            const ImGuiViewportFlags viewport_flags_to_clear = ImGuiViewportFlags_TopMost | ImGuiViewportFlags_NoTaskBarIcon | ImGuiViewportFlags_NoDecoration | ImGuiViewportFlags_NoRendererClear;
            ImGuiViewportFlags viewport_flags = window->Viewport->Flags & ~viewport_flags_to_clear;
            const bool is_modal = (flags & ImGuiWindowFlags_Modal) != 0;
            const bool is_short_lived_floating_window = (flags & (ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup)) != 0;
            if (flags & ImGuiWindowFlags_Tooltip)
                viewport_flags |= ImGuiViewportFlags_TopMost;
            if ((g.IO.ConfigViewportsNoTaskBarIcon || is_short_lived_floating_window) && !is_modal)
                viewport_flags |= ImGuiViewportFlags_NoTaskBarIcon;
            if (g.IO.ConfigViewportsNoDecoration || is_short_lived_floating_window)
                viewport_flags |= ImGuiViewportFlags_NoDecoration;

            if (is_short_lived_floating_window && !is_modal)
                viewport_flags |= ImGuiViewportFlags_NoFocusOnAppearing | ImGuiViewportFlags_NoFocusOnClick;

            if (window->WindowClass.ParentViewportId)
                window->Viewport->ParentViewportId = window->WindowClass.ParentViewportId;
            else if ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && parent_window_in_stack)
                window->Viewport->ParentViewportId = parent_window_in_stack->Viewport->ID;
            else
                window->Viewport->ParentViewportId = g.IO.ConfigViewportsNoDefaultParent ? 0 : IMGUI_VIEWPORT_DEFAULT_ID;
            if (window->WindowClass.ViewportFlagsOverrideSet)
                viewport_flags |= window->WindowClass.ViewportFlagsOverrideSet;
            if (window->WindowClass.ViewportFlagsOverrideClear)
                viewport_flags &= ~window->WindowClass.ViewportFlagsOverrideClear;

            if (!(flags & ImGuiWindowFlags_NoBackground))
                viewport_flags &= ~ImGuiViewportFlags_NoRendererClear;

            window->Viewport->Flags = viewport_flags;
        }

        ImRect viewport_rect(window->Viewport->GetMainRect());
        ImRect viewport_work_rect(window->Viewport->GetWorkRect());
        ImVec2 visibility_padding = ImMax(style.DisplayWindowPadding, style.DisplaySafeAreaPadding);
        ImRect visibility_rect(viewport_work_rect.Min + visibility_padding, viewport_work_rect.Max - visibility_padding);

        if (!window_pos_set_by_api && !(flags & ImGuiWindowFlags_ChildWindow) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
        {
            if (!window->ViewportOwned && viewport_rect.GetWidth() > 0 && viewport_rect.GetHeight() > 0.0f)
            {
                ClampWindowRect(window, visibility_rect);
            }
            else if (window->ViewportOwned && g.PlatformIO.Monitors.Size > 0)
            {
                if (window->Viewport->PlatformMonitor == -1)
                {
                    SetWindowPos(window, g.Viewports[0]->Pos + style.DisplayWindowPadding, ImGuiCond_Always);
                }
                else
                {
                    ImGuiPlatformMonitor& monitor = g.PlatformIO.Monitors[window->Viewport->PlatformMonitor];
                    visibility_rect.Min = monitor.WorkPos + visibility_padding;
                    visibility_rect.Max = monitor.WorkPos + monitor.WorkSize - visibility_padding;
                    ClampWindowRect(window, visibility_rect);
                }
            }
        }
        window->Pos = ImFloor(window->Pos);

        if (window->ViewportOwned || window->DockIsActive)
            window->WindowRounding = 0.0f;
        else
            window->WindowRounding = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildRounding : ((flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupRounding : style.WindowRounding;

        bool want_focus = false;
        if (window_just_activated_by_user && !(flags & ImGuiWindowFlags_NoFocusOnAppearing))
        {
            if (flags & ImGuiWindowFlags_Popup)
                want_focus = true;
            else if ((window->DockIsActive || (flags & ImGuiWindowFlags_ChildWindow) == 0) && !(flags & ImGuiWindowFlags_Tooltip))
                want_focus = true;
        }

        const bool handle_borders_and_resize_grips = (window->DockNodeAsHost || !window->DockIsActive);

        int border_held = -1;
        ImU32 resize_grip_col[4] = {};
        const int resize_grip_count = g.IO.ConfigWindowsResizeFromEdges ? 2 : 1;              
        const float resize_grip_draw_size = IM_FLOOR(ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f));
        if (handle_borders_and_resize_grips && !window->Collapsed)
            if (UpdateWindowManualResize(window, size_auto_fit, &border_held, resize_grip_count, &resize_grip_col[0], visibility_rect))
                use_current_size_for_scrollbar_x = use_current_size_for_scrollbar_y = true;
        window->ResizeBorderHeld = (signed char)border_held;

        if (window->ViewportOwned)
        {
            if (!window->Viewport->PlatformRequestMove)
                window->Viewport->Pos = window->Pos;
            if (!window->Viewport->PlatformRequestResize)
                window->Viewport->Size = window->Size;
            viewport_rect = window->Viewport->GetMainRect();
        }

        window->ViewportPos = window->Viewport->Pos;

        if (!window->Collapsed)
        {
            ImVec2 avail_size_from_current_frame = ImVec2(window->SizeFull.x, window->SizeFull.y - decoration_up_height);
            ImVec2 avail_size_from_last_frame = window->InnerRect.GetSize() + window->ScrollbarSizes;
            ImVec2 needed_size_from_last_frame = window_just_created ? ImVec2(0, 0) : window->ContentSize + window->WindowPadding * 2.0f;
            float size_x_for_scrollbars = use_current_size_for_scrollbar_x ? avail_size_from_current_frame.x : avail_size_from_last_frame.x;
            float size_y_for_scrollbars = use_current_size_for_scrollbar_y ? avail_size_from_current_frame.y : avail_size_from_last_frame.y;
            window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((needed_size_from_last_frame.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar));
            window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((needed_size_from_last_frame.x > size_x_for_scrollbars - (window->ScrollbarY ? style.ScrollbarSize : 0.0f)) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
            if (window->ScrollbarX && !window->ScrollbarY)
                window->ScrollbarY = (needed_size_from_last_frame.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar);
            window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);
        }

        const ImRect host_rect = ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip) ? parent_window->ClipRect : viewport_rect;
        const ImRect outer_rect = window->Rect();
        const ImRect title_bar_rect = window->TitleBarRect();
        window->OuterRectClipped = outer_rect;
        if (window->DockIsActive)
            window->OuterRectClipped.Min.y += window->TitleBarHeight();
        window->OuterRectClipped.ClipWith(host_rect);

        window->InnerRect.Min.x = window->Pos.x;
        window->InnerRect.Min.y = window->Pos.y + decoration_up_height;
        window->InnerRect.Max.x = window->Pos.x + window->Size.x - window->ScrollbarSizes.x;
        window->InnerRect.Max.y = window->Pos.y + window->Size.y - window->ScrollbarSizes.y;

        float top_border_size = (((flags & ImGuiWindowFlags_MenuBar) || !(flags & ImGuiWindowFlags_NoTitleBar)) ? style.FrameBorderSize : window->WindowBorderSize);
        window->InnerClipRect.Min.x = ImFloor(0.5f + window->InnerRect.Min.x + ImMax(ImFloor(window->WindowPadding.x * 0.5f), window->WindowBorderSize));
        window->InnerClipRect.Min.y = ImFloor(0.5f + window->InnerRect.Min.y + top_border_size);
        window->InnerClipRect.Max.x = ImFloor(0.5f + window->InnerRect.Max.x - ImMax(ImFloor(window->WindowPadding.x * 0.5f), window->WindowBorderSize));
        window->InnerClipRect.Max.y = ImFloor(0.5f + window->InnerRect.Max.y - window->WindowBorderSize);
        window->InnerClipRect.ClipWithFull(host_rect);

        if (window->Size.x > 0.0f && !(flags & ImGuiWindowFlags_Tooltip) && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
            window->ItemWidthDefault = ImFloor(window->Size.x * 0.65f);
        else
            window->ItemWidthDefault = ImFloor(g.FontSize * 16.0f);

        window->ScrollMax.x = ImMax(0.0f, window->ContentSize.x + window->WindowPadding.x * 2.0f - window->InnerRect.GetWidth());
        window->ScrollMax.y = ImMax(0.0f, window->ContentSize.y + window->WindowPadding.y * 2.0f - window->InnerRect.GetHeight());

        window->Scroll = CalcNextScrollFromScrollTargetAndClamp(window);
        window->ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);

        IM_ASSERT(window->DrawList->CmdBuffer.Size == 1 && window->DrawList->CmdBuffer[0].ElemCount == 0);
        window->DrawList->PushTextureID(g.Font->ContainerAtlas->TexID);
        PushClipRect(host_rect.Min, host_rect.Max, false);

        ImGuiWindow* window_window_list = g.NavWindowingListWindow;
        const bool dim_bg_for_modal = (flags & ImGuiWindowFlags_Modal) && window == GetTopMostPopupModal() && window->HiddenFramesCannotSkipItems <= 0;
        const bool dim_bg_for_window_list = g.NavWindowingTargetAnim && ((window == g.NavWindowingTargetAnim->RootWindow) || (window == window_window_list && window_window_list->Viewport != g.NavWindowingTargetAnim->Viewport));
        if (dim_bg_for_modal || dim_bg_for_window_list)
        {
            const ImU32 dim_bg_col = GetColorU32(dim_bg_for_modal ? ImGuiCol_ModalWindowDimBg : ImGuiCol_NavWindowingDimBg, g.DimBgRatio);
            window->DrawList->AddRectFilled(viewport_rect.Min, viewport_rect.Max, dim_bg_col);
        }

        if (dim_bg_for_window_list && window == g.NavWindowingTargetAnim)
        {
            ImRect bb = window->Rect();
            bb.Expand(g.FontSize);
            if (!bb.Contains(viewport_rect))           
                window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha * 0.25f), g.Style.WindowRounding);
        }

        const bool is_undocked_or_docked_visible = !window->DockIsActive || window->DockTabIsVisible;
        if (is_undocked_or_docked_visible)
        {
            bool render_decorations_in_parent = false;
            if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip)
                if (window->DrawList->CmdBuffer.back().ElemCount == 0 && parent_window->DrawList->VtxBuffer.Size > 0)
                    render_decorations_in_parent = true;
            if (render_decorations_in_parent)
                window->DrawList = parent_window->DrawList;

            const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
            const bool title_bar_is_highlight = want_focus || (window_to_highlight && (window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight || (window->DockNode && window->DockNode == window_to_highlight->DockNode)));
            RenderWindowDecorations(window, title_bar_rect, title_bar_is_highlight, handle_borders_and_resize_grips, resize_grip_count, resize_grip_col, resize_grip_draw_size);

            if (render_decorations_in_parent)
                window->DrawList = &window->DrawListInst;
        }

        if (g.NavWindowingTargetAnim == window)
        {
            float rounding = ImMax(window->WindowRounding, g.Style.WindowRounding);
            ImRect bb = window->Rect();
            bb.Expand(g.FontSize);
            if (bb.Contains(viewport_rect))            
            {
                bb.Expand(-g.FontSize - 1.0f);
                rounding = window->WindowRounding;
            }
            window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha), rounding, ~0, 3.0f);
        }

        const bool allow_scrollbar_x = !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar);
        const bool allow_scrollbar_y = !(flags & ImGuiWindowFlags_NoScrollbar);
        const float work_rect_size_x = (window->ContentSizeExplicit.x != 0.0f ? window->ContentSizeExplicit.x : ImMax(allow_scrollbar_x ? window->ContentSize.x : 0.0f, window->Size.x - window->WindowPadding.x * 2.0f - window->ScrollbarSizes.x));
        const float work_rect_size_y = (window->ContentSizeExplicit.y != 0.0f ? window->ContentSizeExplicit.y : ImMax(allow_scrollbar_y ? window->ContentSize.y : 0.0f, window->Size.y - window->WindowPadding.y * 2.0f - decoration_up_height - window->ScrollbarSizes.y));
        window->WorkRect.Min.x = ImFloor(window->InnerRect.Min.x - window->Scroll.x + ImMax(window->WindowPadding.x, window->WindowBorderSize));
        window->WorkRect.Min.y = ImFloor(window->InnerRect.Min.y - window->Scroll.y + ImMax(window->WindowPadding.y, window->WindowBorderSize));
        window->WorkRect.Max.x = window->WorkRect.Min.x + work_rect_size_x;
        window->WorkRect.Max.y = window->WorkRect.Min.y + work_rect_size_y;
        window->ParentWorkRect = window->WorkRect;

        window->ContentRegionRect.Min.x = window->Pos.x - window->Scroll.x + window->WindowPadding.x;
        window->ContentRegionRect.Min.y = window->Pos.y - window->Scroll.y + window->WindowPadding.y + decoration_up_height;
        window->ContentRegionRect.Max.x = window->ContentRegionRect.Min.x + (window->ContentSizeExplicit.x != 0.0f ? window->ContentSizeExplicit.x : (window->Size.x - window->WindowPadding.x * 2.0f - window->ScrollbarSizes.x));
        window->ContentRegionRect.Max.y = window->ContentRegionRect.Min.y + (window->ContentSizeExplicit.y != 0.0f ? window->ContentSizeExplicit.y : (window->Size.y - window->WindowPadding.y * 2.0f - decoration_up_height - window->ScrollbarSizes.y));

        window->DC.Indent.x = 0.0f + window->WindowPadding.x - window->Scroll.x;
        window->DC.GroupOffset.x = 0.0f;
        window->DC.ColumnsOffset.x = 0.0f;
        window->DC.CursorStartPos = window->Pos + ImVec2(window->DC.Indent.x + window->DC.ColumnsOffset.x, decoration_up_height + window->WindowPadding.y - window->Scroll.y);
        window->DC.CursorPos = window->DC.CursorStartPos;
        window->DC.CursorPosPrevLine = window->DC.CursorPos;
        window->DC.CursorMaxPos = window->DC.CursorStartPos;
        window->DC.CurrLineSize = window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
        window->DC.CurrLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;

        window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
        window->DC.NavLayerActiveMask = window->DC.NavLayerActiveMaskNext;
        window->DC.NavLayerActiveMaskNext = 0x00;
        window->DC.NavHideHighlightOneFrame = false;
        window->DC.NavHasScroll = (window->ScrollMax.y > 0.0f);

        window->DC.MenuBarAppending = false;
        window->DC.MenuColumns.Update(3, style.ItemSpacing.x, window_just_activated_by_user);
        window->DC.TreeDepth = 0;
        window->DC.TreeJumpToParentOnPopMask = 0x00;
        window->DC.ChildWindows.resize(0);
        window->DC.StateStorage = &window->StateStorage;
        window->DC.CurrentColumns = NULL;
        window->DC.LayoutType = ImGuiLayoutType_Vertical;
        window->DC.ParentLayoutType = parent_window ? parent_window->DC.LayoutType : ImGuiLayoutType_Vertical;
        window->DC.FocusCounterRegular = window->DC.FocusCounterTabStop = -1;

        window->DC.ItemWidth = window->ItemWidthDefault;
        window->DC.TextWrapPos = -1.0f;  
        window->DC.ItemWidthStack.resize(0);
        window->DC.TextWrapPosStack.resize(0);

        if (window->AutoFitFramesX > 0)
            window->AutoFitFramesX--;
        if (window->AutoFitFramesY > 0)
            window->AutoFitFramesY--;

        if (want_focus)
        {
            FocusWindow(window);
            NavInitWindow(window, false);
        }

        if (p_open != NULL && window->Viewport->PlatformRequestClose && window->Viewport != GetMainViewport())
        {
            if (!window->DockIsActive || window->DockTabIsVisible)
            {
                window->Viewport->PlatformRequestClose = false;
                g.NavWindowingToggleLayer = false;                   
                IMGUI_DEBUG_LOG_VIEWPORT("Window '%s' PlatformRequestClose\n", window->Name);
                *p_open = false;
            }
        }

        if (!(flags & ImGuiWindowFlags_NoTitleBar) && !window->DockIsActive)
            RenderWindowTitleBarContents(window, title_bar_rect, name, p_open);

        window->HitTestHoleSize.x = window->HitTestHoleSize.y = 0;

        if (g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            if ((g.MovingWindow == window) && (g.IO.ConfigDockingWithShift == g.IO.KeyShift))
                if ((window->RootWindow->Flags & ImGuiWindowFlags_NoDocking) == 0)
                    BeginDockableDragDropSource(window);

            if (g.DragDropActive && !(flags & ImGuiWindowFlags_NoDocking))
                if (g.MovingWindow == NULL || g.MovingWindow->RootWindow != window)
                    if ((window == window->RootWindow) && !(window->Flags & ImGuiWindowFlags_DockNodeHost))
                        BeginDockableDragDropTarget(window);
        }

        if (window->DockIsActive)
            SetLastItemData(window, window->ID, window->DockTabItemStatusFlags, window->DockTabItemRect);
        else
            SetLastItemData(window, window->MoveId, IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max, false) ? ImGuiItemStatusFlags_HoveredRect : 0, title_bar_rect);

#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (!(window->Flags & ImGuiWindowFlags_NoTitleBar))
            IMGUI_TEST_ENGINE_ITEM_ADD(window->DC.LastItemRect, window->DC.LastItemId);
#endif
    }
    else
    {
        SetCurrentViewport(window, window->Viewport);
        SetCurrentWindow(window);
    }

    window->DC.ItemFlags = g.ItemFlagsStack.back();     
    window->DC.NavFocusScopeIdCurrent = (flags & ImGuiWindowFlags_ChildWindow) ? parent_window->DC.NavFocusScopeIdCurrent : 0;       

    if (!(flags & ImGuiWindowFlags_DockNodeHost))
        PushClipRect(window->InnerClipRect.Min, window->InnerClipRect.Max, true);

    window->WriteAccessed = false;
    window->BeginCount++;
    g.NextWindowData.ClearFlags();

    if (first_begin_of_the_frame)
    {
        if (window->DockIsActive && !window->DockTabIsVisible)
        {
            if (window->LastFrameJustFocused == g.FrameCount)
                window->HiddenFramesCannotSkipItems = 1;
            else
                window->HiddenFramesCanSkipItems = 1;
        }

        if (flags & ImGuiWindowFlags_ChildWindow)
        {
            IM_ASSERT((flags& ImGuiWindowFlags_NoTitleBar) != 0 || (window->DockIsActive));
            if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)       
                if (!g.LogEnabled)
                    if (window->OuterRectClipped.Min.x >= window->OuterRectClipped.Max.x || window->OuterRectClipped.Min.y >= window->OuterRectClipped.Max.y)
                        window->HiddenFramesCanSkipItems = 1;

            if (parent_window && (parent_window->Collapsed || parent_window->HiddenFramesCanSkipItems > 0))
                window->HiddenFramesCanSkipItems = 1;
            if (parent_window && (parent_window->Collapsed || parent_window->HiddenFramesCannotSkipItems > 0))
                window->HiddenFramesCannotSkipItems = 1;
        }

        if (style.Alpha <= 0.0f)
            window->HiddenFramesCanSkipItems = 1;

        window->Hidden = (window->HiddenFramesCanSkipItems > 0) || (window->HiddenFramesCannotSkipItems > 0);

        bool skip_items = false;
        if (window->Collapsed || !window->Active || window->Hidden)
            if (window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && window->HiddenFramesCannotSkipItems <= 0)
                skip_items = true;
        window->SkipItems = skip_items;
    }

    return !window->SkipItems;
}

void ImGui::End()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (g.CurrentWindowStack.Size <= 1 && g.WithinFrameScopeWithImplicitWindow)
    {
        IM_ASSERT_USER_ERROR(g.CurrentWindowStack.Size > 1, "Calling End() too many times!");
        return;
    }
    IM_ASSERT(g.CurrentWindowStack.Size > 0);

    if ((window->Flags & ImGuiWindowFlags_ChildWindow) && !(window->Flags & ImGuiWindowFlags_DockNodeHost) && !window->DockIsActive)
        IM_ASSERT_USER_ERROR(g.WithinEndChild, "Must call EndChild() and not End()!");

    if (window->DC.CurrentColumns)
        EndColumns();
    if (!(window->Flags & ImGuiWindowFlags_DockNodeHost))        
        PopClipRect();

    if (!(window->Flags & ImGuiWindowFlags_ChildWindow))            
        LogFinish();

    if (window->DockNode && window->DockTabIsVisible)
        if (ImGuiWindow* host_window = window->DockNode->HostWindow)          
            host_window->DC.CursorMaxPos = window->DC.CursorMaxPos + window->WindowPadding - host_window->WindowPadding;

    g.CurrentWindowStack.pop_back();
    if (window->Flags & ImGuiWindowFlags_Popup)
        g.BeginPopupStack.pop_back();
    window->DC.StackSizesOnBegin.CompareWithCurrentState();
    SetCurrentWindow(g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back());
    if (g.CurrentWindow)
        SetCurrentViewport(g.CurrentWindow, g.CurrentWindow->Viewport);
}

void ImGui::BringWindowToFocusFront(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.WindowsFocusOrder.back() == window)
        return;
    for (int i = g.WindowsFocusOrder.Size - 2; i >= 0; i--)       
        if (g.WindowsFocusOrder[i] == window)
        {
            memmove(&g.WindowsFocusOrder[i], &g.WindowsFocusOrder[i + 1], (size_t)(g.WindowsFocusOrder.Size - i - 1) * sizeof(ImGuiWindow*));
            g.WindowsFocusOrder[g.WindowsFocusOrder.Size - 1] = window;
            break;
        }
}

void ImGui::BringWindowToDisplayFront(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* current_front_window = g.Windows.back();
    if (current_front_window == window || current_front_window->RootWindow == window)       
        return;
    for (int i = g.Windows.Size - 2; i >= 0; i--)       
        if (g.Windows[i] == window)
        {
            memmove(&g.Windows[i], &g.Windows[i + 1], (size_t)(g.Windows.Size - i - 1) * sizeof(ImGuiWindow*));
            g.Windows[g.Windows.Size - 1] = window;
            break;
        }
}

void ImGui::BringWindowToDisplayBack(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.Windows[0] == window)
        return;
    for (int i = 0; i < g.Windows.Size; i++)
        if (g.Windows[i] == window)
        {
            memmove(&g.Windows[1], &g.Windows[0], (size_t)i * sizeof(ImGuiWindow*));
            g.Windows[0] = window;
            break;
        }
}

void ImGui::FocusWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;

    if (g.NavWindow != window)
    {
        g.NavWindow = window;
        if (window && g.NavDisableMouseHover)
            g.NavMousePosDirty = true;
        g.NavInitRequest = false;
        g.NavId = window ? window->NavLastIds[0] : 0;   
        g.NavFocusScopeId = 0;
        g.NavIdIsAlive = false;
        g.NavLayer = ImGuiNavLayer_Main;
    }

    ClosePopupsOverWindow(window, false);

    IM_ASSERT(window == NULL || window->RootWindow != NULL);
    ImGuiWindow* focus_front_window = window ? window->RootWindowDockStop : NULL;
    ImGuiWindow* display_front_window = window ? window->RootWindow : NULL;
    ImGuiDockNode* dock_node = window ? window->DockNode : NULL;
    bool active_id_window_is_dock_node_host = (g.ActiveIdWindow && dock_node && dock_node->HostWindow == g.ActiveIdWindow);

    if (g.ActiveId != 0 && g.ActiveIdWindow && g.ActiveIdWindow->RootWindowDockStop != focus_front_window)
        if (!g.ActiveIdNoClearOnFocusLoss && !active_id_window_is_dock_node_host)
            ClearActiveID();

    if (!window)
        return;
    window->LastFrameJustFocused = g.FrameCount;

    if (dock_node && dock_node->TabBar)
        dock_node->TabBar->SelectedTabId = dock_node->TabBar->NextSelectedTabId = window->ID;

    BringWindowToFocusFront(focus_front_window);
    if (((window->Flags | focus_front_window->Flags | display_front_window->Flags) & ImGuiWindowFlags_NoBringToFrontOnFocus) == 0)
        BringWindowToDisplayFront(display_front_window);
}

void ImGui::FocusTopMostWindowUnderOne(ImGuiWindow* under_this_window, ImGuiWindow* ignore_window)
{
    ImGuiContext& g = *GImGui;

    int start_idx = g.WindowsFocusOrder.Size - 1;
    if (under_this_window != NULL)
    {
        int under_this_window_idx = FindWindowFocusIndex(under_this_window);
        if (under_this_window_idx != -1)
            start_idx = under_this_window_idx - 1;
    }
    for (int i = start_idx; i >= 0; i--)
    {
        ImGuiWindow* window = g.WindowsFocusOrder[i];
        if (window != ignore_window && window->WasActive && window->RootWindowDockStop == window)
            if ((window->Flags & (ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs)) != (ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs))
            {
                ImGuiWindow* focus_window = NavRestoreLastChildNavWindow(window);
                FocusWindow(focus_window);
                return;
            }
    }
    FocusWindow(NULL);
}

void ImGui::SetCurrentFont(ImFont* font)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(font && font->IsLoaded());               
    IM_ASSERT(font->Scale > 0.0f);
    g.Font = font;
    g.FontBaseSize = ImMax(1.0f, g.IO.FontGlobalScale * g.Font->FontSize * g.Font->Scale);
    g.FontSize = g.CurrentWindow ? g.CurrentWindow->CalcFontSize() : 0.0f;

    ImFontAtlas* atlas = g.Font->ContainerAtlas;
    g.DrawListSharedData.TexUvWhitePixel = atlas->TexUvWhitePixel;
    g.DrawListSharedData.TexUvLines = atlas->TexUvLines;
    g.DrawListSharedData.Font = g.Font;
    g.DrawListSharedData.FontSize = g.FontSize;
}

void ImGui::PushFont(ImFont* font)
{
    ImGuiContext& g = *GImGui;
    if (!font)
        font = GetDefaultFont();
    SetCurrentFont(font);
    g.FontStack.push_back(font);
    g.CurrentWindow->DrawList->PushTextureID(font->ContainerAtlas->TexID);
}

void  ImGui::PopFont()
{
    ImGuiContext& g = *GImGui;
    g.CurrentWindow->DrawList->PopTextureID();
    g.FontStack.pop_back();
    SetCurrentFont(g.FontStack.empty() ? GetDefaultFont() : g.FontStack.back());
}

void ImGui::PushItemFlag(ImGuiItemFlags option, bool enabled)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiItemFlags item_flags = window->DC.ItemFlags;
    IM_ASSERT(item_flags == g.ItemFlagsStack.back());
    if (enabled)
        item_flags |= option;
    else
        item_flags &= ~option;
    window->DC.ItemFlags = item_flags;
    g.ItemFlagsStack.push_back(item_flags);
}

void ImGui::PopItemFlag()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(g.ItemFlagsStack.Size > 1);                  
    g.ItemFlagsStack.pop_back();
    window->DC.ItemFlags = g.ItemFlagsStack.back();
}

void ImGui::PushAllowKeyboardFocus(bool allow_keyboard_focus)
{
    PushItemFlag(ImGuiItemFlags_NoTabStop, !allow_keyboard_focus);
}

void ImGui::PopAllowKeyboardFocus()
{
    PopItemFlag();
}

void ImGui::PushButtonRepeat(bool repeat)
{
    PushItemFlag(ImGuiItemFlags_ButtonRepeat, repeat);
}

void ImGui::PopButtonRepeat()
{
    PopItemFlag();
}

void ImGui::PushTextWrapPos(float wrap_pos_x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.TextWrapPos = wrap_pos_x;
    window->DC.TextWrapPosStack.push_back(wrap_pos_x);
}

void ImGui::PopTextWrapPos()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.TextWrapPosStack.pop_back();
    window->DC.TextWrapPos = window->DC.TextWrapPosStack.empty() ? -1.0f : window->DC.TextWrapPosStack.back();
}

bool ImGui::IsWindowChildOf(ImGuiWindow* window, ImGuiWindow* potential_parent)
{
    if (window->RootWindow == potential_parent)
        return true;
    while (window != NULL)
    {
        if (window == potential_parent)
            return true;
        window = window->ParentWindow;
    }
    return false;
}

bool ImGui::IsWindowAbove(ImGuiWindow* potential_above, ImGuiWindow* potential_below)
{
    ImGuiContext& g = *GImGui;
    for (int i = g.Windows.Size - 1; i >= 0; i--)
    {
        ImGuiWindow* candidate_window = g.Windows[i];
        if (candidate_window == potential_above)
            return true;
        if (candidate_window == potential_below)
            return false;
    }
    return false;
}

bool ImGui::IsWindowHovered(ImGuiHoveredFlags flags)
{
    IM_ASSERT((flags & ImGuiHoveredFlags_AllowWhenOverlapped) == 0);         
    ImGuiContext& g = *GImGui;

    if (flags & ImGuiHoveredFlags_AnyWindow)
    {
        if (g.HoveredWindow == NULL)
            return false;
    }
    else
    {
        switch (flags & (ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows))
        {
        case ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows:
            if (g.HoveredWindow == NULL || g.HoveredWindow->RootWindowDockStop != g.CurrentWindow->RootWindowDockStop)
                return false;
            break;
        case ImGuiHoveredFlags_RootWindow:
            if (g.HoveredWindow != g.CurrentWindow->RootWindowDockStop)
                return false;
            break;
        case ImGuiHoveredFlags_ChildWindows:
            if (g.HoveredWindow == NULL || !IsWindowChildOf(g.HoveredWindow, g.CurrentWindow))
                return false;
            break;
        default:
            if (g.HoveredWindow != g.CurrentWindow)
                return false;
            break;
        }
    }

    if (!IsWindowContentHoverable(g.HoveredWindow, flags))
        return false;
    if (!(flags & ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        if (g.ActiveId != 0 && !g.ActiveIdAllowOverlap && g.ActiveId != g.HoveredWindow->MoveId)
            return false;
    return true;
}

bool ImGui::IsWindowFocused(ImGuiFocusedFlags flags)
{
    ImGuiContext& g = *GImGui;

    if (flags & ImGuiFocusedFlags_AnyWindow)
        return g.NavWindow != NULL;

    IM_ASSERT(g.CurrentWindow);         
    switch (flags & (ImGuiFocusedFlags_RootWindow | ImGuiFocusedFlags_ChildWindows))
    {
    case ImGuiFocusedFlags_RootWindow | ImGuiFocusedFlags_ChildWindows:
        return g.NavWindow && g.NavWindow->RootWindowDockStop == g.CurrentWindow->RootWindowDockStop;
    case ImGuiFocusedFlags_RootWindow:
        return g.NavWindow == g.CurrentWindow->RootWindowDockStop;
    case ImGuiFocusedFlags_ChildWindows:
        return g.NavWindow && IsWindowChildOf(g.NavWindow, g.CurrentWindow);
    default:
        return g.NavWindow == g.CurrentWindow;
    }
}

ImGuiID ImGui::GetWindowDockID()
{
    ImGuiContext& g = *GImGui;
    return g.CurrentWindow->DockId;
}

bool ImGui::IsWindowDocked()
{
    ImGuiContext& g = *GImGui;
    return g.CurrentWindow->DockIsActive;
}

bool ImGui::IsWindowNavFocusable(ImGuiWindow* window)
{
    return window->WasActive && window == window->RootWindowDockStop && !(window->Flags & ImGuiWindowFlags_NoNavFocus);
}

float ImGui::GetWindowWidth()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Size.x;
}

float ImGui::GetWindowHeight()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Size.y;
}

ImVec2 ImGui::GetWindowPos()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    return window->Pos;
}

void ImGui::SetWindowPos(ImGuiWindow* window, const ImVec2& pos, ImGuiCond cond)
{
    if (cond && (window->SetWindowPosAllowFlags & cond) == 0)
        return;

    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond));            
    window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
    window->SetWindowPosVal = ImVec2(FLT_MAX, FLT_MAX);

    const ImVec2 old_pos = window->Pos;
    window->Pos = ImFloor(pos);
    ImVec2 offset = window->Pos - old_pos;
    window->DC.CursorPos += offset;                                    
    window->DC.CursorMaxPos += offset;                     
    window->DC.CursorStartPos += offset;
}

void ImGui::SetWindowPos(const ImVec2& pos, ImGuiCond cond)
{
    ImGuiWindow* window = GetCurrentWindowRead();
    SetWindowPos(window, pos, cond);
}

void ImGui::SetWindowPos(const char* name, const ImVec2& pos, ImGuiCond cond)
{
    if (ImGuiWindow* window = FindWindowByName(name))
        SetWindowPos(window, pos, cond);
}

ImVec2 ImGui::GetWindowSize()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->Size;
}

void ImGui::SetWindowSize(ImGuiWindow* window, const ImVec2& size, ImGuiCond cond)
{
    if (cond && (window->SetWindowSizeAllowFlags & cond) == 0)
        return;

    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond));            
    window->SetWindowSizeAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);

    if (size.x > 0.0f)
    {
        window->AutoFitFramesX = 0;
        window->SizeFull.x = IM_FLOOR(size.x);
    }
    else
    {
        window->AutoFitFramesX = 2;
        window->AutoFitOnlyGrows = false;
    }
    if (size.y > 0.0f)
    {
        window->AutoFitFramesY = 0;
        window->SizeFull.y = IM_FLOOR(size.y);
    }
    else
    {
        window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = false;
    }
}

void ImGui::SetWindowSize(const ImVec2& size, ImGuiCond cond)
{
    SetWindowSize(GImGui->CurrentWindow, size, cond);
}

void ImGui::SetWindowSize(const char* name, const ImVec2& size, ImGuiCond cond)
{
    if (ImGuiWindow* window = FindWindowByName(name))
        SetWindowSize(window, size, cond);
}

void ImGui::SetWindowCollapsed(ImGuiWindow* window, bool collapsed, ImGuiCond cond)
{
    if (cond && (window->SetWindowCollapsedAllowFlags & cond) == 0)
        return;
    window->SetWindowCollapsedAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);

    window->Collapsed = collapsed;
}

void ImGui::SetWindowHitTestHole(ImGuiWindow* window, const ImVec2& pos, const ImVec2& size)
{
    IM_ASSERT(window->HitTestHoleSize.x == 0);            
    window->HitTestHoleSize = ImVec2ih(size);
    window->HitTestHoleOffset = ImVec2ih(pos - window->Pos);
}

void ImGui::SetWindowCollapsed(bool collapsed, ImGuiCond cond)
{
    SetWindowCollapsed(GImGui->CurrentWindow, collapsed, cond);
}

bool ImGui::IsWindowCollapsed()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->Collapsed;
}

bool ImGui::IsWindowAppearing()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->Appearing;
}

void ImGui::SetWindowCollapsed(const char* name, bool collapsed, ImGuiCond cond)
{
    if (ImGuiWindow* window = FindWindowByName(name))
        SetWindowCollapsed(window, collapsed, cond);
}

void ImGui::SetWindowFocus()
{
    FocusWindow(GImGui->CurrentWindow);
}

void ImGui::SetWindowFocus(const char* name)
{
    if (name)
    {
        if (ImGuiWindow* window = FindWindowByName(name))
            FocusWindow(window);
    }
    else
    {
        FocusWindow(NULL);
    }
}

void ImGui::SetNextWindowPos(const ImVec2& pos, ImGuiCond cond, const ImVec2& pivot)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond));            
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasPos;
    g.NextWindowData.PosVal = pos;
    g.NextWindowData.PosPivotVal = pivot;
    g.NextWindowData.PosCond = cond ? cond : ImGuiCond_Always;
    g.NextWindowData.PosUndock = true;
}

void ImGui::SetNextWindowSize(const ImVec2& size, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond));            
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasSize;
    g.NextWindowData.SizeVal = size;
    g.NextWindowData.SizeCond = cond ? cond : ImGuiCond_Always;
}

void ImGui::SetNextWindowSizeConstraints(const ImVec2& size_min, const ImVec2& size_max, ImGuiSizeCallback custom_callback, void* custom_callback_user_data)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasSizeConstraint;
    g.NextWindowData.SizeConstraintRect = ImRect(size_min, size_max);
    g.NextWindowData.SizeCallback = custom_callback;
    g.NextWindowData.SizeCallbackUserData = custom_callback_user_data;
}

void ImGui::SetNextWindowContentSize(const ImVec2& size)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasContentSize;
    g.NextWindowData.ContentSizeVal = size;
}

void ImGui::SetNextWindowScroll(const ImVec2& scroll)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasScroll;
    g.NextWindowData.ScrollVal = scroll;
}

void ImGui::SetNextWindowCollapsed(bool collapsed, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(cond == 0 || ImIsPowerOfTwo(cond));            
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasCollapsed;
    g.NextWindowData.CollapsedVal = collapsed;
    g.NextWindowData.CollapsedCond = cond ? cond : ImGuiCond_Always;
}

void ImGui::SetNextWindowFocus()
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasFocus;
}

void ImGui::SetNextWindowBgAlpha(float alpha)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasBgAlpha;
    g.NextWindowData.BgAlphaVal = alpha;
}

void ImGui::SetNextWindowViewport(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasViewport;
    g.NextWindowData.ViewportId = id;
}

void ImGui::SetNextWindowDockID(ImGuiID id, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasDock;
    g.NextWindowData.DockCond = cond ? cond : ImGuiCond_Always;
    g.NextWindowData.DockId = id;
}

void ImGui::SetNextWindowClass(const ImGuiWindowClass* window_class)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT((window_class->ViewportFlagsOverrideSet & window_class->ViewportFlagsOverrideClear) == 0);           
    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasWindowClass;
    g.NextWindowData.WindowClass = *window_class;
}

ImDrawList* ImGui::GetWindowDrawList()
{
    ImGuiWindow* window = GetCurrentWindow();
    return window->DrawList;
}

float ImGui::GetWindowDpiScale()
{
    ImGuiContext& g = *GImGui;
    return g.CurrentDpiScale;
}

ImGuiViewport* ImGui::GetWindowViewport()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentViewport != NULL && g.CurrentViewport == g.CurrentWindow->Viewport);
    return g.CurrentViewport;
}

ImFont* ImGui::GetFont()
{
    return GImGui->Font;
}

float ImGui::GetFontSize()
{
    return GImGui->FontSize;
}

ImVec2 ImGui::GetFontTexUvWhitePixel()
{
    return GImGui->DrawListSharedData.TexUvWhitePixel;
}

void ImGui::SetWindowFontScale(float scale)
{
    IM_ASSERT(scale > 0.0f);
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->FontWindowScale = scale;
    g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize();
}

void ImGui::ActivateItem(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    g.NavNextActivateId = id;
}

void ImGui::PushFocusScope(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    g.FocusScopeStack.push_back(window->DC.NavFocusScopeIdCurrent);
    window->DC.NavFocusScopeIdCurrent = id;
}

void ImGui::PopFocusScope()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(g.FocusScopeStack.Size > 0);     
    window->DC.NavFocusScopeIdCurrent = g.FocusScopeStack.back();
    g.FocusScopeStack.pop_back();
}

void ImGui::SetKeyboardFocusHere(int offset)
{
    IM_ASSERT(offset >= -1);          
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    g.FocusRequestNextWindow = window;
    g.FocusRequestNextCounterRegular = window->DC.FocusCounterRegular + 1 + offset;
    g.FocusRequestNextCounterTabStop = INT_MAX;
}

void ImGui::SetItemDefaultFocus()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (!window->Appearing)
        return;
    if (g.NavWindow == window->RootWindowForNav && (g.NavInitRequest || g.NavInitResultId != 0) && g.NavLayer == g.NavWindow->DC.NavLayerCurrent)
    {
        g.NavInitRequest = false;
        g.NavInitResultId = g.NavWindow->DC.LastItemId;
        g.NavInitResultRectRel = ImRect(g.NavWindow->DC.LastItemRect.Min - g.NavWindow->Pos, g.NavWindow->DC.LastItemRect.Max - g.NavWindow->Pos);
        NavUpdateAnyRequestFlag();
        if (!IsItemVisible())
            SetScrollHereY();
    }
}

void ImGui::SetStateStorage(ImGuiStorage* tree)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    window->DC.StateStorage = tree ? tree : &window->StateStorage;
}

ImGuiStorage* ImGui::GetStateStorage()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->DC.StateStorage;
}

void ImGui::PushID(const char* str_id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetIDNoKeepAlive(str_id);
    window->IDStack.push_back(id);
}

void ImGui::PushID(const char* str_id_begin, const char* str_id_end)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetIDNoKeepAlive(str_id_begin, str_id_end);
    window->IDStack.push_back(id);
}

void ImGui::PushID(const void* ptr_id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetIDNoKeepAlive(ptr_id);
    window->IDStack.push_back(id);
}

void ImGui::PushID(int int_id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetIDNoKeepAlive(int_id);
    window->IDStack.push_back(id);
}

void ImGui::PushOverrideID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->IDStack.push_back(id);
}

ImGuiID ImGui::GetIDWithSeed(const char* str, const char* str_end, ImGuiID seed)
{
    ImGuiID id = ImHashStr(str, str_end ? (str_end - str) : 0, seed);
    ImGui::KeepAliveID(id);
#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiContext& g = *GImGui;
    IMGUI_TEST_ENGINE_ID_INFO2(id, ImGuiDataType_String, str, str_end);
#endif
    return id;
}

void ImGui::PopID()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    IM_ASSERT(window->IDStack.Size > 1);            
    window->IDStack.pop_back();
}

ImGuiID ImGui::GetID(const char* str_id)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->GetID(str_id);
}

ImGuiID ImGui::GetID(const char* str_id_begin, const char* str_id_end)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->GetID(str_id_begin, str_id_end);
}

ImGuiID ImGui::GetID(const void* ptr_id)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->GetID(ptr_id);
}

bool ImGui::IsRectVisible(const ImVec2& size)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ClipRect.Overlaps(ImRect(window->DC.CursorPos, window->DC.CursorPos + size));
}

bool ImGui::IsRectVisible(const ImVec2& rect_min, const ImVec2& rect_max)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ClipRect.Overlaps(ImRect(rect_min, rect_max));
}


bool ImGui::DebugCheckVersionAndDataLayout(const char* version, size_t sz_io, size_t sz_style, size_t sz_vec2, size_t sz_vec4, size_t sz_vert, size_t sz_idx)
{
    bool error = false;
    if (strcmp(version, IMGUI_VERSION) != 0) { error = true; IM_ASSERT(strcmp(version, IMGUI_VERSION) == 0 && "Mismatched version string!"); }
    if (sz_io != sizeof(ImGuiIO)) { error = true; IM_ASSERT(sz_io == sizeof(ImGuiIO) && "Mismatched struct layout!"); }
    if (sz_style != sizeof(ImGuiStyle)) { error = true; IM_ASSERT(sz_style == sizeof(ImGuiStyle) && "Mismatched struct layout!"); }
    if (sz_vec2 != sizeof(ImVec2)) { error = true; IM_ASSERT(sz_vec2 == sizeof(ImVec2) && "Mismatched struct layout!"); }
    if (sz_vec4 != sizeof(ImVec4)) { error = true; IM_ASSERT(sz_vec4 == sizeof(ImVec4) && "Mismatched struct layout!"); }
    if (sz_vert != sizeof(ImDrawVert)) { error = true; IM_ASSERT(sz_vert == sizeof(ImDrawVert) && "Mismatched struct layout!"); }
    if (sz_idx != sizeof(ImDrawIdx)) { error = true; IM_ASSERT(sz_idx == sizeof(ImDrawIdx) && "Mismatched struct layout!"); }
    return !error;
}

static void ImGui::ErrorCheckNewFrameSanityChecks()
{
    ImGuiContext& g = *GImGui;

    if (true) IM_ASSERT(1); else IM_ASSERT(0);

    IM_ASSERT(g.Initialized);
    IM_ASSERT((g.IO.DeltaTime > 0.0f || g.FrameCount == 0)              && "Need a positive DeltaTime!");
    IM_ASSERT((g.FrameCount == 0 || g.FrameCountEnded == g.FrameCount)  && "Forgot to call Render() or EndFrame() at the end of the previous frame?");
    IM_ASSERT(g.IO.DisplaySize.x >= 0.0f && g.IO.DisplaySize.y >= 0.0f  && "Invalid DisplaySize value!");
    IM_ASSERT(g.IO.Fonts->Fonts.Size > 0                                && "Font Atlas not built. Did you call io.Fonts->GetTexDataAsRGBA32() / GetTexDataAsAlpha8()?");
    IM_ASSERT(g.IO.Fonts->Fonts[0]->IsLoaded()                          && "Font Atlas not built. Did you call io.Fonts->GetTexDataAsRGBA32() / GetTexDataAsAlpha8()?");
    IM_ASSERT(g.Style.CurveTessellationTol > 0.0f                       && "Invalid style setting!");
    IM_ASSERT(g.Style.CircleSegmentMaxError > 0.0f                      && "Invalid style setting!");
    IM_ASSERT(g.Style.Alpha >= 0.0f && g.Style.Alpha <= 1.0f            && "Invalid style setting!");           
    IM_ASSERT(g.Style.WindowMinSize.x >= 1.0f && g.Style.WindowMinSize.y >= 1.0f && "Invalid style setting.");
    IM_ASSERT(g.Style.WindowMenuButtonPosition == ImGuiDir_None || g.Style.WindowMenuButtonPosition == ImGuiDir_Left || g.Style.WindowMenuButtonPosition == ImGuiDir_Right);
    for (int n = 0; n < ImGuiKey_COUNT; n++)
        IM_ASSERT(g.IO.KeyMap[n] >= -1 && g.IO.KeyMap[n] < IM_ARRAYSIZE(g.IO.KeysDown) && "io.KeyMap[] contains an out of bound value (need to be 0..512, or -1 for unmapped key)");

    if (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard)
        IM_ASSERT(g.IO.KeyMap[ImGuiKey_Space] != -1 && "ImGuiKey_Space is not mapped, required for keyboard navigation.");

    if (g.IO.ConfigWindowsResizeFromEdges && !(g.IO.BackendFlags & ImGuiBackendFlags_HasMouseCursors))
        g.IO.ConfigWindowsResizeFromEdges = false;

    if (g.FrameCount == 1 && (g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable) && (g.ConfigFlagsLastFrame & ImGuiConfigFlags_DockingEnable) == 0)
        IM_ASSERT(0 && "Please set DockingEnable before the first call to NewFrame()! Otherwise you will lose your .ini settings!");
    if (g.FrameCount == 1 && (g.IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) && (g.ConfigFlagsLastFrame & ImGuiConfigFlags_ViewportsEnable) == 0)
        IM_ASSERT(0 && "Please set ViewportsEnable before the first call to NewFrame()! Otherwise you will lose your .ini settings!");

    if (g.IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        if ((g.IO.BackendFlags & ImGuiBackendFlags_PlatformHasViewports) && (g.IO.BackendFlags & ImGuiBackendFlags_RendererHasViewports))
        {
            IM_ASSERT((g.FrameCount == 0 || g.FrameCount == g.FrameCountPlatformEnded) && "Forgot to call UpdatePlatformWindows() in main loop after EndFrame()? Check examples/ applications for reference.");
            IM_ASSERT(g.PlatformIO.Platform_CreateWindow  != NULL && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Platform_DestroyWindow != NULL && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Platform_GetWindowPos  != NULL && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Platform_SetWindowPos  != NULL && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Platform_GetWindowSize != NULL && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Platform_SetWindowSize != NULL && "Platform init didn't install handlers?");
            IM_ASSERT(g.PlatformIO.Monitors.Size > 0 && "Platform init didn't setup Monitors list?");
            IM_ASSERT((g.Viewports[0]->PlatformUserData != NULL || g.Viewports[0]->PlatformHandle != NULL) && "Platform init didn't setup main viewport.");
            if (g.IO.ConfigDockingTransparentPayload && (g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
                IM_ASSERT(g.PlatformIO.Platform_SetWindowAlpha != NULL && "Platform_SetWindowAlpha handler is required to use io.ConfigDockingTransparent!");
        }
        else
        {
            g.IO.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        }

        for (int monitor_n = 0; monitor_n < g.PlatformIO.Monitors.Size; monitor_n++)
        {
            ImGuiPlatformMonitor& mon = g.PlatformIO.Monitors[monitor_n];
            IM_ASSERT(mon.MainSize.x > 0.0f && mon.MainSize.y > 0.0f && "Monitor main bounds not setup properly.");
            IM_ASSERT(ImRect(mon.MainPos, mon.MainPos + mon.MainSize).Contains(ImRect(mon.WorkPos, mon.WorkPos + mon.WorkSize)) && "Monitor work bounds not setup properly. If you don't have work area information, just copy MainPos/MainSize into them.");
            IM_ASSERT(mon.DpiScale != 0.0f);
        }
    }
}

static void ImGui::ErrorCheckEndFrameSanityChecks()
{
    ImGuiContext& g = *GImGui;

    const ImGuiKeyModFlags key_mod_flags = GetMergedKeyModFlags();
    IM_ASSERT((key_mod_flags == 0 || g.IO.KeyMods == key_mod_flags) && "Mismatching io.KeyCtrl/io.KeyShift/io.KeyAlt/io.KeySuper vs io.KeyMods");
    IM_UNUSED(key_mod_flags);

    if (g.CurrentWindowStack.Size != 1)
    {
        if (g.CurrentWindowStack.Size > 1)
        {
            IM_ASSERT_USER_ERROR(g.CurrentWindowStack.Size == 1, "Mismatched Begin/BeginChild vs End/EndChild calls: did you forget to call End/EndChild?");
            while (g.CurrentWindowStack.Size > 1)
                End();
        }
        else
        {
            IM_ASSERT_USER_ERROR(g.CurrentWindowStack.Size == 1, "Mismatched Begin/BeginChild vs End/EndChild calls: did you call End/EndChild too much?");
        }
    }

    IM_ASSERT_USER_ERROR(g.GroupStack.Size == 0, "Missing EndGroup call!");
}

void    ImGui::ErrorCheckEndFrameRecover(ImGuiErrorLogCallback log_callback, void* user_data)
{
    ImGuiContext& g = *GImGui;
    while (g.CurrentWindowStack.Size > 0)
    {
#ifdef IMGUI_HAS_TABLE
        while (g.CurrentTable && (g.CurrentTable->OuterWindow == g.CurrentWindow || g.CurrentTable->InnerWindow == g.CurrentWindow))
        {
            if (log_callback) log_callback(user_data, "Recovered from missing EndTable() in '%s'", g.CurrentTable->OuterWindow->Name);
            EndTable();
        }
#endif
        ImGuiWindow* window = g.CurrentWindow;
        IM_ASSERT(window != NULL);
        while (g.CurrentTabBar != NULL) 
        {
            if (log_callback) log_callback(user_data, "Recovered from missing EndTabBar() in '%s'", window->Name);
            EndTabBar();
        }
        while (window->DC.TreeDepth > 0)
        {
            if (log_callback) log_callback(user_data, "Recovered from missing TreePop() in '%s'", window->Name);
            TreePop();
        }
        while (g.GroupStack.Size > window->DC.StackSizesOnBegin.SizeOfGroupStack)
        {
            if (log_callback) log_callback(user_data, "Recovered from missing EndGroup() in '%s'", window->Name);
            EndGroup();
        }
        while (window->IDStack.Size > 1)
        {
            if (log_callback) log_callback(user_data, "Recovered from missing PopID() in '%s'", window->Name);
            PopID();
        }
        while (g.ColorStack.Size > window->DC.StackSizesOnBegin.SizeOfColorStack)
        {
            if (log_callback) log_callback(user_data, "Recovered from missing PopStyleColor() in '%s' for ImGuiCol_%s", window->Name, GetStyleColorName(g.ColorStack.back().Col));
            PopStyleColor();
        }
        while (g.StyleVarStack.Size > window->DC.StackSizesOnBegin.SizeOfStyleVarStack)
        {
            if (log_callback) log_callback(user_data, "Recovered from missing PopStyleVar() in '%s'", window->Name);
            PopStyleVar();
        }
        while (g.FocusScopeStack.Size > window->DC.StackSizesOnBegin.SizeOfFocusScopeStack)
        {
            if (log_callback) log_callback(user_data, "Recovered from missing PopFocusScope() in '%s'", window->Name);
            PopFocusScope();
        }
        if (g.CurrentWindowStack.Size == 1)
        {
            IM_ASSERT(g.CurrentWindow->IsFallbackWindow);
            break;
        }
        IM_ASSERT(window == g.CurrentWindow);
        if (window->Flags & ImGuiWindowFlags_ChildWindow)
        {
            if (log_callback) log_callback(user_data, "Recovered from missing EndChild() for '%s'", window->Name);
            EndChild();
        }
        else
        {
            if (log_callback) log_callback(user_data, "Recovered from missing End() for '%s'", window->Name);
            End();
        }
    }
}

void ImGuiStackSizes::SetToCurrentState()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    SizeOfIDStack = (short)window->IDStack.Size;
    SizeOfColorStack = (short)g.ColorStack.Size;
    SizeOfStyleVarStack = (short)g.StyleVarStack.Size;
    SizeOfFontStack = (short)g.FontStack.Size;
    SizeOfFocusScopeStack = (short)g.FocusScopeStack.Size;
    SizeOfGroupStack = (short)g.GroupStack.Size;
    SizeOfBeginPopupStack = (short)g.BeginPopupStack.Size;
}

void ImGuiStackSizes::CompareWithCurrentState()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_UNUSED(window);

    IM_ASSERT(SizeOfIDStack         == window->IDStack.Size     && "PushID/PopID or TreeNode/TreePop Mismatch!");

    IM_ASSERT(SizeOfGroupStack      == g.GroupStack.Size        && "BeginGroup/EndGroup Mismatch!");
    IM_ASSERT(SizeOfBeginPopupStack == g.BeginPopupStack.Size   && "BeginPopup/EndPopup or BeginMenu/EndMenu Mismatch!");
    IM_ASSERT(SizeOfColorStack      >= g.ColorStack.Size        && "PushStyleColor/PopStyleColor Mismatch!");
    IM_ASSERT(SizeOfStyleVarStack   >= g.StyleVarStack.Size     && "PushStyleVar/PopStyleVar Mismatch!");
    IM_ASSERT(SizeOfFontStack       >= g.FontStack.Size         && "PushFont/PopFont Mismatch!");
    IM_ASSERT(SizeOfFocusScopeStack == g.FocusScopeStack.Size   && "PushFocusScope/PopFocusScope Mismatch!");
}


void ImGui::ItemSize(const ImVec2& size, float text_baseline_y)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    const float offset_to_match_baseline_y = (text_baseline_y >= 0) ? ImMax(0.0f, window->DC.CurrLineTextBaseOffset - text_baseline_y) : 0.0f;
    const float line_height = ImMax(window->DC.CurrLineSize.y, size.y + offset_to_match_baseline_y);

    window->DC.CursorPosPrevLine.x = window->DC.CursorPos.x + size.x;
    window->DC.CursorPosPrevLine.y = window->DC.CursorPos.y;
    window->DC.CursorPos.x = IM_FLOOR(window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x);      
    window->DC.CursorPos.y = IM_FLOOR(window->DC.CursorPos.y + line_height + g.Style.ItemSpacing.y);          
    window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPosPrevLine.x);
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, window->DC.CursorPos.y - g.Style.ItemSpacing.y);
    window->DC.PrevLineSize.y = line_height;
    window->DC.CurrLineSize.y = 0.0f;
    window->DC.PrevLineTextBaseOffset = ImMax(window->DC.CurrLineTextBaseOffset, text_baseline_y);
    window->DC.CurrLineTextBaseOffset = 0.0f;

    if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
        SameLine();
}

void ImGui::ItemSize(const ImRect& bb, float text_baseline_y)
{
    ItemSize(bb.GetSize(), text_baseline_y);
}

bool ImGui::ItemAdd(const ImRect& bb, ImGuiID id, const ImRect* nav_bb_arg)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (id != 0)
    {
        window->DC.NavLayerActiveMaskNext |= (1 << window->DC.NavLayerCurrent);
        if (g.NavId == id || g.NavAnyRequest)
            if (g.NavWindow->RootWindowForNav == window->RootWindowForNav)
                if (window == g.NavWindow || ((window->Flags | g.NavWindow->Flags) & ImGuiWindowFlags_NavFlattened))
                    NavProcessItem(window, nav_bb_arg ? *nav_bb_arg : bb, id);

#ifdef IMGUI_DEBUG_TOOL_ITEM_PICKER_EX
        if (id == g.DebugItemPickerBreakId)
        {
            IM_DEBUG_BREAK();
            g.DebugItemPickerBreakId = 0;
        }
#endif
    }

    window->DC.LastItemId = id;
    window->DC.LastItemRect = bb;
    window->DC.LastItemStatusFlags = ImGuiItemStatusFlags_None;
    g.NextItemData.Flags = ImGuiNextItemDataFlags_None;

#ifdef IMGUI_ENABLE_TEST_ENGINE
    if (id != 0)
        IMGUI_TEST_ENGINE_ITEM_ADD(nav_bb_arg ? *nav_bb_arg : bb, id);
#endif

    const bool is_clipped = IsClippedEx(bb, id, false);
    if (is_clipped)
        return false;
    if (IsMouseHoveringRect(bb.Min, bb.Max))
        window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HoveredRect;
    return true;
}

void ImGui::SameLine(float offset_from_start_x, float spacing_w)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    if (offset_from_start_x != 0.0f)
    {
        if (spacing_w < 0.0f) spacing_w = 0.0f;
        window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + offset_from_start_x + spacing_w + window->DC.GroupOffset.x + window->DC.ColumnsOffset.x;
        window->DC.CursorPos.y = window->DC.CursorPosPrevLine.y;
    }
    else
    {
        if (spacing_w < 0.0f) spacing_w = g.Style.ItemSpacing.x;
        window->DC.CursorPos.x = window->DC.CursorPosPrevLine.x + spacing_w;
        window->DC.CursorPos.y = window->DC.CursorPosPrevLine.y;
    }
    window->DC.CurrLineSize = window->DC.PrevLineSize;
    window->DC.CurrLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
}

ImVec2 ImGui::GetCursorScreenPos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos;
}

void ImGui::SetCursorScreenPos(const ImVec2& pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos = pos;
    window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
}

ImVec2 ImGui::GetCursorPos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos - window->Pos + window->Scroll;
}

float ImGui::GetCursorPosX()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos.x - window->Pos.x + window->Scroll.x;
}

float ImGui::GetCursorPosY()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorPos.y - window->Pos.y + window->Scroll.y;
}

void ImGui::SetCursorPos(const ImVec2& local_pos)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos = window->Pos - window->Scroll + local_pos;
    window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, window->DC.CursorPos);
}

void ImGui::SetCursorPosX(float x)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + x;
    window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPos.x);
}

void ImGui::SetCursorPosY(float y)
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.CursorPos.y = window->Pos.y - window->Scroll.y + y;
    window->DC.CursorMaxPos.y = ImMax(window->DC.CursorMaxPos.y, window->DC.CursorPos.y);
}

ImVec2 ImGui::GetCursorStartPos()
{
    ImGuiWindow* window = GetCurrentWindowRead();
    return window->DC.CursorStartPos - window->Pos;
}

void ImGui::Indent(float indent_w)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.Indent.x += (indent_w != 0.0f) ? indent_w : g.Style.IndentSpacing;
    window->DC.CursorPos.x = window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x;
}

void ImGui::Unindent(float indent_w)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.Indent.x -= (indent_w != 0.0f) ? indent_w : g.Style.IndentSpacing;
    window->DC.CursorPos.x = window->Pos.x + window->DC.Indent.x + window->DC.ColumnsOffset.x;
}

void ImGui::SetNextItemWidth(float item_width)
{
    ImGuiContext& g = *GImGui;
    g.NextItemData.Flags |= ImGuiNextItemDataFlags_HasWidth;
    g.NextItemData.Width = item_width;
}

void ImGui::PushItemWidth(float item_width)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    window->DC.ItemWidth = (item_width == 0.0f ? window->ItemWidthDefault : item_width);
    window->DC.ItemWidthStack.push_back(window->DC.ItemWidth);
    g.NextItemData.Flags &= ~ImGuiNextItemDataFlags_HasWidth;
}

void ImGui::PushMultiItemsWidths(int components, float w_full)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const ImGuiStyle& style = g.Style;
    const float w_item_one  = ImMax(1.0f, IM_FLOOR((w_full - (style.ItemInnerSpacing.x) * (components - 1)) / (float)components));
    const float w_item_last = ImMax(1.0f, IM_FLOOR(w_full - (w_item_one + style.ItemInnerSpacing.x) * (components - 1)));
    window->DC.ItemWidthStack.push_back(w_item_last);
    for (int i = 0; i < components - 1; i++)
        window->DC.ItemWidthStack.push_back(w_item_one);
    window->DC.ItemWidth = window->DC.ItemWidthStack.back();
    g.NextItemData.Flags &= ~ImGuiNextItemDataFlags_HasWidth;
}

void ImGui::PopItemWidth()
{
    ImGuiWindow* window = GetCurrentWindow();
    window->DC.ItemWidthStack.pop_back();
    window->DC.ItemWidth = window->DC.ItemWidthStack.empty() ? window->ItemWidthDefault : window->DC.ItemWidthStack.back();
}

float ImGui::CalcItemWidth()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    float w;
    if (g.NextItemData.Flags & ImGuiNextItemDataFlags_HasWidth)
        w = g.NextItemData.Width;
    else
        w = window->DC.ItemWidth;
    if (w < 0.0f)
    {
        float region_max_x = GetContentRegionMaxAbs().x;
        w = ImMax(1.0f, region_max_x - window->DC.CursorPos.x + w);
    }
    w = IM_FLOOR(w);
    return w;
}

ImVec2 ImGui::CalcItemSize(ImVec2 size, float default_w, float default_h)
{
    ImGuiWindow* window = GImGui->CurrentWindow;

    ImVec2 region_max;
    if (size.x < 0.0f || size.y < 0.0f)
        region_max = GetContentRegionMaxAbs();

    if (size.x == 0.0f)
        size.x = default_w;
    else if (size.x < 0.0f)
        size.x = ImMax(4.0f, region_max.x - window->DC.CursorPos.x + size.x);

    if (size.y == 0.0f)
        size.y = default_h;
    else if (size.y < 0.0f)
        size.y = ImMax(4.0f, region_max.y - window->DC.CursorPos.y + size.y);

    return size;
}

float ImGui::GetTextLineHeight()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize;
}

float ImGui::GetTextLineHeightWithSpacing()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize + g.Style.ItemSpacing.y;
}

float ImGui::GetFrameHeight()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize + g.Style.FramePadding.y * 2.0f;
}

float ImGui::GetFrameHeightWithSpacing()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize + g.Style.FramePadding.y * 2.0f + g.Style.ItemSpacing.y;
}

ImVec2 ImGui::GetContentRegionMax()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImVec2 mx = window->ContentRegionRect.Max - window->Pos;
    if (window->DC.CurrentColumns || g.CurrentTable)
        mx.x = window->WorkRect.Max.x - window->Pos.x;
    return mx;
}

ImVec2 ImGui::GetContentRegionMaxAbs()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImVec2 mx = window->ContentRegionRect.Max;
    if (window->DC.CurrentColumns || g.CurrentTable)
        mx.x = window->WorkRect.Max.x;
    return mx;
}

ImVec2 ImGui::GetContentRegionAvail()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return GetContentRegionMaxAbs() - window->DC.CursorPos;
}

ImVec2 ImGui::GetWindowContentRegionMin()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ContentRegionRect.Min - window->Pos;
}

ImVec2 ImGui::GetWindowContentRegionMax()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ContentRegionRect.Max - window->Pos;
}

float ImGui::GetWindowContentRegionWidth()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ContentRegionRect.GetWidth();
}

void ImGui::BeginGroup()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    g.GroupStack.resize(g.GroupStack.Size + 1);
    ImGuiGroupData& group_data = g.GroupStack.back();
    group_data.WindowID = window->ID;
    group_data.BackupCursorPos = window->DC.CursorPos;
    group_data.BackupCursorMaxPos = window->DC.CursorMaxPos;
    group_data.BackupIndent = window->DC.Indent;
    group_data.BackupGroupOffset = window->DC.GroupOffset;
    group_data.BackupCurrLineSize = window->DC.CurrLineSize;
    group_data.BackupCurrLineTextBaseOffset = window->DC.CurrLineTextBaseOffset;
    group_data.BackupActiveIdIsAlive = g.ActiveIdIsAlive;
    group_data.BackupActiveIdPreviousFrameIsAlive = g.ActiveIdPreviousFrameIsAlive;
    group_data.EmitItem = true;

    window->DC.GroupOffset.x = window->DC.CursorPos.x - window->Pos.x - window->DC.ColumnsOffset.x;
    window->DC.Indent = window->DC.GroupOffset;
    window->DC.CursorMaxPos = window->DC.CursorPos;
    window->DC.CurrLineSize = ImVec2(0.0f, 0.0f);
    if (g.LogEnabled)
        g.LogLinePosY = -FLT_MAX;      
}

void ImGui::EndGroup()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(g.GroupStack.Size > 0);    

    ImGuiGroupData& group_data = g.GroupStack.back();
    IM_ASSERT(group_data.WindowID == window->ID);     

    ImRect group_bb(group_data.BackupCursorPos, ImMax(window->DC.CursorMaxPos, group_data.BackupCursorPos));

    window->DC.CursorPos = group_data.BackupCursorPos;
    window->DC.CursorMaxPos = ImMax(group_data.BackupCursorMaxPos, window->DC.CursorMaxPos);
    window->DC.Indent = group_data.BackupIndent;
    window->DC.GroupOffset = group_data.BackupGroupOffset;
    window->DC.CurrLineSize = group_data.BackupCurrLineSize;
    window->DC.CurrLineTextBaseOffset = group_data.BackupCurrLineTextBaseOffset;
    if (g.LogEnabled)
        g.LogLinePosY = -FLT_MAX;      

    if (!group_data.EmitItem)
    {
        g.GroupStack.pop_back();
        return;
    }

    window->DC.CurrLineTextBaseOffset = ImMax(window->DC.PrevLineTextBaseOffset, group_data.BackupCurrLineTextBaseOffset);                            
    ItemSize(group_bb.GetSize());
    ItemAdd(group_bb, 0);

    const bool group_contains_curr_active_id = (group_data.BackupActiveIdIsAlive != g.ActiveId) && (g.ActiveIdIsAlive == g.ActiveId) && g.ActiveId;
    const bool group_contains_prev_active_id = (group_data.BackupActiveIdPreviousFrameIsAlive == false) && (g.ActiveIdPreviousFrameIsAlive == true);
    if (group_contains_curr_active_id)
        window->DC.LastItemId = g.ActiveId;
    else if (group_contains_prev_active_id)
        window->DC.LastItemId = g.ActiveIdPreviousFrame;
    window->DC.LastItemRect = group_bb;

    if (group_contains_curr_active_id && g.ActiveIdHasBeenEditedThisFrame)
        window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_Edited;

    window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HasDeactivated;
    if (group_contains_prev_active_id && g.ActiveId != g.ActiveIdPreviousFrame)
        window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_Deactivated;

    g.GroupStack.pop_back();
}


static float CalcScrollEdgeSnap(float target, float snap_min, float snap_max, float snap_threshold, float center_ratio)
{
    if (target <= snap_min + snap_threshold)
        return ImLerp(snap_min, target, center_ratio);
    if (target >= snap_max - snap_threshold)
        return ImLerp(target, snap_max, center_ratio);
    return target;
}

static ImVec2 CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window)
{
    ImVec2 scroll = window->Scroll;
    if (window->ScrollTarget.x < FLT_MAX)
    {
        float center_x_ratio = window->ScrollTargetCenterRatio.x;
        float scroll_target_x = window->ScrollTarget.x;
        float snap_x_min = 0.0f;
        float snap_x_max = window->ScrollMax.x + window->Size.x;
        if (window->ScrollTargetEdgeSnapDist.x > 0.0f)
            scroll_target_x = CalcScrollEdgeSnap(scroll_target_x, snap_x_min, snap_x_max, window->ScrollTargetEdgeSnapDist.x, center_x_ratio);
        scroll.x = scroll_target_x - center_x_ratio * (window->SizeFull.x - window->ScrollbarSizes.x);
    }
    if (window->ScrollTarget.y < FLT_MAX)
    {
        float decoration_up_height = window->TitleBarHeight() + window->MenuBarHeight();
        float center_y_ratio = window->ScrollTargetCenterRatio.y;
        float scroll_target_y = window->ScrollTarget.y;
        float snap_y_min = 0.0f;
        float snap_y_max = window->ScrollMax.y + window->Size.y - decoration_up_height;
        if (window->ScrollTargetEdgeSnapDist.y > 0.0f)
            scroll_target_y = CalcScrollEdgeSnap(scroll_target_y, snap_y_min, snap_y_max, window->ScrollTargetEdgeSnapDist.y, center_y_ratio);
        scroll.y = scroll_target_y - center_y_ratio * (window->SizeFull.y - window->ScrollbarSizes.y - decoration_up_height);
    }
    scroll.x = IM_FLOOR(ImMax(scroll.x, 0.0f));
    scroll.y = IM_FLOOR(ImMax(scroll.y, 0.0f));
    if (!window->Collapsed && !window->SkipItems)
    {
        scroll.x = ImMin(scroll.x, window->ScrollMax.x);
        scroll.y = ImMin(scroll.y, window->ScrollMax.y);
    }
    return scroll;
}

ImVec2 ImGui::ScrollToBringRectIntoView(ImGuiWindow* window, const ImRect& item_rect)
{
    ImGuiContext& g = *GImGui;
    ImRect window_rect(window->InnerRect.Min - ImVec2(1, 1), window->InnerRect.Max + ImVec2(1, 1));
    ImVec2 delta_scroll;
    if (!window_rect.Contains(item_rect))
    {
        if (window->ScrollbarX && item_rect.Min.x < window_rect.Min.x)
            SetScrollFromPosX(window, item_rect.Min.x - window->Pos.x - g.Style.ItemSpacing.x, 0.0f);
        else if (window->ScrollbarX && item_rect.Max.x >= window_rect.Max.x)
            SetScrollFromPosX(window, item_rect.Max.x - window->Pos.x + g.Style.ItemSpacing.x, 1.0f);
        if (item_rect.Min.y < window_rect.Min.y)
            SetScrollFromPosY(window, item_rect.Min.y - window->Pos.y - g.Style.ItemSpacing.y, 0.0f);
        else if (item_rect.Max.y >= window_rect.Max.y)
            SetScrollFromPosY(window, item_rect.Max.y - window->Pos.y + g.Style.ItemSpacing.y, 1.0f);

        ImVec2 next_scroll = CalcNextScrollFromScrollTargetAndClamp(window);
        delta_scroll = next_scroll - window->Scroll;
    }

    if (window->Flags & ImGuiWindowFlags_ChildWindow)
        delta_scroll += ScrollToBringRectIntoView(window->ParentWindow, ImRect(item_rect.Min - delta_scroll, item_rect.Max - delta_scroll));

    return delta_scroll;
}

float ImGui::GetScrollX()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Scroll.x;
}

float ImGui::GetScrollY()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->Scroll.y;
}

float ImGui::GetScrollMaxX()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ScrollMax.x;
}

float ImGui::GetScrollMaxY()
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    return window->ScrollMax.y;
}

void ImGui::SetScrollX(ImGuiWindow* window, float scroll_x)
{
    window->ScrollTarget.x = scroll_x;
    window->ScrollTargetCenterRatio.x = 0.0f;
    window->ScrollTargetEdgeSnapDist.x = 0.0f;
}

void ImGui::SetScrollY(ImGuiWindow* window, float scroll_y)
{
    window->ScrollTarget.y = scroll_y;
    window->ScrollTargetCenterRatio.y = 0.0f;
    window->ScrollTargetEdgeSnapDist.y = 0.0f;
}

void ImGui::SetScrollX(float scroll_x)
{
    ImGuiContext& g = *GImGui;
    SetScrollX(g.CurrentWindow, scroll_x);
}

void ImGui::SetScrollY(float scroll_y)
{
    ImGuiContext& g = *GImGui;
    SetScrollY(g.CurrentWindow, scroll_y);
}

void ImGui::SetScrollFromPosX(ImGuiWindow* window, float local_x, float center_x_ratio)
{
    IM_ASSERT(center_x_ratio >= 0.0f && center_x_ratio <= 1.0f);
    window->ScrollTarget.x = IM_FLOOR(local_x + window->Scroll.x);       
    window->ScrollTargetCenterRatio.x = center_x_ratio;
    window->ScrollTargetEdgeSnapDist.x = 0.0f;
}

void ImGui::SetScrollFromPosY(ImGuiWindow* window, float local_y, float center_y_ratio)
{
    IM_ASSERT(center_y_ratio >= 0.0f && center_y_ratio <= 1.0f);
    local_y -= window->TitleBarHeight() + window->MenuBarHeight();               
    window->ScrollTarget.y = IM_FLOOR(local_y + window->Scroll.y);       
    window->ScrollTargetCenterRatio.y = center_y_ratio;
    window->ScrollTargetEdgeSnapDist.y = 0.0f;
}

void ImGui::SetScrollFromPosX(float local_x, float center_x_ratio)
{
    ImGuiContext& g = *GImGui;
    SetScrollFromPosX(g.CurrentWindow, local_x, center_x_ratio);
}

void ImGui::SetScrollFromPosY(float local_y, float center_y_ratio)
{
    ImGuiContext& g = *GImGui;
    SetScrollFromPosY(g.CurrentWindow, local_y, center_y_ratio);
}

void ImGui::SetScrollHereX(float center_x_ratio)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    float spacing_x = g.Style.ItemSpacing.x;
    float target_pos_x = ImLerp(window->DC.LastItemRect.Min.x - spacing_x, window->DC.LastItemRect.Max.x + spacing_x, center_x_ratio);
    SetScrollFromPosX(window, target_pos_x - window->Pos.x, center_x_ratio);       

    window->ScrollTargetEdgeSnapDist.x = ImMax(0.0f, window->WindowPadding.x - spacing_x);
}

void ImGui::SetScrollHereY(float center_y_ratio)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    float spacing_y = g.Style.ItemSpacing.y;
    float target_pos_y = ImLerp(window->DC.CursorPosPrevLine.y - spacing_y, window->DC.CursorPosPrevLine.y + window->DC.PrevLineSize.y + spacing_y, center_y_ratio);
    SetScrollFromPosY(window, target_pos_y - window->Pos.y, center_y_ratio);       

    window->ScrollTargetEdgeSnapDist.y = ImMax(0.0f, window->WindowPadding.y - spacing_y);
}

void ImGui::BeginTooltip()
{
    BeginTooltipEx(ImGuiWindowFlags_None, ImGuiTooltipFlags_None);
}

void ImGui::BeginTooltipEx(ImGuiWindowFlags extra_flags, ImGuiTooltipFlags tooltip_flags)
{
    ImGuiContext& g = *GImGui;

    if (g.DragDropWithinSource || g.DragDropWithinTarget)
    {
        ImVec2 tooltip_pos = g.IO.MousePos + ImVec2(16 * g.Style.MouseCursorScale, 8 * g.Style.MouseCursorScale);
        SetNextWindowPos(tooltip_pos);
        SetNextWindowBgAlpha(g.Style.Colors[ImGuiCol_PopupBg].w * 0.60f);
        tooltip_flags |= ImGuiTooltipFlags_OverridePreviousTooltip;
    }

    char window_name[16];
    ImFormatString(window_name, IM_ARRAYSIZE(window_name), "##Tooltip_%02d", g.TooltipOverrideCount);
    if (tooltip_flags & ImGuiTooltipFlags_OverridePreviousTooltip)
        if (ImGuiWindow* window = FindWindowByName(window_name))
            if (window->Active)
            {
                window->Hidden = true;
                window->HiddenFramesCanSkipItems = 1;
                ImFormatString(window_name, IM_ARRAYSIZE(window_name), "##Tooltip_%02d", ++g.TooltipOverrideCount);
            }
    ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking;
    Begin(window_name, NULL, flags | extra_flags);
}

void ImGui::EndTooltip()
{
    IM_ASSERT(GetCurrentWindowRead()->Flags & ImGuiWindowFlags_Tooltip);      
    End();
}

void ImGui::SetTooltipV(const char* fmt, va_list args)
{
    BeginTooltipEx(0, ImGuiTooltipFlags_OverridePreviousTooltip);
    TextV(fmt, args);
    EndTooltip();
}

void ImGui::SetTooltip(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SetTooltipV(fmt, args);
    va_end(args);
}

bool ImGui::IsPopupOpen(ImGuiID id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    if (popup_flags & ImGuiPopupFlags_AnyPopupId)
    {
        IM_ASSERT(id == 0);
        if (popup_flags & ImGuiPopupFlags_AnyPopupLevel)
            return g.OpenPopupStack.Size > 0;
        else
            return g.OpenPopupStack.Size > g.BeginPopupStack.Size;
    }
    else
    {
        if (popup_flags & ImGuiPopupFlags_AnyPopupLevel)
        {
            for (int n = 0; n < g.OpenPopupStack.Size; n++)
                if (g.OpenPopupStack[n].PopupId == id)
                    return true;
            return false;
        }
        else
        {
            return g.OpenPopupStack.Size > g.BeginPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].PopupId == id;
        }
    }
}

bool ImGui::IsPopupOpen(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiID id = (popup_flags & ImGuiPopupFlags_AnyPopupId) ? 0 : g.CurrentWindow->GetID(str_id);
    if ((popup_flags & ImGuiPopupFlags_AnyPopupLevel) && id != 0)
        IM_ASSERT(0 && "Cannot use IsPopupOpen() with a string id and ImGuiPopupFlags_AnyPopupLevel.");         
    return IsPopupOpen(id, popup_flags);
}

ImGuiWindow* ImGui::GetTopMostPopupModal()
{
    ImGuiContext& g = *GImGui;
    for (int n = g.OpenPopupStack.Size - 1; n >= 0; n--)
        if (ImGuiWindow* popup = g.OpenPopupStack.Data[n].Window)
            if (popup->Flags & ImGuiWindowFlags_Modal)
                return popup;
    return NULL;
}

void ImGui::OpenPopup(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    OpenPopupEx(g.CurrentWindow->GetID(str_id), popup_flags);
}

void ImGui::OpenPopupEx(ImGuiID id, ImGuiPopupFlags popup_flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;
    const int current_stack_size = g.BeginPopupStack.Size;

    if (popup_flags & ImGuiPopupFlags_NoOpenOverExistingPopup)
        if (IsPopupOpen(0u, ImGuiPopupFlags_AnyPopupId))
            return;

    ImGuiPopupData popup_ref;                   
    popup_ref.PopupId = id;
    popup_ref.Window = NULL;
    popup_ref.SourceWindow = g.NavWindow;
    popup_ref.OpenFrameCount = g.FrameCount;
    popup_ref.OpenParentId = parent_window->IDStack.back();
    popup_ref.OpenPopupPos = NavCalcPreferredRefPos();
    popup_ref.OpenMousePos = IsMousePosValid(&g.IO.MousePos) ? g.IO.MousePos : popup_ref.OpenPopupPos;

    IMGUI_DEBUG_LOG_POPUP("OpenPopupEx(0x%08X)\n", id);
    if (g.OpenPopupStack.Size < current_stack_size + 1)
    {
        g.OpenPopupStack.push_back(popup_ref);
    }
    else
    {
        if (g.OpenPopupStack[current_stack_size].PopupId == id && g.OpenPopupStack[current_stack_size].OpenFrameCount == g.FrameCount - 1)
        {
            g.OpenPopupStack[current_stack_size].OpenFrameCount = popup_ref.OpenFrameCount;
        }
        else
        {
            ClosePopupToLevel(current_stack_size, false);
            g.OpenPopupStack.push_back(popup_ref);
        }

    }
}

void ImGui::ClosePopupsOverWindow(ImGuiWindow* ref_window, bool restore_focus_to_window_under_popup)
{
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.Size == 0)
        return;

    int popup_count_to_keep = 0;
    if (ref_window)
    {
        for (; popup_count_to_keep < g.OpenPopupStack.Size; popup_count_to_keep++)
        {
            ImGuiPopupData& popup = g.OpenPopupStack[popup_count_to_keep];
            if (!popup.Window)
                continue;
            IM_ASSERT((popup.Window->Flags & ImGuiWindowFlags_Popup) != 0);
            if (popup.Window->Flags & ImGuiWindowFlags_ChildWindow)
                continue;

            bool ref_window_is_descendent_of_popup = false;
            for (int n = popup_count_to_keep; n < g.OpenPopupStack.Size; n++)
                if (ImGuiWindow* popup_window = g.OpenPopupStack[n].Window)
                    if (popup_window->RootWindow == ref_window->RootWindow)
                    {
                        ref_window_is_descendent_of_popup = true;
                        break;
                    }
            if (!ref_window_is_descendent_of_popup)
                break;
        }
    }
    if (popup_count_to_keep < g.OpenPopupStack.Size)                  
    {
        IMGUI_DEBUG_LOG_POPUP("ClosePopupsOverWindow(\"%s\") -> ClosePopupToLevel(%d)\n", ref_window->Name, popup_count_to_keep);
        ClosePopupToLevel(popup_count_to_keep, restore_focus_to_window_under_popup);
    }
}

void ImGui::ClosePopupToLevel(int remaining, bool restore_focus_to_window_under_popup)
{
    ImGuiContext& g = *GImGui;
    IMGUI_DEBUG_LOG_POPUP("ClosePopupToLevel(%d), restore_focus_to_window_under_popup=%d\n", remaining, restore_focus_to_window_under_popup);
    IM_ASSERT(remaining >= 0 && remaining < g.OpenPopupStack.Size);

    ImGuiWindow* focus_window = g.OpenPopupStack[remaining].SourceWindow;
    ImGuiWindow* popup_window = g.OpenPopupStack[remaining].Window;
    g.OpenPopupStack.resize(remaining);

    if (restore_focus_to_window_under_popup)
    {
        if (focus_window && !focus_window->WasActive && popup_window)
        {
            FocusTopMostWindowUnderOne(popup_window, NULL);
        }
        else
        {
            if (g.NavLayer == ImGuiNavLayer_Main && focus_window)
                focus_window = NavRestoreLastChildNavWindow(focus_window);
            FocusWindow(focus_window);
        }
    }
}

void ImGui::CloseCurrentPopup()
{
    ImGuiContext& g = *GImGui;
    int popup_idx = g.BeginPopupStack.Size - 1;
    if (popup_idx < 0 || popup_idx >= g.OpenPopupStack.Size || g.BeginPopupStack[popup_idx].PopupId != g.OpenPopupStack[popup_idx].PopupId)
        return;

    while (popup_idx > 0)
    {
        ImGuiWindow* popup_window = g.OpenPopupStack[popup_idx].Window;
        ImGuiWindow* parent_popup_window = g.OpenPopupStack[popup_idx - 1].Window;
        bool close_parent = false;
        if (popup_window && (popup_window->Flags & ImGuiWindowFlags_ChildMenu))
            if (parent_popup_window == NULL || !(parent_popup_window->Flags & ImGuiWindowFlags_Modal))
                close_parent = true;
        if (!close_parent)
            break;
        popup_idx--;
    }
    IMGUI_DEBUG_LOG_POPUP("CloseCurrentPopup %d -> %d\n", g.BeginPopupStack.Size - 1, popup_idx);
    ClosePopupToLevel(popup_idx, true);

    if (ImGuiWindow* window = g.NavWindow)
        window->DC.NavHideHighlightOneFrame = true;
}

bool ImGui::BeginPopupEx(ImGuiID id, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (!IsPopupOpen(id, ImGuiPopupFlags_None))
    {
        g.NextWindowData.ClearFlags();           
        return false;
    }

    char name[20];
    if (flags & ImGuiWindowFlags_ChildMenu)
        ImFormatString(name, IM_ARRAYSIZE(name), "##Menu_%02d", g.BeginPopupStack.Size);      
    else
        ImFormatString(name, IM_ARRAYSIZE(name), "##Popup_%08x", id);           

    flags |= ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoDocking;
    bool is_open = Begin(name, NULL, flags);
    if (!is_open)                
        EndPopup();

    return is_open;
}

bool ImGui::BeginPopup(const char* str_id, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.Size <= g.BeginPopupStack.Size)     
    {
        g.NextWindowData.ClearFlags();           
        return false;
    }
    flags |= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
    return BeginPopupEx(g.CurrentWindow->GetID(str_id), flags);
}

bool ImGui::BeginPopupModal(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const ImGuiID id = window->GetID(name);
    if (!IsPopupOpen(id, ImGuiPopupFlags_None))
    {
        g.NextWindowData.ClearFlags();           
        return false;
    }

    if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos) == 0)
    {
        ImGuiViewportP* viewport = window->WasActive ? window->Viewport : (ImGuiViewportP*)GetMainViewport();        
        SetNextWindowPos(viewport->GetMainRect().GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    }

    flags |= ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
    const bool is_open = Begin(name, p_open, flags);
    if (!is_open || (p_open && !*p_open))                
    {
        EndPopup();
        if (is_open)
            ClosePopupToLevel(g.BeginPopupStack.Size, true);
        return false;
    }
    return is_open;
}

void ImGui::EndPopup()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(window->Flags & ImGuiWindowFlags_Popup);     
    IM_ASSERT(g.BeginPopupStack.Size > 0);

    if (g.NavWindow == window)
        NavMoveRequestTryWrapping(window, ImGuiNavMoveFlags_LoopY);

    IM_ASSERT(g.WithinEndChild == false);
    if (window->Flags & ImGuiWindowFlags_ChildWindow)
        g.WithinEndChild = true;
    End();
    g.WithinEndChild = false;
}

void ImGui::OpenPopupOnItemClick(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
    if (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
    {
        ImGuiID id = str_id ? window->GetID(str_id) : window->DC.LastItemId;                    
        IM_ASSERT(id != 0);                                                                   
        OpenPopupEx(id, popup_flags);
    }
}

bool ImGui::BeginPopupContextItem(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    if (window->SkipItems)
        return false;
    ImGuiID id = str_id ? window->GetID(str_id) : window->DC.LastItemId;                    
    IM_ASSERT(id != 0);                                                                   
    int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
    if (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
        OpenPopupEx(id, popup_flags);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
}

bool ImGui::BeginPopupContextWindow(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    if (!str_id)
        str_id = "window_context";
    ImGuiID id = window->GetID(str_id);
    int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
    if (IsMouseReleased(mouse_button) && IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
        if (!(popup_flags & ImGuiPopupFlags_NoOpenOverItems) || !IsAnyItemHovered())
            OpenPopupEx(id, popup_flags);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
}

bool ImGui::BeginPopupContextVoid(const char* str_id, ImGuiPopupFlags popup_flags)
{
    ImGuiWindow* window = GImGui->CurrentWindow;
    if (!str_id)
        str_id = "void_context";
    ImGuiID id = window->GetID(str_id);
    int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
    if (IsMouseReleased(mouse_button) && !IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        if (GetTopMostPopupModal() == NULL)
            OpenPopupEx(id, popup_flags);
    return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
}

ImVec2 ImGui::FindBestWindowPosForPopupEx(const ImVec2& ref_pos, const ImVec2& size, ImGuiDir* last_dir, const ImRect& r_outer, const ImRect& r_avoid, ImGuiPopupPositionPolicy policy)
{
    ImVec2 base_pos_clamped = ImClamp(ref_pos, r_outer.Min, r_outer.Max - size);
    if (policy == ImGuiPopupPositionPolicy_ComboBox)
    {
        const ImGuiDir dir_prefered_order[ImGuiDir_COUNT] = { ImGuiDir_Down, ImGuiDir_Right, ImGuiDir_Left, ImGuiDir_Up };
        for (int n = (*last_dir != ImGuiDir_None) ? -1 : 0; n < ImGuiDir_COUNT; n++)
        {
            const ImGuiDir dir = (n == -1) ? *last_dir : dir_prefered_order[n];
            if (n != -1 && dir == *last_dir)     
                continue;
            ImVec2 pos;
            if (dir == ImGuiDir_Down)  pos = ImVec2(r_avoid.Min.x, r_avoid.Max.y);              
            if (dir == ImGuiDir_Right) pos = ImVec2(r_avoid.Min.x, r_avoid.Min.y - size.y);    
            if (dir == ImGuiDir_Left)  pos = ImVec2(r_avoid.Max.x - size.x, r_avoid.Max.y);    
            if (dir == ImGuiDir_Up)    pos = ImVec2(r_avoid.Max.x - size.x, r_avoid.Min.y - size.y);    
            if (!r_outer.Contains(ImRect(pos, pos + size)))
                continue;
            *last_dir = dir;
            return pos;
        }
    }

    if (policy == ImGuiPopupPositionPolicy_Tooltip || policy == ImGuiPopupPositionPolicy_Default)
    {
        const ImGuiDir dir_prefered_order[ImGuiDir_COUNT] = { ImGuiDir_Right, ImGuiDir_Down, ImGuiDir_Up, ImGuiDir_Left };
        for (int n = (*last_dir != ImGuiDir_None) ? -1 : 0; n < ImGuiDir_COUNT; n++)
        {
            const ImGuiDir dir = (n == -1) ? *last_dir : dir_prefered_order[n];
            if (n != -1 && dir == *last_dir)     
                continue;

            const float avail_w = (dir == ImGuiDir_Left ? r_avoid.Min.x : r_outer.Max.x) - (dir == ImGuiDir_Right ? r_avoid.Max.x : r_outer.Min.x);
            const float avail_h = (dir == ImGuiDir_Up ? r_avoid.Min.y : r_outer.Max.y) - (dir == ImGuiDir_Down ? r_avoid.Max.y : r_outer.Min.y);

            if (avail_w < size.x && (dir == ImGuiDir_Left || dir == ImGuiDir_Right))
                continue;
            if (avail_h < size.y && (dir == ImGuiDir_Up || dir == ImGuiDir_Down))
                continue;

            ImVec2 pos;
            pos.x = (dir == ImGuiDir_Left) ? r_avoid.Min.x - size.x : (dir == ImGuiDir_Right) ? r_avoid.Max.x : base_pos_clamped.x;
            pos.y = (dir == ImGuiDir_Up) ? r_avoid.Min.y - size.y : (dir == ImGuiDir_Down) ? r_avoid.Max.y : base_pos_clamped.y;

            pos.x = ImMax(pos.x, r_outer.Min.x);
            pos.y = ImMax(pos.y, r_outer.Min.y);

            *last_dir = dir;
            return pos;
        }
    }

    *last_dir = ImGuiDir_None;

    if (policy == ImGuiPopupPositionPolicy_Tooltip)
        return ref_pos + ImVec2(2, 2);

    ImVec2 pos = ref_pos;
    pos.x = ImMax(ImMin(pos.x + size.x, r_outer.Max.x) - size.x, r_outer.Min.x);
    pos.y = ImMax(ImMin(pos.y + size.y, r_outer.Max.y) - size.y, r_outer.Min.y);
    return pos;
}

ImRect ImGui::GetWindowAllowedExtentRect(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    ImRect r_screen;
    if (window->ViewportAllowPlatformMonitorExtend >= 0)
    {
        const ImGuiPlatformMonitor& monitor = g.PlatformIO.Monitors[window->ViewportAllowPlatformMonitorExtend];
        r_screen.Min = monitor.WorkPos;
        r_screen.Max = monitor.WorkPos + monitor.WorkSize;
    }
    else
    {
        r_screen.Min = window->Viewport->Pos;
        r_screen.Max = window->Viewport->Pos + window->Viewport->Size;
    }
    ImVec2 padding = g.Style.DisplaySafeAreaPadding;
    r_screen.Expand(ImVec2((r_screen.GetWidth() > padding.x * 2) ? -padding.x : 0.0f, (r_screen.GetHeight() > padding.y * 2) ? -padding.y : 0.0f));
    return r_screen;
}

ImVec2 ImGui::FindBestWindowPosForPopup(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (window->Flags & ImGuiWindowFlags_ChildMenu)
    {
        ImGuiWindow* parent_window = window->ParentWindow;
        float horizontal_overlap = g.Style.ItemInnerSpacing.x;                      
        ImRect r_outer = GetWindowAllowedExtentRect(window);
        ImRect r_avoid;
        if (parent_window->DC.MenuBarAppending)
            r_avoid = ImRect(-FLT_MAX, parent_window->ClipRect.Min.y, FLT_MAX, parent_window->ClipRect.Max.y);                       
        else
            r_avoid = ImRect(parent_window->Pos.x + horizontal_overlap, -FLT_MAX, parent_window->Pos.x + parent_window->Size.x - horizontal_overlap - parent_window->ScrollbarSizes.x, FLT_MAX);
        return FindBestWindowPosForPopupEx(window->Pos, window->Size, &window->AutoPosLastDirection, r_outer, r_avoid, ImGuiPopupPositionPolicy_Default);
    }
    if (window->Flags & ImGuiWindowFlags_Popup)
    {
        ImRect r_outer = GetWindowAllowedExtentRect(window);
        ImRect r_avoid = ImRect(window->Pos.x - 1, window->Pos.y - 1, window->Pos.x + 1, window->Pos.y + 1);
        return FindBestWindowPosForPopupEx(window->Pos, window->Size, &window->AutoPosLastDirection, r_outer, r_avoid, ImGuiPopupPositionPolicy_Default);
    }
    if (window->Flags & ImGuiWindowFlags_Tooltip)
    {
        float sc = g.Style.MouseCursorScale;
        ImVec2 ref_pos = NavCalcPreferredRefPos();
        ImRect r_outer = GetWindowAllowedExtentRect(window);
        ImRect r_avoid;
        if (!g.NavDisableHighlight && g.NavDisableMouseHover && !(g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableSetMousePos))
            r_avoid = ImRect(ref_pos.x - 16, ref_pos.y - 8, ref_pos.x + 16, ref_pos.y + 8);
        else
            r_avoid = ImRect(ref_pos.x - 16, ref_pos.y - 8, ref_pos.x + 24 * sc, ref_pos.y + 24 * sc);              
        return FindBestWindowPosForPopupEx(ref_pos, window->Size, &window->AutoPosLastDirection, r_outer, r_avoid, ImGuiPopupPositionPolicy_Tooltip);
    }
    IM_ASSERT(0);
    return window->Pos;
}

void ImGui::SetNavID(ImGuiID id, int nav_layer, ImGuiID focus_scope_id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindow);
    IM_ASSERT(nav_layer == 0 || nav_layer == 1);
    g.NavId = id;
    g.NavFocusScopeId = focus_scope_id;
    g.NavWindow->NavLastIds[nav_layer] = id;
}

void ImGui::SetNavIDWithRectRel(ImGuiID id, int nav_layer, ImGuiID focus_scope_id, const ImRect& rect_rel)
{
    ImGuiContext& g = *GImGui;
    SetNavID(id, nav_layer, focus_scope_id);
    g.NavWindow->NavRectRel[nav_layer] = rect_rel;
    g.NavMousePosDirty = true;
    g.NavDisableHighlight = false;
    g.NavDisableMouseHover = true;
}

void ImGui::SetFocusID(ImGuiID id, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(id != 0);

    const ImGuiNavLayer nav_layer = window->DC.NavLayerCurrent;
    if (g.NavWindow != window)
        g.NavInitRequest = false;
    g.NavWindow = window;
    g.NavId = id;
    g.NavLayer = nav_layer;
    g.NavFocusScopeId = window->DC.NavFocusScopeIdCurrent;
    window->NavLastIds[nav_layer] = id;
    if (window->DC.LastItemId == id)
        window->NavRectRel[nav_layer] = ImRect(window->DC.LastItemRect.Min - window->Pos, window->DC.LastItemRect.Max - window->Pos);

    if (g.ActiveIdSource == ImGuiInputSource_Nav)
        g.NavDisableMouseHover = true;
    else
        g.NavDisableHighlight = true;
}

ImGuiDir ImGetDirQuadrantFromDelta(float dx, float dy)
{
    if (ImFabs(dx) > ImFabs(dy))
        return (dx > 0.0f) ? ImGuiDir_Right : ImGuiDir_Left;
    return (dy > 0.0f) ? ImGuiDir_Down : ImGuiDir_Up;
}

static float inline NavScoreItemDistInterval(float a0, float a1, float b0, float b1)
{
    if (a1 < b0)
        return a1 - b0;
    if (b1 < a0)
        return a0 - b1;
    return 0.0f;
}

static void inline NavClampRectToVisibleAreaForMoveDir(ImGuiDir move_dir, ImRect& r, const ImRect& clip_rect)
{
    if (move_dir == ImGuiDir_Left || move_dir == ImGuiDir_Right)
    {
        r.Min.y = ImClamp(r.Min.y, clip_rect.Min.y, clip_rect.Max.y);
        r.Max.y = ImClamp(r.Max.y, clip_rect.Min.y, clip_rect.Max.y);
    }
    else
    {
        r.Min.x = ImClamp(r.Min.x, clip_rect.Min.x, clip_rect.Max.x);
        r.Max.x = ImClamp(r.Max.x, clip_rect.Min.x, clip_rect.Max.x);
    }
}

static bool ImGui::NavScoreItem(ImGuiNavMoveResult* result, ImRect cand)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (g.NavLayer != window->DC.NavLayerCurrent)
        return false;

    const ImRect& curr = g.NavScoringRect;                      
    g.NavScoringCount++;

    if (window->ParentWindow == g.NavWindow)
    {
        IM_ASSERT((window->Flags | g.NavWindow->Flags) & ImGuiWindowFlags_NavFlattened);
        if (!window->ClipRect.Overlaps(cand))
            return false;
        cand.ClipWithFull(window->ClipRect);               
    }

    NavClampRectToVisibleAreaForMoveDir(g.NavMoveClipDir, cand, window->ClipRect);

    float dbx = NavScoreItemDistInterval(cand.Min.x, cand.Max.x, curr.Min.x, curr.Max.x);
    float dby = NavScoreItemDistInterval(ImLerp(cand.Min.y, cand.Max.y, 0.2f), ImLerp(cand.Min.y, cand.Max.y, 0.8f), ImLerp(curr.Min.y, curr.Max.y, 0.2f), ImLerp(curr.Min.y, curr.Max.y, 0.8f));             
    if (dby != 0.0f && dbx != 0.0f)
        dbx = (dbx / 1000.0f) + ((dbx > 0.0f) ? +1.0f : -1.0f);
    float dist_box = ImFabs(dbx) + ImFabs(dby);

    float dcx = (cand.Min.x + cand.Max.x) - (curr.Min.x + curr.Max.x);
    float dcy = (cand.Min.y + cand.Max.y) - (curr.Min.y + curr.Max.y);
    float dist_center = ImFabs(dcx) + ImFabs(dcy);         

    ImGuiDir quadrant;
    float dax = 0.0f, day = 0.0f, dist_axial = 0.0f;
    if (dbx != 0.0f || dby != 0.0f)
    {
        dax = dbx;
        day = dby;
        dist_axial = dist_box;
        quadrant = ImGetDirQuadrantFromDelta(dbx, dby);
    }
    else if (dcx != 0.0f || dcy != 0.0f)
    {
        dax = dcx;
        day = dcy;
        dist_axial = dist_center;
        quadrant = ImGetDirQuadrantFromDelta(dcx, dcy);
    }
    else
    {
        quadrant = (window->DC.LastItemId < g.NavId) ? ImGuiDir_Left : ImGuiDir_Right;
    }

#if IMGUI_DEBUG_NAV_SCORING
    char buf[128];
    if (IsMouseHoveringRect(cand.Min, cand.Max))
    {
        ImFormatString(buf, IM_ARRAYSIZE(buf), "dbox (%.2f,%.2f->%.4f)\ndcen (%.2f,%.2f->%.4f)\nd (%.2f,%.2f->%.4f)\nnav %c, quadrant %c", dbx, dby, dist_box, dcx, dcy, dist_center, dax, day, dist_axial, "WENS"[g.NavMoveDir], "WENS"[quadrant]);
        ImDrawList* draw_list = GetForegroundDrawList(window);
        draw_list->AddRect(curr.Min, curr.Max, IM_COL32(255,200,0,100));
        draw_list->AddRect(cand.Min, cand.Max, IM_COL32(255,255,0,200));
        draw_list->AddRectFilled(cand.Max - ImVec2(4, 4), cand.Max + CalcTextSize(buf) + ImVec2(4, 4), IM_COL32(40,0,0,150));
        draw_list->AddText(g.IO.FontDefault, 13.0f, cand.Max, ~0U, buf);
    }
    else if (g.IO.KeyCtrl)            
    {
        if (IsKeyPressedMap(ImGuiKey_C)) { g.NavMoveDirLast = (ImGuiDir)((g.NavMoveDirLast + 1) & 3); g.IO.KeysDownDuration[g.IO.KeyMap[ImGuiKey_C]] = 0.01f; }
        if (quadrant == g.NavMoveDir)
        {
            ImFormatString(buf, IM_ARRAYSIZE(buf), "%.0f/%.0f", dist_box, dist_center);
            ImDrawList* draw_list = GetForegroundDrawList(window);
            draw_list->AddRectFilled(cand.Min, cand.Max, IM_COL32(255, 0, 0, 200));
            draw_list->AddText(g.IO.FontDefault, 13.0f, cand.Min, IM_COL32(255, 255, 255, 255), buf);
        }
    }
#endif

    bool new_best = false;
    if (quadrant == g.NavMoveDir)
    {
        if (dist_box < result->DistBox)
        {
            result->DistBox = dist_box;
            result->DistCenter = dist_center;
            return true;
        }
        if (dist_box == result->DistBox)
        {
            if (dist_center < result->DistCenter)
            {
                result->DistCenter = dist_center;
                new_best = true;
            }
            else if (dist_center == result->DistCenter)
            {
                if (((g.NavMoveDir == ImGuiDir_Up || g.NavMoveDir == ImGuiDir_Down) ? dby : dbx) < 0.0f)        
                    new_best = true;
            }
        }
    }

    if (result->DistBox == FLT_MAX && dist_axial < result->DistAxial)     
        if (g.NavLayer == ImGuiNavLayer_Menu && !(g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
            if ((g.NavMoveDir == ImGuiDir_Left && dax < 0.0f) || (g.NavMoveDir == ImGuiDir_Right && dax > 0.0f) || (g.NavMoveDir == ImGuiDir_Up && day < 0.0f) || (g.NavMoveDir == ImGuiDir_Down && day > 0.0f))
            {
                result->DistAxial = dist_axial;
                new_best = true;
            }

    return new_best;
}

static void ImGui::NavApplyItemToResult(ImGuiNavMoveResult* result, ImGuiWindow* window, ImGuiID id, const ImRect& nav_bb_rel)
{
    result->Window = window;
    result->ID = id;
    result->FocusScopeId = window->DC.NavFocusScopeIdCurrent;
    result->RectRel = nav_bb_rel;
}

static void ImGui::NavProcessItem(ImGuiWindow* window, const ImRect& nav_bb, const ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    const ImGuiItemFlags item_flags = window->DC.ItemFlags;
    const ImRect nav_bb_rel(nav_bb.Min - window->Pos, nav_bb.Max - window->Pos);

    if (g.NavInitRequest && g.NavLayer == window->DC.NavLayerCurrent)
    {
        if (!(item_flags & ImGuiItemFlags_NoNavDefaultFocus) || g.NavInitResultId == 0)
        {
            g.NavInitResultId = id;
            g.NavInitResultRectRel = nav_bb_rel;
        }
        if (!(item_flags & ImGuiItemFlags_NoNavDefaultFocus))
        {
            g.NavInitRequest = false;      
            NavUpdateAnyRequestFlag();
        }
    }

    if ((g.NavId != id || (g.NavMoveRequestFlags & ImGuiNavMoveFlags_AllowCurrentNavId)) && !(item_flags & (ImGuiItemFlags_Disabled | ImGuiItemFlags_NoNav)))
    {
        ImGuiNavMoveResult* result = (window == g.NavWindow) ? &g.NavMoveResultLocal : &g.NavMoveResultOther;
#if IMGUI_DEBUG_NAV_SCORING
        if (!g.NavMoveRequest)
            g.NavMoveDir = g.NavMoveDirLast;
        bool new_best = NavScoreItem(result, nav_bb) && g.NavMoveRequest;
#else
        bool new_best = g.NavMoveRequest && NavScoreItem(result, nav_bb);
#endif
        if (new_best)
            NavApplyItemToResult(result, window, id, nav_bb_rel);

        const float VISIBLE_RATIO = 0.70f;
        if ((g.NavMoveRequestFlags & ImGuiNavMoveFlags_AlsoScoreVisibleSet) && window->ClipRect.Overlaps(nav_bb))
            if (ImClamp(nav_bb.Max.y, window->ClipRect.Min.y, window->ClipRect.Max.y) - ImClamp(nav_bb.Min.y, window->ClipRect.Min.y, window->ClipRect.Max.y) >= (nav_bb.Max.y - nav_bb.Min.y) * VISIBLE_RATIO)
                if (NavScoreItem(&g.NavMoveResultLocalVisibleSet, nav_bb))
                    NavApplyItemToResult(&g.NavMoveResultLocalVisibleSet, window, id, nav_bb_rel);
    }

    if (g.NavId == id)
    {
        g.NavWindow = window;                                                        
        g.NavLayer = window->DC.NavLayerCurrent;
        g.NavFocusScopeId = window->DC.NavFocusScopeIdCurrent;
        g.NavIdIsAlive = true;
        g.NavIdTabCounter = window->DC.FocusCounterTabStop;
        window->NavRectRel[window->DC.NavLayerCurrent] = nav_bb_rel;            
    }
}

bool ImGui::NavMoveRequestButNoResultYet()
{
    ImGuiContext& g = *GImGui;
    return g.NavMoveRequest && g.NavMoveResultLocal.ID == 0 && g.NavMoveResultOther.ID == 0;
}

void ImGui::NavMoveRequestCancel()
{
    ImGuiContext& g = *GImGui;
    g.NavMoveRequest = false;
    NavUpdateAnyRequestFlag();
}

void ImGui::NavMoveRequestForward(ImGuiDir move_dir, ImGuiDir clip_dir, const ImRect& bb_rel, ImGuiNavMoveFlags move_flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavMoveRequestForward == ImGuiNavForward_None);
    NavMoveRequestCancel();
    g.NavMoveDir = move_dir;
    g.NavMoveClipDir = clip_dir;
    g.NavMoveRequestForward = ImGuiNavForward_ForwardQueued;
    g.NavMoveRequestFlags = move_flags;
    g.NavWindow->NavRectRel[g.NavLayer] = bb_rel;
}

void ImGui::NavMoveRequestTryWrapping(ImGuiWindow* window, ImGuiNavMoveFlags move_flags)
{
    ImGuiContext& g = *GImGui;

    g.NavWrapRequestWindow = window;
    g.NavWrapRequestFlags = move_flags;
}

static void ImGui::NavSaveLastChildNavWindowIntoParent(ImGuiWindow* nav_window)
{
    ImGuiWindow* parent = nav_window;
    while (parent && parent->RootWindowDockStop != parent && (parent->Flags & ImGuiWindowFlags_ChildWindow) != 0 && (parent->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu)) == 0)
        parent = parent->ParentWindow;
    if (parent && parent != nav_window)
        parent->NavLastChildNavWindow = nav_window;
}

static ImGuiWindow* ImGui::NavRestoreLastChildNavWindow(ImGuiWindow* window)
{
    if (window->NavLastChildNavWindow && window->NavLastChildNavWindow->WasActive)
        return window->NavLastChildNavWindow;
    if (window->DockNodeAsHost && window->DockNodeAsHost->TabBar)
        if (ImGuiTabItem* tab = TabBarFindMostRecentlySelectedTabForActiveWindow(window->DockNodeAsHost->TabBar))
            return tab->Window;
    return window;
}

static void NavRestoreLayer(ImGuiNavLayer layer)
{
    ImGuiContext& g = *GImGui;
    g.NavLayer = layer;
    if (layer == 0)
        g.NavWindow = ImGui::NavRestoreLastChildNavWindow(g.NavWindow);
    ImGuiWindow* window = g.NavWindow;
    if (window->NavLastIds[layer] != 0)
        ImGui::SetNavIDWithRectRel(window->NavLastIds[layer], layer, 0, g.NavWindow->NavRectRel[layer]);
    else
        ImGui::NavInitWindow(window, true);
}

static inline void ImGui::NavUpdateAnyRequestFlag()
{
    ImGuiContext& g = *GImGui;
    g.NavAnyRequest = g.NavMoveRequest || g.NavInitRequest || (IMGUI_DEBUG_NAV_SCORING && g.NavWindow != NULL);
    if (g.NavAnyRequest)
        IM_ASSERT(g.NavWindow != NULL);
}

void ImGui::NavInitWindow(ImGuiWindow* window, bool force_reinit)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(window == g.NavWindow);
    bool init_for_nav = false;
    if (!(window->Flags & ImGuiWindowFlags_NoNavInputs))
        if (!(window->Flags & ImGuiWindowFlags_ChildWindow) || (window->Flags & ImGuiWindowFlags_Popup) || (window->NavLastIds[0] == 0) || force_reinit)
            init_for_nav = true;
    IMGUI_DEBUG_LOG_NAV("[nav] NavInitRequest: from NavInitWindow(), init_for_nav=%d, window=\"%s\", layer=%d\n", init_for_nav, window->Name, g.NavLayer);
    if (init_for_nav)
    {
        SetNavID(0, g.NavLayer, 0);
        g.NavInitRequest = true;
        g.NavInitRequestFromMove = false;
        g.NavInitResultId = 0;
        g.NavInitResultRectRel = ImRect();
        NavUpdateAnyRequestFlag();
    }
    else
    {
        g.NavId = window->NavLastIds[0];
        g.NavFocusScopeId = 0;
    }
}

static ImVec2 ImGui::NavCalcPreferredRefPos()
{
    ImGuiContext& g = *GImGui;
    if (g.NavDisableHighlight || !g.NavDisableMouseHover || !g.NavWindow)
    {
        if (IsMousePosValid(&g.IO.MousePos))
            return g.IO.MousePos;
        return g.LastValidMousePos;
    }
    else
    {
        const ImRect& rect_rel = g.NavWindow->NavRectRel[g.NavLayer];
        ImVec2 pos = g.NavWindow->Pos + ImVec2(rect_rel.Min.x + ImMin(g.Style.FramePadding.x * 4, rect_rel.GetWidth()), rect_rel.Max.y - ImMin(g.Style.FramePadding.y, rect_rel.GetHeight()));
        ImRect visible_rect = g.NavWindow->Viewport->GetMainRect();
        return ImFloor(ImClamp(pos, visible_rect.Min, visible_rect.Max));                      
    }
}

float ImGui::GetNavInputAmount(ImGuiNavInput n, ImGuiInputReadMode mode)
{
    ImGuiContext& g = *GImGui;
    if (mode == ImGuiInputReadMode_Down)
        return g.IO.NavInputs[n];                                  

    const float t = g.IO.NavInputsDownDuration[n];
    if (t < 0.0f && mode == ImGuiInputReadMode_Released)            
        return (g.IO.NavInputsDownDurationPrev[n] >= 0.0f ? 1.0f : 0.0f);
    if (t < 0.0f)
        return 0.0f;
    if (mode == ImGuiInputReadMode_Pressed)                         
        return (t == 0.0f) ? 1.0f : 0.0f;
    if (mode == ImGuiInputReadMode_Repeat)
        return (float)CalcTypematicRepeatAmount(t - g.IO.DeltaTime, t, g.IO.KeyRepeatDelay * 0.72f, g.IO.KeyRepeatRate * 0.80f);
    if (mode == ImGuiInputReadMode_RepeatSlow)
        return (float)CalcTypematicRepeatAmount(t - g.IO.DeltaTime, t, g.IO.KeyRepeatDelay * 1.25f, g.IO.KeyRepeatRate * 2.00f);
    if (mode == ImGuiInputReadMode_RepeatFast)
        return (float)CalcTypematicRepeatAmount(t - g.IO.DeltaTime, t, g.IO.KeyRepeatDelay * 0.72f, g.IO.KeyRepeatRate * 0.30f);
    return 0.0f;
}

ImVec2 ImGui::GetNavInputAmount2d(ImGuiNavDirSourceFlags dir_sources, ImGuiInputReadMode mode, float slow_factor, float fast_factor)
{
    ImVec2 delta(0.0f, 0.0f);
    if (dir_sources & ImGuiNavDirSourceFlags_Keyboard)
        delta += ImVec2(GetNavInputAmount(ImGuiNavInput_KeyRight_, mode)   - GetNavInputAmount(ImGuiNavInput_KeyLeft_,   mode), GetNavInputAmount(ImGuiNavInput_KeyDown_,   mode) - GetNavInputAmount(ImGuiNavInput_KeyUp_,   mode));
    if (dir_sources & ImGuiNavDirSourceFlags_PadDPad)
        delta += ImVec2(GetNavInputAmount(ImGuiNavInput_DpadRight, mode)   - GetNavInputAmount(ImGuiNavInput_DpadLeft,   mode), GetNavInputAmount(ImGuiNavInput_DpadDown,   mode) - GetNavInputAmount(ImGuiNavInput_DpadUp,   mode));
    if (dir_sources & ImGuiNavDirSourceFlags_PadLStick)
        delta += ImVec2(GetNavInputAmount(ImGuiNavInput_LStickRight, mode) - GetNavInputAmount(ImGuiNavInput_LStickLeft, mode), GetNavInputAmount(ImGuiNavInput_LStickDown, mode) - GetNavInputAmount(ImGuiNavInput_LStickUp, mode));
    if (slow_factor != 0.0f && IsNavInputDown(ImGuiNavInput_TweakSlow))
        delta *= slow_factor;
    if (fast_factor != 0.0f && IsNavInputDown(ImGuiNavInput_TweakFast))
        delta *= fast_factor;
    return delta;
}

static void ImGui::NavUpdate()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    io.WantSetMousePos = false;
    g.NavWrapRequestWindow = NULL;
    g.NavWrapRequestFlags = ImGuiNavMoveFlags_None;
#if 0
    if (g.NavScoringCount > 0) IMGUI_DEBUG_LOG("NavScoringCount %d for '%s' layer %d (Init:%d, Move:%d)\n", g.FrameCount, g.NavScoringCount, g.NavWindow ? g.NavWindow->Name : "NULL", g.NavLayer, g.NavInitRequest || g.NavInitResultId != 0, g.NavMoveRequest);
#endif

    bool nav_keyboard_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
    bool nav_gamepad_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
    if (nav_gamepad_active && g.NavInputSource != ImGuiInputSource_NavGamepad)
    {
        if (io.NavInputs[ImGuiNavInput_Activate] > 0.0f || io.NavInputs[ImGuiNavInput_Input] > 0.0f || io.NavInputs[ImGuiNavInput_Cancel] > 0.0f || io.NavInputs[ImGuiNavInput_Menu] > 0.0f
            || io.NavInputs[ImGuiNavInput_DpadLeft] > 0.0f || io.NavInputs[ImGuiNavInput_DpadRight] > 0.0f || io.NavInputs[ImGuiNavInput_DpadUp] > 0.0f || io.NavInputs[ImGuiNavInput_DpadDown] > 0.0f)
            g.NavInputSource = ImGuiInputSource_NavGamepad;
    }

    if (nav_keyboard_active)
    {
        #define NAV_MAP_KEY(_KEY, _NAV_INPUT)  do { if (IsKeyDown(io.KeyMap[_KEY])) { io.NavInputs[_NAV_INPUT] = 1.0f; g.NavInputSource = ImGuiInputSource_NavKeyboard; } } while (0)
        NAV_MAP_KEY(ImGuiKey_Space,     ImGuiNavInput_Activate );
        NAV_MAP_KEY(ImGuiKey_Enter,     ImGuiNavInput_Input    );
        NAV_MAP_KEY(ImGuiKey_Escape,    ImGuiNavInput_Cancel   );
        NAV_MAP_KEY(ImGuiKey_LeftArrow, ImGuiNavInput_KeyLeft_ );
        NAV_MAP_KEY(ImGuiKey_RightArrow,ImGuiNavInput_KeyRight_);
        NAV_MAP_KEY(ImGuiKey_UpArrow,   ImGuiNavInput_KeyUp_   );
        NAV_MAP_KEY(ImGuiKey_DownArrow, ImGuiNavInput_KeyDown_ );
        if (io.KeyCtrl)
            io.NavInputs[ImGuiNavInput_TweakSlow] = 1.0f;
        if (io.KeyShift)
            io.NavInputs[ImGuiNavInput_TweakFast] = 1.0f;
        if (io.KeyAlt && !io.KeyCtrl)                 
            io.NavInputs[ImGuiNavInput_KeyMenu_]  = 1.0f;
        #undef NAV_MAP_KEY
    }
    memcpy(io.NavInputsDownDurationPrev, io.NavInputsDownDuration, sizeof(io.NavInputsDownDuration));
    for (int i = 0; i < IM_ARRAYSIZE(io.NavInputs); i++)
        io.NavInputsDownDuration[i] = (io.NavInputs[i] > 0.0f) ? (io.NavInputsDownDuration[i] < 0.0f ? 0.0f : io.NavInputsDownDuration[i] + io.DeltaTime) : -1.0f;

    if (g.NavInitResultId != 0 && (!g.NavDisableHighlight || g.NavInitRequestFromMove))
        NavUpdateInitResult();
    g.NavInitRequest = false;
    g.NavInitRequestFromMove = false;
    g.NavInitResultId = 0;
    g.NavJustMovedToId = 0;

    if (g.NavMoveRequest)
        NavUpdateMoveResult();

    if (g.NavMoveRequestForward == ImGuiNavForward_ForwardActive)
    {
        IM_ASSERT(g.NavMoveRequest);
        if (g.NavMoveResultLocal.ID == 0 && g.NavMoveResultOther.ID == 0)
            g.NavDisableHighlight = false;
        g.NavMoveRequestForward = ImGuiNavForward_None;
    }

    if (g.NavMousePosDirty && g.NavIdIsAlive)
    {
        if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableSetMousePos) && (io.BackendFlags & ImGuiBackendFlags_HasSetMousePos))
        {
            if (!g.NavDisableHighlight && g.NavDisableMouseHover && g.NavWindow)
            {
                io.MousePos = io.MousePosPrev = NavCalcPreferredRefPos();
                io.WantSetMousePos = true;
            }
        }
        g.NavMousePosDirty = false;
    }
    g.NavIdIsAlive = false;
    g.NavJustTabbedId = 0;
    IM_ASSERT(g.NavLayer == 0 || g.NavLayer == 1);

    if (g.NavWindow)
        NavSaveLastChildNavWindowIntoParent(g.NavWindow);
    if (g.NavWindow && g.NavWindow->NavLastChildNavWindow != NULL && g.NavLayer == ImGuiNavLayer_Main)
        g.NavWindow->NavLastChildNavWindow = NULL;

    NavUpdateWindowing();

    io.NavActive = (nav_keyboard_active || nav_gamepad_active) && g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs);
    io.NavVisible = (io.NavActive && g.NavId != 0 && !g.NavDisableHighlight) || (g.NavWindowingTarget != NULL);

    if (IsNavInputTest(ImGuiNavInput_Cancel, ImGuiInputReadMode_Pressed))
    {
        IMGUI_DEBUG_LOG_NAV("[nav] ImGuiNavInput_Cancel\n");
        if (g.ActiveId != 0)
        {
            if (!IsActiveIdUsingNavInput(ImGuiNavInput_Cancel))
                ClearActiveID();
        }
        else if (g.NavWindow && (g.NavWindow->Flags & ImGuiWindowFlags_ChildWindow) && !(g.NavWindow->Flags & ImGuiWindowFlags_Popup) && g.NavWindow->ParentWindow && g.NavWindow != g.NavWindow->RootWindowDockStop)
        {
            ImGuiWindow* child_window = g.NavWindow;
            ImGuiWindow* parent_window = g.NavWindow->ParentWindow;
            IM_ASSERT(child_window->ChildId != 0);
            FocusWindow(parent_window);
            SetNavID(child_window->ChildId, 0, 0);
            g.NavIdIsAlive = false;      
            if (g.NavDisableMouseHover)
                g.NavMousePosDirty = true;
        }
        else if (g.OpenPopupStack.Size > 0)
        {
            if (!(g.OpenPopupStack.back().Window->Flags & ImGuiWindowFlags_Modal))
                ClosePopupToLevel(g.OpenPopupStack.Size - 1, true);
        }
        else if (g.NavLayer != ImGuiNavLayer_Main)
        {
            NavRestoreLayer(ImGuiNavLayer_Main);
        }
        else
        {
            if (g.NavWindow && ((g.NavWindow->Flags & ImGuiWindowFlags_Popup) || !(g.NavWindow->Flags & ImGuiWindowFlags_ChildWindow)))
                g.NavWindow->NavLastIds[0] = 0;
            g.NavId = g.NavFocusScopeId = 0;
        }
    }

    g.NavActivateId = g.NavActivateDownId = g.NavActivatePressedId = g.NavInputId = 0;
    if (g.NavId != 0 && !g.NavDisableHighlight && !g.NavWindowingTarget && g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
    {
        bool activate_down = IsNavInputDown(ImGuiNavInput_Activate);
        bool activate_pressed = activate_down && IsNavInputTest(ImGuiNavInput_Activate, ImGuiInputReadMode_Pressed);
        if (g.ActiveId == 0 && activate_pressed)
            g.NavActivateId = g.NavId;
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && activate_down)
            g.NavActivateDownId = g.NavId;
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && activate_pressed)
            g.NavActivatePressedId = g.NavId;
        if ((g.ActiveId == 0 || g.ActiveId == g.NavId) && IsNavInputTest(ImGuiNavInput_Input, ImGuiInputReadMode_Pressed))
            g.NavInputId = g.NavId;
    }
    if (g.NavWindow && (g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
        g.NavDisableHighlight = true;
    if (g.NavActivateId != 0)
        IM_ASSERT(g.NavActivateDownId == g.NavActivateId);
    g.NavMoveRequest = false;

    if (g.NavNextActivateId != 0)
        g.NavActivateId = g.NavActivateDownId = g.NavActivatePressedId = g.NavInputId = g.NavNextActivateId;
    g.NavNextActivateId = 0;

    if (g.NavMoveRequestForward == ImGuiNavForward_None)
    {
        g.NavMoveDir = ImGuiDir_None;
        g.NavMoveRequestFlags = ImGuiNavMoveFlags_None;
        if (g.NavWindow && !g.NavWindowingTarget && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs))
        {
            const ImGuiInputReadMode read_mode = ImGuiInputReadMode_Repeat;
            if (!IsActiveIdUsingNavDir(ImGuiDir_Left)  && (IsNavInputTest(ImGuiNavInput_DpadLeft,  read_mode) || IsNavInputTest(ImGuiNavInput_KeyLeft_,  read_mode))) { g.NavMoveDir = ImGuiDir_Left; }
            if (!IsActiveIdUsingNavDir(ImGuiDir_Right) && (IsNavInputTest(ImGuiNavInput_DpadRight, read_mode) || IsNavInputTest(ImGuiNavInput_KeyRight_, read_mode))) { g.NavMoveDir = ImGuiDir_Right; }
            if (!IsActiveIdUsingNavDir(ImGuiDir_Up)    && (IsNavInputTest(ImGuiNavInput_DpadUp,    read_mode) || IsNavInputTest(ImGuiNavInput_KeyUp_,    read_mode))) { g.NavMoveDir = ImGuiDir_Up; }
            if (!IsActiveIdUsingNavDir(ImGuiDir_Down)  && (IsNavInputTest(ImGuiNavInput_DpadDown,  read_mode) || IsNavInputTest(ImGuiNavInput_KeyDown_,  read_mode))) { g.NavMoveDir = ImGuiDir_Down; }
        }
        g.NavMoveClipDir = g.NavMoveDir;
    }
    else
    {
        IM_ASSERT(g.NavMoveDir != ImGuiDir_None && g.NavMoveClipDir != ImGuiDir_None);
        IM_ASSERT(g.NavMoveRequestForward == ImGuiNavForward_ForwardQueued);
        IMGUI_DEBUG_LOG_NAV("[nav] NavMoveRequestForward %d\n", g.NavMoveDir);
        g.NavMoveRequestForward = ImGuiNavForward_ForwardActive;
    }

    float nav_scoring_rect_offset_y = 0.0f;
    if (nav_keyboard_active)
        nav_scoring_rect_offset_y = NavUpdatePageUpPageDown();

    if (g.NavMoveDir != ImGuiDir_None)
    {
        g.NavMoveRequest = true;
        g.NavMoveRequestKeyMods = io.KeyMods;
        g.NavMoveDirLast = g.NavMoveDir;
    }
    if (g.NavMoveRequest && g.NavId == 0)
    {
        IMGUI_DEBUG_LOG_NAV("[nav] NavInitRequest: from move, window \"%s\", layer=%d\n", g.NavWindow->Name, g.NavLayer);
        g.NavInitRequest = g.NavInitRequestFromMove = true;
        g.NavInitResultId = 0;      
        g.NavDisableHighlight = false;
    }
    NavUpdateAnyRequestFlag();

    if (g.NavWindow && !(g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs) && !g.NavWindowingTarget)
    {
        ImGuiWindow* window = g.NavWindow;
        const float scroll_speed = IM_ROUND(window->CalcFontSize() * 100 * io.DeltaTime);             
        if (window->DC.NavLayerActiveMask == 0x00 && window->DC.NavHasScroll && g.NavMoveRequest)
        {
            if (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right)
                SetScrollX(window, ImFloor(window->Scroll.x + ((g.NavMoveDir == ImGuiDir_Left) ? -1.0f : +1.0f) * scroll_speed));
            if (g.NavMoveDir == ImGuiDir_Up || g.NavMoveDir == ImGuiDir_Down)
                SetScrollY(window, ImFloor(window->Scroll.y + ((g.NavMoveDir == ImGuiDir_Up) ? -1.0f : +1.0f) * scroll_speed));
        }

        ImVec2 scroll_dir = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadLStick, ImGuiInputReadMode_Down, 1.0f / 10.0f, 10.0f);
        if (scroll_dir.x != 0.0f && window->ScrollbarX)
            SetScrollX(window, ImFloor(window->Scroll.x + scroll_dir.x * scroll_speed));
        if (scroll_dir.y != 0.0f)
            SetScrollY(window, ImFloor(window->Scroll.y + scroll_dir.y * scroll_speed));
    }

    g.NavMoveResultLocal.Clear();
    g.NavMoveResultLocalVisibleSet.Clear();
    g.NavMoveResultOther.Clear();

    if (g.NavMoveRequest && g.NavInputSource == ImGuiInputSource_NavGamepad && g.NavLayer == ImGuiNavLayer_Main)
    {
        ImGuiWindow* window = g.NavWindow;
        ImRect window_rect_rel(window->InnerRect.Min - window->Pos - ImVec2(1, 1), window->InnerRect.Max - window->Pos + ImVec2(1, 1));
        if (!window_rect_rel.Contains(window->NavRectRel[g.NavLayer]))
        {
            IMGUI_DEBUG_LOG_NAV("[nav] NavMoveRequest: clamp NavRectRel\n");
            float pad = window->CalcFontSize() * 0.5f;
            window_rect_rel.Expand(ImVec2(-ImMin(window_rect_rel.GetWidth(), pad), -ImMin(window_rect_rel.GetHeight(), pad)));              
            window->NavRectRel[g.NavLayer].ClipWithFull(window_rect_rel);
            g.NavId = g.NavFocusScopeId = 0;
        }
    }

    ImRect nav_rect_rel = (g.NavWindow && !g.NavWindow->NavRectRel[g.NavLayer].IsInverted()) ? g.NavWindow->NavRectRel[g.NavLayer] : ImRect(0, 0, 0, 0);
    g.NavScoringRect = g.NavWindow ? ImRect(g.NavWindow->Pos + nav_rect_rel.Min, g.NavWindow->Pos + nav_rect_rel.Max) : ImRect(0, 0, 0, 0);
    g.NavScoringRect.TranslateY(nav_scoring_rect_offset_y);
    g.NavScoringRect.Min.x = ImMin(g.NavScoringRect.Min.x + 1.0f, g.NavScoringRect.Max.x);
    g.NavScoringRect.Max.x = g.NavScoringRect.Min.x;
    IM_ASSERT(!g.NavScoringRect.IsInverted());                     
    g.NavScoringCount = 0;
#if IMGUI_DEBUG_NAV_RECTS
    if (g.NavWindow)
    {
        ImDrawList* draw_list = GetForegroundDrawList(g.NavWindow);
        if (1) { for (int layer = 0; layer < 2; layer++) draw_list->AddRect(g.NavWindow->Pos + g.NavWindow->NavRectRel[layer].Min, g.NavWindow->Pos + g.NavWindow->NavRectRel[layer].Max, IM_COL32(255,200,0,255)); }  
        if (1) { ImU32 col = (!g.NavWindow->Hidden) ? IM_COL32(255,0,255,255) : IM_COL32(255,0,0,255); ImVec2 p = NavCalcPreferredRefPos(); char buf[32]; ImFormatString(buf, 32, "%d", g.NavLayer); draw_list->AddCircleFilled(p, 3.0f, col); draw_list->AddText(NULL, 13.0f, p + ImVec2(8,-4), col, buf); }
    }
#endif
}

static void ImGui::NavUpdateInitResult()
{
    ImGuiContext& g = *GImGui;
    if (!g.NavWindow)
        return;

    IMGUI_DEBUG_LOG_NAV("[nav] NavInitRequest: result NavID 0x%08X in Layer %d Window \"%s\"\n", g.NavInitResultId, g.NavLayer, g.NavWindow->Name);
    if (g.NavInitRequestFromMove)
        SetNavIDWithRectRel(g.NavInitResultId, g.NavLayer, 0, g.NavInitResultRectRel);
    else
        SetNavID(g.NavInitResultId, g.NavLayer, 0);
    g.NavWindow->NavRectRel[g.NavLayer] = g.NavInitResultRectRel;
}

static void ImGui::NavUpdateMoveResult()
{
    ImGuiContext& g = *GImGui;
    if (g.NavMoveResultLocal.ID == 0 && g.NavMoveResultOther.ID == 0)
    {
        if (g.NavId != 0)
        {
            g.NavDisableHighlight = false;
            g.NavDisableMouseHover = true;
        }
        return;
    }

    ImGuiNavMoveResult* result = (g.NavMoveResultLocal.ID != 0) ? &g.NavMoveResultLocal : &g.NavMoveResultOther;

    if (g.NavMoveRequestFlags & ImGuiNavMoveFlags_AlsoScoreVisibleSet)
        if (g.NavMoveResultLocalVisibleSet.ID != 0 && g.NavMoveResultLocalVisibleSet.ID != g.NavId)
            result = &g.NavMoveResultLocalVisibleSet;

    if (result != &g.NavMoveResultOther && g.NavMoveResultOther.ID != 0 && g.NavMoveResultOther.Window->ParentWindow == g.NavWindow)
        if ((g.NavMoveResultOther.DistBox < result->DistBox) || (g.NavMoveResultOther.DistBox == result->DistBox && g.NavMoveResultOther.DistCenter < result->DistCenter))
            result = &g.NavMoveResultOther;
    IM_ASSERT(g.NavWindow && result->Window);

    if (g.NavLayer == ImGuiNavLayer_Main)
    {
        ImVec2 delta_scroll;
        if (g.NavMoveRequestFlags & ImGuiNavMoveFlags_ScrollToEdge)
        {
            float scroll_target = (g.NavMoveDir == ImGuiDir_Up) ? result->Window->ScrollMax.y : 0.0f;
            delta_scroll.y = result->Window->Scroll.y - scroll_target;
            SetScrollY(result->Window, scroll_target);
        }
        else
        {
            ImRect rect_abs = ImRect(result->RectRel.Min + result->Window->Pos, result->RectRel.Max + result->Window->Pos);
            delta_scroll = ScrollToBringRectIntoView(result->Window, rect_abs);
        }

        result->RectRel.TranslateX(-delta_scroll.x);
        result->RectRel.TranslateY(-delta_scroll.y);
    }

    ClearActiveID();
    g.NavWindow = result->Window;
    if (g.NavId != result->ID)
    {
        g.NavJustMovedToId = result->ID;
        g.NavJustMovedToFocusScopeId = result->FocusScopeId;
        g.NavJustMovedToKeyMods = g.NavMoveRequestKeyMods;
    }
    IMGUI_DEBUG_LOG_NAV("[nav] NavMoveRequest: result NavID 0x%08X in Layer %d Window \"%s\"\n", result->ID, g.NavLayer, g.NavWindow->Name);
    SetNavIDWithRectRel(result->ID, g.NavLayer, result->FocusScopeId, result->RectRel);
}

static float ImGui::NavUpdatePageUpPageDown()
{
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;

    if (g.NavMoveDir != ImGuiDir_None || g.NavWindow == NULL)
        return 0.0f;
    if ((g.NavWindow->Flags & ImGuiWindowFlags_NoNavInputs) || g.NavWindowingTarget != NULL || g.NavLayer != ImGuiNavLayer_Main)
        return 0.0f;

    ImGuiWindow* window = g.NavWindow;
    const bool page_up_held = IsKeyDown(io.KeyMap[ImGuiKey_PageUp]) && !IsActiveIdUsingKey(ImGuiKey_PageUp);
    const bool page_down_held = IsKeyDown(io.KeyMap[ImGuiKey_PageDown]) && !IsActiveIdUsingKey(ImGuiKey_PageDown);
    const bool home_pressed = IsKeyPressed(io.KeyMap[ImGuiKey_Home]) && !IsActiveIdUsingKey(ImGuiKey_Home);
    const bool end_pressed = IsKeyPressed(io.KeyMap[ImGuiKey_End]) && !IsActiveIdUsingKey(ImGuiKey_End);
    if (page_up_held != page_down_held || home_pressed != end_pressed)       
    {
        if (window->DC.NavLayerActiveMask == 0x00 && window->DC.NavHasScroll)
        {
            if (IsKeyPressed(io.KeyMap[ImGuiKey_PageUp], true))
                SetScrollY(window, window->Scroll.y - window->InnerRect.GetHeight());
            else if (IsKeyPressed(io.KeyMap[ImGuiKey_PageDown], true))
                SetScrollY(window, window->Scroll.y + window->InnerRect.GetHeight());
            else if (home_pressed)
                SetScrollY(window, 0.0f);
            else if (end_pressed)
                SetScrollY(window, window->ScrollMax.y);
        }
        else
        {
            ImRect& nav_rect_rel = window->NavRectRel[g.NavLayer];
            const float page_offset_y = ImMax(0.0f, window->InnerRect.GetHeight() - window->CalcFontSize() * 1.0f + nav_rect_rel.GetHeight());
            float nav_scoring_rect_offset_y = 0.0f;
            if (IsKeyPressed(io.KeyMap[ImGuiKey_PageUp], true))
            {
                nav_scoring_rect_offset_y = -page_offset_y;
                g.NavMoveDir = ImGuiDir_Down;                      
                g.NavMoveClipDir = ImGuiDir_Up;
                g.NavMoveRequestFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_AlsoScoreVisibleSet;
            }
            else if (IsKeyPressed(io.KeyMap[ImGuiKey_PageDown], true))
            {
                nav_scoring_rect_offset_y = +page_offset_y;
                g.NavMoveDir = ImGuiDir_Up;                      
                g.NavMoveClipDir = ImGuiDir_Down;
                g.NavMoveRequestFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_AlsoScoreVisibleSet;
            }
            else if (home_pressed)
            {
                nav_rect_rel.Min.y = nav_rect_rel.Max.y = -window->Scroll.y;
                if (nav_rect_rel.IsInverted())
                    nav_rect_rel.Min.x = nav_rect_rel.Max.x = 0.0f;
                g.NavMoveDir = ImGuiDir_Down;
                g.NavMoveRequestFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_ScrollToEdge;
            }
            else if (end_pressed)
            {
                nav_rect_rel.Min.y = nav_rect_rel.Max.y = window->ScrollMax.y + window->SizeFull.y - window->Scroll.y;
                if (nav_rect_rel.IsInverted())
                    nav_rect_rel.Min.x = nav_rect_rel.Max.x = 0.0f;
                g.NavMoveDir = ImGuiDir_Up;
                g.NavMoveRequestFlags = ImGuiNavMoveFlags_AllowCurrentNavId | ImGuiNavMoveFlags_ScrollToEdge;
            }
            return nav_scoring_rect_offset_y;
        }
    }
    return 0.0f;
}

static void ImGui::NavEndFrame()
{
    ImGuiContext& g = *GImGui;

    if (g.NavWindowingTarget != NULL)
        NavUpdateWindowingOverlay();

    ImGuiWindow* window = g.NavWrapRequestWindow;
    ImGuiNavMoveFlags move_flags = g.NavWrapRequestFlags;
    if (window != NULL && g.NavWindow == window && NavMoveRequestButNoResultYet() && g.NavMoveRequestForward == ImGuiNavForward_None && g.NavLayer == ImGuiNavLayer_Main)
    {
        IM_ASSERT(move_flags != 0);        
        ImRect bb_rel = window->NavRectRel[0];

        ImGuiDir clip_dir = g.NavMoveDir;
        if (g.NavMoveDir == ImGuiDir_Left && (move_flags & (ImGuiNavMoveFlags_WrapX | ImGuiNavMoveFlags_LoopX)))
        {
            bb_rel.Min.x = bb_rel.Max.x =
                ImMax(window->SizeFull.x, window->ContentSize.x + window->WindowPadding.x * 2.0f) - window->Scroll.x;
            if (move_flags & ImGuiNavMoveFlags_WrapX)
            {
                bb_rel.TranslateY(-bb_rel.GetHeight());
                clip_dir = ImGuiDir_Up;
            }
            NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
        }
        if (g.NavMoveDir == ImGuiDir_Right && (move_flags & (ImGuiNavMoveFlags_WrapX | ImGuiNavMoveFlags_LoopX)))
        {
            bb_rel.Min.x = bb_rel.Max.x = -window->Scroll.x;
            if (move_flags & ImGuiNavMoveFlags_WrapX)
            {
                bb_rel.TranslateY(+bb_rel.GetHeight());
                clip_dir = ImGuiDir_Down;
            }
            NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
        }
        if (g.NavMoveDir == ImGuiDir_Up && (move_flags & (ImGuiNavMoveFlags_WrapY | ImGuiNavMoveFlags_LoopY)))
        {
            bb_rel.Min.y = bb_rel.Max.y =
                ImMax(window->SizeFull.y, window->ContentSize.y + window->WindowPadding.y * 2.0f) - window->Scroll.y;
            if (move_flags & ImGuiNavMoveFlags_WrapY)
            {
                bb_rel.TranslateX(-bb_rel.GetWidth());
                clip_dir = ImGuiDir_Left;
            }
            NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
        }
        if (g.NavMoveDir == ImGuiDir_Down && (move_flags & (ImGuiNavMoveFlags_WrapY | ImGuiNavMoveFlags_LoopY)))
        {
            bb_rel.Min.y = bb_rel.Max.y = -window->Scroll.y;
            if (move_flags & ImGuiNavMoveFlags_WrapY)
            {
                bb_rel.TranslateX(+bb_rel.GetWidth());
                clip_dir = ImGuiDir_Right;
            }
            NavMoveRequestForward(g.NavMoveDir, clip_dir, bb_rel, move_flags);
        }
    }
}

static int ImGui::FindWindowFocusIndex(ImGuiWindow* window)   
{
    ImGuiContext& g = *GImGui;
    for (int i = g.WindowsFocusOrder.Size - 1; i >= 0; i--)
        if (g.WindowsFocusOrder[i] == window)
            return i;
    return -1;
}

static ImGuiWindow* FindWindowNavFocusable(int i_start, int i_stop, int dir)   
{
    ImGuiContext& g = *GImGui;
    for (int i = i_start; i >= 0 && i < g.WindowsFocusOrder.Size && i != i_stop; i += dir)
        if (ImGui::IsWindowNavFocusable(g.WindowsFocusOrder[i]))
            return g.WindowsFocusOrder[i];
    return NULL;
}

static void NavUpdateWindowingHighlightWindow(int focus_change_dir)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindowingTarget);
    if (g.NavWindowingTarget->Flags & ImGuiWindowFlags_Modal)
        return;

    const int i_current = ImGui::FindWindowFocusIndex(g.NavWindowingTarget);
    ImGuiWindow* window_target = FindWindowNavFocusable(i_current + focus_change_dir, -INT_MAX, focus_change_dir);
    if (!window_target)
        window_target = FindWindowNavFocusable((focus_change_dir < 0) ? (g.WindowsFocusOrder.Size - 1) : 0, i_current, focus_change_dir);
    if (window_target)             
        g.NavWindowingTarget = g.NavWindowingTargetAnim = window_target;
    g.NavWindowingToggleLayer = false;
}

static void ImGui::NavUpdateWindowing()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* apply_focus_window = NULL;
    bool apply_toggle_layer = false;

    ImGuiWindow* modal_window = GetTopMostPopupModal();
    bool allow_windowing = (modal_window == NULL);
    if (!allow_windowing)
        g.NavWindowingTarget = NULL;

    if (g.NavWindowingTargetAnim && g.NavWindowingTarget == NULL)
    {
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha - g.IO.DeltaTime * 10.0f, 0.0f);
        if (g.DimBgRatio <= 0.0f && g.NavWindowingHighlightAlpha <= 0.0f)
            g.NavWindowingTargetAnim = NULL;
    }

    bool start_windowing_with_gamepad = allow_windowing && !g.NavWindowingTarget && IsNavInputTest(ImGuiNavInput_Menu, ImGuiInputReadMode_Pressed);
    bool start_windowing_with_keyboard = allow_windowing && !g.NavWindowingTarget && g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_Tab) && (g.IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard);
    if (start_windowing_with_gamepad || start_windowing_with_keyboard)
        if (ImGuiWindow* window = g.NavWindow ? g.NavWindow : FindWindowNavFocusable(g.WindowsFocusOrder.Size - 1, -INT_MAX, -1))
        {
            g.NavWindowingTarget = g.NavWindowingTargetAnim = window->RootWindowDockStop;
            g.NavWindowingTimer = g.NavWindowingHighlightAlpha = 0.0f;
            g.NavWindowingToggleLayer = start_windowing_with_keyboard ? false : true;
            g.NavInputSource = start_windowing_with_keyboard ? ImGuiInputSource_NavKeyboard : ImGuiInputSource_NavGamepad;
        }

    g.NavWindowingTimer += g.IO.DeltaTime;
    if (g.NavWindowingTarget && g.NavInputSource == ImGuiInputSource_NavGamepad)
    {
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha, ImSaturate((g.NavWindowingTimer - NAV_WINDOWING_HIGHLIGHT_DELAY) / 0.05f));

        const int focus_change_dir = (int)IsNavInputTest(ImGuiNavInput_FocusPrev, ImGuiInputReadMode_RepeatSlow) - (int)IsNavInputTest(ImGuiNavInput_FocusNext, ImGuiInputReadMode_RepeatSlow);
        if (focus_change_dir != 0)
        {
            NavUpdateWindowingHighlightWindow(focus_change_dir);
            g.NavWindowingHighlightAlpha = 1.0f;
        }

        if (!IsNavInputDown(ImGuiNavInput_Menu))
        {
            g.NavWindowingToggleLayer &= (g.NavWindowingHighlightAlpha < 1.0f);               
            if (g.NavWindowingToggleLayer && g.NavWindow)
                apply_toggle_layer = true;
            else if (!g.NavWindowingToggleLayer)
                apply_focus_window = g.NavWindowingTarget;
            g.NavWindowingTarget = NULL;
        }
    }

    if (g.NavWindowingTarget && g.NavInputSource == ImGuiInputSource_NavKeyboard)
    {
        g.NavWindowingHighlightAlpha = ImMax(g.NavWindowingHighlightAlpha, ImSaturate((g.NavWindowingTimer - NAV_WINDOWING_HIGHLIGHT_DELAY) / 0.05f));  
        if (IsKeyPressedMap(ImGuiKey_Tab, true))
            NavUpdateWindowingHighlightWindow(g.IO.KeyShift ? +1 : -1);
        if (!g.IO.KeyCtrl)
            apply_focus_window = g.NavWindowingTarget;
    }

    if (IsNavInputTest(ImGuiNavInput_KeyMenu_, ImGuiInputReadMode_Pressed))
        g.NavWindowingToggleLayer = true;
    if ((g.ActiveId == 0 || g.ActiveIdAllowOverlap) && g.NavWindowingToggleLayer && IsNavInputTest(ImGuiNavInput_KeyMenu_, ImGuiInputReadMode_Released))
        if (IsMousePosValid(&g.IO.MousePos) == IsMousePosValid(&g.IO.MousePosPrev))
            apply_toggle_layer = true;

    if (g.NavWindowingTarget && !(g.NavWindowingTarget->Flags & ImGuiWindowFlags_NoMove))
    {
        ImVec2 move_delta;
        if (g.NavInputSource == ImGuiInputSource_NavKeyboard && !g.IO.KeyShift)
            move_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard, ImGuiInputReadMode_Down);
        if (g.NavInputSource == ImGuiInputSource_NavGamepad)
            move_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadLStick, ImGuiInputReadMode_Down);
        if (move_delta.x != 0.0f || move_delta.y != 0.0f)
        {
            const float NAV_MOVE_SPEED = 800.0f;
            const float move_speed = ImFloor(NAV_MOVE_SPEED * g.IO.DeltaTime * ImMin(g.IO.DisplayFramebufferScale.x, g.IO.DisplayFramebufferScale.y));        
            ImGuiWindow* moving_window = g.NavWindowingTarget->RootWindow;
            SetWindowPos(moving_window, moving_window->Pos + move_delta * move_speed, ImGuiCond_Always);
            MarkIniSettingsDirty(moving_window);
            g.NavDisableMouseHover = true;
        }
    }

    if (apply_focus_window && (g.NavWindow == NULL || apply_focus_window != g.NavWindow->RootWindowDockStop))
    {
        ImGuiViewport* previous_viewport = g.NavWindow ? g.NavWindow->Viewport : NULL;
        ClearActiveID();
        g.NavDisableHighlight = false;
        g.NavDisableMouseHover = true;
        apply_focus_window = NavRestoreLastChildNavWindow(apply_focus_window);
        ClosePopupsOverWindow(apply_focus_window, false);
        FocusWindow(apply_focus_window);
        if (apply_focus_window->NavLastIds[0] == 0)
            NavInitWindow(apply_focus_window, false);

        if (apply_focus_window->DC.NavLayerActiveMask == (1 << ImGuiNavLayer_Menu))
            g.NavLayer = ImGuiNavLayer_Menu;

        if (apply_focus_window->Viewport != previous_viewport && g.PlatformIO.Platform_SetWindowFocus)
            g.PlatformIO.Platform_SetWindowFocus(apply_focus_window->Viewport);
    }
    if (apply_focus_window)
        g.NavWindowingTarget = NULL;

    if (apply_toggle_layer && g.NavWindow)
    {
        ImGuiWindow* new_nav_window = g.NavWindow;
        while (new_nav_window->ParentWindow
            && (new_nav_window->DC.NavLayerActiveMask & (1 << ImGuiNavLayer_Menu)) == 0
            && (new_nav_window->Flags & ImGuiWindowFlags_ChildWindow) != 0
            && (new_nav_window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu)) == 0)
            new_nav_window = new_nav_window->ParentWindow;
        if (new_nav_window != g.NavWindow)
        {
            ImGuiWindow* old_nav_window = g.NavWindow;
            FocusWindow(new_nav_window);
            new_nav_window->NavLastChildNavWindow = old_nav_window;
        }
        g.NavDisableHighlight = false;
        g.NavDisableMouseHover = true;

        const ImGuiNavLayer new_nav_layer = (g.NavWindow->DC.NavLayerActiveMask & (1 << ImGuiNavLayer_Menu)) ? (ImGuiNavLayer)((int)g.NavLayer ^ 1) : ImGuiNavLayer_Main;
        const bool preserve_layer_1_nav_id = (new_nav_window->DockNodeAsHost != NULL);
        if (new_nav_layer == ImGuiNavLayer_Menu && !preserve_layer_1_nav_id)
            g.NavWindow->NavLastIds[ImGuiNavLayer_Menu] = 0;
        NavRestoreLayer(new_nav_layer);
    }
}

static const char* GetFallbackWindowNameForWindowingList(ImGuiWindow* window)
{
    if (window->Flags & ImGuiWindowFlags_Popup)
        return "(Popup)";
    if ((window->Flags & ImGuiWindowFlags_MenuBar) && strcmp(window->Name, "##MainMenuBar") == 0)
        return "(Main menu bar)";
    if (window->DockNodeAsHost)
        return "(Dock node)";
    return "(Untitled)";
}

void ImGui::NavUpdateWindowingOverlay()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.NavWindowingTarget != NULL);

    if (g.NavWindowingTimer < NAV_WINDOWING_LIST_APPEAR_DELAY)
        return;

    if (g.NavWindowingListWindow == NULL)
        g.NavWindowingListWindow = FindWindowByName("###NavWindowingList");
    ImGuiViewportP* viewport =     (ImGuiViewportP*)GetMainViewport();
    SetNextWindowSizeConstraints(ImVec2(viewport->Size.x * 0.20f, viewport->Size.y * 0.20f), ImVec2(FLT_MAX, FLT_MAX));
    SetNextWindowPos(viewport->Pos + viewport->Size * 0.5f, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    PushStyleVar(ImGuiStyleVar_WindowPadding, g.Style.WindowPadding * 2.0f);
    Begin("###NavWindowingList", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    for (int n = g.WindowsFocusOrder.Size - 1; n >= 0; n--)
    {
        ImGuiWindow* window = g.WindowsFocusOrder[n];
        if (!IsWindowNavFocusable(window))
            continue;
        const char* label = window->Name;
        if (label == FindRenderedTextEnd(label))
            label = GetFallbackWindowNameForWindowingList(window);
        Selectable(label, g.NavWindowingTarget == window);
    }
    End();
    PopStyleVar();
}


void ImGui::ClearDragDrop()
{
    ImGuiContext& g = *GImGui;
    g.DragDropActive = false;
    g.DragDropPayload.Clear();
    g.DragDropAcceptFlags = ImGuiDragDropFlags_None;
    g.DragDropAcceptIdCurr = g.DragDropAcceptIdPrev = 0;
    g.DragDropAcceptIdCurrRectSurface = FLT_MAX;
    g.DragDropAcceptFrameCount = -1;

    g.DragDropPayloadBufHeap.clear();
    memset(&g.DragDropPayloadBufLocal, 0, sizeof(g.DragDropPayloadBufLocal));
}

bool ImGui::BeginDragDropSource(ImGuiDragDropFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    bool source_drag_active = false;
    ImGuiID source_id = 0;
    ImGuiID source_parent_id = 0;
    ImGuiMouseButton mouse_button = ImGuiMouseButton_Left;
    if (!(flags & ImGuiDragDropFlags_SourceExtern))
    {
        source_id = window->DC.LastItemId;
        if (source_id != 0 && g.ActiveId != source_id)       
            return false;
        if (g.IO.MouseDown[mouse_button] == false)
            return false;

        if (source_id == 0)
        {
            if (!(flags & ImGuiDragDropFlags_SourceAllowNullID))
            {
                IM_ASSERT(0);
                return false;
            }

            if ((window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) == 0 && (g.ActiveId == 0 || g.ActiveIdWindow != window))
                return false;

            source_id = window->DC.LastItemId = window->GetIDFromRectangle(window->DC.LastItemRect);
            bool is_hovered = ItemHoverable(window->DC.LastItemRect, source_id);
            if (is_hovered && g.IO.MouseClicked[mouse_button])
            {
                SetActiveID(source_id, window);
                FocusWindow(window);
            }
            if (g.ActiveId == source_id)                   
                g.ActiveIdAllowOverlap = is_hovered;
        }
        else
        {
            g.ActiveIdAllowOverlap = false;
        }
        if (g.ActiveId != source_id)
            return false;
        source_parent_id = window->IDStack.back();
        source_drag_active = IsMouseDragging(mouse_button);

        g.ActiveIdUsingNavDirMask = ~(ImU32)0;
        g.ActiveIdUsingNavInputMask = ~(ImU32)0;
        g.ActiveIdUsingKeyInputMask = ~(ImU64)0;
    }
    else
    {
        window = NULL;
        source_id = ImHashStr("#SourceExtern");
        source_drag_active = true;
    }

    if (source_drag_active)
    {
        if (!g.DragDropActive)
        {
            IM_ASSERT(source_id != 0);
            ClearDragDrop();
            ImGuiPayload& payload = g.DragDropPayload;
            payload.SourceId = source_id;
            payload.SourceParentId = source_parent_id;
            g.DragDropActive = true;
            g.DragDropSourceFlags = flags;
            g.DragDropMouseButton = mouse_button;
            if (payload.SourceId == g.ActiveId)
                g.ActiveIdNoClearOnFocusLoss = true;
        }
        g.DragDropSourceFrameCount = g.FrameCount;
        g.DragDropWithinSource = true;

        if (!(flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
        {
            BeginTooltip();
            if (g.DragDropAcceptIdPrev && (g.DragDropAcceptFlags & ImGuiDragDropFlags_AcceptNoPreviewTooltip))
            {
                ImGuiWindow* tooltip_window = g.CurrentWindow;
                tooltip_window->SkipItems = true;
                tooltip_window->HiddenFramesCanSkipItems = 1;
            }
        }

        if (!(flags & ImGuiDragDropFlags_SourceNoDisableHover) && !(flags & ImGuiDragDropFlags_SourceExtern))
            window->DC.LastItemStatusFlags &= ~ImGuiItemStatusFlags_HoveredRect;

        return true;
    }
    return false;
}

void ImGui::EndDragDropSource()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.DragDropActive);
    IM_ASSERT(g.DragDropWithinSource && "Not after a BeginDragDropSource()?");

    if (!(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
        EndTooltip();

    if (g.DragDropPayload.DataFrameCount == -1)
        ClearDragDrop();
    g.DragDropWithinSource = false;
}

bool ImGui::SetDragDropPayload(const char* type, const void* data, size_t data_size, ImGuiCond cond)
{
    ImGuiContext& g = *GImGui;
    ImGuiPayload& payload = g.DragDropPayload;
    if (cond == 0)
        cond = ImGuiCond_Always;

    IM_ASSERT(type != NULL);
    IM_ASSERT(strlen(type) < IM_ARRAYSIZE(payload.DataType) && "Payload type can be at most 32 characters long");
    IM_ASSERT((data != NULL && data_size > 0) || (data == NULL && data_size == 0));
    IM_ASSERT(cond == ImGuiCond_Always || cond == ImGuiCond_Once);
    IM_ASSERT(payload.SourceId != 0);                                     

    if (cond == ImGuiCond_Always || payload.DataFrameCount == -1)
    {
        ImStrncpy(payload.DataType, type, IM_ARRAYSIZE(payload.DataType));
        g.DragDropPayloadBufHeap.resize(0);
        if (data_size > sizeof(g.DragDropPayloadBufLocal))
        {
            g.DragDropPayloadBufHeap.resize((int)data_size);
            payload.Data = g.DragDropPayloadBufHeap.Data;
            memcpy(payload.Data, data, data_size);
        }
        else if (data_size > 0)
        {
            memset(&g.DragDropPayloadBufLocal, 0, sizeof(g.DragDropPayloadBufLocal));
            payload.Data = g.DragDropPayloadBufLocal;
            memcpy(payload.Data, data, data_size);
        }
        else
        {
            payload.Data = NULL;
        }
        payload.DataSize = (int)data_size;
    }
    payload.DataFrameCount = g.FrameCount;

    return (g.DragDropAcceptFrameCount == g.FrameCount) || (g.DragDropAcceptFrameCount == g.FrameCount - 1);
}

bool ImGui::BeginDragDropTargetCustom(const ImRect& bb, ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    if (!g.DragDropActive)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    ImGuiWindow* hovered_window = g.HoveredWindowUnderMovingWindow;
    if (hovered_window == NULL || window->RootWindow != hovered_window->RootWindow)
        return false;
    IM_ASSERT(id != 0);
    if (!IsMouseHoveringRect(bb.Min, bb.Max) || (id == g.DragDropPayload.SourceId))
        return false;
    if (window->SkipItems)
        return false;

    IM_ASSERT(g.DragDropWithinTarget == false);
    g.DragDropTargetRect = bb;
    g.DragDropTargetId = id;
    g.DragDropWithinTarget = true;
    return true;
}

bool ImGui::BeginDragDropTarget()
{
    ImGuiContext& g = *GImGui;
    if (!g.DragDropActive)
        return false;

    ImGuiWindow* window = g.CurrentWindow;
    if (!(window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect))
        return false;
    ImGuiWindow* hovered_window = g.HoveredWindowUnderMovingWindow;
    if (hovered_window == NULL || window->RootWindow != hovered_window->RootWindow)
        return false;

    const ImRect& display_rect = (window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HasDisplayRect) ? window->DC.LastItemDisplayRect : window->DC.LastItemRect;
    ImGuiID id = window->DC.LastItemId;
    if (id == 0)
        id = window->GetIDFromRectangle(display_rect);
    if (g.DragDropPayload.SourceId == id)
        return false;

    IM_ASSERT(g.DragDropWithinTarget == false);
    g.DragDropTargetRect = display_rect;
    g.DragDropTargetId = id;
    g.DragDropWithinTarget = true;
    return true;
}

bool ImGui::IsDragDropPayloadBeingAccepted()
{
    ImGuiContext& g = *GImGui;
    return g.DragDropActive && g.DragDropAcceptIdPrev != 0;
}

const ImGuiPayload* ImGui::AcceptDragDropPayload(const char* type, ImGuiDragDropFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiPayload& payload = g.DragDropPayload;
    IM_ASSERT(g.DragDropActive);                               
    IM_ASSERT(payload.DataFrameCount != -1);                 
    if (type != NULL && !payload.IsDataType(type))
        return NULL;

    const bool was_accepted_previously = (g.DragDropAcceptIdPrev == g.DragDropTargetId);
    ImRect r = g.DragDropTargetRect;
    float r_surface = r.GetWidth() * r.GetHeight();
    if (r_surface <= g.DragDropAcceptIdCurrRectSurface)
    {
        g.DragDropAcceptFlags = flags;
        g.DragDropAcceptIdCurr = g.DragDropTargetId;
        g.DragDropAcceptIdCurrRectSurface = r_surface;
    }

    payload.Preview = was_accepted_previously;
    flags |= (g.DragDropSourceFlags & ImGuiDragDropFlags_AcceptNoDrawDefaultRect);                
    if (!(flags & ImGuiDragDropFlags_AcceptNoDrawDefaultRect) && payload.Preview)
    {
        r.Expand(3.5f);
        bool push_clip_rect = !window->ClipRect.Contains(r);
        if (push_clip_rect) window->DrawList->PushClipRect(r.Min - ImVec2(1, 1), r.Max + ImVec2(1, 1));
        window->DrawList->AddRect(r.Min, r.Max, GetColorU32(ImGuiCol_DragDropTarget), 0.0f, ~0, 2.0f);
        if (push_clip_rect) window->DrawList->PopClipRect();
    }

    g.DragDropAcceptFrameCount = g.FrameCount;
    payload.Delivery = was_accepted_previously && !IsMouseDown(g.DragDropMouseButton);                  
    if (!payload.Delivery && !(flags & ImGuiDragDropFlags_AcceptBeforeDelivery))
        return NULL;

    return &payload;
}

const ImGuiPayload* ImGui::GetDragDropPayload()
{
    ImGuiContext& g = *GImGui;
    return g.DragDropActive ? &g.DragDropPayload : NULL;
}

void ImGui::EndDragDropTarget()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.DragDropActive);
    IM_ASSERT(g.DragDropWithinTarget);
    g.DragDropWithinTarget = false;
}

void ImGui::LogText(const char* fmt, ...)
{
    ImGuiContext& g = *GImGui;
    if (!g.LogEnabled)
        return;

    va_list args;
    va_start(args, fmt);
    if (g.LogFile)
    {
        g.LogBuffer.Buf.resize(0);
        g.LogBuffer.appendfv(fmt, args);
        ImFileWrite(g.LogBuffer.c_str(), sizeof(char), (ImU64)g.LogBuffer.size(), g.LogFile);
    }
    else
    {
        g.LogBuffer.appendfv(fmt, args);
    }
    va_end(args);
}

void ImGui::LogRenderedText(const ImVec2* ref_pos, const char* text, const char* text_end)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    if (!text_end)
        text_end = FindRenderedTextEnd(text, text_end);

    const bool log_new_line = ref_pos && (ref_pos->y > g.LogLinePosY + 1);
    if (ref_pos)
        g.LogLinePosY = ref_pos->y;
    if (log_new_line)
        g.LogLineFirstItem = true;

    const char* text_remaining = text;
    if (g.LogDepthRef > window->DC.TreeDepth)             
        g.LogDepthRef = window->DC.TreeDepth;
    const int tree_depth = (window->DC.TreeDepth - g.LogDepthRef);
    for (;;)
    {
        const char* line_start = text_remaining;
        const char* line_end = ImStreolRange(line_start, text_end);
        const bool is_first_line = (line_start == text);
        const bool is_last_line = (line_end == text_end);
        if (!is_last_line || (line_start != line_end))
        {
            const int char_count = (int)(line_end - line_start);
            if (log_new_line || !is_first_line)
                LogText(IM_NEWLINE "%*s%.*s", tree_depth * 4, "", char_count, line_start);
            else if (g.LogLineFirstItem)
                LogText("%*s%.*s", tree_depth * 4, "", char_count, line_start);
            else
                LogText(" %.*s", char_count, line_start);
            g.LogLineFirstItem = false;
        }
        else if (log_new_line)
        {
            LogText(IM_NEWLINE);
            break;
        }

        if (is_last_line)
            break;
        text_remaining = line_end + 1;
    }
}

void ImGui::LogBegin(ImGuiLogType type, int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    IM_ASSERT(g.LogEnabled == false);
    IM_ASSERT(g.LogFile == NULL);
    IM_ASSERT(g.LogBuffer.empty());
    g.LogEnabled = true;
    g.LogType = type;
    g.LogDepthRef = window->DC.TreeDepth;
    g.LogDepthToExpand = ((auto_open_depth >= 0) ? auto_open_depth : g.LogDepthToExpandDefault);
    g.LogLinePosY = FLT_MAX;
    g.LogLineFirstItem = true;
}

void ImGui::LogToTTY(int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    IM_UNUSED(auto_open_depth);
#ifndef IMGUI_DISABLE_TTY_FUNCTIONS
    LogBegin(ImGuiLogType_TTY, auto_open_depth);
    g.LogFile = stdout;
#endif
}

void ImGui::LogToFile(int auto_open_depth, const char* filename)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;

    if (!filename)
        filename = g.IO.LogFilename;
    if (!filename || !filename[0])
        return;
    ImFileHandle f = ImFileOpen(filename, "ab");
    if (!f)
    {
        IM_ASSERT(0);
        return;
    }

    LogBegin(ImGuiLogType_File, auto_open_depth);
    g.LogFile = f;
}

void ImGui::LogToClipboard(int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    LogBegin(ImGuiLogType_Clipboard, auto_open_depth);
}

void ImGui::LogToBuffer(int auto_open_depth)
{
    ImGuiContext& g = *GImGui;
    if (g.LogEnabled)
        return;
    LogBegin(ImGuiLogType_Buffer, auto_open_depth);
}

void ImGui::LogFinish()
{
    ImGuiContext& g = *GImGui;
    if (!g.LogEnabled)
        return;

    LogText(IM_NEWLINE);
    switch (g.LogType)
    {
    case ImGuiLogType_TTY:
#ifndef IMGUI_DISABLE_TTY_FUNCTIONS
        fflush(g.LogFile);
#endif
        break;
    case ImGuiLogType_File:
        ImFileClose(g.LogFile);
        break;
    case ImGuiLogType_Buffer:
        break;
    case ImGuiLogType_Clipboard:
        if (!g.LogBuffer.empty())
            SetClipboardText(g.LogBuffer.begin());
        break;
    case ImGuiLogType_None:
        IM_ASSERT(0);
        break;
    }

    g.LogEnabled = false;
    g.LogType = ImGuiLogType_None;
    g.LogFile = NULL;
    g.LogBuffer.clear();
}

void ImGui::LogButtons()
{
    ImGuiContext& g = *GImGui;

    PushID("LogButtons");
#ifndef IMGUI_DISABLE_TTY_FUNCTIONS
    const bool log_to_tty = Button("Log To TTY"); SameLine();
#else
    const bool log_to_tty = false;
#endif
    const bool log_to_file = Button("Log To File"); SameLine();
    const bool log_to_clipboard = Button("Log To Clipboard"); SameLine();
    PushAllowKeyboardFocus(false);
    SetNextItemWidth(80.0f);
    SliderInt("Default Depth", &g.LogDepthToExpandDefault, 0, 9, NULL);
    PopAllowKeyboardFocus();
    PopID();

    if (log_to_tty)
        LogToTTY();
    if (log_to_file)
        LogToFile();
    if (log_to_clipboard)
        LogToClipboard();
}


void ImGui::UpdateSettings()
{
    ImGuiContext& g = *GImGui;
    if (!g.SettingsLoaded)
    {
        IM_ASSERT(g.SettingsWindows.empty());
        if (g.IO.IniFilename)
            LoadIniSettingsFromDisk(g.IO.IniFilename);
        g.SettingsLoaded = true;
    }

    if (g.SettingsDirtyTimer > 0.0f)
    {
        g.SettingsDirtyTimer -= g.IO.DeltaTime;
        if (g.SettingsDirtyTimer <= 0.0f)
        {
            if (g.IO.IniFilename != NULL)
                SaveIniSettingsToDisk(g.IO.IniFilename);
            else
                g.IO.WantSaveIniSettings = true;                
            g.SettingsDirtyTimer = 0.0f;
        }
    }
}

void ImGui::MarkIniSettingsDirty()
{
    ImGuiContext& g = *GImGui;
    if (g.SettingsDirtyTimer <= 0.0f)
        g.SettingsDirtyTimer = g.IO.IniSavingRate;
}

void ImGui::MarkIniSettingsDirty(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (!(window->Flags & ImGuiWindowFlags_NoSavedSettings))
        if (g.SettingsDirtyTimer <= 0.0f)
            g.SettingsDirtyTimer = g.IO.IniSavingRate;
}

ImGuiWindowSettings* ImGui::CreateNewWindowSettings(const char* name)
{
    ImGuiContext& g = *GImGui;

#if !IMGUI_DEBUG_INI_SETTINGS
    if (const char* p = strstr(name, "###"))
        name = p;
#endif
    const size_t name_len = strlen(name);

    const size_t chunk_size = sizeof(ImGuiWindowSettings) + name_len + 1;
    ImGuiWindowSettings* settings = g.SettingsWindows.alloc_chunk(chunk_size);
    IM_PLACEMENT_NEW(settings) ImGuiWindowSettings();
    settings->ID = ImHashStr(name, name_len);
    memcpy(settings->GetName(), name, name_len + 1);       

    return settings;
}

ImGuiWindowSettings* ImGui::FindWindowSettings(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
        if (settings->ID == id)
            return settings;
    return NULL;
}

ImGuiWindowSettings* ImGui::FindOrCreateWindowSettings(const char* name)
{
    if (ImGuiWindowSettings* settings = FindWindowSettings(ImHashStr(name)))
        return settings;
    return CreateNewWindowSettings(name);
}

ImGuiSettingsHandler* ImGui::FindSettingsHandler(const char* type_name)
{
    ImGuiContext& g = *GImGui;
    const ImGuiID type_hash = ImHashStr(type_name);
    for (int handler_n = 0; handler_n < g.SettingsHandlers.Size; handler_n++)
        if (g.SettingsHandlers[handler_n].TypeHash == type_hash)
            return &g.SettingsHandlers[handler_n];
    return NULL;
}

void ImGui::ClearIniSettings()
{
    ImGuiContext& g = *GImGui;
    g.SettingsIniData.clear();
    for (int handler_n = 0; handler_n < g.SettingsHandlers.Size; handler_n++)
        if (g.SettingsHandlers[handler_n].ClearAllFn)
            g.SettingsHandlers[handler_n].ClearAllFn(&g, &g.SettingsHandlers[handler_n]);
}

void ImGui::LoadIniSettingsFromDisk(const char* ini_filename)
{
    size_t file_data_size = 0;
    char* file_data = (char*)ImFileLoadToMemory(ini_filename, "rb", &file_data_size);
    if (!file_data)
        return;
    LoadIniSettingsFromMemory(file_data, (size_t)file_data_size);
    IM_FREE(file_data);
}

void ImGui::LoadIniSettingsFromMemory(const char* ini_data, size_t ini_size)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.Initialized);
    if (ini_size == 0)
        ini_size = strlen(ini_data);
    g.SettingsIniData.Buf.resize((int)ini_size + 1);
    char* const buf = g.SettingsIniData.Buf.Data;
    char* const buf_end = buf + ini_size;
    memcpy(buf, ini_data, ini_size);
    buf_end[0] = 0;

    for (int handler_n = 0; handler_n < g.SettingsHandlers.Size; handler_n++)
        if (g.SettingsHandlers[handler_n].ReadInitFn)
            g.SettingsHandlers[handler_n].ReadInitFn(&g, &g.SettingsHandlers[handler_n]);

    void* entry_data = NULL;
    ImGuiSettingsHandler* entry_handler = NULL;

    char* line_end = NULL;
    for (char* line = buf; line < buf_end; line = line_end + 1)
    {
        while (*line == '\n' || *line == '\r')
            line++;
        line_end = line;
        while (line_end < buf_end && *line_end != '\n' && *line_end != '\r')
            line_end++;
        line_end[0] = 0;
        if (line[0] == ';')
            continue;
        if (line[0] == '[' && line_end > line && line_end[-1] == ']')
        {
            line_end[-1] = 0;
            const char* name_end = line_end - 1;
            const char* type_start = line + 1;
            char* type_end = (char*)(void*)ImStrchrRange(type_start, name_end, ']');
            const char* name_start = type_end ? ImStrchrRange(type_end + 1, name_end, '[') : NULL;
            if (!type_end || !name_start)
                continue;
            *type_end = 0;    
            name_start++;     
            entry_handler = FindSettingsHandler(type_start);
            entry_data = entry_handler ? entry_handler->ReadOpenFn(&g, entry_handler, name_start) : NULL;
        }
        else if (entry_handler != NULL && entry_data != NULL)
        {
            entry_handler->ReadLineFn(&g, entry_handler, entry_data, line);
        }
    }
    g.SettingsLoaded = true;

    memcpy(buf, ini_data, ini_size);

    for (int handler_n = 0; handler_n < g.SettingsHandlers.Size; handler_n++)
        if (g.SettingsHandlers[handler_n].ApplyAllFn)
            g.SettingsHandlers[handler_n].ApplyAllFn(&g, &g.SettingsHandlers[handler_n]);
}

void ImGui::SaveIniSettingsToDisk(const char* ini_filename)
{
    ImGuiContext& g = *GImGui;
    g.SettingsDirtyTimer = 0.0f;
    if (!ini_filename)
        return;

    size_t ini_data_size = 0;
    const char* ini_data = SaveIniSettingsToMemory(&ini_data_size);
    ImFileHandle f = ImFileOpen(ini_filename, "wt");
    if (!f)
        return;
    ImFileWrite(ini_data, sizeof(char), ini_data_size, f);
    ImFileClose(f);
}

const char* ImGui::SaveIniSettingsToMemory(size_t* out_size)
{
    ImGuiContext& g = *GImGui;
    g.SettingsDirtyTimer = 0.0f;
    g.SettingsIniData.Buf.resize(0);
    g.SettingsIniData.Buf.push_back(0);
    for (int handler_n = 0; handler_n < g.SettingsHandlers.Size; handler_n++)
    {
        ImGuiSettingsHandler* handler = &g.SettingsHandlers[handler_n];
        handler->WriteAllFn(&g, handler, &g.SettingsIniData);
    }
    if (out_size)
        *out_size = (size_t)g.SettingsIniData.size();
    return g.SettingsIniData.c_str();
}

static void WindowSettingsHandler_ClearAll(ImGuiContext* ctx, ImGuiSettingsHandler*)
{
    ImGuiContext& g = *ctx;
    for (int i = 0; i != g.Windows.Size; i++)
        g.Windows[i]->SettingsOffset = -1;
    g.SettingsWindows.clear();
}

static void* WindowSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    ImGuiWindowSettings* settings = ImGui::FindOrCreateWindowSettings(name);
    ImGuiID id = settings->ID;
    *settings = ImGuiWindowSettings();       
    settings->ID = id;
    settings->WantApply = true;
    return (void*)settings;
}

static void WindowSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    ImGuiWindowSettings* settings = (ImGuiWindowSettings*)entry;
    int x, y;
    int i;
    ImU32 u1;
    if (sscanf(line, "Pos=%i,%i", &x, &y) == 2)             { settings->Pos = ImVec2ih((short)x, (short)y); }
    else if (sscanf(line, "Size=%i,%i", &x, &y) == 2)       { settings->Size = ImVec2ih((short)x, (short)y); }
    else if (sscanf(line, "ViewportId=0x%08X", &u1) == 1)   { settings->ViewportId = u1; }
    else if (sscanf(line, "ViewportPos=%i,%i", &x, &y) == 2){ settings->ViewportPos = ImVec2ih((short)x, (short)y); }
    else if (sscanf(line, "Collapsed=%d", &i) == 1)         { settings->Collapsed = (i != 0); }
    else if (sscanf(line, "DockId=0x%X,%d", &u1, &i) == 2)  { settings->DockId = u1; settings->DockOrder = (short)i; }
    else if (sscanf(line, "DockId=0x%X", &u1) == 1)         { settings->DockId = u1; settings->DockOrder = -1; }
    else if (sscanf(line, "ClassId=0x%X", &u1) == 1)        { settings->ClassId = u1; }
}

static void WindowSettingsHandler_ApplyAll(ImGuiContext* ctx, ImGuiSettingsHandler*)
{
    ImGuiContext& g = *ctx;
    for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
        if (settings->WantApply)
        {
            if (ImGuiWindow* window = ImGui::FindWindowByID(settings->ID))
                ApplyWindowSettings(window, settings);
            settings->WantApply = false;
        }
}

static void WindowSettingsHandler_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    ImGuiContext& g = *ctx;
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* window = g.Windows[i];
        if (window->Flags & ImGuiWindowFlags_NoSavedSettings)
            continue;

        ImGuiWindowSettings* settings = (window->SettingsOffset != -1) ? g.SettingsWindows.ptr_from_offset(window->SettingsOffset) : ImGui::FindWindowSettings(window->ID);
        if (!settings)
        {
            settings = ImGui::CreateNewWindowSettings(window->Name);
            window->SettingsOffset = g.SettingsWindows.offset_from_ptr(settings);
        }
        IM_ASSERT(settings->ID == window->ID);
        settings->Pos = ImVec2ih(window->Pos - window->ViewportPos);
        settings->Size = ImVec2ih(window->SizeFull);
        settings->ViewportId = window->ViewportId;
        settings->ViewportPos = ImVec2ih(window->ViewportPos);
        IM_ASSERT(window->DockNode == NULL || window->DockNode->ID == window->DockId);
        settings->DockId = window->DockId;
        settings->ClassId = window->WindowClass.ClassId;
        settings->DockOrder = window->DockOrder;
        settings->Collapsed = window->Collapsed;
    }

    buf->reserve(buf->size() + g.SettingsWindows.size() * 6);   
    for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
    {
        const char* settings_name = settings->GetName();
        buf->appendf("[%s][%s]\n", handler->TypeName, settings_name);
        if (settings->ViewportId != 0 && settings->ViewportId != ImGui::IMGUI_VIEWPORT_DEFAULT_ID)
        {
            buf->appendf("ViewportPos=%d,%d\n", settings->ViewportPos.x, settings->ViewportPos.y);
            buf->appendf("ViewportId=0x%08X\n", settings->ViewportId);
        }
        if (settings->Pos.x != 0 || settings->Pos.y != 0 || settings->ViewportId == ImGui::IMGUI_VIEWPORT_DEFAULT_ID)
            buf->appendf("Pos=%d,%d\n", settings->Pos.x, settings->Pos.y);
        if (settings->Size.x != 0 || settings->Size.y != 0)
            buf->appendf("Size=%d,%d\n", settings->Size.x, settings->Size.y);
        buf->appendf("Collapsed=%d\n", settings->Collapsed);
        if (settings->DockId != 0)
        {
            if (settings->DockOrder == -1)
                buf->appendf("DockId=0x%08X\n", settings->DockId);
            else
                buf->appendf("DockId=0x%08X,%d\n", settings->DockId, settings->DockOrder);
            if (settings->ClassId != 0)
                buf->appendf("ClassId=0x%08X\n", settings->ClassId);
        }
        buf->append("\n");
    }
}


ImGuiViewport* ImGui::GetMainViewport()
{
    ImGuiContext& g = *GImGui;
    return g.Viewports[0];
}

ImGuiViewport* ImGui::FindViewportByID(ImGuiID id)
{
    ImGuiContext& g = *GImGui;
    for (int n = 0; n < g.Viewports.Size; n++)
        if (g.Viewports[n]->ID == id)
            return g.Viewports[n];
    return NULL;
}

ImGuiViewport* ImGui::FindViewportByPlatformHandle(void* platform_handle)
{
    ImGuiContext& g = *GImGui;
    for (int i = 0; i != g.Viewports.Size; i++)
        if (g.Viewports[i]->PlatformHandle == platform_handle)
            return g.Viewports[i];
    return NULL;
}

void ImGui::SetCurrentViewport(ImGuiWindow* current_window, ImGuiViewportP* viewport)
{
    ImGuiContext& g = *GImGui;
    (void)current_window;

    if (viewport)
        viewport->LastFrameActive = g.FrameCount;
    if (g.CurrentViewport == viewport)
        return;
    g.CurrentDpiScale = viewport ? viewport->DpiScale : 1.0f;
    g.CurrentViewport = viewport;
    if (g.CurrentViewport && g.PlatformIO.Platform_OnChangedViewport)
        g.PlatformIO.Platform_OnChangedViewport(g.CurrentViewport);
}

static void SetWindowViewport(ImGuiWindow* window, ImGuiViewportP* viewport)
{
    window->Viewport = viewport;
    window->ViewportId = viewport->ID;
    window->ViewportOwned = (viewport->Window == window);
}

static bool ImGui::GetWindowAlwaysWantOwnViewport(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.IO.ConfigViewportsNoAutoMerge || (window->WindowClass.ViewportFlagsOverrideSet & ImGuiViewportFlags_NoAutoMerge))
        if (g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable)
            if (!window->DockIsActive)
                if ((window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_Tooltip)) == 0)
                    if ((window->Flags & ImGuiWindowFlags_Popup) == 0 || (window->Flags & ImGuiWindowFlags_Modal) != 0)
                        return true;
    return false;
}

static bool ImGui::UpdateTryMergeWindowIntoHostViewport(ImGuiWindow* window, ImGuiViewportP* viewport)
{
    ImGuiContext& g = *GImGui;
    if (window->Viewport == viewport)
        return false;
    if ((viewport->Flags & ImGuiViewportFlags_CanHostOtherWindows) == 0)
        return false;
    if ((viewport->Flags & ImGuiViewportFlags_Minimized) != 0)
        return false;
    if (!viewport->GetMainRect().Contains(window->Rect()))
        return false;
    if (GetWindowAlwaysWantOwnViewport(window))
        return false;

    for (int n = 0; n < g.Windows.Size; n++)
    {
        ImGuiWindow* window_behind = g.Windows[n];
        if (window_behind == window)
            break;
        if (window_behind->WasActive && window_behind->ViewportOwned && !(window_behind->Flags & ImGuiWindowFlags_ChildWindow))
            if (window_behind->Viewport->GetMainRect().Overlaps(window->Rect()))
                return false;
    }

    ImGuiViewportP* old_viewport = window->Viewport;
    if (window->ViewportOwned)
        for (int n = 0; n < g.Windows.Size; n++)
            if (g.Windows[n]->Viewport == old_viewport)
                SetWindowViewport(g.Windows[n], viewport);
    SetWindowViewport(window, viewport);
    BringWindowToDisplayFront(window);

    return true;
}

static bool ImGui::UpdateTryMergeWindowIntoHostViewports(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    return UpdateTryMergeWindowIntoHostViewport(window, g.Viewports[0]);
}

void ImGui::TranslateWindowsInViewport(ImGuiViewportP* viewport, const ImVec2& old_pos, const ImVec2& new_pos)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(viewport->Window == NULL && (viewport->Flags & ImGuiViewportFlags_CanHostOtherWindows));

    const bool translate_all_windows = (g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable) != (g.ConfigFlagsLastFrame & ImGuiConfigFlags_ViewportsEnable);
    ImRect test_still_fit_rect(old_pos, old_pos + viewport->Size);
    ImVec2 delta_pos = new_pos - old_pos;
    for (int window_n = 0; window_n < g.Windows.Size; window_n++)  
        if (translate_all_windows || (g.Windows[window_n]->Viewport == viewport && test_still_fit_rect.Contains(g.Windows[window_n]->Rect())))
            TranslateWindow(g.Windows[window_n], delta_pos);
}

void ImGui::ScaleWindowsInViewport(ImGuiViewportP* viewport, float scale)
{
    ImGuiContext& g = *GImGui;
    if (viewport->Window)
    {
        ScaleWindow(viewport->Window, scale);
    }
    else
    {
        for (int i = 0; i != g.Windows.Size; i++)
            if (g.Windows[i]->Viewport == viewport)
                ScaleWindow(g.Windows[i], scale);
    }
}

static ImGuiViewportP* FindHoveredViewportFromPlatformWindowStack(const ImVec2 mouse_platform_pos)
{
    ImGuiContext& g = *GImGui;
    ImGuiViewportP* best_candidate = NULL;
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        if (!(viewport->Flags & (ImGuiViewportFlags_NoInputs | ImGuiViewportFlags_Minimized)) && viewport->GetMainRect().Contains(mouse_platform_pos))
            if (best_candidate == NULL || best_candidate->LastFrontMostStampCount < viewport->LastFrontMostStampCount)
                best_candidate = viewport;
    }
    return best_candidate;
}

static void ImGui::UpdateViewportsNewFrame()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.PlatformIO.Viewports.Size <= g.Viewports.Size);

    const bool viewports_enabled = (g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable) != 0;
    if (viewports_enabled)
    {
        for (int n = 0; n < g.Viewports.Size; n++)
        {
            ImGuiViewportP* viewport = g.Viewports[n];
            const bool platform_funcs_available = viewport->PlatformWindowCreated;
            if (g.PlatformIO.Platform_GetWindowMinimized && platform_funcs_available)
            {
                bool minimized = g.PlatformIO.Platform_GetWindowMinimized(viewport);
                if (minimized)
                    viewport->Flags |= ImGuiViewportFlags_Minimized;
                else
                    viewport->Flags &= ~ImGuiViewportFlags_Minimized;
            }
        }
    }

    ImGuiViewportP* main_viewport = g.Viewports[0];
    IM_ASSERT(main_viewport->ID == IMGUI_VIEWPORT_DEFAULT_ID);
    IM_ASSERT(main_viewport->Window == NULL);
    ImVec2 main_viewport_pos = viewports_enabled ? g.PlatformIO.Platform_GetWindowPos(main_viewport) : ImVec2(0.0f, 0.0f);
    ImVec2 main_viewport_size = g.IO.DisplaySize;
    if (viewports_enabled && (main_viewport->Flags & ImGuiViewportFlags_Minimized))
    {
        main_viewport_pos = main_viewport->Pos;                      
        main_viewport_size = main_viewport->Size;
    }
    AddUpdateViewport(NULL, IMGUI_VIEWPORT_DEFAULT_ID, main_viewport_pos, main_viewport_size, ImGuiViewportFlags_CanHostOtherWindows);

    g.CurrentDpiScale = 0.0f;
    g.CurrentViewport = NULL;
    g.MouseViewport = NULL;
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        viewport->Idx = n;

        if (n > 0 && viewport->LastFrameActive < g.FrameCount - 2)
        {
            for (int window_n = 0; window_n < g.Windows.Size; window_n++)
                if (g.Windows[window_n]->Viewport == viewport)
                {
                    g.Windows[window_n]->Viewport = NULL;
                    g.Windows[window_n]->ViewportOwned = false;
                }
            if (viewport == g.MouseLastHoveredViewport)
                g.MouseLastHoveredViewport = NULL;
            g.Viewports.erase(g.Viewports.Data + n);

            IMGUI_DEBUG_LOG_VIEWPORT("Delete Viewport %08X (%s)\n", viewport->ID, viewport->Window ? viewport->Window->Name : "n/a");
            DestroyPlatformWindow(viewport);            
            IM_ASSERT(g.PlatformIO.Viewports.contains(viewport) == false);
            IM_DELETE(viewport);
            n--;
            continue;
        }

        const bool platform_funcs_available = viewport->PlatformWindowCreated;
        if (viewports_enabled)
        {
            if (!(viewport->Flags & ImGuiViewportFlags_Minimized) && platform_funcs_available)
            {
                if (viewport->PlatformRequestMove)
                    viewport->Pos = viewport->LastPlatformPos = g.PlatformIO.Platform_GetWindowPos(viewport);
                if (viewport->PlatformRequestResize)
                    viewport->Size = viewport->LastPlatformSize = g.PlatformIO.Platform_GetWindowSize(viewport);
            }
        }

        UpdateViewportPlatformMonitor(viewport);

        viewport->WorkOffsetMin = viewport->CurrWorkOffsetMin;
        viewport->WorkOffsetMax = viewport->CurrWorkOffsetMax;
        viewport->CurrWorkOffsetMin = viewport->CurrWorkOffsetMax = ImVec2(0.0f, 0.0f);

        viewport->Alpha = 1.0f;

        const ImVec2 viewport_delta_pos = viewport->Pos - viewport->LastPos;
        if ((viewport->Flags & ImGuiViewportFlags_CanHostOtherWindows) && (viewport_delta_pos.x != 0.0f || viewport_delta_pos.y != 0.0f))
            TranslateWindowsInViewport(viewport, viewport->LastPos, viewport->Pos);

        float new_dpi_scale;
        if (g.PlatformIO.Platform_GetWindowDpiScale && platform_funcs_available)
            new_dpi_scale = g.PlatformIO.Platform_GetWindowDpiScale(viewport);
        else if (viewport->PlatformMonitor != -1)
            new_dpi_scale = g.PlatformIO.Monitors[viewport->PlatformMonitor].DpiScale;
        else
            new_dpi_scale = (viewport->DpiScale != 0.0f) ? viewport->DpiScale : 1.0f;
        if (viewport->DpiScale != 0.0f && new_dpi_scale != viewport->DpiScale)
        {
            float scale_factor = new_dpi_scale / viewport->DpiScale;
            if (g.IO.ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
                ScaleWindowsInViewport(viewport, scale_factor);
        }
        viewport->DpiScale = new_dpi_scale;
    }

    if (!viewports_enabled)
    {
        g.MouseViewport = main_viewport;
        return;
    }

    ImGuiViewportP* viewport_hovered = NULL;
    if (g.IO.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
    {
        viewport_hovered = g.IO.MouseHoveredViewport ? (ImGuiViewportP*)FindViewportByID(g.IO.MouseHoveredViewport) : NULL;
        if (viewport_hovered && (viewport_hovered->Flags & ImGuiViewportFlags_NoInputs))
        {
            IM_ASSERT(0);
            viewport_hovered = FindHoveredViewportFromPlatformWindowStack(g.IO.MousePos);
        }
    }
    else
    {
        viewport_hovered = FindHoveredViewportFromPlatformWindowStack(g.IO.MousePos);
    }
    if (viewport_hovered != NULL)
        g.MouseLastHoveredViewport = viewport_hovered;
    else if (g.MouseLastHoveredViewport == NULL)
        g.MouseLastHoveredViewport = g.Viewports[0];

    if (g.MovingWindow)
        g.MouseViewport = g.MovingWindow->Viewport;
    else
        g.MouseViewport = g.MouseLastHoveredViewport;

    const bool is_mouse_dragging_with_an_expected_destination = g.DragDropActive;
    if (is_mouse_dragging_with_an_expected_destination && viewport_hovered == NULL)
        viewport_hovered = g.MouseLastHoveredViewport;
    if (is_mouse_dragging_with_an_expected_destination || g.ActiveId == 0 || !IsAnyMouseDown())
        if (viewport_hovered != NULL && viewport_hovered != g.MouseViewport && !(viewport_hovered->Flags & ImGuiViewportFlags_NoInputs))
            g.MouseViewport = viewport_hovered;

    IM_ASSERT(g.MouseViewport != NULL);
}

static void ImGui::UpdateViewportsEndFrame()
{
    ImGuiContext& g = *GImGui;
    g.PlatformIO.MainViewport = g.Viewports[0];
    g.PlatformIO.Viewports.resize(0);
    for (int i = 0; i < g.Viewports.Size; i++)
    {
        ImGuiViewportP* viewport = g.Viewports[i];
        viewport->LastPos = viewport->Pos;
        if (viewport->LastFrameActive < g.FrameCount || viewport->Size.x <= 0.0f || viewport->Size.y <= 0.0f)
            if (i > 0)        
                continue;
        if (viewport->Window && !IsWindowActiveAndVisible(viewport->Window))
            continue;
        if (i > 0)
            IM_ASSERT(viewport->Window != NULL);
        g.PlatformIO.Viewports.push_back(viewport);
    }
    g.Viewports[0]->ClearRequestFlags();                
}

ImGuiViewportP* ImGui::AddUpdateViewport(ImGuiWindow* window, ImGuiID id, const ImVec2& pos, const ImVec2& size, ImGuiViewportFlags flags)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(id != 0);

    if (window != NULL)
    {
        if (g.MovingWindow && g.MovingWindow->RootWindow == window)
            flags |= ImGuiViewportFlags_NoInputs | ImGuiViewportFlags_NoFocusOnAppearing;
        if ((window->Flags & ImGuiWindowFlags_NoMouseInputs) && (window->Flags & ImGuiWindowFlags_NoNavInputs))
            flags |= ImGuiViewportFlags_NoInputs;
        if (window->Flags & ImGuiWindowFlags_NoFocusOnAppearing)
            flags |= ImGuiViewportFlags_NoFocusOnAppearing;
    }

    ImGuiViewportP* viewport = (ImGuiViewportP*)FindViewportByID(id);
    if (viewport)
    {
        if (!viewport->PlatformRequestMove)
            viewport->Pos = pos;
        if (!viewport->PlatformRequestResize)
            viewport->Size = size;
        viewport->Flags = flags | (viewport->Flags & ImGuiViewportFlags_Minimized);    
    }
    else
    {
        viewport = IM_NEW(ImGuiViewportP)();
        viewport->ID = id;
        viewport->Idx = g.Viewports.Size;
        viewport->Pos = viewport->LastPos = pos;
        viewport->Size = size;
        viewport->Flags = flags;
        UpdateViewportPlatformMonitor(viewport);
        g.Viewports.push_back(viewport);
        IMGUI_DEBUG_LOG_VIEWPORT("Add Viewport %08X (%s)\n", id, window->Name);

        g.DrawListSharedData.ClipRectFullscreen.x = ImMin(g.DrawListSharedData.ClipRectFullscreen.x, viewport->Pos.x);
        g.DrawListSharedData.ClipRectFullscreen.y = ImMin(g.DrawListSharedData.ClipRectFullscreen.y, viewport->Pos.y);
        g.DrawListSharedData.ClipRectFullscreen.z = ImMax(g.DrawListSharedData.ClipRectFullscreen.z, viewport->Pos.x + viewport->Size.x);
        g.DrawListSharedData.ClipRectFullscreen.w = ImMax(g.DrawListSharedData.ClipRectFullscreen.w, viewport->Pos.y + viewport->Size.y);

        if (viewport->PlatformMonitor != -1)
            viewport->DpiScale = g.PlatformIO.Monitors[viewport->PlatformMonitor].DpiScale;
    }

    viewport->Window = window;
    viewport->LastFrameActive = g.FrameCount;
    IM_ASSERT(window == NULL || viewport->ID == window->ID);

    if (window != NULL)
        window->ViewportOwned = true;

    return viewport;
}

static void ImGui::UpdateSelectWindowViewport(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindowFlags flags = window->Flags;
    window->ViewportAllowPlatformMonitorExtend = -1;

    ImGuiViewportP* main_viewport = g.Viewports[0];
    if (!(g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable))
    {
        SetWindowViewport(window, main_viewport);
        return;
    }
    window->ViewportOwned = false;

    if ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && window->Appearing)
    {
        window->Viewport = NULL;
        window->ViewportId = 0;
    }

    if ((g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasViewport) == 0)
    {
        if (window->Viewport == NULL && window->ParentWindow && !window->ParentWindow->IsFallbackWindow)
            window->Viewport = window->ParentWindow->Viewport;

        if (window->Viewport == NULL && window->ViewportId != 0)
        {
            window->Viewport = (ImGuiViewportP*)FindViewportByID(window->ViewportId);
            if (window->Viewport == NULL && window->ViewportPos.x != FLT_MAX && window->ViewportPos.y != FLT_MAX)
                window->Viewport = AddUpdateViewport(window, window->ID, window->ViewportPos, window->Size, ImGuiViewportFlags_None);
        }
    }

    bool lock_viewport = false;
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasViewport)
    {
        window->Viewport = (ImGuiViewportP*)FindViewportByID(g.NextWindowData.ViewportId);
        window->ViewportId = g.NextWindowData.ViewportId;         
        lock_viewport = true;
    }
    else if ((flags & ImGuiWindowFlags_ChildWindow) || (flags & ImGuiWindowFlags_ChildMenu))
    {
        window->Viewport = window->ParentWindow->Viewport;
    }
    else if (flags & ImGuiWindowFlags_Tooltip)
    {
        window->Viewport = g.MouseViewport;
    }
    else if (GetWindowAlwaysWantOwnViewport(window))
    {
        window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_None);
    }
    else if (g.MovingWindow && g.MovingWindow->RootWindow == window && IsMousePosValid())
    {
        if (window->Viewport != NULL && window->Viewport->Window == window)
            window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_None);
    }
    else
    {
        bool try_to_merge_into_host_viewport = (window->Viewport && window == window->Viewport->Window && g.ActiveId == 0);
        if (try_to_merge_into_host_viewport)
            UpdateTryMergeWindowIntoHostViewports(window);
    }

    if (window->Viewport == NULL)
        if (!UpdateTryMergeWindowIntoHostViewport(window, main_viewport))
            window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_None);
    
    if (!lock_viewport)
    {
        if (flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup))
        {
            ImVec2 mouse_ref = (flags & ImGuiWindowFlags_Tooltip) ? g.IO.MousePos : g.BeginPopupStack.back().OpenMousePos;
            bool use_mouse_ref = (g.NavDisableHighlight || !g.NavDisableMouseHover || !g.NavWindow);
            bool mouse_valid = IsMousePosValid(&mouse_ref);
            if ((window->Appearing || (flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_ChildMenu))) && (!use_mouse_ref || mouse_valid))
                window->ViewportAllowPlatformMonitorExtend = FindPlatformMonitorForPos((use_mouse_ref && mouse_valid) ? mouse_ref : NavCalcPreferredRefPos());
            else
                window->ViewportAllowPlatformMonitorExtend = window->Viewport->PlatformMonitor;
        }
        else if (window->Viewport && window != window->Viewport->Window && window->Viewport->Window && !(flags & ImGuiWindowFlags_ChildWindow))
        {
            const bool will_be_visible = (window->DockIsActive && !window->DockTabIsVisible) ? false : true;
            if ((window->Flags & ImGuiWindowFlags_DockNodeHost) && window->Viewport->LastFrameActive < g.FrameCount && will_be_visible)
            {
                IMGUI_DEBUG_LOG_VIEWPORT("Window '%s' steal Viewport %08X from Window '%s'\n", window->Name, window->Viewport->ID, window->Viewport->Window->Name);
                window->Viewport->Window = window;
                window->Viewport->ID = window->ID;
                window->Viewport->LastNameHash = 0;
            }
            else if (!UpdateTryMergeWindowIntoHostViewports(window))  
            {
                window->Viewport = AddUpdateViewport(window, window->ID, window->Pos, window->Size, ImGuiViewportFlags_NoFocusOnAppearing);
            }
        }
        else if (window->ViewportAllowPlatformMonitorExtend < 0 && (flags & ImGuiWindowFlags_ChildWindow) == 0)
        {
            window->ViewportAllowPlatformMonitorExtend = window->Viewport->PlatformMonitor;
        }
    }

    window->ViewportOwned = (window == window->Viewport->Window);
    window->ViewportId = window->Viewport->ID;

}

void ImGui::UpdatePlatformWindows()
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.FrameCountEnded == g.FrameCount && "Forgot to call Render() or EndFrame() before UpdatePlatformWindows()?");
    IM_ASSERT(g.FrameCountPlatformEnded < g.FrameCount);
    g.FrameCountPlatformEnded = g.FrameCount;
    if (!(g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable))
        return;

    for (int i = 1; i < g.Viewports.Size; i++)
    {
        ImGuiViewportP* viewport = g.Viewports[i];

        bool destroy_platform_window = false;
        destroy_platform_window |= (viewport->LastFrameActive < g.FrameCount - 1);
        destroy_platform_window |= (viewport->Window && !IsWindowActiveAndVisible(viewport->Window));
        if (destroy_platform_window)
        {
            DestroyPlatformWindow(viewport);
            continue;
        }

        if (viewport->LastFrameActive < g.FrameCount || viewport->Size.x <= 0 || viewport->Size.y <= 0)
            continue;

        bool is_new_platform_window = (viewport->PlatformWindowCreated == false);
        if (is_new_platform_window)
        {
            IMGUI_DEBUG_LOG_VIEWPORT("Create Platform Window %08X (%s)\n", viewport->ID, viewport->Window ? viewport->Window->Name : "n/a");
            g.PlatformIO.Platform_CreateWindow(viewport);
            if (g.PlatformIO.Renderer_CreateWindow != NULL)
                g.PlatformIO.Renderer_CreateWindow(viewport);
            viewport->LastNameHash = 0;
            viewport->LastPlatformPos = viewport->LastPlatformSize = ImVec2(FLT_MAX, FLT_MAX);                 
            viewport->LastRendererSize = viewport->Size;                                                     
            viewport->PlatformWindowCreated = true;
        }

        if ((viewport->LastPlatformPos.x != viewport->Pos.x || viewport->LastPlatformPos.y != viewport->Pos.y) && !viewport->PlatformRequestMove)
            g.PlatformIO.Platform_SetWindowPos(viewport, viewport->Pos);
        if ((viewport->LastPlatformSize.x != viewport->Size.x || viewport->LastPlatformSize.y != viewport->Size.y) && !viewport->PlatformRequestResize)
            g.PlatformIO.Platform_SetWindowSize(viewport, viewport->Size);
        if ((viewport->LastRendererSize.x != viewport->Size.x || viewport->LastRendererSize.y != viewport->Size.y) && g.PlatformIO.Renderer_SetWindowSize)
            g.PlatformIO.Renderer_SetWindowSize(viewport, viewport->Size);
        viewport->LastPlatformPos = viewport->Pos;
        viewport->LastPlatformSize = viewport->LastRendererSize = viewport->Size;

        if (ImGuiWindow* window_for_title = GetWindowForTitleDisplay(viewport->Window))
        {
            const char* title_begin = window_for_title->Name;
            char* title_end = (char*)(intptr_t)FindRenderedTextEnd(title_begin);
            const ImGuiID title_hash = ImHashStr(title_begin, title_end - title_begin);
            if (viewport->LastNameHash != title_hash)
            {
                char title_end_backup_c = *title_end;
                *title_end = 0;             
                g.PlatformIO.Platform_SetWindowTitle(viewport, title_begin);
                *title_end = title_end_backup_c;
                viewport->LastNameHash = title_hash;
            }
        }

        if (viewport->LastAlpha != viewport->Alpha && g.PlatformIO.Platform_SetWindowAlpha)
            g.PlatformIO.Platform_SetWindowAlpha(viewport, viewport->Alpha);
        viewport->LastAlpha = viewport->Alpha;

        if (g.PlatformIO.Platform_UpdateWindow)
            g.PlatformIO.Platform_UpdateWindow(viewport);

        if (is_new_platform_window)
        {
            if (g.FrameCount < 3)
                viewport->Flags |= ImGuiViewportFlags_NoFocusOnAppearing;

            g.PlatformIO.Platform_ShowWindow(viewport);

            if (viewport->LastFrontMostStampCount != g.ViewportFrontMostStampCount)
                viewport->LastFrontMostStampCount = ++g.ViewportFrontMostStampCount;
        }

        viewport->ClearRequestFlags();
    }

    if (g.PlatformIO.Platform_GetWindowFocus != NULL)
    {
        ImGuiViewportP* focused_viewport = NULL;
        for (int n = 0; n < g.Viewports.Size && focused_viewport == NULL; n++)
        {
            ImGuiViewportP* viewport = g.Viewports[n];
            if (viewport->PlatformWindowCreated)
                if (g.PlatformIO.Platform_GetWindowFocus(viewport))
                    focused_viewport = viewport;
        }

        if (focused_viewport && focused_viewport->LastFrontMostStampCount != g.ViewportFrontMostStampCount)
            focused_viewport->LastFrontMostStampCount = ++g.ViewportFrontMostStampCount;
    }
}

void ImGui::RenderPlatformWindowsDefault(void* platform_render_arg, void* renderer_render_arg)
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int i = 1; i < platform_io.Viewports.Size; i++)
    {
        ImGuiViewport* viewport = platform_io.Viewports[i];
        if (viewport->Flags & ImGuiViewportFlags_Minimized)
            continue;
        if (platform_io.Platform_RenderWindow) platform_io.Platform_RenderWindow(viewport, platform_render_arg);
        if (platform_io.Renderer_RenderWindow) platform_io.Renderer_RenderWindow(viewport, renderer_render_arg);
    }
    for (int i = 1; i < platform_io.Viewports.Size; i++)
    {
        ImGuiViewport* viewport = platform_io.Viewports[i];
        if (viewport->Flags & ImGuiViewportFlags_Minimized)
            continue;
        if (platform_io.Platform_SwapBuffers) platform_io.Platform_SwapBuffers(viewport, platform_render_arg);
        if (platform_io.Renderer_SwapBuffers) platform_io.Renderer_SwapBuffers(viewport, renderer_render_arg);
    }
}

static int ImGui::FindPlatformMonitorForPos(const ImVec2& pos)
{
    ImGuiContext& g = *GImGui;
    for (int monitor_n = 0; monitor_n < g.PlatformIO.Monitors.Size; monitor_n++)
    {
        const ImGuiPlatformMonitor& monitor = g.PlatformIO.Monitors[monitor_n];
        if (ImRect(monitor.MainPos, monitor.MainPos + monitor.MainSize).Contains(pos))
            return monitor_n;
    }
    return -1;
}

static int ImGui::FindPlatformMonitorForRect(const ImRect& rect)
{
    ImGuiContext& g = *GImGui;

    const int monitor_count = g.PlatformIO.Monitors.Size;
    if (monitor_count <= 1)
        return monitor_count - 1;

    const float surface_threshold = ImMax(rect.GetWidth() * rect.GetHeight() * 0.5f, 1.0f);
    int best_monitor_n = -1;
    float best_monitor_surface = 0.001f;

    for (int monitor_n = 0; monitor_n < g.PlatformIO.Monitors.Size && best_monitor_surface < surface_threshold; monitor_n++)
    {
        const ImGuiPlatformMonitor& monitor = g.PlatformIO.Monitors[monitor_n];
        const ImRect monitor_rect = ImRect(monitor.MainPos, monitor.MainPos + monitor.MainSize);
        if (monitor_rect.Contains(rect))
            return monitor_n;
        ImRect overlapping_rect = rect;
        overlapping_rect.ClipWithFull(monitor_rect);
        float overlapping_surface = overlapping_rect.GetWidth() * overlapping_rect.GetHeight();
        if (overlapping_surface < best_monitor_surface)
            continue;
        best_monitor_surface = overlapping_surface;
        best_monitor_n = monitor_n;
    }
    return best_monitor_n;
}

static void ImGui::UpdateViewportPlatformMonitor(ImGuiViewportP* viewport)
{
    viewport->PlatformMonitor = (short)FindPlatformMonitorForRect(viewport->GetMainRect());
}

void ImGui::DestroyPlatformWindow(ImGuiViewportP* viewport)
{
    ImGuiContext& g = *GImGui;
    if (viewport->PlatformWindowCreated)
    {
        if (g.PlatformIO.Renderer_DestroyWindow)
            g.PlatformIO.Renderer_DestroyWindow(viewport);
        if (g.PlatformIO.Platform_DestroyWindow)
            g.PlatformIO.Platform_DestroyWindow(viewport);
        IM_ASSERT(viewport->RendererUserData == NULL && viewport->PlatformUserData == NULL);

        if (viewport->ID != IMGUI_VIEWPORT_DEFAULT_ID)
            viewport->PlatformWindowCreated = false;
    }
    else
    {
        IM_ASSERT(viewport->RendererUserData == NULL && viewport->PlatformUserData == NULL && viewport->PlatformHandle == NULL);
    }
    viewport->RendererUserData = viewport->PlatformUserData = viewport->PlatformHandle = NULL;
    viewport->ClearRequestFlags();
}

void ImGui::DestroyPlatformWindows()
{
    ImGuiContext& g = *GImGui;
    for (int i = 0; i < g.Viewports.Size; i++)
        DestroyPlatformWindow(g.Viewports[i]);
}


enum ImGuiDockRequestType
{
    ImGuiDockRequestType_None = 0,
    ImGuiDockRequestType_Dock,
    ImGuiDockRequestType_Undock,
    ImGuiDockRequestType_Split                            
};

struct ImGuiDockRequest
{
    ImGuiDockRequestType    Type;
    ImGuiWindow*            DockTargetWindow;                          
    ImGuiDockNode*          DockTargetNode;          
    ImGuiWindow*            DockPayload;                     
    ImGuiDir                DockSplitDir;
    float                   DockSplitRatio;
    bool                    DockSplitOuter;
    ImGuiWindow*            UndockTargetWindow;
    ImGuiDockNode*          UndockTargetNode;

    ImGuiDockRequest()
    {
        Type = ImGuiDockRequestType_None;
        DockTargetWindow = DockPayload = UndockTargetWindow = NULL;
        DockTargetNode = UndockTargetNode = NULL;
        DockSplitDir = ImGuiDir_None;
        DockSplitRatio = 0.5f;
        DockSplitOuter = false;
    }
};

struct ImGuiDockPreviewData
{
    ImGuiDockNode   FutureNode;
    bool            IsDropAllowed;
    bool            IsCenterAvailable;
    bool            IsSidesAvailable;                
    bool            IsSplitDirExplicit;                      
    ImGuiDockNode*  SplitNode;
    ImGuiDir        SplitDir;
    float           SplitRatio;
    ImRect          DropRectsDraw[ImGuiDir_COUNT + 1];             

    ImGuiDockPreviewData() : FutureNode(0) { IsDropAllowed = IsCenterAvailable = IsSidesAvailable = IsSplitDirExplicit = false; SplitNode = NULL; SplitDir = ImGuiDir_None; SplitRatio = 0.f; for (int n = 0; n < IM_ARRAYSIZE(DropRectsDraw); n++) DropRectsDraw[n] = ImRect(+FLT_MAX, +FLT_MAX, -FLT_MAX, -FLT_MAX); }
};

struct ImGuiDockNodeSettings
{
    ImGuiID             ID;
    ImGuiID             ParentNodeId;
    ImGuiID             ParentWindowId;
    ImGuiID             SelectedWindowId;
    signed char         SplitAxis;
    char                Depth;
    ImGuiDockNodeFlags  Flags;                              
    ImVec2ih            Pos;
    ImVec2ih            Size;
    ImVec2ih            SizeRef;
    ImGuiDockNodeSettings() { ID = ParentNodeId = ParentWindowId = SelectedWindowId = 0; SplitAxis = ImGuiAxis_None; Depth = 0; Flags = ImGuiDockNodeFlags_None; }
};

namespace ImGui
{
    static ImGuiDockNode*   DockContextAddNode(ImGuiContext* ctx, ImGuiID id);
    static void             DockContextRemoveNode(ImGuiContext* ctx, ImGuiDockNode* node, bool merge_sibling_into_parent_node);
    static void             DockContextQueueNotifyRemovedNode(ImGuiContext* ctx, ImGuiDockNode* node);
    static void             DockContextProcessDock(ImGuiContext* ctx, ImGuiDockRequest* req);
    static void             DockContextProcessUndockWindow(ImGuiContext* ctx, ImGuiWindow* window, bool clear_persistent_docking_ref = true);
    static void             DockContextProcessUndockNode(ImGuiContext* ctx, ImGuiDockNode* node);
    static void             DockContextPruneUnusedSettingsNodes(ImGuiContext* ctx);
    static ImGuiDockNode*   DockContextFindNodeByID(ImGuiContext* ctx, ImGuiID id);
    static ImGuiDockNode*   DockContextBindNodeToWindow(ImGuiContext* ctx, ImGuiWindow* window);
    static void             DockContextBuildNodesFromSettings(ImGuiContext* ctx, ImGuiDockNodeSettings* node_settings_array, int node_settings_count);
    static void             DockContextBuildAddWindowsToNodes(ImGuiContext* ctx, ImGuiID root_id);                                 

    static void             DockNodeAddWindow(ImGuiDockNode* node, ImGuiWindow* window, bool add_to_tab_bar);
    static void             DockNodeMoveWindows(ImGuiDockNode* dst_node, ImGuiDockNode* src_node);
    static void             DockNodeMoveChildNodes(ImGuiDockNode* dst_node, ImGuiDockNode* src_node);
    static ImGuiWindow*     DockNodeFindWindowByID(ImGuiDockNode* node, ImGuiID id);
    static void             DockNodeApplyPosSizeToWindows(ImGuiDockNode* node);
    static void             DockNodeRemoveWindow(ImGuiDockNode* node, ImGuiWindow* window, ImGuiID save_dock_id);
    static void             DockNodeHideHostWindow(ImGuiDockNode* node);
    static void             DockNodeUpdate(ImGuiDockNode* node);
    static void             DockNodeUpdateForRootNode(ImGuiDockNode* node);
    static void             DockNodeUpdateVisibleFlagAndInactiveChilds(ImGuiDockNode* node);
    static void             DockNodeUpdateTabBar(ImGuiDockNode* node, ImGuiWindow* host_window);
    static void             DockNodeAddTabBar(ImGuiDockNode* node);
    static void             DockNodeRemoveTabBar(ImGuiDockNode* node);
    static ImGuiID          DockNodeUpdateWindowMenu(ImGuiDockNode* node, ImGuiTabBar* tab_bar);
    static void             DockNodeUpdateVisibleFlag(ImGuiDockNode* node);
    static void             DockNodeStartMouseMovingWindow(ImGuiDockNode* node, ImGuiWindow* window);
    static bool             DockNodeIsDropAllowed(ImGuiWindow* host_window, ImGuiWindow* payload_window);
    static void             DockNodePreviewDockSetup(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* payload_window, ImGuiDockPreviewData* preview_data, bool is_explicit_target, bool is_outer_docking);
    static void             DockNodePreviewDockRender(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* payload_window, const ImGuiDockPreviewData* preview_data);
    static void             DockNodeCalcTabBarLayout(const ImGuiDockNode* node, ImRect* out_title_rect, ImRect* out_tab_bar_rect, ImVec2* out_window_menu_button_pos);
    static void             DockNodeCalcSplitRects(ImVec2& pos_old, ImVec2& size_old, ImVec2& pos_new, ImVec2& size_new, ImGuiDir dir, ImVec2 size_new_desired);
    static bool             DockNodeCalcDropRectsAndTestMousePos(const ImRect& parent, ImGuiDir dir, ImRect& out_draw, bool outer_docking, ImVec2* test_mouse_pos);
    static const char*      DockNodeGetHostWindowTitle(ImGuiDockNode* node, char* buf, int buf_size) { ImFormatString(buf, buf_size, "##DockNode_%02X", node->ID); return buf; }
    static int              DockNodeGetTabOrder(ImGuiWindow* window);

    static void             DockNodeTreeSplit(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiAxis split_axis, int split_first_child, float split_ratio, ImGuiDockNode* new_node);
    static void             DockNodeTreeMerge(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiDockNode* merge_lead_child);
    static void             DockNodeTreeUpdatePosSize(ImGuiDockNode* node, ImVec2 pos, ImVec2 size, bool only_write_to_marked_nodes = false);
    static void             DockNodeTreeUpdateSplitter(ImGuiDockNode* node);
    static ImGuiDockNode*   DockNodeTreeFindVisibleNodeByPos(ImGuiDockNode* node, ImVec2 pos);
    static ImGuiDockNode*   DockNodeTreeFindFallbackLeafNode(ImGuiDockNode* node);

    static void             DockSettingsRenameNodeReferences(ImGuiID old_node_id, ImGuiID new_node_id);
    static void             DockSettingsRemoveNodeReferences(ImGuiID* node_ids, int node_ids_count);
    static ImGuiDockNodeSettings*   DockSettingsFindNodeSettings(ImGuiContext* ctx, ImGuiID node_id);
    static void             DockSettingsHandler_ClearAll(ImGuiContext*, ImGuiSettingsHandler*);
    static void             DockSettingsHandler_ApplyAll(ImGuiContext*, ImGuiSettingsHandler*);
    static void*            DockSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
    static void             DockSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
    static void             DockSettingsHandler_WriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);
}

void ImGui::DockContextInitialize(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;

    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "Docking";
    ini_handler.TypeHash = ImHashStr("Docking");
    ini_handler.ClearAllFn = DockSettingsHandler_ClearAll;
    ini_handler.ReadInitFn = DockSettingsHandler_ClearAll;     
    ini_handler.ReadOpenFn = DockSettingsHandler_ReadOpen;
    ini_handler.ReadLineFn = DockSettingsHandler_ReadLine;
    ini_handler.ApplyAllFn = DockSettingsHandler_ApplyAll;
    ini_handler.WriteAllFn = DockSettingsHandler_WriteAll;
    g.SettingsHandlers.push_back(ini_handler);
}

void ImGui::DockContextShutdown(ImGuiContext* ctx)
{
    ImGuiDockContext* dc  = &ctx->DockContext;
    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
            IM_DELETE(node);
}

void ImGui::DockContextClearNodes(ImGuiContext* ctx, ImGuiID root_id, bool clear_settings_refs)
{
    IM_UNUSED(ctx);
    IM_ASSERT(ctx == GImGui);
    DockBuilderRemoveNodeDockedWindows(root_id, clear_settings_refs);
    DockBuilderRemoveNodeChildNodes(root_id);
}

void ImGui::DockContextRebuildNodes(ImGuiContext* ctx)
{
    IMGUI_DEBUG_LOG_DOCKING("DockContextRebuild()\n");
    ImGuiDockContext* dc  = &ctx->DockContext;
    SaveIniSettingsToMemory();
    ImGuiID root_id = 0;   
    DockContextClearNodes(ctx, root_id, false);
    DockContextBuildNodesFromSettings(ctx, dc->NodesSettings.Data, dc->NodesSettings.Size);
    DockContextBuildAddWindowsToNodes(ctx, root_id);
}

void ImGui::DockContextUpdateUndocking(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc  = &ctx->DockContext;
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
    {
        if (dc->Nodes.Data.Size > 0 || dc->Requests.Size > 0)
            DockContextClearNodes(ctx, 0, true);
        return;
    }

    if (g.IO.ConfigDockingNoSplit)
        for (int n = 0; n < dc->Nodes.Data.Size; n++)
            if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
                if (node->IsRootNode() && node->IsSplitNode())
                {
                    DockBuilderRemoveNodeChildNodes(node->ID);
                }

#if 0
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
        dc->WantFullRebuild = true;
#endif
    if (dc->WantFullRebuild)
    {
        DockContextRebuildNodes(ctx);
        dc->WantFullRebuild = false;
    }

    for (int n = 0; n < dc->Requests.Size; n++)
    {
        ImGuiDockRequest* req = &dc->Requests[n];
        if (req->Type == ImGuiDockRequestType_Undock && req->UndockTargetWindow)
            DockContextProcessUndockWindow(ctx, req->UndockTargetWindow);
        else if (req->Type == ImGuiDockRequestType_Undock && req->UndockTargetNode)
            DockContextProcessUndockNode(ctx, req->UndockTargetNode);
    }
}

void ImGui::DockContextUpdateDocking(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc  = &ctx->DockContext;
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
        return;

    g.HoveredDockNode = NULL;
    if (ImGuiWindow* hovered_window = g.HoveredWindowUnderMovingWindow)
    {
        if (hovered_window->DockNodeAsHost)
            g.HoveredDockNode = DockNodeTreeFindVisibleNodeByPos(hovered_window->DockNodeAsHost, g.IO.MousePos);
        else if (hovered_window->RootWindowDockStop->DockNode)
            g.HoveredDockNode = hovered_window->RootWindowDockStop->DockNode;
    }

    for (int n = 0; n < dc->Requests.Size; n++)
        if (dc->Requests[n].Type == ImGuiDockRequestType_Dock)
            DockContextProcessDock(ctx, &dc->Requests[n]);
    dc->Requests.resize(0);

    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
            if (node->IsFloatingNode())
                DockNodeUpdate(node);
}

static ImGuiDockNode* ImGui::DockContextFindNodeByID(ImGuiContext* ctx, ImGuiID id)
{
    return (ImGuiDockNode*)ctx->DockContext.Nodes.GetVoidPtr(id);
}

ImGuiID ImGui::DockContextGenNodeID(ImGuiContext* ctx)
{
    ImGuiID id = 0x0001;
    while (DockContextFindNodeByID(ctx, id) != NULL)
        id++;
    return id;
}

static ImGuiDockNode* ImGui::DockContextAddNode(ImGuiContext* ctx, ImGuiID id)
{
    if (id == 0)
        id = DockContextGenNodeID(ctx);
    else
        IM_ASSERT(DockContextFindNodeByID(ctx, id) == NULL);

    IMGUI_DEBUG_LOG_DOCKING("DockContextAddNode 0x%08X\n", id);
    ImGuiDockNode* node = IM_NEW(ImGuiDockNode)(id);
    ctx->DockContext.Nodes.SetVoidPtr(node->ID, node);
    return node;
}

static void ImGui::DockContextRemoveNode(ImGuiContext* ctx, ImGuiDockNode* node, bool merge_sibling_into_parent_node)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc  = &ctx->DockContext;

    IMGUI_DEBUG_LOG_DOCKING("DockContextRemoveNode 0x%08X\n", node->ID);
    IM_ASSERT(DockContextFindNodeByID(ctx, node->ID) == node);
    IM_ASSERT(node->ChildNodes[0] == NULL && node->ChildNodes[1] == NULL);
    IM_ASSERT(node->Windows.Size == 0);

    if (node->HostWindow)
        node->HostWindow->DockNodeAsHost = NULL;

    ImGuiDockNode* parent_node = node->ParentNode;
    const bool merge = (merge_sibling_into_parent_node && parent_node != NULL);
    if (merge)
    {
        IM_ASSERT(parent_node->ChildNodes[0] == node || parent_node->ChildNodes[1] == node);
        ImGuiDockNode* sibling_node = (parent_node->ChildNodes[0] == node ? parent_node->ChildNodes[1] : parent_node->ChildNodes[0]);
        DockNodeTreeMerge(&g, parent_node, sibling_node);
    }
    else
    {
        for (int n = 0; parent_node && n < IM_ARRAYSIZE(parent_node->ChildNodes); n++)
            if (parent_node->ChildNodes[n] == node)
                node->ParentNode->ChildNodes[n] = NULL;
        dc->Nodes.SetVoidPtr(node->ID, NULL);
        IM_DELETE(node);
    }
}

static int IMGUI_CDECL DockNodeComparerDepthMostFirst(const void* lhs, const void* rhs)
{
    const ImGuiDockNode* a = *(const ImGuiDockNode* const*)lhs;
    const ImGuiDockNode* b = *(const ImGuiDockNode* const*)rhs;
    return ImGui::DockNodeGetDepth(b) - ImGui::DockNodeGetDepth(a);
}

struct ImGuiDockContextPruneNodeData
{
    int         CountWindows, CountChildWindows, CountChildNodes;
    ImGuiID     RootId;
    ImGuiDockContextPruneNodeData() { CountWindows = CountChildWindows = CountChildNodes = 0; RootId = 0; }
};

static void ImGui::DockContextPruneUnusedSettingsNodes(ImGuiContext* ctx)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc  = &ctx->DockContext;
    IM_ASSERT(g.Windows.Size == 0);

    ImPool<ImGuiDockContextPruneNodeData> pool;
    pool.Reserve(dc->NodesSettings.Size);

    for (int settings_n = 0; settings_n < dc->NodesSettings.Size; settings_n++)
    {
        ImGuiDockNodeSettings* settings = &dc->NodesSettings[settings_n];
        ImGuiDockContextPruneNodeData* parent_data = settings->ParentNodeId ? pool.GetByKey(settings->ParentNodeId) : 0;
        pool.GetOrAddByKey(settings->ID)->RootId = parent_data ? parent_data->RootId : settings->ID;
        if (settings->ParentNodeId)
            pool.GetOrAddByKey(settings->ParentNodeId)->CountChildNodes++;
    }

    for (int settings_n = 0; settings_n < dc->NodesSettings.Size; settings_n++)
    {
        ImGuiDockNodeSettings* settings = &dc->NodesSettings[settings_n];
        if (settings->ParentWindowId != 0)
            if (ImGuiWindowSettings* window_settings = FindWindowSettings(settings->ParentWindowId))
                if (window_settings->DockId)
                    if (ImGuiDockContextPruneNodeData* data = pool.GetByKey(window_settings->DockId))
                        data->CountChildNodes++;
    }

    for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
        if (ImGuiID dock_id = settings->DockId)
            if (ImGuiDockContextPruneNodeData* data = pool.GetByKey(dock_id))
            {
                data->CountWindows++;
                if (ImGuiDockContextPruneNodeData* data_root = (data->RootId == dock_id) ? data : pool.GetByKey(data->RootId))
                    data_root->CountChildWindows++;
            }

    for (int settings_n = 0; settings_n < dc->NodesSettings.Size; settings_n++)
    {
        ImGuiDockNodeSettings* settings = &dc->NodesSettings[settings_n];
        ImGuiDockContextPruneNodeData* data = pool.GetByKey(settings->ID);
        if (data->CountWindows > 1)
            continue;
        ImGuiDockContextPruneNodeData* data_root = (data->RootId == settings->ID) ? data : pool.GetByKey(data->RootId);

        bool remove = false;
        remove |= (data->CountWindows == 1 && settings->ParentNodeId == 0 && data->CountChildNodes == 0 && !(settings->Flags & ImGuiDockNodeFlags_CentralNode));         
        remove |= (data->CountWindows == 0 && settings->ParentNodeId == 0 && data->CountChildNodes == 0);      
        remove |= (data_root->CountChildWindows == 0);
        if (remove)
        {
            IMGUI_DEBUG_LOG_DOCKING("DockContextPruneUnusedSettingsNodes: Prune 0x%08X\n", settings->ID);
            DockSettingsRemoveNodeReferences(&settings->ID, 1);
            settings->ID = 0;
        }
    }
}

static void ImGui::DockContextBuildNodesFromSettings(ImGuiContext* ctx, ImGuiDockNodeSettings* node_settings_array, int node_settings_count)
{
    for (int node_n = 0; node_n < node_settings_count; node_n++)
    {
        ImGuiDockNodeSettings* settings = &node_settings_array[node_n];
        if (settings->ID == 0)
            continue;
        ImGuiDockNode* node = DockContextAddNode(ctx, settings->ID);
        node->ParentNode = settings->ParentNodeId ? DockContextFindNodeByID(ctx, settings->ParentNodeId) : NULL;
        node->Pos = ImVec2(settings->Pos.x, settings->Pos.y);
        node->Size = ImVec2(settings->Size.x, settings->Size.y);
        node->SizeRef = ImVec2(settings->SizeRef.x, settings->SizeRef.y);
        node->AuthorityForPos = node->AuthorityForSize = node->AuthorityForViewport = ImGuiDataAuthority_DockNode;
        if (node->ParentNode && node->ParentNode->ChildNodes[0] == NULL)
            node->ParentNode->ChildNodes[0] = node;
        else if (node->ParentNode && node->ParentNode->ChildNodes[1] == NULL)
            node->ParentNode->ChildNodes[1] = node;
        node->SelectedTabId = settings->SelectedWindowId;
        node->SplitAxis = (ImGuiAxis)settings->SplitAxis;
        node->LocalFlags |= (settings->Flags & ImGuiDockNodeFlags_SavedFlagsMask_);

        char host_window_title[20];
        ImGuiDockNode* root_node = DockNodeGetRootNode(node);
        node->HostWindow = FindWindowByName(DockNodeGetHostWindowTitle(root_node, host_window_title, IM_ARRAYSIZE(host_window_title)));
    }
}

void ImGui::DockContextBuildAddWindowsToNodes(ImGuiContext* ctx, ImGuiID root_id)
{
    ImGuiContext& g = *ctx;
    for (int n = 0; n < g.Windows.Size; n++)
    {
        ImGuiWindow* window = g.Windows[n];
        if (window->DockId == 0 || window->LastFrameActive < g.FrameCount - 1)
            continue;
        if (window->DockNode != NULL)
            continue;

        ImGuiDockNode* node = DockContextFindNodeByID(ctx, window->DockId);
        IM_ASSERT(node != NULL);          
        if (root_id == 0 || DockNodeGetRootNode(node)->ID == root_id)
            DockNodeAddWindow(node, window, true);
    }
}

void ImGui::DockContextQueueDock(ImGuiContext* ctx, ImGuiWindow* target, ImGuiDockNode* target_node, ImGuiWindow* payload, ImGuiDir split_dir, float split_ratio, bool split_outer)
{
    IM_ASSERT(target != payload);
    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Dock;
    req.DockTargetWindow = target;
    req.DockTargetNode = target_node;
    req.DockPayload = payload;
    req.DockSplitDir = split_dir;
    req.DockSplitRatio = split_ratio;
    req.DockSplitOuter = split_outer;
    ctx->DockContext.Requests.push_back(req);
}

void ImGui::DockContextQueueUndockWindow(ImGuiContext* ctx, ImGuiWindow* window)
{
    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Undock;
    req.UndockTargetWindow = window;
    ctx->DockContext.Requests.push_back(req);
}

void ImGui::DockContextQueueUndockNode(ImGuiContext* ctx, ImGuiDockNode* node)
{
    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Undock;
    req.UndockTargetNode = node;
    ctx->DockContext.Requests.push_back(req);
}

void ImGui::DockContextQueueNotifyRemovedNode(ImGuiContext* ctx, ImGuiDockNode* node)
{
    ImGuiDockContext* dc  = &ctx->DockContext;
    for (int n = 0; n < dc->Requests.Size; n++)
        if (dc->Requests[n].DockTargetNode == node)
            dc->Requests[n].Type = ImGuiDockRequestType_None;
}

void ImGui::DockContextProcessDock(ImGuiContext* ctx, ImGuiDockRequest* req)
{
    IM_ASSERT((req->Type == ImGuiDockRequestType_Dock && req->DockPayload != NULL) || (req->Type == ImGuiDockRequestType_Split && req->DockPayload == NULL));
    IM_ASSERT(req->DockTargetWindow != NULL || req->DockTargetNode != NULL);

    ImGuiContext& g = *ctx;
    IM_UNUSED(g);

    ImGuiWindow* payload_window = req->DockPayload;      
    ImGuiWindow* target_window = req->DockTargetWindow;
    ImGuiDockNode* node = req->DockTargetNode;
    if (payload_window)
        IMGUI_DEBUG_LOG_DOCKING("DockContextProcessDock node 0x%08X target '%s' dock window '%s', split_dir %d\n", node ? node->ID : 0, target_window ? target_window->Name : "NULL", payload_window ? payload_window->Name : "NULL", req->DockSplitDir);
    else
        IMGUI_DEBUG_LOG_DOCKING("DockContextProcessDock node 0x%08X, split_dir %d\n", node ? node->ID : 0, req->DockSplitDir);

    ImGuiID next_selected_id = 0;
    ImGuiDockNode* payload_node = NULL;
    if (payload_window)
    {
        payload_node = payload_window->DockNodeAsHost;
        payload_window->DockNodeAsHost = NULL;                    
        if (payload_node && payload_node->IsLeafNode())
            next_selected_id = payload_node->TabBar->NextSelectedTabId ? payload_node->TabBar->NextSelectedTabId : payload_node->TabBar->SelectedTabId;
        if (payload_node == NULL)
            next_selected_id = payload_window->ID;
    }

    if (node)
        IM_ASSERT(node->LastFrameAlive <= g.FrameCount);
    if (node && target_window && node == target_window->DockNodeAsHost)
        IM_ASSERT(node->Windows.Size > 0 || node->IsSplitNode() || node->IsCentralNode());

    if (node == NULL)
    {
        node = DockContextAddNode(ctx, 0);
        node->Pos = target_window->Pos;
        node->Size = target_window->Size;
        if (target_window->DockNodeAsHost == NULL)
        {
            DockNodeAddWindow(node, target_window, true);
            node->TabBar->Tabs[0].Flags &= ~ImGuiTabItemFlags_Unsorted;
            target_window->DockIsActive = true;
        }
    }

    ImGuiDir split_dir = req->DockSplitDir;
    if (split_dir != ImGuiDir_None)
    {
        const ImGuiAxis split_axis = (split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;
        const int split_inheritor_child_idx = (split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? 1 : 0;          
        const float split_ratio = req->DockSplitRatio;
        DockNodeTreeSplit(ctx, node, split_axis, split_inheritor_child_idx, split_ratio, payload_node);       
        ImGuiDockNode* new_node = node->ChildNodes[split_inheritor_child_idx ^ 1];
        new_node->HostWindow = node->HostWindow;
        node = new_node;
    }
    node->LocalFlags &= ~ImGuiDockNodeFlags_HiddenTabBar;

    if (node != payload_node)
    {
        if (node->Windows.Size > 0 && node->TabBar == NULL)
        {
            DockNodeAddTabBar(node);
            for (int n = 0; n < node->Windows.Size; n++)
                TabBarAddTab(node->TabBar, ImGuiTabItemFlags_None, node->Windows[n]);
        }

        if (payload_node != NULL)
        {
            if (payload_node->IsSplitNode())
            {
                if (node->Windows.Size > 0)
                {
                    IM_ASSERT(payload_node->OnlyNodeWithWindows != NULL);              
                    ImGuiDockNode* visible_node = payload_node->OnlyNodeWithWindows;
                    if (visible_node->TabBar)
                        IM_ASSERT(visible_node->TabBar->Tabs.Size > 0);
                    DockNodeMoveWindows(node, visible_node);
                    DockNodeMoveWindows(visible_node, node);
                    DockSettingsRenameNodeReferences(node->ID, visible_node->ID);
                }
                if (node->IsCentralNode())
                {
                    ImGuiDockNode* last_focused_node = DockContextFindNodeByID(ctx, payload_node->LastFocusedNodeId);
                    IM_ASSERT(last_focused_node != NULL);
                    ImGuiDockNode* last_focused_root_node = DockNodeGetRootNode(last_focused_node);
                    IM_ASSERT(last_focused_root_node == DockNodeGetRootNode(payload_node));
                    last_focused_node->LocalFlags |= ImGuiDockNodeFlags_CentralNode;
                    node->LocalFlags &= ~ImGuiDockNodeFlags_CentralNode;
                    last_focused_root_node->CentralNode = last_focused_node;
                }

                IM_ASSERT(node->Windows.Size == 0);
                DockNodeMoveChildNodes(node, payload_node);
            }
            else
            {
                const ImGuiID payload_dock_id = payload_node->ID;
                DockNodeMoveWindows(node, payload_node);
                DockSettingsRenameNodeReferences(payload_dock_id, node->ID);
            }
            DockContextRemoveNode(ctx, payload_node, true);
        }
        else if (payload_window)
        {
            const ImGuiID payload_dock_id = payload_window->DockId;
            node->VisibleWindow = payload_window;
            DockNodeAddWindow(node, payload_window, true);
            if (payload_dock_id != 0)
                DockSettingsRenameNodeReferences(payload_dock_id, node->ID);
        }
    }
    else
    {
        node->WantHiddenTabBarUpdate = true;
    }

    if (ImGuiTabBar* tab_bar = node->TabBar)
        tab_bar->NextSelectedTabId = next_selected_id;
    MarkIniSettingsDirty();
}

void ImGui::DockContextProcessUndockWindow(ImGuiContext* ctx, ImGuiWindow* window, bool clear_persistent_docking_ref)
{
    IMGUI_DEBUG_LOG_DOCKING("DockContextProcessUndockWindow window '%s', clear_persistent_docking_ref = %d\n", window->Name, clear_persistent_docking_ref);
    IM_UNUSED(ctx);
    if (window->DockNode)
        DockNodeRemoveWindow(window->DockNode, window, clear_persistent_docking_ref ? 0 : window->DockId);
    else
        window->DockId = 0;
    window->Collapsed = false;
    window->DockIsActive = false;
    window->DockTabIsVisible = false;
    MarkIniSettingsDirty();
}

void ImGui::DockContextProcessUndockNode(ImGuiContext* ctx, ImGuiDockNode* node)
{
    IMGUI_DEBUG_LOG_DOCKING("DockContextProcessUndockNode node %08X\n", node->ID);
    IM_ASSERT(node->IsLeafNode());
    IM_ASSERT(node->Windows.Size >= 1);

    if (node->IsRootNode() || node->IsCentralNode())
    {
        ImGuiDockNode* new_node = DockContextAddNode(ctx, 0);
        DockNodeMoveWindows(new_node, node);
        DockSettingsRenameNodeReferences(node->ID, new_node->ID);
        for (int n = 0; n < new_node->Windows.Size; n++)
            UpdateWindowParentAndRootLinks(new_node->Windows[n], new_node->Windows[n]->Flags, NULL);
        node = new_node;
    }
    else
    {
        IM_ASSERT(node->ParentNode->ChildNodes[0] == node || node->ParentNode->ChildNodes[1] == node);
        int index_in_parent = (node->ParentNode->ChildNodes[0] == node) ? 0 : 1;
        node->ParentNode->ChildNodes[index_in_parent] = NULL;
        DockNodeTreeMerge(ctx, node->ParentNode, node->ParentNode->ChildNodes[index_in_parent ^ 1]);
        node->ParentNode->AuthorityForViewport = ImGuiDataAuthority_Window;                     
        node->ParentNode = NULL;
    }
    node->AuthorityForPos = node->AuthorityForSize = ImGuiDataAuthority_Window;
    node->WantMouseMove = true;
    MarkIniSettingsDirty();
}

bool ImGui::DockContextCalcDropPosForDocking(ImGuiWindow* target, ImGuiDockNode* target_node, ImGuiWindow* payload, ImGuiDir split_dir, bool split_outer, ImVec2* out_pos)
{
    if (split_outer)
    {
        IM_ASSERT(0);
    }
    else
    {
        ImGuiDockPreviewData split_data;
        DockNodePreviewDockSetup(target, target_node, payload, &split_data, false, split_outer);
        if (split_data.DropRectsDraw[split_dir+1].IsInverted())
            return false;
        *out_pos = split_data.DropRectsDraw[split_dir+1].GetCenter();
        return true;
    }
    return false;
}

ImGuiDockNode::ImGuiDockNode(ImGuiID id)
{
    ID = id;
    SharedFlags = LocalFlags = ImGuiDockNodeFlags_None;
    ParentNode = ChildNodes[0] = ChildNodes[1] = NULL;
    TabBar = NULL;
    SplitAxis = ImGuiAxis_None;

    State = ImGuiDockNodeState_Unknown;
    HostWindow = VisibleWindow = NULL;
    CentralNode = OnlyNodeWithWindows = NULL;
    LastFrameAlive = LastFrameActive = LastFrameFocused = -1;
    LastFocusedNodeId = 0;
    SelectedTabId = 0;
    WantCloseTabId = 0;
    AuthorityForPos = AuthorityForSize = ImGuiDataAuthority_DockNode;
    AuthorityForViewport = ImGuiDataAuthority_Auto;
    IsVisible = true;
    IsFocused = HasCloseButton = HasWindowMenuButton = EnableCloseButton = false;
    WantCloseAll = WantLockSizeOnce = WantMouseMove = WantHiddenTabBarUpdate = WantHiddenTabBarToggle = false;
    MarkedForPosSizeWrite = false;
}

ImGuiDockNode::~ImGuiDockNode()
{
    IM_DELETE(TabBar);
    TabBar = NULL;
    ChildNodes[0] = ChildNodes[1] = NULL;
}

int ImGui::DockNodeGetTabOrder(ImGuiWindow* window)
{
    ImGuiTabBar* tab_bar = window->DockNode->TabBar;
    if (tab_bar == NULL)
        return -1;
    ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, window->ID);
    return tab ? tab_bar->GetTabOrder(tab) : -1;
}

static void ImGui::DockNodeAddWindow(ImGuiDockNode* node, ImGuiWindow* window, bool add_to_tab_bar)
{
    ImGuiContext& g = *GImGui; (void)g;
    if (window->DockNode)
    {
        IM_ASSERT(window->DockNode->ID != node->ID);
        DockNodeRemoveWindow(window->DockNode, window, 0);
    }
    IM_ASSERT(window->DockNode == NULL || window->DockNodeAsHost == NULL);
    IMGUI_DEBUG_LOG_DOCKING("DockNodeAddWindow node 0x%08X window '%s'\n", node->ID, window->Name);

    node->Windows.push_back(window);
    node->WantHiddenTabBarUpdate = true;
    window->DockNode = node;
    window->DockId = node->ID;
    window->DockIsActive = (node->Windows.Size > 1);
    window->DockTabWantClose = false;

    if (node->HostWindow == NULL && node->Windows.Size == 2 && node->Windows[0]->WasActive == false)
    {
        node->Windows[0]->Hidden = true;
        node->Windows[0]->HiddenFramesCanSkipItems = 1;
    }

    if (node->HostWindow == NULL && node->IsFloatingNode())
    {
        if (node->AuthorityForPos == ImGuiDataAuthority_Auto)
            node->AuthorityForPos = ImGuiDataAuthority_Window;
        if (node->AuthorityForSize == ImGuiDataAuthority_Auto)
            node->AuthorityForSize = ImGuiDataAuthority_Window;
        if (node->AuthorityForViewport == ImGuiDataAuthority_Auto)
            node->AuthorityForViewport = ImGuiDataAuthority_Window;
    }

    if (add_to_tab_bar)
    {
        if (node->TabBar == NULL)
        {
            DockNodeAddTabBar(node);
            node->TabBar->SelectedTabId = node->TabBar->NextSelectedTabId = node->SelectedTabId;

            for (int n = 0; n < node->Windows.Size - 1; n++)
                TabBarAddTab(node->TabBar, ImGuiTabItemFlags_None, node->Windows[n]);
        }
        TabBarAddTab(node->TabBar, ImGuiTabItemFlags_Unsorted, window);
    }

    DockNodeUpdateVisibleFlag(node);

    if (node->HostWindow)
        UpdateWindowParentAndRootLinks(window, window->Flags | ImGuiWindowFlags_ChildWindow, node->HostWindow);
}

static void ImGui::DockNodeRemoveWindow(ImGuiDockNode* node, ImGuiWindow* window, ImGuiID save_dock_id)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(window->DockNode == node);
    IM_ASSERT(save_dock_id == 0 || save_dock_id == node->ID);
    IMGUI_DEBUG_LOG_DOCKING("DockNodeRemoveWindow node 0x%08X window '%s'\n", node->ID, window->Name);

    window->DockNode = NULL;
    window->DockIsActive = window->DockTabWantClose = false;
    window->DockId = save_dock_id;
    UpdateWindowParentAndRootLinks(window, window->Flags & ~ImGuiWindowFlags_ChildWindow, NULL);   

    bool erased = false;
    for (int n = 0; n < node->Windows.Size; n++)
        if (node->Windows[n] == window)
        {
            node->Windows.erase(node->Windows.Data + n);
            erased = true;
            break;
        }
    IM_ASSERT(erased);
    if (node->VisibleWindow == window)
        node->VisibleWindow = NULL;

    node->WantHiddenTabBarUpdate = true;
    if (node->TabBar)
    {
        TabBarRemoveTab(node->TabBar, window->ID);
        const int tab_count_threshold_for_tab_bar = node->IsCentralNode() ? 1 : 2;
        if (node->Windows.Size < tab_count_threshold_for_tab_bar)
            DockNodeRemoveTabBar(node);
    }

    if (node->Windows.Size == 0 && !node->IsCentralNode() && !node->IsDockSpace() && window->DockId != node->ID)
    {
        DockContextRemoveNode(&g, node, true);
        return;
    }

    if (node->Windows.Size == 1 && !node->IsCentralNode() && node->HostWindow)
    {
        ImGuiWindow* remaining_window = node->Windows[0];
        if (node->HostWindow->ViewportOwned && node->IsRootNode())
        {
            IM_ASSERT(node->HostWindow->Viewport->Window == node->HostWindow);
            node->HostWindow->Viewport->Window = remaining_window;
            node->HostWindow->Viewport->ID = remaining_window->ID;
        }
        remaining_window->Collapsed = node->HostWindow->Collapsed;
    }

    DockNodeUpdateVisibleFlag(node);
}

static void ImGui::DockNodeMoveChildNodes(ImGuiDockNode* dst_node, ImGuiDockNode* src_node)
{
    IM_ASSERT(dst_node->Windows.Size == 0);
    dst_node->ChildNodes[0] = src_node->ChildNodes[0];
    dst_node->ChildNodes[1] = src_node->ChildNodes[1];
    if (dst_node->ChildNodes[0])
        dst_node->ChildNodes[0]->ParentNode = dst_node;
    if (dst_node->ChildNodes[1])
        dst_node->ChildNodes[1]->ParentNode = dst_node;
    dst_node->SplitAxis = src_node->SplitAxis;
    dst_node->SizeRef = src_node->SizeRef;
    src_node->ChildNodes[0] = src_node->ChildNodes[1] = NULL;
}

static void ImGui::DockNodeMoveWindows(ImGuiDockNode* dst_node, ImGuiDockNode* src_node)
{
    IM_ASSERT(src_node && dst_node && dst_node != src_node);
    ImGuiTabBar* src_tab_bar = src_node->TabBar;
    if (src_tab_bar != NULL)
        IM_ASSERT(src_node->Windows.Size <= src_node->TabBar->Tabs.Size);

    bool move_tab_bar = (src_tab_bar != NULL) && (dst_node->TabBar == NULL);
    if (move_tab_bar)
    {
        dst_node->TabBar = src_node->TabBar;
        src_node->TabBar = NULL;
    }

    for (int n = 0; n < src_node->Windows.Size; n++)
    {
        if (ImGuiWindow* window = src_tab_bar ? src_tab_bar->Tabs[n].Window : src_node->Windows[n])
        {
            window->DockNode = NULL;
            window->DockIsActive = false;
            DockNodeAddWindow(dst_node, window, move_tab_bar ? false : true);
        }
    }
    src_node->Windows.clear();

    if (!move_tab_bar && src_node->TabBar)
    {
        if (dst_node->TabBar)
            dst_node->TabBar->SelectedTabId = src_node->TabBar->SelectedTabId;
        DockNodeRemoveTabBar(src_node);
    }
}

static void ImGui::DockNodeApplyPosSizeToWindows(ImGuiDockNode* node)
{
    for (int n = 0; n < node->Windows.Size; n++)
    {
        SetWindowPos(node->Windows[n], node->Pos, ImGuiCond_Always);                  
        SetWindowSize(node->Windows[n], node->Size, ImGuiCond_Always);
    }
}

static void ImGui::DockNodeHideHostWindow(ImGuiDockNode* node)
{
    if (node->HostWindow)
    {
        if (node->HostWindow->DockNodeAsHost == node)
            node->HostWindow->DockNodeAsHost = NULL;
        node->HostWindow = NULL;
    }

    if (node->Windows.Size == 1)
    {
        node->VisibleWindow = node->Windows[0];
        node->Windows[0]->DockIsActive = false;
    }

    if (node->TabBar)
        DockNodeRemoveTabBar(node);
}

struct ImGuiDockNodeFindInfoResults
{
    ImGuiDockNode*      CentralNode;
    ImGuiDockNode*      FirstNodeWithWindows;
    int                 CountNodesWithWindows;
    ImGuiDockNodeFindInfoResults() { CentralNode = FirstNodeWithWindows = NULL; CountNodesWithWindows = 0; }
};

static void DockNodeFindInfo(ImGuiDockNode* node, ImGuiDockNodeFindInfoResults* results)
{
    if (node->Windows.Size > 0)
    {
        if (results->FirstNodeWithWindows == NULL)
            results->FirstNodeWithWindows = node;
        results->CountNodesWithWindows++;
    }
    if (node->IsCentralNode())
    {
        IM_ASSERT(results->CentralNode == NULL);     
        IM_ASSERT(node->IsLeafNode() && "If you get this assert: please submit .ini file + repro of actions leading to this.");
        results->CentralNode = node;
    }
    if (results->CountNodesWithWindows > 1 && results->CentralNode != NULL)
        return;
    if (node->ChildNodes[0])
        DockNodeFindInfo(node->ChildNodes[0], results);
    if (node->ChildNodes[1])
        DockNodeFindInfo(node->ChildNodes[1], results);
}

static ImGuiWindow* ImGui::DockNodeFindWindowByID(ImGuiDockNode* node, ImGuiID id)
{
    IM_ASSERT(id != 0);
    for (int n = 0; n < node->Windows.Size; n++)
        if (node->Windows[n]->ID == id)
            return node->Windows[n];
    return NULL;
}

static void ImGui::DockNodeUpdateVisibleFlagAndInactiveChilds(ImGuiDockNode* node)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(node->ParentNode == NULL || node->ParentNode->ChildNodes[0] == node || node->ParentNode->ChildNodes[1] == node);

    if (node->ParentNode)
        node->SharedFlags = node->ParentNode->SharedFlags & ImGuiDockNodeFlags_SharedFlagsInheritMask_;

    if (node->ChildNodes[0])
        DockNodeUpdateVisibleFlagAndInactiveChilds(node->ChildNodes[0]);
    if (node->ChildNodes[1])
        DockNodeUpdateVisibleFlagAndInactiveChilds(node->ChildNodes[1]);

    for (int window_n = 0; window_n < node->Windows.Size; window_n++)
    {
        ImGuiWindow* window = node->Windows[window_n];
        IM_ASSERT(window->DockNode == node);

        bool node_was_active = (node->LastFrameActive + 1 == g.FrameCount);
        bool remove = false;
        remove |= node_was_active && (window->LastFrameActive + 1 < g.FrameCount);
        remove |= node_was_active && (node->WantCloseAll || node->WantCloseTabId == window->ID) && window->HasCloseButton && !(window->Flags & ImGuiWindowFlags_UnsavedDocument);         
        remove |= (window->DockTabWantClose);
        if (remove)
        {
            window->DockTabWantClose = false;
            if (node->Windows.Size == 1 && !node->IsCentralNode())
            {
                DockNodeHideHostWindow(node);
                node->State = ImGuiDockNodeState_HostWindowHiddenBecauseSingleWindow;
                DockNodeRemoveWindow(node, window, node->ID);           
                return;
            }
            DockNodeRemoveWindow(node, window, node->ID);
            window_n--;
        }
        else
        {
            node->LocalFlags &= ~window->WindowClass.DockNodeFlagsOverrideClear;
            node->LocalFlags |= window->WindowClass.DockNodeFlagsOverrideSet;
        }
    }

    ImGuiDockNodeFlags node_flags = node->GetMergedFlags();
    if (node->WantHiddenTabBarUpdate && node->Windows.Size == 1 && (node_flags & ImGuiDockNodeFlags_AutoHideTabBar) && !node->IsHiddenTabBar())
        node->WantHiddenTabBarToggle = true;
    node->WantHiddenTabBarUpdate = false;

    if (node->WantHiddenTabBarToggle && node->VisibleWindow && (node->VisibleWindow->WindowClass.DockNodeFlagsOverrideSet & ImGuiDockNodeFlags_HiddenTabBar))
        node->WantHiddenTabBarToggle = false;

    if (node->Windows.Size > 1)
        node->LocalFlags &= ~ImGuiDockNodeFlags_HiddenTabBar;
    else if (node->WantHiddenTabBarToggle)
        node->LocalFlags ^= ImGuiDockNodeFlags_HiddenTabBar;
    node->WantHiddenTabBarToggle = false;

    DockNodeUpdateVisibleFlag(node);
}

static void ImGui::DockNodeUpdateVisibleFlag(ImGuiDockNode* node)
{
    bool is_visible = (node->ParentNode == NULL) ? node->IsDockSpace() : node->IsCentralNode();
    is_visible |= (node->Windows.Size > 0);
    is_visible |= (node->ChildNodes[0] && node->ChildNodes[0]->IsVisible);
    is_visible |= (node->ChildNodes[1] && node->ChildNodes[1]->IsVisible);
    node->IsVisible = is_visible;
}

static void ImGui::DockNodeStartMouseMovingWindow(ImGuiDockNode* node, ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(node->WantMouseMove == true);
    StartMouseMovingWindow(window);
    g.ActiveIdClickOffset = g.IO.MouseClickedPos[0] - node->Pos;
    g.MovingWindow = window;                  
    node->WantMouseMove = false;
}

static void ImGui::DockNodeUpdateForRootNode(ImGuiDockNode* node)
{
    DockNodeUpdateVisibleFlagAndInactiveChilds(node);

    ImGuiDockNodeFindInfoResults results;
    DockNodeFindInfo(node, &results);
    node->CentralNode = results.CentralNode;
    node->OnlyNodeWithWindows = (results.CountNodesWithWindows == 1) ? results.FirstNodeWithWindows : NULL;
    if (node->LastFocusedNodeId == 0 && results.FirstNodeWithWindows != NULL)
        node->LastFocusedNodeId = results.FirstNodeWithWindows->ID;

    if (ImGuiDockNode* first_node_with_windows = results.FirstNodeWithWindows)
    {
        node->WindowClass = first_node_with_windows->Windows[0]->WindowClass;
        for (int n = 1; n < first_node_with_windows->Windows.Size; n++)
            if (first_node_with_windows->Windows[n]->WindowClass.DockingAllowUnclassed == false)
            {
                node->WindowClass = first_node_with_windows->Windows[n]->WindowClass;
                break;
            }
    }
}

static void ImGui::DockNodeUpdate(ImGuiDockNode* node)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(node->LastFrameActive != g.FrameCount);
    node->LastFrameAlive = g.FrameCount;
    node->MarkedForPosSizeWrite = false;

    node->CentralNode = node->OnlyNodeWithWindows = NULL;
    if (node->IsRootNode())
        DockNodeUpdateForRootNode(node);

    if (node->TabBar && node->IsNoTabBar())
        DockNodeRemoveTabBar(node);

    bool want_to_hide_host_window = false;
    if (node->Windows.Size <= 1 && node->IsFloatingNode() && node->IsLeafNode())
        if (!g.IO.ConfigDockingAlwaysTabBar && (node->Windows.Size == 0 || !node->Windows[0]->WindowClass.DockingAlwaysTabBar))
            want_to_hide_host_window = true;
    if (want_to_hide_host_window)
    {
        if (node->Windows.Size == 1)
        {
            ImGuiWindow* single_window = node->Windows[0];
            node->Pos = single_window->Pos;
            node->Size = single_window->SizeFull;
            node->AuthorityForPos = node->AuthorityForSize = node->AuthorityForViewport = ImGuiDataAuthority_Window;

            if (node->HostWindow && g.NavWindow == node->HostWindow)
                FocusWindow(single_window);
            if (node->HostWindow)
            {
                single_window->Viewport = node->HostWindow->Viewport;
                single_window->ViewportId = node->HostWindow->ViewportId;
                if (node->HostWindow->ViewportOwned)
                {
                    single_window->Viewport->Window = single_window;
                    single_window->ViewportOwned = true;
                }
            }
        }

        DockNodeHideHostWindow(node);
        node->State = ImGuiDockNodeState_HostWindowHiddenBecauseSingleWindow;
        node->WantCloseAll = false;
        node->WantCloseTabId = 0;
        node->HasCloseButton = node->HasWindowMenuButton = node->EnableCloseButton = false;
        node->LastFrameActive = g.FrameCount;

        if (node->WantMouseMove && node->Windows.Size == 1)
            DockNodeStartMouseMovingWindow(node, node->Windows[0]);
        return;
    }

    if (node->IsVisible && node->HostWindow == NULL && node->IsFloatingNode() && node->IsLeafNode())
    {
        IM_ASSERT(node->Windows.Size > 0);
        ImGuiWindow* ref_window = NULL;
        if (node->SelectedTabId != 0)                 
            ref_window = DockNodeFindWindowByID(node, node->SelectedTabId);
        if (ref_window == NULL)
            ref_window = node->Windows[0];
        if (ref_window->AutoFitFramesX > 0 || ref_window->AutoFitFramesY > 0)
        {
            node->State = ImGuiDockNodeState_HostWindowHiddenBecauseWindowsAreResizing;
            return;
        }
    }

    const ImGuiDockNodeFlags node_flags = node->GetMergedFlags();

    ImGuiWindow* host_window = NULL;
    bool beginned_into_host_window = false;
    if (node->IsDockSpace())
    {
        IM_ASSERT(node->HostWindow);
        node->EnableCloseButton = false;
        node->HasCloseButton = (node_flags & ImGuiDockNodeFlags_NoCloseButton) == 0;
        node->HasWindowMenuButton = (node_flags & ImGuiDockNodeFlags_NoWindowMenuButton) == 0;
        host_window = node->HostWindow;
    }
    else
    {
        node->EnableCloseButton = false;
        node->HasCloseButton = (node->Windows.Size > 0) && (node_flags & ImGuiDockNodeFlags_NoCloseButton) == 0;
        node->HasWindowMenuButton = (node->Windows.Size > 0) && (node_flags & ImGuiDockNodeFlags_NoWindowMenuButton) == 0;
        for (int window_n = 0; window_n < node->Windows.Size; window_n++)
        {
            ImGuiWindow* window = node->Windows[window_n];
            window->DockIsActive = (node->Windows.Size > 1);
            node->EnableCloseButton |= window->HasCloseButton;
        }

        if (node->IsRootNode() && node->IsVisible)
        {
            ImGuiWindow* ref_window = (node->Windows.Size > 0) ? node->Windows[0] : NULL;

            if (node->AuthorityForPos == ImGuiDataAuthority_Window && ref_window)
                SetNextWindowPos(ref_window->Pos);
            else if (node->AuthorityForPos == ImGuiDataAuthority_DockNode)
                SetNextWindowPos(node->Pos);

            if (node->AuthorityForSize == ImGuiDataAuthority_Window && ref_window)
                SetNextWindowSize(ref_window->SizeFull);
            else if (node->AuthorityForSize == ImGuiDataAuthority_DockNode)
                SetNextWindowSize(node->Size);

            if (node->AuthorityForSize == ImGuiDataAuthority_Window && ref_window)
                SetNextWindowCollapsed(ref_window->Collapsed);

            if (node->AuthorityForViewport == ImGuiDataAuthority_Window && ref_window)
                SetNextWindowViewport(ref_window->ViewportId);

            SetNextWindowClass(&node->WindowClass);

            char window_label[20];
            DockNodeGetHostWindowTitle(node, window_label, IM_ARRAYSIZE(window_label));
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_DockNodeHost;
            window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
            window_flags |= ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoCollapse;
            window_flags |= ImGuiWindowFlags_NoTitleBar;

            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            Begin(window_label, NULL, window_flags);
            PopStyleVar();
            beginned_into_host_window = true;

            node->HostWindow = host_window = g.CurrentWindow;
            host_window->DockNodeAsHost = node;
            host_window->DC.CursorPos = host_window->Pos;
            node->Pos = host_window->Pos;
            node->Size = host_window->Size;

            if (node->HostWindow->Appearing)
                BringWindowToDisplayFront(node->HostWindow);

            node->AuthorityForPos = node->AuthorityForSize = node->AuthorityForViewport = ImGuiDataAuthority_Auto;
        }
        else if (node->ParentNode)
        {
            node->HostWindow = host_window = node->ParentNode->HostWindow;
            node->AuthorityForPos = node->AuthorityForSize = node->AuthorityForViewport = ImGuiDataAuthority_Auto;
        }
        if (node->WantMouseMove && node->HostWindow)
            DockNodeStartMouseMovingWindow(node, node->HostWindow);
    }

    if (node->IsSplitNode())
        IM_ASSERT(node->TabBar == NULL);
    if (node->IsRootNode())
        if (g.NavWindow && g.NavWindow->RootWindowDockStop->DockNode && g.NavWindow->RootWindowDockStop->ParentWindow == host_window)
            node->LastFocusedNodeId = g.NavWindow->RootWindowDockStop->DockNode->ID;

    const bool render_dockspace_bg = node->IsRootNode() && host_window && (node_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0;
    if (render_dockspace_bg)
    {
        host_window->DrawList->ChannelsSplit(2);
        host_window->DrawList->ChannelsSetCurrent(1);
    }

    const ImGuiDockNode* central_node = node->CentralNode;
    const bool central_node_hole = node->IsRootNode() && host_window && (node_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0 && central_node != NULL && central_node->IsEmpty();
    bool central_node_hole_register_hit_test_hole = central_node_hole;
    if (central_node_hole)
        if (const ImGuiPayload* payload = ImGui::GetDragDropPayload())
            if (payload->IsDataType(IMGUI_PAYLOAD_TYPE_WINDOW) && DockNodeIsDropAllowed(host_window, *(ImGuiWindow**)payload->Data))
                central_node_hole_register_hit_test_hole = false;
    if (central_node_hole_register_hit_test_hole)
    {
        IM_ASSERT(node->IsDockSpace());                    
        ImRect central_hole(central_node->Pos, central_node->Pos + central_node->Size);
        central_hole.Expand(ImVec2(-WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS, -WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS));
        if (central_node_hole && !central_hole.IsInverted())
        {
            SetWindowHitTestHole(host_window, central_hole.Min, central_hole.Max - central_hole.Min);
            SetWindowHitTestHole(host_window->ParentWindow, central_hole.Min, central_hole.Max - central_hole.Min);
        }
    }

    if (node->IsRootNode() && host_window)
    {
        DockNodeTreeUpdatePosSize(node, host_window->Pos, host_window->Size);
        DockNodeTreeUpdateSplitter(node);
    }

    if (host_window && node->IsEmpty() && node->IsVisible && !(node_flags & ImGuiDockNodeFlags_PassthruCentralNode))
        host_window->DrawList->AddRectFilled(node->Pos, node->Pos + node->Size, GetColorU32(ImGuiCol_DockingEmptyBg));

    if (render_dockspace_bg && node->IsVisible)
    {
        host_window->DrawList->ChannelsSetCurrent(0);
        if (central_node_hole)
            RenderRectFilledWithHole(host_window->DrawList, node->Rect(), central_node->Rect(), GetColorU32(ImGuiCol_WindowBg), 0.0f);
        else
            host_window->DrawList->AddRectFilled(node->Pos, node->Pos + node->Size, GetColorU32(ImGuiCol_WindowBg), 0.0f);
        host_window->DrawList->ChannelsMerge();
    }

    if (host_window && node->Windows.Size > 0)
    {
        DockNodeUpdateTabBar(node, host_window);
    }
    else
    {
        node->WantCloseAll = false;
        node->WantCloseTabId = 0;
        node->IsFocused = false;
    }
    if (node->TabBar && node->TabBar->SelectedTabId)
        node->SelectedTabId = node->TabBar->SelectedTabId;
    else if (node->Windows.Size > 0)
        node->SelectedTabId = node->Windows[0]->ID;

    if (host_window && node->IsVisible)
        if (node->IsRootNode() && (g.MovingWindow == NULL || g.MovingWindow->RootWindow != host_window))
            BeginDockableDragDropTarget(host_window);

    node->LastFrameActive = g.FrameCount;

    if (host_window)
    {
        if (node->ChildNodes[0])
            DockNodeUpdate(node->ChildNodes[0]);
        if (node->ChildNodes[1])
            DockNodeUpdate(node->ChildNodes[1]);

        if (node->IsRootNode())
            RenderWindowOuterBorders(host_window);
    }

    if (beginned_into_host_window) 
        End();
}

static int IMGUI_CDECL TabItemComparerByDockOrder(const void* lhs, const void* rhs)
{
    ImGuiWindow* a = ((const ImGuiTabItem*)lhs)->Window;
    ImGuiWindow* b = ((const ImGuiTabItem*)rhs)->Window;
    if (int d = ((a->DockOrder == -1) ? INT_MAX : a->DockOrder) - ((b->DockOrder == -1) ? INT_MAX : b->DockOrder))
        return d;
    return (a->BeginOrderWithinContext - b->BeginOrderWithinContext);
}

static ImGuiID ImGui::DockNodeUpdateWindowMenu(ImGuiDockNode* node, ImGuiTabBar* tab_bar)
{
    ImGuiContext& g = *GImGui;
    ImGuiID ret_tab_id = 0;
    if (g.Style.WindowMenuButtonPosition == ImGuiDir_Left)
        SetNextWindowPos(ImVec2(node->Pos.x, node->Pos.y + GetFrameHeight()), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    else
        SetNextWindowPos(ImVec2(node->Pos.x + node->Size.x, node->Pos.y + GetFrameHeight()), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    if (BeginPopup("#WindowMenu"))
    {
        node->IsFocused = true;
        if (tab_bar->Tabs.Size == 1)
        {
            if (MenuItem("Hide tab bar", NULL, node->IsHiddenTabBar()))
                node->WantHiddenTabBarToggle = true;
        }
        else
        {
            for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
            {
                ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
                if (tab->Flags & ImGuiTabItemFlags_Button)
                    continue;
                if (Selectable(tab_bar->GetTabName(tab), tab->ID == tab_bar->SelectedTabId))
                    ret_tab_id = tab->ID;
                SameLine();
                Text("   ");
            }
        }
        EndPopup();
    }
    return ret_tab_id;
}

bool ImGui::DockNodeBeginAmendTabBar(ImGuiDockNode* node)
{
    if (node->TabBar == NULL || node->HostWindow == NULL)
        return false;
    if (node->SharedFlags & ImGuiDockNodeFlags_KeepAliveOnly)
        return false;
    Begin(node->HostWindow->Name);
    PushOverrideID(node->ID);
    bool ret = BeginTabBarEx(node->TabBar, node->TabBar->BarRect, node->TabBar->Flags, node);
    IM_ASSERT(ret);
    return true;
}

void ImGui::DockNodeEndAmendTabBar()
{
    EndTabBar();
    PopID();
    End();
}

static void ImGui::DockNodeUpdateTabBar(ImGuiDockNode* node, ImGuiWindow* host_window)
{
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;

    const bool node_was_active = (node->LastFrameActive + 1 == g.FrameCount);
    const bool closed_all = node->WantCloseAll && node_was_active;
    const ImGuiID closed_one = node->WantCloseTabId && node_was_active;
    node->WantCloseAll = false;
    node->WantCloseTabId = 0;

    bool is_focused = false;
    ImGuiDockNode* root_node = DockNodeGetRootNode(node);
    if (g.NavWindowingTarget)
        is_focused = (g.NavWindowingTarget->DockNode == node);
    else if (g.NavWindow && g.NavWindow->RootWindowForTitleBarHighlight == host_window->RootWindow && root_node->LastFocusedNodeId == node->ID)
        is_focused = true;

    if (node->IsHiddenTabBar() || node->IsNoTabBar())
    {
        node->VisibleWindow = (node->Windows.Size > 0) ? node->Windows[0] : NULL;
        node->IsFocused = is_focused;
        if (is_focused)
            node->LastFrameFocused = g.FrameCount;
        if (node->VisibleWindow)
        {
            if (is_focused || root_node->VisibleWindow == NULL)
                root_node->VisibleWindow = node->VisibleWindow;
            if (node->TabBar)
                node->TabBar->VisibleTabId = node->VisibleWindow->ID;
        }
        return;
    }

    bool backup_skip_item = host_window->SkipItems;
    if (!node->IsDockSpace())
    {
        host_window->SkipItems = false;
        host_window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
    }

    PushOverrideID(node->ID);
    ImGuiTabBar* tab_bar = node->TabBar;
    bool tab_bar_is_recreated = (tab_bar == NULL);           
    if (tab_bar == NULL)
    {
        DockNodeAddTabBar(node);
        tab_bar = node->TabBar;
    }

    ImGuiID focus_tab_id = 0;
    node->IsFocused = is_focused;

    const ImGuiDockNodeFlags node_flags = node->GetMergedFlags();
    const bool has_window_menu_button = (node_flags & ImGuiDockNodeFlags_NoWindowMenuButton) == 0 && (style.WindowMenuButtonPosition != ImGuiDir_None);
    const bool has_close_button = (node_flags & ImGuiDockNodeFlags_NoCloseButton) == 0;

    if (has_window_menu_button && IsPopupOpen("#WindowMenu"))
    {
        if (ImGuiID tab_id = DockNodeUpdateWindowMenu(node, tab_bar))
            focus_tab_id = tab_bar->NextSelectedTabId = tab_id;
        is_focused |= node->IsFocused;
    }

    ImRect title_bar_rect, tab_bar_rect;
    ImVec2 window_menu_button_pos;
    DockNodeCalcTabBarLayout(node, &title_bar_rect, &tab_bar_rect, &window_menu_button_pos);

    if (is_focused)
        node->LastFrameFocused = g.FrameCount;
    ImU32 title_bar_col = GetColorU32(host_window->Collapsed ? ImGuiCol_TitleBgCollapsed : is_focused ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
    host_window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, host_window->WindowRounding, ImDrawCornerFlags_Top);

    if (has_window_menu_button)
    {
        if (CollapseButton(host_window->GetID("#COLLAPSE"), window_menu_button_pos, node))
            OpenPopup("#WindowMenu");
        if (IsItemActive())
            focus_tab_id = tab_bar->SelectedTabId;
    }

    const int tabs_count_old = tab_bar->Tabs.Size;
    for (int window_n = 0; window_n < node->Windows.Size; window_n++)
    {
        ImGuiWindow* window = node->Windows[window_n];
        if (g.NavWindow && g.NavWindow->RootWindowDockStop == window)
            tab_bar->SelectedTabId = window->ID;
        if (TabBarFindTabByID(tab_bar, window->ID) == NULL)
            TabBarAddTab(tab_bar, ImGuiTabItemFlags_Unsorted, window);
    }

    int tabs_unsorted_start = tab_bar->Tabs.Size;
    for (int tab_n = tab_bar->Tabs.Size - 1; tab_n >= 0 && (tab_bar->Tabs[tab_n].Flags & ImGuiTabItemFlags_Unsorted); tab_n--)
    {
        tab_bar->Tabs[tab_n].Flags &= ~ImGuiTabItemFlags_Unsorted;
        tabs_unsorted_start = tab_n;
    }
    if (tab_bar->Tabs.Size > tabs_unsorted_start)
    {
        IMGUI_DEBUG_LOG_DOCKING("In node 0x%08X: %d new appearing tabs:%s\n", node->ID, tab_bar->Tabs.Size - tabs_unsorted_start, (tab_bar->Tabs.Size > tabs_unsorted_start + 1) ? " (will sort)" : "");
        for (int tab_n = tabs_unsorted_start; tab_n < tab_bar->Tabs.Size; tab_n++)
            IMGUI_DEBUG_LOG_DOCKING(" - Tab '%s' Order %d\n", tab_bar->Tabs[tab_n].Window->Name, tab_bar->Tabs[tab_n].Window->DockOrder);
        if (tab_bar->Tabs.Size > tabs_unsorted_start + 1)
            ImQsort(tab_bar->Tabs.Data + tabs_unsorted_start, tab_bar->Tabs.Size - tabs_unsorted_start, sizeof(ImGuiTabItem), TabItemComparerByDockOrder);
    }

    if (tab_bar_is_recreated && TabBarFindTabByID(tab_bar, node->SelectedTabId) != NULL)
        tab_bar->SelectedTabId = tab_bar->NextSelectedTabId = node->SelectedTabId;
    else if (tab_bar->Tabs.Size > tabs_count_old)
        tab_bar->SelectedTabId = tab_bar->NextSelectedTabId = tab_bar->Tabs.back().Window->ID;

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs;   
    tab_bar_flags |= ImGuiTabBarFlags_SaveSettings | ImGuiTabBarFlags_DockNode;
    if (!host_window->Collapsed && is_focused)
        tab_bar_flags |= ImGuiTabBarFlags_IsFocused;
    BeginTabBarEx(tab_bar, tab_bar_rect, tab_bar_flags, node);
    node->VisibleWindow = NULL;
    for (int window_n = 0; window_n < node->Windows.Size; window_n++)
    {
        ImGuiWindow* window = node->Windows[window_n];
        if ((closed_all || closed_one == window->ID) && window->HasCloseButton && !(window->Flags & ImGuiWindowFlags_UnsavedDocument))
            continue;
        if (window->LastFrameActive + 1 >= g.FrameCount || !node_was_active)
        {
            ImGuiTabItemFlags tab_item_flags = 0;
            if (window->Flags & ImGuiWindowFlags_UnsavedDocument)
                tab_item_flags |= ImGuiTabItemFlags_UnsavedDocument;
            if (tab_bar->Flags & ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)
                tab_item_flags |= ImGuiTabItemFlags_NoCloseWithMiddleMouseButton;

            bool tab_open = true;
            TabItemEx(tab_bar, window->Name, window->HasCloseButton ? &tab_open : NULL, tab_item_flags, window);
            if (!tab_open)
                node->WantCloseTabId = window->ID;
            if (tab_bar->VisibleTabId == window->ID)
                node->VisibleWindow = window;

            window->DockTabItemStatusFlags = host_window->DC.LastItemStatusFlags;
            window->DockTabItemRect = host_window->DC.LastItemRect;

            if (g.NavWindow && g.NavWindow->RootWindowDockStop == window && (window->DC.NavLayerActiveMask & (1 << ImGuiNavLayer_Menu)) == 0)
                host_window->NavLastIds[1] = window->ID;
        }
    }

    if (node->VisibleWindow)
        if (is_focused || root_node->VisibleWindow == NULL)
            root_node->VisibleWindow = node->VisibleWindow;

    if (has_close_button && node->VisibleWindow)
    {
        if (!node->VisibleWindow->HasCloseButton)
        {
            PushItemFlag(ImGuiItemFlags_Disabled, true);
            PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_Text] * ImVec4(1.0f,1.0f,1.0f,0.5f));
        }
        const float button_sz = g.FontSize;
        if (CloseButton(host_window->GetID("#CLOSE"), title_bar_rect.GetTR() + ImVec2(-style.FramePadding.x * 2.0f - button_sz, 0.0f)))
            if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, tab_bar->VisibleTabId))
            {
                node->WantCloseTabId = tab->ID;
                TabBarCloseTab(tab_bar, tab);
            }
        if (!node->VisibleWindow->HasCloseButton)
        {
            PopStyleColor();
            PopItemFlag();
        }
    }

    ImGuiID title_bar_id = host_window->GetID("#TITLEBAR");
    if (g.HoveredId == 0 || g.HoveredId == title_bar_id || g.ActiveId == title_bar_id)
    {
        bool held;
        ButtonBehavior(title_bar_rect, title_bar_id, NULL, &held, ImGuiButtonFlags_AllowItemOverlap);
        if (g.HoveredId == title_bar_id)
        {
            host_window->DC.LastItemId = title_bar_id;
            SetItemAllowOverlap();
        }
        if (held)
        {
            if (IsMouseClicked(0))
                focus_tab_id = tab_bar->SelectedTabId;

            if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, tab_bar->SelectedTabId))
                StartMouseMovingWindowOrNode(tab->Window ? tab->Window : node->HostWindow, node, false);
        }
    }

    if (tab_bar->NextSelectedTabId)
        focus_tab_id = tab_bar->NextSelectedTabId;

    if (focus_tab_id != 0)
        if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, focus_tab_id))
            if (tab->Window)
            {
                FocusWindow(tab->Window);
                NavInitWindow(tab->Window, false);
            }

    EndTabBar();
    PopID();

    if (!node->IsDockSpace())
    {
        host_window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
        host_window->SkipItems = backup_skip_item;
    }
}

static void ImGui::DockNodeAddTabBar(ImGuiDockNode* node)
{
    IM_ASSERT(node->TabBar == NULL);
    node->TabBar = IM_NEW(ImGuiTabBar);
}

static void ImGui::DockNodeRemoveTabBar(ImGuiDockNode* node)
{
    if (node->TabBar == NULL)
        return;
    IM_DELETE(node->TabBar);
    node->TabBar = NULL;
}

static bool DockNodeIsDropAllowedOne(ImGuiWindow* payload, ImGuiWindow* host_window)
{
    if (host_window->DockNodeAsHost && host_window->DockNodeAsHost->IsDockSpace() && payload->BeginOrderWithinContext < host_window->BeginOrderWithinContext)
        return false;

    ImGuiWindowClass* host_class = host_window->DockNodeAsHost ? &host_window->DockNodeAsHost->WindowClass : &host_window->WindowClass;
    ImGuiWindowClass* payload_class = &payload->WindowClass;
    if (host_class->ClassId != payload_class->ClassId)
    {
        if (host_class->ClassId != 0 && host_class->DockingAllowUnclassed && payload_class->ClassId == 0)
            return true;
        if (payload_class->ClassId != 0 && payload_class->DockingAllowUnclassed && host_class->ClassId == 0)
            return true;
        return false;
    }

    return true;
}

static bool ImGui::DockNodeIsDropAllowed(ImGuiWindow* host_window, ImGuiWindow* root_payload)
{
    if (root_payload->DockNodeAsHost && root_payload->DockNodeAsHost->IsSplitNode())
        return true;

    const int payload_count = root_payload->DockNodeAsHost ? root_payload->DockNodeAsHost->Windows.Size : 1;
    for (int payload_n = 0; payload_n < payload_count; payload_n++)
    {
        ImGuiWindow* payload = root_payload->DockNodeAsHost ? root_payload->DockNodeAsHost->Windows[payload_n] : root_payload;
        if (DockNodeIsDropAllowedOne(payload, host_window))
            return true;
    }
    return false;
}

static void ImGui::DockNodeCalcTabBarLayout(const ImGuiDockNode* node, ImRect* out_title_rect, ImRect* out_tab_bar_rect, ImVec2* out_window_menu_button_pos)
{
    ImGuiContext& g = *GImGui;
    ImRect r = ImRect(node->Pos.x, node->Pos.y, node->Pos.x + node->Size.x, node->Pos.y + g.FontSize + g.Style.FramePadding.y * 2.0f);
    if (out_title_rect) { *out_title_rect = r; }

    ImVec2 window_menu_button_pos = r.Min;
    r.Min.x += g.Style.FramePadding.x;
    r.Max.x -= g.Style.FramePadding.x;
    if (node->HasCloseButton)
    {
        r.Max.x -= g.FontSize;                
    }
    if (node->HasWindowMenuButton && g.Style.WindowMenuButtonPosition == ImGuiDir_Left)
    {
        r.Min.x += g.FontSize;                       
    }
    else if (node->HasWindowMenuButton && g.Style.WindowMenuButtonPosition == ImGuiDir_Right)
    {
        r.Max.x -= g.FontSize + g.Style.FramePadding.x;
        window_menu_button_pos = ImVec2(r.Max.x, r.Min.y);
    }
    if (out_tab_bar_rect) { *out_tab_bar_rect = r; }
    if (out_window_menu_button_pos) { *out_window_menu_button_pos = window_menu_button_pos; }
}

void ImGui::DockNodeCalcSplitRects(ImVec2& pos_old, ImVec2& size_old, ImVec2& pos_new, ImVec2& size_new, ImGuiDir dir, ImVec2 size_new_desired)
{
    ImGuiContext& g = *GImGui;
    const float dock_spacing = g.Style.ItemInnerSpacing.x;
    const ImGuiAxis axis = (dir == ImGuiDir_Left || dir == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;
    pos_new[axis ^ 1] = pos_old[axis ^ 1];
    size_new[axis ^ 1] = size_old[axis ^ 1];

    const float w_avail = size_old[axis] - dock_spacing;
    if (size_new_desired[axis] > 0.0f && size_new_desired[axis] <= w_avail * 0.5f)
    {
        size_new[axis] = size_new_desired[axis];
        size_old[axis] = IM_FLOOR(w_avail - size_new[axis]);
    }
    else
    {
        size_new[axis] = IM_FLOOR(w_avail * 0.5f);
        size_old[axis] = IM_FLOOR(w_avail - size_new[axis]);
    }

    if (dir == ImGuiDir_Right || dir == ImGuiDir_Down)
    {
        pos_new[axis] = pos_old[axis] + size_old[axis] + dock_spacing;
    }
    else if (dir == ImGuiDir_Left || dir == ImGuiDir_Up)
    {
        pos_new[axis] = pos_old[axis];
        pos_old[axis] = pos_new[axis] + size_new[axis] + dock_spacing;
    }
}

bool ImGui::DockNodeCalcDropRectsAndTestMousePos(const ImRect& parent, ImGuiDir dir, ImRect& out_r, bool outer_docking, ImVec2* test_mouse_pos)
{
    ImGuiContext& g = *GImGui;

    const float parent_smaller_axis = ImMin(parent.GetWidth(), parent.GetHeight());
    const float hs_for_central_nodes = ImMin(g.FontSize * 1.5f, ImMax(g.FontSize * 0.5f, parent_smaller_axis / 8.0f));
    float hs_w;    
    float hs_h;    
    ImVec2 off;      
    if (outer_docking)
    {
        hs_w = ImFloor(hs_for_central_nodes * 1.50f);
        hs_h = ImFloor(hs_for_central_nodes * 0.80f);
        off = ImVec2(ImFloor(parent.GetWidth() * 0.5f - hs_h), ImFloor(parent.GetHeight() * 0.5f - hs_h));
    }
    else
    {
        hs_w = ImFloor(hs_for_central_nodes);
        hs_h = ImFloor(hs_for_central_nodes * 0.90f);
        off = ImVec2(ImFloor(hs_w * 2.40f), ImFloor(hs_w * 2.40f));
    }

    ImVec2 c = ImFloor(parent.GetCenter());
    if      (dir == ImGuiDir_None)  { out_r = ImRect(c.x - hs_w, c.y - hs_w,         c.x + hs_w, c.y + hs_w);         }
    else if (dir == ImGuiDir_Up)    { out_r = ImRect(c.x - hs_w, c.y - off.y - hs_h, c.x + hs_w, c.y - off.y + hs_h); }
    else if (dir == ImGuiDir_Down)  { out_r = ImRect(c.x - hs_w, c.y + off.y - hs_h, c.x + hs_w, c.y + off.y + hs_h); }
    else if (dir == ImGuiDir_Left)  { out_r = ImRect(c.x - off.x - hs_h, c.y - hs_w, c.x - off.x + hs_h, c.y + hs_w); }
    else if (dir == ImGuiDir_Right) { out_r = ImRect(c.x + off.x - hs_h, c.y - hs_w, c.x + off.x + hs_h, c.y + hs_w); }

    if (test_mouse_pos == NULL)
        return false;

    ImRect hit_r = out_r;
    if (!outer_docking)
    {
        hit_r.Expand(ImFloor(hs_w * 0.30f));
        ImVec2 mouse_delta = (*test_mouse_pos - c);
        float mouse_delta_len2 = ImLengthSqr(mouse_delta);
        float r_threshold_center = hs_w * 1.4f;
        float r_threshold_sides = hs_w * (1.4f + 1.2f);
        if (mouse_delta_len2 < r_threshold_center * r_threshold_center)
            return (dir == ImGuiDir_None);
        if (mouse_delta_len2 < r_threshold_sides * r_threshold_sides)
            return (dir == ImGetDirQuadrantFromDelta(mouse_delta.x, mouse_delta.y));
    }
    return hit_r.Contains(*test_mouse_pos);
}

static void ImGui::DockNodePreviewDockSetup(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* root_payload, ImGuiDockPreviewData* data, bool is_explicit_target, bool is_outer_docking)
{
    ImGuiContext& g = *GImGui;

    ImGuiDockNode* root_payload_as_host = root_payload->DockNodeAsHost;
    ImGuiDockNode* ref_node_for_rect = (host_node && !host_node->IsVisible) ? DockNodeGetRootNode(host_node) : host_node;
    if (ref_node_for_rect)
        IM_ASSERT(ref_node_for_rect->IsVisible);

    ImGuiDockNodeFlags src_node_flags = root_payload_as_host ? root_payload_as_host->GetMergedFlags() : root_payload->WindowClass.DockNodeFlagsOverrideSet;
    ImGuiDockNodeFlags dst_node_flags = host_node ? host_node->GetMergedFlags() : host_window->WindowClass.DockNodeFlagsOverrideSet;
    data->IsCenterAvailable = true;
    if (is_outer_docking)
        data->IsCenterAvailable = false;
    else if (dst_node_flags & ImGuiDockNodeFlags_NoDocking)
        data->IsCenterAvailable = false;
    else if (host_node && (dst_node_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) && host_node->IsCentralNode())
        data->IsCenterAvailable = false;
    else if ((!host_node || !host_node->IsEmpty()) && root_payload_as_host && root_payload_as_host->IsSplitNode() && (root_payload_as_host->OnlyNodeWithWindows == NULL))    
        data->IsCenterAvailable = false;
    else if ((dst_node_flags & ImGuiDockNodeFlags_NoDockingOverMe) || (src_node_flags & ImGuiDockNodeFlags_NoDockingOverOther))
        data->IsCenterAvailable = false;

    data->IsSidesAvailable = true;
    if ((dst_node_flags & ImGuiDockNodeFlags_NoSplit) || g.IO.ConfigDockingNoSplit)
        data->IsSidesAvailable = false;
    else if (!is_outer_docking && host_node && host_node->ParentNode == NULL && host_node->IsCentralNode())
        data->IsSidesAvailable = false;
    else if ((dst_node_flags & ImGuiDockNodeFlags_NoDockingSplitMe) || (src_node_flags & ImGuiDockNodeFlags_NoDockingSplitOther))
        data->IsSidesAvailable = false;

    data->FutureNode.HasCloseButton = (host_node ? host_node->HasCloseButton : host_window->HasCloseButton) || (root_payload->HasCloseButton);
    data->FutureNode.HasWindowMenuButton = host_node ? true : ((host_window->Flags & ImGuiWindowFlags_NoCollapse) == 0);
    data->FutureNode.Pos = ref_node_for_rect ? ref_node_for_rect->Pos : host_window->Pos;
    data->FutureNode.Size = ref_node_for_rect ? ref_node_for_rect->Size : host_window->Size;

    IM_ASSERT(ImGuiDir_None == -1);
    data->SplitNode = host_node;
    data->SplitDir = ImGuiDir_None;
    data->IsSplitDirExplicit = false;
    if (!host_window->Collapsed)
        for (int dir = ImGuiDir_None; dir < ImGuiDir_COUNT; dir++)
        {
            if (dir == ImGuiDir_None && !data->IsCenterAvailable)
                continue;
            if (dir != ImGuiDir_None && !data->IsSidesAvailable)
                continue;
            if (DockNodeCalcDropRectsAndTestMousePos(data->FutureNode.Rect(), (ImGuiDir)dir, data->DropRectsDraw[dir+1], is_outer_docking, &g.IO.MousePos))
            {
                data->SplitDir = (ImGuiDir)dir;
                data->IsSplitDirExplicit = true;
            }
        }

    data->IsDropAllowed = (data->SplitDir != ImGuiDir_None) || (data->IsCenterAvailable);
    if (!is_explicit_target && !data->IsSplitDirExplicit && !g.IO.ConfigDockingWithShift)
        data->IsDropAllowed = false;

    data->SplitRatio = 0.0f;
    if (data->SplitDir != ImGuiDir_None)
    {
        ImGuiDir split_dir = data->SplitDir;
        ImGuiAxis split_axis = (split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Right) ? ImGuiAxis_X : ImGuiAxis_Y;
        ImVec2 pos_new, pos_old = data->FutureNode.Pos;
        ImVec2 size_new, size_old = data->FutureNode.Size;
        DockNodeCalcSplitRects(pos_old, size_old, pos_new, size_new, split_dir, root_payload->Size);

        float split_ratio = ImSaturate(size_new[split_axis] / data->FutureNode.Size[split_axis]);
        data->FutureNode.Pos = pos_new;
        data->FutureNode.Size = size_new;
        data->SplitRatio = (split_dir == ImGuiDir_Right || split_dir == ImGuiDir_Down) ? (1.0f - split_ratio) : (split_ratio);
    }
}

static void ImGui::DockNodePreviewDockRender(ImGuiWindow* host_window, ImGuiDockNode* host_node, ImGuiWindow* root_payload, const ImGuiDockPreviewData* data)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.CurrentWindow == host_window);             

    const bool is_transparent_payload = g.IO.ConfigDockingTransparentPayload;

    int overlay_draw_lists_count = 0;
    ImDrawList* overlay_draw_lists[2];
    overlay_draw_lists[overlay_draw_lists_count++] = GetForegroundDrawList(host_window->Viewport);
    if (host_window->Viewport != root_payload->Viewport && !is_transparent_payload)
        overlay_draw_lists[overlay_draw_lists_count++] = GetForegroundDrawList(root_payload->Viewport);

    const ImU32 overlay_col_tabs = GetColorU32(ImGuiCol_TabActive);
    const ImU32 overlay_col_main = GetColorU32(ImGuiCol_DockingPreview, is_transparent_payload ? 0.60f : 0.40f);
    const ImU32 overlay_col_drop = GetColorU32(ImGuiCol_DockingPreview, is_transparent_payload ? 0.90f : 0.70f);
    const ImU32 overlay_col_drop_hovered = GetColorU32(ImGuiCol_DockingPreview, is_transparent_payload ? 1.20f : 1.00f);
    const ImU32 overlay_col_lines = GetColorU32(ImGuiCol_NavWindowingHighlight, is_transparent_payload ? 0.80f : 0.60f);

    const bool can_preview_tabs = (root_payload->DockNodeAsHost == NULL || root_payload->DockNodeAsHost->Windows.Size > 0);
    if (data->IsDropAllowed)
    {
        ImRect overlay_rect = data->FutureNode.Rect();
        if (data->SplitDir == ImGuiDir_None && can_preview_tabs)
            overlay_rect.Min.y += GetFrameHeight();
        if (data->SplitDir != ImGuiDir_None || data->IsCenterAvailable)
            for (int overlay_n = 0; overlay_n < overlay_draw_lists_count; overlay_n++)
                overlay_draw_lists[overlay_n]->AddRectFilled(overlay_rect.Min, overlay_rect.Max, overlay_col_main, host_window->WindowRounding);
    }

    if (data->IsDropAllowed && can_preview_tabs && data->SplitDir == ImGuiDir_None && data->IsCenterAvailable)
    {
        ImRect tab_bar_rect;
        DockNodeCalcTabBarLayout(&data->FutureNode, NULL, &tab_bar_rect, NULL);
        ImVec2 tab_pos = tab_bar_rect.Min;
        if (host_node && host_node->TabBar)
        {
            if (!host_node->IsHiddenTabBar() && !host_node->IsNoTabBar())
                tab_pos.x += host_node->TabBar->WidthAllTabs + g.Style.ItemInnerSpacing.x;                  
            else
                tab_pos.x += g.Style.ItemInnerSpacing.x + TabItemCalcSize(host_node->Windows[0]->Name, host_node->Windows[0]->HasCloseButton).x;
        }
        else if (!(host_window->Flags & ImGuiWindowFlags_DockNodeHost))
        {
            tab_pos.x += g.Style.ItemInnerSpacing.x + TabItemCalcSize(host_window->Name, host_window->HasCloseButton).x;                 
        }

        if (root_payload->DockNodeAsHost)
            IM_ASSERT(root_payload->DockNodeAsHost->Windows.Size <= root_payload->DockNodeAsHost->TabBar->Tabs.Size);
        ImGuiTabBar* tab_bar_with_payload = root_payload->DockNodeAsHost ? root_payload->DockNodeAsHost->TabBar : NULL;
        const int payload_count = tab_bar_with_payload ? tab_bar_with_payload->Tabs.Size : 1;
        for (int payload_n = 0; payload_n < payload_count; payload_n++)
        {
            ImGuiWindow* payload_window = tab_bar_with_payload ? tab_bar_with_payload->Tabs[payload_n].Window : root_payload;
            if (tab_bar_with_payload && payload_window == NULL)
                continue;
            if (!DockNodeIsDropAllowedOne(payload_window, host_window))
                continue;

            ImVec2 tab_size = TabItemCalcSize(payload_window->Name, payload_window->HasCloseButton);
            ImRect tab_bb(tab_pos.x, tab_pos.y, tab_pos.x + tab_size.x, tab_pos.y + tab_size.y);
            tab_pos.x += tab_size.x + g.Style.ItemInnerSpacing.x;
            for (int overlay_n = 0; overlay_n < overlay_draw_lists_count; overlay_n++)
            {
                ImGuiTabItemFlags tab_flags = ImGuiTabItemFlags_Preview | ((payload_window->Flags & ImGuiWindowFlags_UnsavedDocument) ? ImGuiTabItemFlags_UnsavedDocument : 0);
                if (!tab_bar_rect.Contains(tab_bb))
                    overlay_draw_lists[overlay_n]->PushClipRect(tab_bar_rect.Min, tab_bar_rect.Max);
                TabItemBackground(overlay_draw_lists[overlay_n], tab_bb, tab_flags, overlay_col_tabs);
                TabItemLabelAndCloseButton(overlay_draw_lists[overlay_n], tab_bb, tab_flags, g.Style.FramePadding, payload_window->Name, 0, 0, false, NULL, NULL);
                if (!tab_bar_rect.Contains(tab_bb))
                    overlay_draw_lists[overlay_n]->PopClipRect();
            }
        }
    }

    const float overlay_rounding = ImMax(3.0f, g.Style.FrameRounding);
    for (int dir = ImGuiDir_None; dir < ImGuiDir_COUNT; dir++)
    {
        if (!data->DropRectsDraw[dir + 1].IsInverted())
        {
            ImRect draw_r = data->DropRectsDraw[dir + 1];
            ImRect draw_r_in = draw_r;
            draw_r_in.Expand(-2.0f);
            ImU32 overlay_col = (data->SplitDir == (ImGuiDir)dir && data->IsSplitDirExplicit) ? overlay_col_drop_hovered : overlay_col_drop;
            for (int overlay_n = 0; overlay_n < overlay_draw_lists_count; overlay_n++)
            {
                ImVec2 center = ImFloor(draw_r_in.GetCenter());
                overlay_draw_lists[overlay_n]->AddRectFilled(draw_r.Min, draw_r.Max, overlay_col, overlay_rounding);
                overlay_draw_lists[overlay_n]->AddRect(draw_r_in.Min, draw_r_in.Max, overlay_col_lines, overlay_rounding);
                if (dir == ImGuiDir_Left || dir == ImGuiDir_Right)
                    overlay_draw_lists[overlay_n]->AddLine(ImVec2(center.x, draw_r_in.Min.y), ImVec2(center.x, draw_r_in.Max.y), overlay_col_lines);
                if (dir == ImGuiDir_Up || dir == ImGuiDir_Down)
                    overlay_draw_lists[overlay_n]->AddLine(ImVec2(draw_r_in.Min.x, center.y), ImVec2(draw_r_in.Max.x, center.y), overlay_col_lines);
            }
        }

        if ((host_node && (host_node->GetMergedFlags() & ImGuiDockNodeFlags_NoSplit)) || g.IO.ConfigDockingNoSplit)
            return;
    }
}

void ImGui::DockNodeTreeSplit(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiAxis split_axis, int split_inheritor_child_idx, float split_ratio, ImGuiDockNode* new_node)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(split_axis != ImGuiAxis_None);

    ImGuiDockNode* child_0 = (new_node && split_inheritor_child_idx != 0) ? new_node : DockContextAddNode(ctx, 0);
    child_0->ParentNode = parent_node;

    ImGuiDockNode* child_1 = (new_node && split_inheritor_child_idx != 1) ? new_node : DockContextAddNode(ctx, 0);
    child_1->ParentNode = parent_node;

    ImGuiDockNode* child_inheritor = (split_inheritor_child_idx == 0) ? child_0 : child_1;
    DockNodeMoveChildNodes(child_inheritor, parent_node);
    parent_node->ChildNodes[0] = child_0;
    parent_node->ChildNodes[1] = child_1;
    parent_node->ChildNodes[split_inheritor_child_idx]->VisibleWindow = parent_node->VisibleWindow;
    parent_node->SplitAxis = split_axis;
    parent_node->VisibleWindow = NULL;
    parent_node->AuthorityForPos = parent_node->AuthorityForSize = ImGuiDataAuthority_DockNode;

    float size_avail = (parent_node->Size[split_axis] - DOCKING_SPLITTER_SIZE);
    size_avail = ImMax(size_avail, g.Style.WindowMinSize[split_axis] * 2.0f);
    IM_ASSERT(size_avail > 0.0f);                 
    child_0->SizeRef = child_1->SizeRef = parent_node->Size;
    child_0->SizeRef[split_axis] = ImFloor(size_avail * split_ratio);
    child_1->SizeRef[split_axis] = ImFloor(size_avail - child_0->SizeRef[split_axis]);

    DockNodeMoveWindows(parent_node->ChildNodes[split_inheritor_child_idx], parent_node);
    DockNodeTreeUpdatePosSize(parent_node, parent_node->Pos, parent_node->Size);

    child_0->SharedFlags = parent_node->SharedFlags & ImGuiDockNodeFlags_SharedFlagsInheritMask_;
    child_1->SharedFlags = parent_node->SharedFlags & ImGuiDockNodeFlags_SharedFlagsInheritMask_;
    child_inheritor->LocalFlags = parent_node->LocalFlags & ImGuiDockNodeFlags_LocalFlagsTransferMask_;
    parent_node->LocalFlags &= ~ImGuiDockNodeFlags_LocalFlagsTransferMask_;
    if (child_inheritor->IsCentralNode())
        DockNodeGetRootNode(parent_node)->CentralNode = child_inheritor;
}

void ImGui::DockNodeTreeMerge(ImGuiContext* ctx, ImGuiDockNode* parent_node, ImGuiDockNode* merge_lead_child)
{
    ImGuiDockNode* child_0 = parent_node->ChildNodes[0];
    ImGuiDockNode* child_1 = parent_node->ChildNodes[1];
    IM_ASSERT(child_0 || child_1);
    IM_ASSERT(merge_lead_child == child_0 || merge_lead_child == child_1);
    if ((child_0 && child_0->Windows.Size > 0) || (child_1 && child_1->Windows.Size > 0))
    {
        IM_ASSERT(parent_node->TabBar == NULL);
        IM_ASSERT(parent_node->Windows.Size == 0);
    }
    IMGUI_DEBUG_LOG_DOCKING("DockNodeTreeMerge 0x%08X & 0x%08X back into parent 0x%08X\n", child_0 ? child_0->ID : 0, child_1 ? child_1->ID : 0, parent_node->ID);

    ImVec2 backup_last_explicit_size = parent_node->SizeRef;
    DockNodeMoveChildNodes(parent_node, merge_lead_child);
    if (child_0)
    {
        DockNodeMoveWindows(parent_node, child_0);            
        DockSettingsRenameNodeReferences(child_0->ID, parent_node->ID);
    }
    if (child_1)
    {
        DockNodeMoveWindows(parent_node, child_1);
        DockSettingsRenameNodeReferences(child_1->ID, parent_node->ID);
    }
    DockNodeApplyPosSizeToWindows(parent_node);
    parent_node->AuthorityForPos = parent_node->AuthorityForSize = parent_node->AuthorityForViewport = ImGuiDataAuthority_Auto;
    parent_node->VisibleWindow = merge_lead_child->VisibleWindow;
    parent_node->SizeRef = backup_last_explicit_size;

    parent_node->LocalFlags &= ~ImGuiDockNodeFlags_LocalFlagsTransferMask_;    
    parent_node->LocalFlags |= (child_0 ? child_0->LocalFlags : 0) & ImGuiDockNodeFlags_LocalFlagsTransferMask_;
    parent_node->LocalFlags |= (child_1 ? child_1->LocalFlags : 0) & ImGuiDockNodeFlags_LocalFlagsTransferMask_;

    if (child_0)
    {
        ctx->DockContext.Nodes.SetVoidPtr(child_0->ID, NULL);
        IM_DELETE(child_0);
    }
    if (child_1)
    {
        ctx->DockContext.Nodes.SetVoidPtr(child_1->ID, NULL);
        IM_DELETE(child_1);
    }
}

void ImGui::DockNodeTreeUpdatePosSize(ImGuiDockNode* node, ImVec2 pos, ImVec2 size, bool only_write_to_marked_nodes)
{
    const bool write_to_node = (only_write_to_marked_nodes == false) || (node->MarkedForPosSizeWrite);
    if (write_to_node)
    {
        node->Pos = pos;
        node->Size = size;
    }

    if (node->IsLeafNode())
        return;

    ImGuiDockNode* child_0 = node->ChildNodes[0];
    ImGuiDockNode* child_1 = node->ChildNodes[1];
    ImVec2 child_0_pos = pos, child_1_pos = pos;
    ImVec2 child_0_size = size, child_1_size = size;
    if (child_0->IsVisible && child_1->IsVisible)
    {
        const float spacing = DOCKING_SPLITTER_SIZE;
        const ImGuiAxis axis = (ImGuiAxis)node->SplitAxis;
        const float size_avail = ImMax(size[axis] - spacing, 0.0f);

        ImGuiContext& g = *GImGui;
        const float size_min_each = ImFloor(ImMin(size_avail, g.Style.WindowMinSize[axis] * 2.0f) * 0.5f);

        if (child_0->WantLockSizeOnce && !child_1->WantLockSizeOnce)
        {
            child_0_size[axis] = child_0->SizeRef[axis] = ImMin(size_avail - 1.0f, child_0->Size[axis]);
            child_1_size[axis] = child_1->SizeRef[axis] = (size_avail - child_0_size[axis]);
            IM_ASSERT(child_0->SizeRef[axis] > 0.0f && child_1->SizeRef[axis] > 0.0f);
        }
        else if (child_1->WantLockSizeOnce && !child_0->WantLockSizeOnce)
        {
            child_1_size[axis] = child_1->SizeRef[axis] = ImMin(size_avail - 1.0f, child_1->Size[axis]);
            child_0_size[axis] = child_0->SizeRef[axis] = (size_avail - child_1_size[axis]);
            IM_ASSERT(child_0->SizeRef[axis] > 0.0f && child_1->SizeRef[axis] > 0.0f);
        }
        else if (child_0->WantLockSizeOnce && child_1->WantLockSizeOnce)
        {
            float ratio_0 = child_0_size[axis] / (child_0_size[axis] + child_1_size[axis]);
            child_0_size[axis] = child_0->SizeRef[axis] = ImFloor(size_avail * ratio_0);
            child_1_size[axis] = child_1->SizeRef[axis] = (size_avail - child_0_size[axis]);
            IM_ASSERT(child_0->SizeRef[axis] > 0.0f && child_1->SizeRef[axis] > 0.0f);
        }

        else if (child_1->IsCentralNode() && child_0->SizeRef[axis] != 0.0f)
        {
            child_0_size[axis] = ImMin(size_avail - size_min_each, child_0->SizeRef[axis]);
            child_1_size[axis] = (size_avail - child_0_size[axis]);
        }
        else if (child_0->IsCentralNode() && child_1->SizeRef[axis] != 0.0f)
        {
            child_1_size[axis] = ImMin(size_avail - size_min_each, child_1->SizeRef[axis]);
            child_0_size[axis] = (size_avail - child_1_size[axis]);
        }
        else
        {
            float split_ratio = child_0->SizeRef[axis] / (child_0->SizeRef[axis] + child_1->SizeRef[axis]);
            child_0_size[axis] = ImMax(size_min_each, ImFloor(size_avail * split_ratio + 0.5F));
            child_1_size[axis] = (size_avail - child_0_size[axis]);
        }

        child_1_pos[axis] += spacing + child_0_size[axis];
    }
    child_0->WantLockSizeOnce = child_1->WantLockSizeOnce = false;

    if (child_0->IsVisible)
        DockNodeTreeUpdatePosSize(child_0, child_0_pos, child_0_size);
    if (child_1->IsVisible)
        DockNodeTreeUpdatePosSize(child_1, child_1_pos, child_1_size);
}

static void DockNodeTreeUpdateSplitterFindTouchingNode(ImGuiDockNode* node, ImGuiAxis axis, int side, ImVector<ImGuiDockNode*>* touching_nodes)
{
    if (node->IsLeafNode())
    {
        touching_nodes->push_back(node);
        return;
    }
    if (node->ChildNodes[0]->IsVisible)
        if (node->SplitAxis != axis || side == 0 || !node->ChildNodes[1]->IsVisible)
            DockNodeTreeUpdateSplitterFindTouchingNode(node->ChildNodes[0], axis, side, touching_nodes);
    if (node->ChildNodes[1]->IsVisible)
        if (node->SplitAxis != axis || side == 1 || !node->ChildNodes[0]->IsVisible)
            DockNodeTreeUpdateSplitterFindTouchingNode(node->ChildNodes[1], axis, side, touching_nodes);
}

void ImGui::DockNodeTreeUpdateSplitter(ImGuiDockNode* node)
{
    if (node->IsLeafNode())
        return;

    ImGuiContext& g = *GImGui;

    ImGuiDockNode* child_0 = node->ChildNodes[0];
    ImGuiDockNode* child_1 = node->ChildNodes[1];
    if (child_0->IsVisible && child_1->IsVisible)
    {
        const ImGuiAxis axis = (ImGuiAxis)node->SplitAxis;
        IM_ASSERT(axis != ImGuiAxis_None);
        ImRect bb;
        bb.Min = child_0->Pos;
        bb.Max = child_1->Pos;
        bb.Min[axis] += child_0->Size[axis];
        bb.Max[axis ^ 1] += child_1->Size[axis ^ 1];
        const ImGuiDockNodeFlags merged_flags = child_0->GetMergedFlags() | child_1->GetMergedFlags();
        const ImGuiDockNodeFlags no_resize_axis_flag = (axis == ImGuiAxis_X) ? ImGuiDockNodeFlags_NoResizeX : ImGuiDockNodeFlags_NoResizeY;
        if ((merged_flags & ImGuiDockNodeFlags_NoResize) || (merged_flags & no_resize_axis_flag))
        {
            ImGuiWindow* window = g.CurrentWindow;
            window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_Separator), g.Style.FrameRounding);
        }
        else
        {
            PushID(node->ID);

            ImVector<ImGuiDockNode*> touching_nodes[2];
            float min_size = g.Style.WindowMinSize[axis];
            float resize_limits[2];
            resize_limits[0] = node->ChildNodes[0]->Pos[axis] + min_size;
            resize_limits[1] = node->ChildNodes[1]->Pos[axis] + node->ChildNodes[1]->Size[axis] - min_size;

            ImGuiID splitter_id = GetID("##Splitter");
            if (g.ActiveId == splitter_id)
            {
                DockNodeTreeUpdateSplitterFindTouchingNode(child_0, axis, 1, &touching_nodes[0]);
                DockNodeTreeUpdateSplitterFindTouchingNode(child_1, axis, 0, &touching_nodes[1]);
                for (int touching_node_n = 0; touching_node_n < touching_nodes[0].Size; touching_node_n++)
                    resize_limits[0] = ImMax(resize_limits[0], touching_nodes[0][touching_node_n]->Rect().Min[axis] + min_size);
                for (int touching_node_n = 0; touching_node_n < touching_nodes[1].Size; touching_node_n++)
                    resize_limits[1] = ImMin(resize_limits[1], touching_nodes[1][touching_node_n]->Rect().Max[axis] - min_size);

            }

            float cur_size_0 = child_0->Size[axis];
            float cur_size_1 = child_1->Size[axis];
            float min_size_0 = resize_limits[0] - child_0->Pos[axis];
            float min_size_1 = child_1->Pos[axis] + child_1->Size[axis] - resize_limits[1];
            if (SplitterBehavior(bb, GetID("##Splitter"), axis, &cur_size_0, &cur_size_1, min_size_0, min_size_1, WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS, WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER))
            {
                if (touching_nodes[0].Size > 0 && touching_nodes[1].Size > 0)
                {
                    child_0->Size[axis] = child_0->SizeRef[axis] = cur_size_0;
                    child_1->Pos[axis] -= cur_size_1 - child_1->Size[axis];
                    child_1->Size[axis] = child_1->SizeRef[axis] = cur_size_1;

                    for (int side_n = 0; side_n < 2; side_n++)
                        for (int touching_node_n = 0; touching_node_n < touching_nodes[side_n].Size; touching_node_n++)
                        {
                            ImGuiDockNode* touching_node = touching_nodes[side_n][touching_node_n];
                            while (touching_node->ParentNode != node)
                            {
                                if (touching_node->ParentNode->SplitAxis == axis)
                                {
                                    ImGuiDockNode* node_to_preserve = touching_node->ParentNode->ChildNodes[side_n];
                                    node_to_preserve->WantLockSizeOnce = true;
                                }
                                touching_node = touching_node->ParentNode;
                            }
                        }

                    DockNodeTreeUpdatePosSize(child_0, child_0->Pos, child_0->Size);
                    DockNodeTreeUpdatePosSize(child_1, child_1->Pos, child_1->Size);
                    MarkIniSettingsDirty();
                }
            }
            PopID();
        }
    }

    if (child_0->IsVisible)
        DockNodeTreeUpdateSplitter(child_0);
    if (child_1->IsVisible)
        DockNodeTreeUpdateSplitter(child_1);
}

ImGuiDockNode* ImGui::DockNodeTreeFindFallbackLeafNode(ImGuiDockNode* node)
{
    if (node->IsLeafNode())
        return node;
    if (ImGuiDockNode* leaf_node = DockNodeTreeFindFallbackLeafNode(node->ChildNodes[0]))
        return leaf_node;
    if (ImGuiDockNode* leaf_node = DockNodeTreeFindFallbackLeafNode(node->ChildNodes[1]))
        return leaf_node;
    return NULL;
}

ImGuiDockNode* ImGui::DockNodeTreeFindVisibleNodeByPos(ImGuiDockNode* node, ImVec2 pos)
{
    if (!node->IsVisible)
        return NULL;

    const float dock_spacing = 0.0f;      
    ImRect r(node->Pos, node->Pos + node->Size);
    r.Expand(dock_spacing * 0.5f);
    bool inside = r.Contains(pos);
    if (!inside)
        return NULL;

    if (node->IsLeafNode())
        return node;
    if (ImGuiDockNode* hovered_node = DockNodeTreeFindVisibleNodeByPos(node->ChildNodes[0], pos))
        return hovered_node;
    if (ImGuiDockNode* hovered_node = DockNodeTreeFindVisibleNodeByPos(node->ChildNodes[1], pos))
        return hovered_node;

    return NULL;
}

void ImGui::SetWindowDock(ImGuiWindow* window, ImGuiID dock_id, ImGuiCond cond)
{
    if (cond && (window->SetWindowDockAllowFlags & cond) == 0)
        return;
    window->SetWindowDockAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);

    if (window->DockId == dock_id)
        return;

    ImGuiContext* ctx = GImGui;
    if (ImGuiDockNode* new_node = DockContextFindNodeByID(ctx, dock_id))
        if (new_node->IsSplitNode())
        {
            new_node = DockNodeGetRootNode(new_node);
            if (new_node->CentralNode)
            {
                IM_ASSERT(new_node->CentralNode->IsCentralNode());
                dock_id = new_node->CentralNode->ID;
            }
            else
            {
                dock_id = new_node->LastFocusedNodeId;
            }
        }

    if (window->DockId == dock_id)
        return;

    if (window->DockNode)
        DockNodeRemoveWindow(window->DockNode, window, 0);
    window->DockId = dock_id;
}

void ImGui::DockSpace(ImGuiID id, const ImVec2& size_arg, ImGuiDockNodeFlags flags, const ImGuiWindowClass* window_class)
{
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;
    ImGuiWindow* window = GetCurrentWindow();
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
        return;

    if (window->SkipItems)
        flags |= ImGuiDockNodeFlags_KeepAliveOnly;

    IM_ASSERT((flags & ImGuiDockNodeFlags_DockSpace) == 0);
    IM_ASSERT(id != 0);
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, id);
    if (!node)
    {
        IMGUI_DEBUG_LOG_DOCKING("DockSpace: dockspace node 0x%08X created\n", id);
        node = DockContextAddNode(ctx, id);
        node->LocalFlags |= ImGuiDockNodeFlags_CentralNode;
    }
    if (window_class && window_class->ClassId != node->WindowClass.ClassId)
        IMGUI_DEBUG_LOG_DOCKING("DockSpace: dockspace node 0x%08X: setup WindowClass 0x%08X -> 0x%08X\n", id, node->WindowClass.ClassId, window_class->ClassId);
    node->SharedFlags = flags;
    node->WindowClass = window_class ? *window_class : ImGuiWindowClass();

    if (node->LastFrameActive == g.FrameCount && !(flags & ImGuiDockNodeFlags_KeepAliveOnly))
    {
        IM_ASSERT(node->IsDockSpace() == false && "Cannot call DockSpace() twice a frame with the same ID");
        node->LocalFlags |= ImGuiDockNodeFlags_DockSpace;
        return;
    }
    node->LocalFlags |= ImGuiDockNodeFlags_DockSpace;

    if (flags & ImGuiDockNodeFlags_KeepAliveOnly)
    {
        node->LastFrameAlive = g.FrameCount;
        return;
    }

    const ImVec2 content_avail = GetContentRegionAvail();
    ImVec2 size = ImFloor(size_arg);
    if (size.x <= 0.0f)
        size.x = ImMax(content_avail.x + size.x, 4.0f);          
    if (size.y <= 0.0f)
        size.y = ImMax(content_avail.y + size.y, 4.0f);
    IM_ASSERT(size.x > 0.0f && size.y > 0.0f);

    node->Pos = window->DC.CursorPos;
    node->Size = node->SizeRef = size;
    SetNextWindowPos(node->Pos);
    SetNextWindowSize(node->Size);
    g.NextWindowData.PosUndock = false;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_DockNodeHost;
    window_flags |= ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    char title[256];
    ImFormatString(title, IM_ARRAYSIZE(title), "%s/DockSpace_%08X", window->Name, id);

    if (node->Windows.Size > 0 || node->IsSplitNode())
        PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
    PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    Begin(title, NULL, window_flags);
    PopStyleVar();
    if (node->Windows.Size > 0 || node->IsSplitNode())
        PopStyleColor();

    ImGuiWindow* host_window = g.CurrentWindow;
    host_window->DockNodeAsHost = node;
    host_window->ChildId = window->GetID(title);
    node->HostWindow = host_window;
    node->OnlyNodeWithWindows = NULL;

    IM_ASSERT(node->IsRootNode());

    if (node->IsLeafNode() && !node->IsCentralNode())
        node->LocalFlags |= ImGuiDockNodeFlags_CentralNode;

    DockNodeUpdate(node);

    End();
    ItemSize(size);
}

ImGuiID ImGui::DockSpaceOverViewport(ImGuiViewport* viewport, ImGuiDockNodeFlags dockspace_flags, const ImGuiWindowClass* window_class)
{
    if (viewport == NULL)
        viewport = GetMainViewport();

    SetNextWindowPos(viewport->GetWorkPos());
    SetNextWindowSize(viewport->GetWorkSize());
    SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags host_window_flags = 0;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        host_window_flags |= ImGuiWindowFlags_NoBackground;

    char label[32];
    ImFormatString(label, IM_ARRAYSIZE(label), "DockSpaceViewport_%08X", viewport->ID);

    PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    Begin(label, NULL, host_window_flags);
    PopStyleVar(3);

    ImGuiID dockspace_id = GetID("DockSpace");
    DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags, window_class);
    End();

    return dockspace_id;
}

void ImGui::DockBuilderDockWindow(const char* window_name, ImGuiID node_id)
{
    ImGuiID window_id = ImHashStr(window_name);
    if (ImGuiWindow* window = FindWindowByID(window_id))
    {
        SetWindowDock(window, node_id, ImGuiCond_Always);
        window->DockOrder = -1;
    }
    else
    {
        ImGuiWindowSettings* settings = FindWindowSettings(window_id);
        if (settings == NULL)
            settings = CreateNewWindowSettings(window_name);
        settings->DockId = node_id;
        settings->DockOrder = -1;
    }
}

ImGuiDockNode* ImGui::DockBuilderGetNode(ImGuiID node_id)
{
    ImGuiContext* ctx = GImGui;
    return DockContextFindNodeByID(ctx, node_id);
}

void ImGui::DockBuilderSetNodePos(ImGuiID node_id, ImVec2 pos)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, node_id);
    if (node == NULL)
        return;
    node->Pos = pos;
    node->AuthorityForPos = ImGuiDataAuthority_DockNode;
}

void ImGui::DockBuilderSetNodeSize(ImGuiID node_id, ImVec2 size)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, node_id);
    if (node == NULL)
        return;
    IM_ASSERT(size.x > 0.0f && size.y > 0.0f);
    node->Size = node->SizeRef = size;
    node->AuthorityForSize = ImGuiDataAuthority_DockNode;
}

ImGuiID ImGui::DockBuilderAddNode(ImGuiID id, ImGuiDockNodeFlags flags)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* node = NULL;

    if (id != 0)
        DockBuilderRemoveNode(id);

    if (flags & ImGuiDockNodeFlags_DockSpace)
    {
        DockSpace(id, ImVec2(0, 0), (flags & ~ImGuiDockNodeFlags_DockSpace) | ImGuiDockNodeFlags_KeepAliveOnly);
        node = DockContextFindNodeByID(ctx, id);
    }
    else
    {
        node = DockContextAddNode(ctx, id);
        node->LocalFlags = flags;
    }
    node->LastFrameAlive = ctx->FrameCount;             
    return node->ID;
}

void ImGui::DockBuilderRemoveNode(ImGuiID node_id)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, node_id);
    if (node == NULL)
        return;
    DockBuilderRemoveNodeDockedWindows(node_id, true);
    DockBuilderRemoveNodeChildNodes(node_id);
    if (node->IsCentralNode() && node->ParentNode)
        node->ParentNode->LocalFlags |= ImGuiDockNodeFlags_CentralNode;
    DockContextRemoveNode(ctx, node, true);
}

void ImGui::DockBuilderRemoveNodeChildNodes(ImGuiID root_id)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockContext* dc  = &ctx->DockContext;

    ImGuiDockNode* root_node = root_id ? DockContextFindNodeByID(ctx, root_id) : NULL;
    if (root_id && root_node == NULL)
        return;
    bool has_central_node = false;

    ImGuiDataAuthority backup_root_node_authority_for_pos = root_node ? root_node->AuthorityForPos : ImGuiDataAuthority_Auto;
    ImGuiDataAuthority backup_root_node_authority_for_size = root_node ? root_node->AuthorityForSize : ImGuiDataAuthority_Auto;

    ImVector<ImGuiDockNode*> nodes_to_remove;
    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
        {
            bool want_removal = (root_id == 0) || (node->ID != root_id && DockNodeGetRootNode(node)->ID == root_id);
            if (want_removal)
            {
                if (node->IsCentralNode())
                    has_central_node = true;
                if (root_id != 0)
                    DockContextQueueNotifyRemovedNode(ctx, node);
                if (root_node)
                    DockNodeMoveWindows(root_node, node);
                nodes_to_remove.push_back(node);
            }
        }

    if (root_node)
    {
        root_node->AuthorityForPos = backup_root_node_authority_for_pos;
        root_node->AuthorityForSize = backup_root_node_authority_for_size;
    }

    for (ImGuiWindowSettings* settings = ctx->SettingsWindows.begin(); settings != NULL; settings = ctx->SettingsWindows.next_chunk(settings))
        if (ImGuiID window_settings_dock_id = settings->DockId)
            for (int n = 0; n < nodes_to_remove.Size; n++)
                if (nodes_to_remove[n]->ID == window_settings_dock_id)
                {
                    settings->DockId = root_id;
                    break;
                }

    if (nodes_to_remove.Size > 1)
        ImQsort(nodes_to_remove.Data, nodes_to_remove.Size, sizeof(ImGuiDockNode*), DockNodeComparerDepthMostFirst);
    for (int n = 0; n < nodes_to_remove.Size; n++)
        DockContextRemoveNode(ctx, nodes_to_remove[n], false);

    if (root_id == 0)
    {
        dc->Nodes.Clear();
        dc->Requests.clear();
    }
    else if (has_central_node)
    {
        root_node->LocalFlags |= ImGuiDockNodeFlags_CentralNode;
        root_node->CentralNode = root_node;
    }
}

void ImGui::DockBuilderRemoveNodeDockedWindows(ImGuiID root_id, bool clear_settings_refs)
{
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;
    if (clear_settings_refs)
    {
        for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
        {
            bool want_removal = (root_id == 0) || (settings->DockId == root_id);
            if (!want_removal && settings->DockId != 0)
                if (ImGuiDockNode* node = DockContextFindNodeByID(ctx, settings->DockId))
                    if (DockNodeGetRootNode(node)->ID == root_id)
                        want_removal = true;
            if (want_removal)
                settings->DockId = 0;
        }
    }

    for (int n = 0; n < g.Windows.Size; n++)
    {
        ImGuiWindow* window = g.Windows[n];
        bool want_removal = (root_id == 0) || (window->DockNode && DockNodeGetRootNode(window->DockNode)->ID == root_id) || (window->DockNodeAsHost && window->DockNodeAsHost->ID == root_id);
        if (want_removal)
        {
            const ImGuiID backup_dock_id = window->DockId;
            IM_UNUSED(backup_dock_id);
            DockContextProcessUndockWindow(ctx, window, clear_settings_refs);
            if (!clear_settings_refs)
                IM_ASSERT(window->DockId == backup_dock_id);
        }
    }
}

ImGuiID ImGui::DockBuilderSplitNode(ImGuiID id, ImGuiDir split_dir, float size_ratio_for_node_at_dir, ImGuiID* out_id_at_dir, ImGuiID* out_id_at_opposite_dir)
{
    ImGuiContext* ctx = GImGui;
    IM_ASSERT(split_dir != ImGuiDir_None);
    IMGUI_DEBUG_LOG_DOCKING("DockBuilderSplitNode node 0x%08X, split_dir %d\n", id, split_dir);

    ImGuiDockNode* node = DockContextFindNodeByID(ctx, id);
    if (node == NULL)
    {
        IM_ASSERT(node != NULL);
        return 0;
    }

    IM_ASSERT(!node->IsSplitNode());     

    ImGuiDockRequest req;
    req.Type = ImGuiDockRequestType_Split;
    req.DockTargetWindow = NULL;
    req.DockTargetNode = node;
    req.DockPayload = NULL;
    req.DockSplitDir = split_dir;
    req.DockSplitRatio = ImSaturate((split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? size_ratio_for_node_at_dir : 1.0f - size_ratio_for_node_at_dir);
    req.DockSplitOuter = false;
    DockContextProcessDock(ctx, &req);

    ImGuiID id_at_dir = node->ChildNodes[(split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? 0 : 1]->ID;
    ImGuiID id_at_opposite_dir = node->ChildNodes[(split_dir == ImGuiDir_Left || split_dir == ImGuiDir_Up) ? 1 : 0]->ID;
    if (out_id_at_dir)
        *out_id_at_dir = id_at_dir;
    if (out_id_at_opposite_dir)
        *out_id_at_opposite_dir = id_at_opposite_dir;
    return id_at_dir;
}

static ImGuiDockNode* DockBuilderCopyNodeRec(ImGuiDockNode* src_node, ImGuiID dst_node_id_if_known, ImVector<ImGuiID>* out_node_remap_pairs)
{
    ImGuiContext* ctx = GImGui;
    ImGuiDockNode* dst_node = ImGui::DockContextAddNode(ctx, dst_node_id_if_known);
    dst_node->SharedFlags = src_node->SharedFlags;
    dst_node->LocalFlags = src_node->LocalFlags;
    dst_node->Pos = src_node->Pos;
    dst_node->Size = src_node->Size;
    dst_node->SizeRef = src_node->SizeRef;
    dst_node->SplitAxis = src_node->SplitAxis;

    out_node_remap_pairs->push_back(src_node->ID);
    out_node_remap_pairs->push_back(dst_node->ID);

    for (int child_n = 0; child_n < IM_ARRAYSIZE(src_node->ChildNodes); child_n++)
        if (src_node->ChildNodes[child_n])
        {
            dst_node->ChildNodes[child_n] = DockBuilderCopyNodeRec(src_node->ChildNodes[child_n], 0, out_node_remap_pairs);
            dst_node->ChildNodes[child_n]->ParentNode = dst_node;
        }

    IMGUI_DEBUG_LOG_DOCKING("Fork node %08X -> %08X (%d childs)\n", src_node->ID, dst_node->ID, dst_node->IsSplitNode() ? 2 : 0);
    return dst_node;
}

void ImGui::DockBuilderCopyNode(ImGuiID src_node_id, ImGuiID dst_node_id, ImVector<ImGuiID>* out_node_remap_pairs)
{
    ImGuiContext* ctx = GImGui;
    IM_ASSERT(src_node_id != 0);
    IM_ASSERT(dst_node_id != 0);
    IM_ASSERT(out_node_remap_pairs != NULL);

    ImGuiDockNode* src_node = DockContextFindNodeByID(ctx, src_node_id);
    IM_ASSERT(src_node != NULL);

    out_node_remap_pairs->clear();
    DockBuilderRemoveNode(dst_node_id);
    DockBuilderCopyNodeRec(src_node, dst_node_id, out_node_remap_pairs);

    IM_ASSERT((out_node_remap_pairs->Size % 2) == 0);
}

void ImGui::DockBuilderCopyWindowSettings(const char* src_name, const char* dst_name)
{
    ImGuiWindow* src_window = FindWindowByName(src_name);
    if (src_window == NULL)
        return;
    if (ImGuiWindow* dst_window = FindWindowByName(dst_name))
    {
        dst_window->Pos = src_window->Pos;
        dst_window->Size = src_window->Size;
        dst_window->SizeFull = src_window->SizeFull;
        dst_window->Collapsed = src_window->Collapsed;
    }
    else if (ImGuiWindowSettings* dst_settings = FindOrCreateWindowSettings(dst_name))
    {
        ImVec2ih window_pos_2ih = ImVec2ih(src_window->Pos);
        if (src_window->ViewportId != 0 && src_window->ViewportId != IMGUI_VIEWPORT_DEFAULT_ID)
        {
            dst_settings->ViewportPos = window_pos_2ih;
            dst_settings->ViewportId = src_window->ViewportId;
            dst_settings->Pos = ImVec2ih(0, 0);
        }
        else
        {
            dst_settings->Pos = window_pos_2ih;
        }
        dst_settings->Size = ImVec2ih(src_window->SizeFull);
        dst_settings->Collapsed = src_window->Collapsed;
    }
}

void ImGui::DockBuilderCopyDockSpace(ImGuiID src_dockspace_id, ImGuiID dst_dockspace_id, ImVector<const char*>* in_window_remap_pairs)
{
    IM_ASSERT(src_dockspace_id != 0);
    IM_ASSERT(dst_dockspace_id != 0);
    IM_ASSERT(in_window_remap_pairs != NULL);
    IM_ASSERT((in_window_remap_pairs->Size % 2) == 0);

    ImVector<ImGuiID> node_remap_pairs;
    DockBuilderCopyNode(src_dockspace_id, dst_dockspace_id, &node_remap_pairs);

    ImVector<ImGuiID> src_windows;
    for (int remap_window_n = 0; remap_window_n < in_window_remap_pairs->Size; remap_window_n += 2)
    {
        const char* src_window_name = (*in_window_remap_pairs)[remap_window_n];
        const char* dst_window_name = (*in_window_remap_pairs)[remap_window_n + 1];
        ImGuiID src_window_id = ImHashStr(src_window_name);
        src_windows.push_back(src_window_id);

        ImGuiID src_dock_id = 0;
        if (ImGuiWindow* src_window = FindWindowByID(src_window_id))
            src_dock_id = src_window->DockId;
        else if (ImGuiWindowSettings* src_window_settings = FindWindowSettings(src_window_id))
            src_dock_id = src_window_settings->DockId;
        ImGuiID dst_dock_id = 0;
        for (int dock_remap_n = 0; dock_remap_n < node_remap_pairs.Size; dock_remap_n += 2)
            if (node_remap_pairs[dock_remap_n] == src_dock_id)
            {
                dst_dock_id = node_remap_pairs[dock_remap_n + 1];
                break;
            }

        if (dst_dock_id != 0)
        {
            IMGUI_DEBUG_LOG_DOCKING("Remap live window '%s' 0x%08X -> '%s' 0x%08X\n", src_window_name, src_dock_id, dst_window_name, dst_dock_id);
            DockBuilderDockWindow(dst_window_name, dst_dock_id);
        }
        else
        {
            IMGUI_DEBUG_LOG_DOCKING("Remap window settings '%s' -> '%s'\n", src_window_name, dst_window_name);
            DockBuilderCopyWindowSettings(src_window_name, dst_window_name);
        }
    }

    for (int dock_remap_n = 0; dock_remap_n < node_remap_pairs.Size; dock_remap_n += 2)
        if (ImGuiID src_dock_id = node_remap_pairs[dock_remap_n])
        {
            ImGuiID dst_dock_id = node_remap_pairs[dock_remap_n + 1];
            ImGuiDockNode* node = DockBuilderGetNode(src_dock_id);
            for (int window_n = 0; window_n < node->Windows.Size; window_n++)
            {
                ImGuiWindow* window = node->Windows[window_n];
                if (src_windows.contains(window->ID))
                    continue;

                IMGUI_DEBUG_LOG_DOCKING("Remap window '%s' %08X -> %08X\n", window->Name, src_dock_id, dst_dock_id);
                DockBuilderDockWindow(window->Name, dst_dock_id);
            }
        }
}

void ImGui::DockBuilderFinish(ImGuiID root_id)
{
    ImGuiContext* ctx = GImGui;
    DockContextBuildAddWindowsToNodes(ctx, root_id);
}

bool ImGui::GetWindowAlwaysWantOwnTabBar(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    if (g.IO.ConfigDockingAlwaysTabBar || window->WindowClass.DockingAlwaysTabBar)
        if ((window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking)) == 0)
            if (!window->IsFallbackWindow)                 
                return true;
    return false;
}

static ImGuiDockNode* ImGui::DockContextBindNodeToWindow(ImGuiContext* ctx, ImGuiWindow* window)
{
    ImGuiContext& g = *ctx;
    ImGuiDockNode* node = DockContextFindNodeByID(ctx, window->DockId);
    IM_ASSERT(window->DockNode == NULL);

    if (node && node->IsSplitNode())
    {
        DockContextProcessUndockWindow(ctx, window);
        return NULL;
    }

    if (node == NULL)
    {
        node = DockContextAddNode(ctx, window->DockId);
        node->AuthorityForPos = node->AuthorityForSize = node->AuthorityForViewport = ImGuiDataAuthority_Window;
        node->LastFrameAlive = g.FrameCount;
    }

    if (!node->IsVisible)
    {
        ImGuiDockNode* ancestor_node = node;
        while (!ancestor_node->IsVisible)
        {
            ancestor_node->IsVisible = true;
            ancestor_node->MarkedForPosSizeWrite = true;
            if (ancestor_node->ParentNode)
                ancestor_node = ancestor_node->ParentNode;
        }
        IM_ASSERT(ancestor_node->Size.x > 0.0f && ancestor_node->Size.y > 0.0f);
        DockNodeTreeUpdatePosSize(ancestor_node, ancestor_node->Pos, ancestor_node->Size, true);
    }

    DockNodeAddWindow(node, window, true);
    IM_ASSERT(node == window->DockNode);
    return node;
}

void ImGui::BeginDocked(ImGuiWindow* window, bool* p_open)
{
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;

    const bool auto_dock_node = GetWindowAlwaysWantOwnTabBar(window);
    if (auto_dock_node)
    {
        if (window->DockId == 0)
        {
            IM_ASSERT(window->DockNode == NULL);
            window->DockId = DockContextGenNodeID(ctx);
        }
    }
    else
    {
        bool want_undock = false;
        want_undock |= (window->Flags & ImGuiWindowFlags_NoDocking) != 0;
        want_undock |= (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasPos) && (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) && g.NextWindowData.PosUndock;
        if (want_undock)
        {
            DockContextProcessUndockWindow(ctx, window);
            return;
        }
    }

    ImGuiDockNode* node = window->DockNode;
    if (node != NULL)
        IM_ASSERT(window->DockId == node->ID);
    if (window->DockId != 0 && node == NULL)
    {
        node = DockContextBindNodeToWindow(ctx, window);
        if (node == NULL)
            return;
    }

#if 0
    if (node->IsCentralNode && (node->Flags & ImGuiDockNodeFlags_NoDockingInCentralNode))
    {
        DockContextProcessUndockWindow(ctx, window);
        return;
    }
#endif

    if (node->LastFrameAlive < g.FrameCount)
    {
        ImGuiDockNode* root_node = DockNodeGetRootNode(node);
        if (root_node->LastFrameAlive < g.FrameCount)
        {
            DockContextProcessUndockWindow(ctx, window);
        }
        else
        {
            window->DockIsActive = true;
            window->DockTabIsVisible = false;
        }
        return;
    }

    if (node->HostWindow == NULL)
    {
        window->DockIsActive = (node->State == ImGuiDockNodeState_HostWindowHiddenBecauseWindowsAreResizing);
        window->DockTabIsVisible = false;
        return;
    }

    IM_ASSERT(node->HostWindow);
    IM_ASSERT(node->IsLeafNode());
    IM_ASSERT(node->Size.x >= 0.0f && node->Size.y >= 0.0f);
    node->State = ImGuiDockNodeState_HostWindowVisible;

    if (window->BeginOrderWithinContext < node->HostWindow->BeginOrderWithinContext)
    {
        DockContextProcessUndockWindow(ctx, window);
        return;
    }

    SetNextWindowPos(node->Pos);
    SetNextWindowSize(node->Size);
    g.NextWindowData.PosUndock = false;      
    window->DockIsActive = true;
    window->DockTabIsVisible = false;
    if (node->SharedFlags & ImGuiDockNodeFlags_KeepAliveOnly)
        return;

    if (node->VisibleWindow == window)
        window->DockTabIsVisible = true;

    IM_ASSERT((window->Flags & ImGuiWindowFlags_ChildWindow) == 0);
    window->Flags |= ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoResize;
    if (node->IsHiddenTabBar() || node->IsNoTabBar())
        window->Flags |= ImGuiWindowFlags_NoTitleBar;
    else
        window->Flags &= ~ImGuiWindowFlags_NoTitleBar;                                  

    if (node->TabBar && node->TabBar->CurrFrameVisible != -1)
        window->DockOrder = (short)DockNodeGetTabOrder(window);

    if ((node->WantCloseAll || node->WantCloseTabId == window->ID) && p_open != NULL)
        *p_open = false;

    ImGuiWindow* parent_window = window->DockNode->HostWindow;
    window->ChildId = parent_window->GetID(window->Name);
}

void ImGui::BeginDockableDragDropSource(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ActiveId == window->MoveId);
    IM_ASSERT(g.MovingWindow == window);

    window->DC.LastItemId = window->MoveId;
    window = window->RootWindow;
    IM_ASSERT((window->Flags & ImGuiWindowFlags_NoDocking) == 0);
    bool is_drag_docking = (g.IO.ConfigDockingWithShift) || ImRect(0, 0, window->SizeFull.x, GetFrameHeight()).Contains(g.ActiveIdClickOffset);
    if (is_drag_docking && BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip | ImGuiDragDropFlags_SourceNoHoldToOpenOthers | ImGuiDragDropFlags_SourceAutoExpirePayload))
    {
        SetDragDropPayload(IMGUI_PAYLOAD_TYPE_WINDOW, &window, sizeof(window));
        EndDragDropSource();
    }
}

void ImGui::BeginDockableDragDropTarget(ImGuiWindow* window)
{
    ImGuiContext* ctx = GImGui;
    ImGuiContext& g = *ctx;

    IM_ASSERT((window->Flags & ImGuiWindowFlags_NoDocking) == 0);
    if (!g.DragDropActive)
        return;
    if (!BeginDragDropTargetCustom(window->Rect(), window->ID))
        return;

    const ImGuiPayload* payload = &g.DragDropPayload;
    if (!payload->IsDataType(IMGUI_PAYLOAD_TYPE_WINDOW) || !DockNodeIsDropAllowed(window, *(ImGuiWindow**)payload->Data))
    {
        EndDragDropTarget();
        return;
    }

    ImGuiWindow* payload_window = *(ImGuiWindow**)payload->Data;
    if (AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_WINDOW, ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
    {
        bool dock_into_floating_window = false;
        ImGuiDockNode* node = NULL;
        if (window->DockNodeAsHost)
        {
            node = DockNodeTreeFindVisibleNodeByPos(window->DockNodeAsHost, g.IO.MousePos);

            if (node && node->IsDockSpace() && node->IsRootNode())
                node = (node->CentralNode && node->IsLeafNode()) ? node->CentralNode : DockNodeTreeFindFallbackLeafNode(node);
        }
        else
        {
            if (window->DockNode)
                node = window->DockNode;
            else
                dock_into_floating_window = true;      
        }

        const ImRect explicit_target_rect = (node && node->TabBar && !node->IsHiddenTabBar() && !node->IsNoTabBar()) ? node->TabBar->BarRect : ImRect(window->Pos, window->Pos + ImVec2(window->Size.x, GetFrameHeight()));
        const bool is_explicit_target = g.IO.ConfigDockingWithShift || IsMouseHoveringRect(explicit_target_rect.Min, explicit_target_rect.Max);

        const bool do_preview = payload->IsPreview() || payload->IsDelivery();
        if (do_preview && (node != NULL || dock_into_floating_window))
        {
            ImGuiDockPreviewData split_inner;
            ImGuiDockPreviewData split_outer;
            ImGuiDockPreviewData* split_data = &split_inner;
            if (node && (node->ParentNode || node->IsCentralNode()))
                if (ImGuiDockNode* root_node = DockNodeGetRootNode(node))
                {
                    DockNodePreviewDockSetup(window, root_node, payload_window, &split_outer, is_explicit_target, true);
                    if (split_outer.IsSplitDirExplicit)
                        split_data = &split_outer;
                }
            DockNodePreviewDockSetup(window, node, payload_window, &split_inner, is_explicit_target, false);
            if (split_data == &split_outer)
                split_inner.IsDropAllowed = false;

            DockNodePreviewDockRender(window, node, payload_window, &split_inner);
            DockNodePreviewDockRender(window, node, payload_window, &split_outer);

            if (split_data->IsDropAllowed && payload->IsDelivery())
                DockContextQueueDock(ctx, window, split_data->SplitNode, payload_window, split_data->SplitDir, split_data->SplitRatio, split_data == &split_outer);
        }
    }
    EndDragDropTarget();
}

static void ImGui::DockSettingsRenameNodeReferences(ImGuiID old_node_id, ImGuiID new_node_id)
{
    ImGuiContext& g = *GImGui;
    IMGUI_DEBUG_LOG_DOCKING("DockSettingsRenameNodeReferences: from 0x%08X -> to 0x%08X\n", old_node_id, new_node_id);
    for (int window_n = 0; window_n < g.Windows.Size; window_n++)
    {
        ImGuiWindow* window = g.Windows[window_n];
        if (window->DockId == old_node_id && window->DockNode == NULL)
            window->DockId = new_node_id;
    }
    for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
        if (settings->DockId == old_node_id)
            settings->DockId = new_node_id;
}

static void ImGui::DockSettingsRemoveNodeReferences(ImGuiID* node_ids, int node_ids_count)
{
    ImGuiContext& g = *GImGui;
    int found = 0;
    for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
        for (int node_n = 0; node_n < node_ids_count; node_n++)
            if (settings->DockId == node_ids[node_n])
            {
                settings->DockId = 0;
                settings->DockOrder = -1;
                if (++found < node_ids_count)
                    break;
                return;
            }
}

static ImGuiDockNodeSettings* ImGui::DockSettingsFindNodeSettings(ImGuiContext* ctx, ImGuiID id)
{
    ImGuiDockContext* dc  = &ctx->DockContext;
    for (int n = 0; n < dc->NodesSettings.Size; n++)
        if (dc->NodesSettings[n].ID == id)
            return &dc->NodesSettings[n];
    return NULL;
}

static void ImGui::DockSettingsHandler_ClearAll(ImGuiContext* ctx, ImGuiSettingsHandler*)
{
    ImGuiDockContext* dc  = &ctx->DockContext;
    dc->NodesSettings.clear();
    DockContextClearNodes(ctx, 0, true);
}

static void ImGui::DockSettingsHandler_ApplyAll(ImGuiContext* ctx, ImGuiSettingsHandler*)
{
    ImGuiDockContext* dc  = &ctx->DockContext;
    if (ctx->Windows.Size == 0)
        DockContextPruneUnusedSettingsNodes(ctx);
    DockContextBuildNodesFromSettings(ctx, dc->NodesSettings.Data, dc->NodesSettings.Size);
    DockContextBuildAddWindowsToNodes(ctx, 0);
}

static void* ImGui::DockSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    if (strcmp(name, "Data") != 0)
        return NULL;
    return (void*)1;
}

static void ImGui::DockSettingsHandler_ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler*, void*, const char* line)
{
    char c = 0;
    int x = 0, y = 0;
    int r = 0;

    ImGuiDockNodeSettings node;
    line = ImStrSkipBlank(line);
    if      (strncmp(line, "DockNode", 8) == 0)  { line = ImStrSkipBlank(line + strlen("DockNode")); }
    else if (strncmp(line, "DockSpace", 9) == 0) { line = ImStrSkipBlank(line + strlen("DockSpace")); node.Flags |= ImGuiDockNodeFlags_DockSpace; }
    else return;
    if (sscanf(line, "ID=0x%08X%n",      &node.ID, &r) == 1)            { line += r; } else return;
    if (sscanf(line, " Parent=0x%08X%n", &node.ParentNodeId, &r) == 1)  { line += r; if (node.ParentNodeId == 0) return; }
    if (sscanf(line, " Window=0x%08X%n", &node.ParentWindowId, &r) ==1) { line += r; if (node.ParentWindowId == 0) return; }
    if (node.ParentNodeId == 0)
    {
        if (sscanf(line, " Pos=%i,%i%n",  &x, &y, &r) == 2)         { line += r; node.Pos = ImVec2ih((short)x, (short)y); } else return;
        if (sscanf(line, " Size=%i,%i%n", &x, &y, &r) == 2)         { line += r; node.Size = ImVec2ih((short)x, (short)y); } else return;
    }
    else
    {
        if (sscanf(line, " SizeRef=%i,%i%n", &x, &y, &r) == 2)      { line += r; node.SizeRef = ImVec2ih((short)x, (short)y); }
    }
    if (sscanf(line, " Split=%c%n", &c, &r) == 1)                   { line += r; if (c == 'X') node.SplitAxis = ImGuiAxis_X; else if (c == 'Y') node.SplitAxis = ImGuiAxis_Y; }
    if (sscanf(line, " NoResize=%d%n", &x, &r) == 1)                { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_NoResize; }
    if (sscanf(line, " CentralNode=%d%n", &x, &r) == 1)             { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_CentralNode; }
    if (sscanf(line, " NoTabBar=%d%n", &x, &r) == 1)                { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_NoTabBar; }
    if (sscanf(line, " HiddenTabBar=%d%n", &x, &r) == 1)            { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_HiddenTabBar; }
    if (sscanf(line, " NoWindowMenuButton=%d%n", &x, &r) == 1)      { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_NoWindowMenuButton; }
    if (sscanf(line, " NoCloseButton=%d%n", &x, &r) == 1)           { line += r; if (x != 0) node.Flags |= ImGuiDockNodeFlags_NoCloseButton; }
    if (sscanf(line, " Selected=0x%08X%n", &node.SelectedWindowId,&r) == 1) { line += r; }
    if (node.ParentNodeId != 0)
        if (ImGuiDockNodeSettings* parent_settings = DockSettingsFindNodeSettings(ctx, node.ParentNodeId))
            node.Depth = parent_settings->Depth + 1;
    ctx->DockContext.NodesSettings.push_back(node);
}

static void DockSettingsHandler_DockNodeToSettings(ImGuiDockContext* dc, ImGuiDockNode* node, int depth)
{
    ImGuiDockNodeSettings node_settings;
    IM_ASSERT(depth < (1 << (sizeof(node_settings.Depth) << 3)));
    node_settings.ID = node->ID;
    node_settings.ParentNodeId = node->ParentNode ? node->ParentNode->ID : 0;
    node_settings.ParentWindowId = (node->IsDockSpace() && node->HostWindow && node->HostWindow->ParentWindow) ? node->HostWindow->ParentWindow->ID : 0;
    node_settings.SelectedWindowId = node->SelectedTabId;
    node_settings.SplitAxis = (signed char)(node->IsSplitNode() ? node->SplitAxis : ImGuiAxis_None);
    node_settings.Depth = (char)depth;
    node_settings.Flags = (node->LocalFlags & ImGuiDockNodeFlags_SavedFlagsMask_);
    node_settings.Pos = ImVec2ih(node->Pos);
    node_settings.Size = ImVec2ih(node->Size);
    node_settings.SizeRef = ImVec2ih(node->SizeRef);
    dc->NodesSettings.push_back(node_settings);
    if (node->ChildNodes[0])
        DockSettingsHandler_DockNodeToSettings(dc, node->ChildNodes[0], depth + 1);
    if (node->ChildNodes[1])
        DockSettingsHandler_DockNodeToSettings(dc, node->ChildNodes[1], depth + 1);
}

static void ImGui::DockSettingsHandler_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    ImGuiContext& g = *ctx;
    ImGuiDockContext* dc = &ctx->DockContext;
    if (!(g.IO.ConfigFlags & ImGuiConfigFlags_DockingEnable))
        return;

    dc->NodesSettings.resize(0);
    dc->NodesSettings.reserve(dc->Nodes.Data.Size);
    for (int n = 0; n < dc->Nodes.Data.Size; n++)
        if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
            if (node->IsRootNode())
                DockSettingsHandler_DockNodeToSettings(dc, node, 0);

    int max_depth = 0;
    for (int node_n = 0; node_n < dc->NodesSettings.Size; node_n++)
        max_depth = ImMax((int)dc->NodesSettings[node_n].Depth, max_depth);

    buf->appendf("[%s][Data]\n", handler->TypeName);
    for (int node_n = 0; node_n < dc->NodesSettings.Size; node_n++)
    {
        const int line_start_pos = buf->size(); (void)line_start_pos;
        const ImGuiDockNodeSettings* node_settings = &dc->NodesSettings[node_n];
        buf->appendf("%*s%s%*s", node_settings->Depth * 2, "", (node_settings->Flags & ImGuiDockNodeFlags_DockSpace) ? "DockSpace" : "DockNode ", (max_depth - node_settings->Depth) * 2, "");           
        buf->appendf(" ID=0x%08X", node_settings->ID);
        if (node_settings->ParentNodeId)
        {
            buf->appendf(" Parent=0x%08X SizeRef=%d,%d", node_settings->ParentNodeId, node_settings->SizeRef.x, node_settings->SizeRef.y);
        }
        else
        {
            if (node_settings->ParentWindowId)
                buf->appendf(" Window=0x%08X", node_settings->ParentWindowId);
            buf->appendf(" Pos=%d,%d Size=%d,%d", node_settings->Pos.x, node_settings->Pos.y, node_settings->Size.x, node_settings->Size.y);
        }
        if (node_settings->SplitAxis != ImGuiAxis_None)
            buf->appendf(" Split=%c", (node_settings->SplitAxis == ImGuiAxis_X) ? 'X' : 'Y');
        if (node_settings->Flags & ImGuiDockNodeFlags_NoResize)
            buf->appendf(" NoResize=1");
        if (node_settings->Flags & ImGuiDockNodeFlags_CentralNode)
            buf->appendf(" CentralNode=1");
        if (node_settings->Flags & ImGuiDockNodeFlags_NoTabBar)
            buf->appendf(" NoTabBar=1");
        if (node_settings->Flags & ImGuiDockNodeFlags_HiddenTabBar)
            buf->appendf(" HiddenTabBar=1");
        if (node_settings->Flags & ImGuiDockNodeFlags_NoWindowMenuButton)
            buf->appendf(" NoWindowMenuButton=1");
        if (node_settings->Flags & ImGuiDockNodeFlags_NoCloseButton)
            buf->appendf(" NoCloseButton=1");
        if (node_settings->SelectedWindowId)
            buf->appendf(" Selected=0x%08X", node_settings->SelectedWindowId);

#if IMGUI_DEBUG_INI_SETTINGS
        if (ImGuiDockNode* node = DockContextFindNodeByID(ctx, node_settings->ID))
        {
            buf->appendf("%*s", ImMax(2, (line_start_pos + 92) - buf->size()), "");       
            if (node->IsDockSpace() && node->HostWindow && node->HostWindow->ParentWindow)
                buf->appendf(" ; in '%s'", node->HostWindow->ParentWindow->Name);
            int contains_window = 0;
            for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
                if (settings->DockId == node_settings->ID)
                {
                    if (contains_window++ == 0)
                        buf->appendf(" ; contains ");
                    buf->appendf("'%s' ", settings->GetName());
                }
        }
#endif
        buf->appendf("\n");
    }
    buf->appendf("\n");
}


#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS)

#ifdef _MSC_VER
#pragma comment(lib, "user32")
#pragma comment(lib, "kernel32")
#endif

static const char* GetClipboardTextFn_DefaultImpl(void*)
{
    ImGuiContext& g = *GImGui;
    g.ClipboardHandlerData.clear();
    if (!::OpenClipboard(NULL))
        return NULL;
    HANDLE wbuf_handle = ::GetClipboardData(CF_UNICODETEXT);
    if (wbuf_handle == NULL)
    {
        ::CloseClipboard();
        return NULL;
    }
    if (const WCHAR* wbuf_global = (const WCHAR*)::GlobalLock(wbuf_handle))
    {
        int buf_len = ::WideCharToMultiByte(CP_UTF8, 0, wbuf_global, -1, NULL, 0, NULL, NULL);
        g.ClipboardHandlerData.resize(buf_len);
        ::WideCharToMultiByte(CP_UTF8, 0, wbuf_global, -1, g.ClipboardHandlerData.Data, buf_len, NULL, NULL);
    }
    ::GlobalUnlock(wbuf_handle);
    ::CloseClipboard();
    return g.ClipboardHandlerData.Data;
}

static void SetClipboardTextFn_DefaultImpl(void*, const char* text)
{
    if (!::OpenClipboard(NULL))
        return;
    const int wbuf_length = ::MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    HGLOBAL wbuf_handle = ::GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wbuf_length * sizeof(WCHAR));
    if (wbuf_handle == NULL)
    {
        ::CloseClipboard();
        return;
    }
    WCHAR* wbuf_global = (WCHAR*)::GlobalLock(wbuf_handle);
    ::MultiByteToWideChar(CP_UTF8, 0, text, -1, wbuf_global, wbuf_length);
    ::GlobalUnlock(wbuf_handle);
    ::EmptyClipboard();
    if (::SetClipboardData(CF_UNICODETEXT, wbuf_handle) == NULL)
        ::GlobalFree(wbuf_handle);
    ::CloseClipboard();
}

#elif defined(__APPLE__) && TARGET_OS_OSX && defined(IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS)

#include <Carbon/Carbon.h>            
static PasteboardRef main_clipboard = 0;

static void SetClipboardTextFn_DefaultImpl(void*, const char* text)
{
    if (!main_clipboard)
        PasteboardCreate(kPasteboardClipboard, &main_clipboard);
    PasteboardClear(main_clipboard);
    CFDataRef cf_data = CFDataCreate(kCFAllocatorDefault, (const UInt8*)text, strlen(text));
    if (cf_data)
    {
        PasteboardPutItemFlavor(main_clipboard, (PasteboardItemID)1, CFSTR("public.utf8-plain-text"), cf_data, 0);
        CFRelease(cf_data);
    }
}

static const char* GetClipboardTextFn_DefaultImpl(void*)
{
    if (!main_clipboard)
        PasteboardCreate(kPasteboardClipboard, &main_clipboard);
    PasteboardSynchronize(main_clipboard);

    ItemCount item_count = 0;
    PasteboardGetItemCount(main_clipboard, &item_count);
    for (ItemCount i = 0; i < item_count; i++)
    {
        PasteboardItemID item_id = 0;
        PasteboardGetItemIdentifier(main_clipboard, i + 1, &item_id);
        CFArrayRef flavor_type_array = 0;
        PasteboardCopyItemFlavors(main_clipboard, item_id, &flavor_type_array);
        for (CFIndex j = 0, nj = CFArrayGetCount(flavor_type_array); j < nj; j++)
        {
            CFDataRef cf_data;
            if (PasteboardCopyItemFlavorData(main_clipboard, item_id, CFSTR("public.utf8-plain-text"), &cf_data) == noErr)
            {
                ImGuiContext& g = *GImGui;
                g.ClipboardHandlerData.clear();
                int length = (int)CFDataGetLength(cf_data);
                g.ClipboardHandlerData.resize(length + 1);
                CFDataGetBytes(cf_data, CFRangeMake(0, length), (UInt8*)g.ClipboardHandlerData.Data);
                g.ClipboardHandlerData[length] = 0;
                CFRelease(cf_data);
                return g.ClipboardHandlerData.Data;
            }
        }
    }
    return NULL;
}

#else

static const char* GetClipboardTextFn_DefaultImpl(void*)
{
    ImGuiContext& g = *GImGui;
    return g.ClipboardHandlerData.empty() ? NULL : g.ClipboardHandlerData.begin();
}

static void SetClipboardTextFn_DefaultImpl(void*, const char* text)
{
    ImGuiContext& g = *GImGui;
    g.ClipboardHandlerData.clear();
    const char* text_end = text + strlen(text);
    g.ClipboardHandlerData.resize((int)(text_end - text) + 1);
    memcpy(&g.ClipboardHandlerData[0], text, (size_t)(text_end - text));
    g.ClipboardHandlerData[(int)(text_end - text)] = 0;
}

#endif

#ifndef IMGUI_DISABLE_METRICS_WINDOW

static void RenderViewportThumbnail(ImDrawList* draw_list, ImGuiViewportP* viewport, const ImRect& bb)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImVec2 scale = bb.GetSize() / viewport->Size;
    ImVec2 off = bb.Min - viewport->Pos * scale;
    float alpha_mul = (viewport->Flags & ImGuiViewportFlags_Minimized) ? 0.30f : 1.00f;
    window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border, alpha_mul * 0.40f));
    for (int i = 0; i != g.Windows.Size; i++)
    {
        ImGuiWindow* thumb_window = g.Windows[i];
        if (!thumb_window->WasActive || ((thumb_window->Flags & ImGuiWindowFlags_ChildWindow)))
            continue;
        if (thumb_window->SkipItems && (thumb_window->Flags & ImGuiWindowFlags_ChildWindow))         
            continue;
        if (thumb_window->Viewport != viewport)
            continue;

        ImRect thumb_r = thumb_window->Rect();
        ImRect title_r = thumb_window->TitleBarRect();
        ImRect thumb_r_scaled = ImRect(ImFloor(off + thumb_r.Min * scale), ImFloor(off +  thumb_r.Max * scale));
        ImRect title_r_scaled = ImRect(ImFloor(off + title_r.Min * scale), ImFloor(off +  ImVec2(title_r.Max.x, title_r.Min.y) * scale) + ImVec2(0,5));     
        thumb_r_scaled.ClipWithFull(bb);
        title_r_scaled.ClipWithFull(bb);
        const bool window_is_focused = (g.NavWindow && thumb_window->RootWindowForTitleBarHighlight == g.NavWindow->RootWindowForTitleBarHighlight);
        window->DrawList->AddRectFilled(thumb_r_scaled.Min, thumb_r_scaled.Max, ImGui::GetColorU32(ImGuiCol_WindowBg, alpha_mul));
        window->DrawList->AddRectFilled(title_r_scaled.Min, title_r_scaled.Max, ImGui::GetColorU32(window_is_focused ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg, alpha_mul));
        window->DrawList->AddRect(thumb_r_scaled.Min, thumb_r_scaled.Max, ImGui::GetColorU32(ImGuiCol_Border, alpha_mul));
        if (ImGuiWindow* window_for_title = GetWindowForTitleDisplay(thumb_window))
            window->DrawList->AddText(g.Font, g.FontSize * 1.0f, title_r_scaled.Min, ImGui::GetColorU32(ImGuiCol_Text, alpha_mul), window_for_title->Name, ImGui::FindRenderedTextEnd(window_for_title->Name));
    }
    draw_list->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border, alpha_mul));
}

static void RenderViewportsThumbnails()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    float SCALE = 1.0f / 8.0f;
    ImRect bb_full;
    for (int n = 0; n < g.Viewports.Size; n++)
        bb_full.Add(g.Viewports[n]->GetMainRect());
    ImVec2 p = window->DC.CursorPos;
    ImVec2 off = p - bb_full.Min * SCALE;
    for (int n = 0; n < g.Viewports.Size; n++)
    {
        ImGuiViewportP* viewport = g.Viewports[n];
        ImRect viewport_draw_bb(off + (viewport->Pos) * SCALE, off + (viewport->Pos + viewport->Size) * SCALE);
        RenderViewportThumbnail(window->DrawList, viewport, viewport_draw_bb);
    }
    ImGui::Dummy(bb_full.GetSize() * SCALE);
}

static void MetricsHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void ImGui::ShowMetricsWindow(bool* p_open)
{
    if (!Begin("Dear ImGui Metrics/Debugger", p_open))
    {
        End();
        return;
    }

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    ImGuiMetricsConfig* cfg = &g.DebugMetricsConfig;

    Text("Dear ImGui %s", GetVersion());
    Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);
    Text("%d active windows (%d visible)", io.MetricsActiveWindows, io.MetricsRenderWindows);
    Text("%d active allocations", io.MetricsActiveAllocations);
    Separator();

    enum { WRT_OuterRect, WRT_OuterRectClipped, WRT_InnerRect, WRT_InnerClipRect, WRT_WorkRect, WRT_Content, WRT_ContentRegionRect, WRT_Count };    
    const char* wrt_rects_names[WRT_Count] = { "OuterRect", "OuterRectClipped", "InnerRect", "InnerClipRect", "WorkRect", "Content", "ContentRegionRect" };
    enum { TRT_OuterRect, TRT_WorkRect, TRT_HostClipRect, TRT_InnerClipRect, TRT_BackgroundClipRect, TRT_ColumnsRect, TRT_ColumnsClipRect, TRT_ColumnsContentHeadersUsed, TRT_ColumnsContentHeadersIdeal, TRT_ColumnsContentFrozen, TRT_ColumnsContentUnfrozen, TRT_Count };    
    const char* trt_rects_names[TRT_Count] = { "OuterRect", "WorkRect", "HostClipRect", "InnerClipRect", "BackgroundClipRect", "ColumnsRect", "ColumnsClipRect", "ColumnsContentHeadersUsed", "ColumnsContentHeadersIdeal", "ColumnsContentFrozen", "ColumnsContentUnfrozen" };
    if (cfg->ShowWindowsRectsType < 0)
        cfg->ShowWindowsRectsType = WRT_WorkRect;
    if (cfg->ShowTablesRectsType < 0)
        cfg->ShowTablesRectsType = TRT_WorkRect;

    struct Funcs
    {
        static ImRect GetTableRect(ImGuiTable* table, int rect_type, int n)
        {
            if (rect_type == TRT_OuterRect)                     { return table->OuterRect; }
            else if (rect_type == TRT_WorkRect)                 { return table->WorkRect; }
            else if (rect_type == TRT_HostClipRect)             { return table->HostClipRect; }
            else if (rect_type == TRT_InnerClipRect)            { return table->InnerClipRect; }
            else if (rect_type == TRT_BackgroundClipRect)       { return table->BgClipRect; }
            else if (rect_type == TRT_ColumnsRect)              { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->MinX, table->InnerClipRect.Min.y, c->MaxX, table->InnerClipRect.Min.y + table->LastOuterHeight); }
            else if (rect_type == TRT_ColumnsClipRect)          { ImGuiTableColumn* c = &table->Columns[n]; return c->ClipRect; }
            else if (rect_type == TRT_ColumnsContentHeadersUsed){ ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y, c->ContentMaxXHeadersUsed, table->InnerClipRect.Min.y + table->LastFirstRowHeight); }      
            else if (rect_type == TRT_ColumnsContentHeadersIdeal){ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y, c->ContentMaxXHeadersIdeal, table->InnerClipRect.Min.y + table->LastFirstRowHeight); }
            else if (rect_type == TRT_ColumnsContentFrozen)     { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y, c->ContentMaxXFrozen, table->InnerClipRect.Min.y + table->LastFirstRowHeight); }
            else if (rect_type == TRT_ColumnsContentUnfrozen)   { ImGuiTableColumn* c = &table->Columns[n]; return ImRect(c->WorkMinX, table->InnerClipRect.Min.y + table->LastFirstRowHeight, c->ContentMaxXUnfrozen, table->InnerClipRect.Max.y); }
            IM_ASSERT(0);
            return ImRect();
        }

        static ImRect GetWindowRect(ImGuiWindow* window, int rect_type)
        {
            if (rect_type == WRT_OuterRect)                 { return window->Rect(); }
            else if (rect_type == WRT_OuterRectClipped)     { return window->OuterRectClipped; }
            else if (rect_type == WRT_InnerRect)            { return window->InnerRect; }
            else if (rect_type == WRT_InnerClipRect)        { return window->InnerClipRect; }
            else if (rect_type == WRT_WorkRect)             { return window->WorkRect; }
            else if (rect_type == WRT_Content)              { ImVec2 min = window->InnerRect.Min - window->Scroll + window->WindowPadding; return ImRect(min, min + window->ContentSize); }
            else if (rect_type == WRT_ContentRegionRect)    { return window->ContentRegionRect; }
            IM_ASSERT(0);
            return ImRect();
        }
    };

    if (TreeNode("Tools"))
    {
        if (Button("Item Picker.."))
            DebugStartItemPicker();
        SameLine();
        MetricsHelpMarker("Will call the IM_DEBUG_BREAK() macro to break in debugger.\nWarning: If you don't have a debugger attached, this will probably crash.");

        Checkbox("Show windows begin order", &cfg->ShowWindowsBeginOrder);
        Checkbox("Show windows rectangles", &cfg->ShowWindowsRects);
        SameLine();
        SetNextItemWidth(GetFontSize() * 12);
        cfg->ShowWindowsRects |= Combo("##show_windows_rect_type", &cfg->ShowWindowsRectsType, wrt_rects_names, WRT_Count, WRT_Count);
        if (cfg->ShowWindowsRects && g.NavWindow != NULL)
        {
            BulletText("'%s':", g.NavWindow->Name);
            Indent();
            for (int rect_n = 0; rect_n < WRT_Count; rect_n++)
            {
                ImRect r = Funcs::GetWindowRect(g.NavWindow, rect_n);
                Text("(%6.1f,%6.1f) (%6.1f,%6.1f) Size (%6.1f,%6.1f) %s", r.Min.x, r.Min.y, r.Max.x, r.Max.y, r.GetWidth(), r.GetHeight(), wrt_rects_names[rect_n]);
            }
            Unindent();
        }
        Checkbox("Show ImDrawCmd mesh when hovering", &cfg->ShowDrawCmdMesh);
        Checkbox("Show ImDrawCmd bounding boxes when hovering", &cfg->ShowDrawCmdBoundingBoxes);

        Checkbox("Show tables rectangles", &cfg->ShowTablesRects);
        SameLine();
        SetNextItemWidth(GetFontSize() * 12);
        cfg->ShowTablesRects |= Combo("##show_table_rects_type", &cfg->ShowTablesRectsType, trt_rects_names, TRT_Count, TRT_Count);
        if (cfg->ShowTablesRects && g.NavWindow != NULL)
        {
            for (int table_n = 0; table_n < g.Tables.GetSize(); table_n++)
            {
                ImGuiTable* table = g.Tables.GetByIndex(table_n);
                if (table->LastFrameActive < g.FrameCount - 1 || (table->OuterWindow != g.NavWindow && table->InnerWindow != g.NavWindow))
                    continue;

                BulletText("Table 0x%08X (%d columns, in '%s')", table->ID, table->ColumnsCount, table->OuterWindow->Name);
                if (IsItemHovered())
                    GetForegroundDrawList()->AddRect(table->OuterRect.Min - ImVec2(1, 1), table->OuterRect.Max + ImVec2(1, 1), IM_COL32(255, 255, 0, 255), 0.0f, ~0, 2.0f);
                Indent();
                char buf[128];
                for (int rect_n = 0; rect_n < TRT_Count; rect_n++)
                {
                    if (rect_n >= TRT_ColumnsRect)
                    {
                        if (rect_n != TRT_ColumnsRect && rect_n != TRT_ColumnsClipRect)
                            continue;
                        for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
                        {
                            ImRect r = Funcs::GetTableRect(table, rect_n, column_n);
                            ImFormatString(buf, IM_ARRAYSIZE(buf), "(%6.1f,%6.1f) (%6.1f,%6.1f) Size (%6.1f,%6.1f) Col %d %s", r.Min.x, r.Min.y, r.Max.x, r.Max.y, r.GetWidth(), r.GetHeight(), column_n, trt_rects_names[rect_n]);
                            Selectable(buf);
                            if (IsItemHovered())
                                GetForegroundDrawList()->AddRect(r.Min - ImVec2(1, 1), r.Max + ImVec2(1, 1), IM_COL32(255, 255, 0, 255), 0.0f, ~0, 2.0f);
                        }
                    }
                    else
                    {
                        ImRect r = Funcs::GetTableRect(table, rect_n, -1);
                        ImFormatString(buf, IM_ARRAYSIZE(buf), "(%6.1f,%6.1f) (%6.1f,%6.1f) Size (%6.1f,%6.1f) %s", r.Min.x, r.Min.y, r.Max.x, r.Max.y, r.GetWidth(), r.GetHeight(), trt_rects_names[rect_n]);
                        Selectable(buf);
                        if (IsItemHovered())
                            GetForegroundDrawList()->AddRect(r.Min - ImVec2(1, 1), r.Max + ImVec2(1, 1), IM_COL32(255, 255, 0, 255), 0.0f, ~0, 2.0f);
                    }
                }
                Unindent();
            }
        }

        TreePop();
    }

    DebugNodeWindowsList(&g.Windows, "Windows");
    if (TreeNode("Viewports", "Viewports (%d)", g.Viewports.Size))
    {
        Indent(GetTreeNodeToLabelSpacing());
        RenderViewportsThumbnails();
        Unindent(GetTreeNodeToLabelSpacing());
        bool open = TreeNode("Monitors", "Monitors (%d)", g.PlatformIO.Monitors.Size);
        ImGui::SameLine();
        MetricsHelpMarker("Dear ImGui uses monitor data:\n- to query DPI settings on a per monitor basis\n- to position popup/tooltips so they don't straddle monitors.");
        if (open)
        {
            for (int i = 0; i < g.PlatformIO.Monitors.Size; i++)
            {
                const ImGuiPlatformMonitor& mon = g.PlatformIO.Monitors[i];
                BulletText("Monitor #%d: DPI %.0f%%\n MainMin (%.0f,%.0f), MainMax (%.0f,%.0f), MainSize (%.0f,%.0f)\n WorkMin (%.0f,%.0f), WorkMax (%.0f,%.0f), WorkSize (%.0f,%.0f)",
                    i, mon.DpiScale * 100.0f,
                    mon.MainPos.x, mon.MainPos.y, mon.MainPos.x + mon.MainSize.x, mon.MainPos.y + mon.MainSize.y, mon.MainSize.x, mon.MainSize.y,
                    mon.WorkPos.x, mon.WorkPos.y, mon.WorkPos.x + mon.WorkSize.x, mon.WorkPos.y + mon.WorkSize.y, mon.WorkSize.x, mon.WorkSize.y);
            }
            TreePop();
        }
        for (int i = 0; i < g.Viewports.Size; i++)
            DebugNodeViewport(g.Viewports[i]);
        TreePop();
    }

    if (TreeNode("Popups", "Popups (%d)", g.OpenPopupStack.Size))
    {
        for (int i = 0; i < g.OpenPopupStack.Size; i++)
        {
            ImGuiWindow* window = g.OpenPopupStack[i].Window;
            BulletText("PopupID: %08x, Window: '%s'%s%s", g.OpenPopupStack[i].PopupId, window ? window->Name : "NULL", window && (window->Flags & ImGuiWindowFlags_ChildWindow) ? " ChildWindow" : "", window && (window->Flags & ImGuiWindowFlags_ChildMenu) ? " ChildMenu" : "");
        }
        TreePop();
    }

    if (TreeNode("TabBars", "Tab Bars (%d)", g.TabBars.GetSize()))
    {
        for (int n = 0; n < g.TabBars.GetSize(); n++)
            DebugNodeTabBar(g.TabBars.GetByIndex(n), "TabBar");
        TreePop();
    }

#ifdef IMGUI_HAS_TABLE
    if (TreeNode("Tables", "Tables (%d)", g.Tables.GetSize()))
    {
        for (int n = 0; n < g.Tables.GetSize(); n++)
            DebugNodeTable(g.Tables.GetByIndex(n));
        TreePop();
    }
#endif   

#ifdef IMGUI_HAS_DOCK
    if (TreeNode("Docking"))
    {
        static bool root_nodes_only = true;
        ImGuiDockContext* dc = &g.DockContext;
        Checkbox("List root nodes", &root_nodes_only);
        Checkbox("Ctrl shows window dock info", &cfg->ShowDockingNodes);
        if (SmallButton("Clear nodes")) { DockContextClearNodes(&g, 0, true); }
        SameLine();
        if (SmallButton("Rebuild all")) { dc->WantFullRebuild = true; }
        for (int n = 0; n < dc->Nodes.Data.Size; n++)
            if (ImGuiDockNode* node = (ImGuiDockNode*)dc->Nodes.Data[n].val_p)
                if (!root_nodes_only || node->IsRootNode())
                    DebugNodeDockNode(node, "Node");
        TreePop();
    }
#endif   

    if (TreeNode("Settings"))
    {
        if (SmallButton("Clear"))
            ClearIniSettings();
        SameLine();
        if (SmallButton("Save to memory"))
            SaveIniSettingsToMemory();
        SameLine();
        if (SmallButton("Save to disk"))
            SaveIniSettingsToDisk(g.IO.IniFilename);
        SameLine();
        if (g.IO.IniFilename)
            Text("\"%s\"", g.IO.IniFilename);
        else
            TextUnformatted("<NULL>");
        Text("SettingsDirtyTimer %.2f", g.SettingsDirtyTimer);
        if (TreeNode("SettingsHandlers", "Settings handlers: (%d)", g.SettingsHandlers.Size))
        {
            for (int n = 0; n < g.SettingsHandlers.Size; n++)
                BulletText("%s", g.SettingsHandlers[n].TypeName);
            TreePop();
        }
        if (TreeNode("SettingsWindows", "Settings packed data: Windows: %d bytes", g.SettingsWindows.size()))
        {
            for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
                DebugNodeWindowSettings(settings);
            TreePop();
        }

#ifdef IMGUI_HAS_TABLE
        if (TreeNode("SettingsTables", "Settings packed data: Tables: %d bytes", g.SettingsTables.size()))
        {
            for (ImGuiTableSettings* settings = g.SettingsTables.begin(); settings != NULL; settings = g.SettingsTables.next_chunk(settings))
                DebugNodeTableSettings(settings);
            TreePop();
        }
#endif   

#ifdef IMGUI_HAS_DOCK
        if (ImGui::TreeNode("SettingsDocking", "Settings packed data: Docking"))
        {
            ImGuiDockContext* dc = &g.DockContext;
            ImGui::Text("In SettingsWindows:");
            for (ImGuiWindowSettings* settings = g.SettingsWindows.begin(); settings != NULL; settings = g.SettingsWindows.next_chunk(settings))
                if (settings->DockId != 0)
                    ImGui::BulletText("Window '%s' -> DockId %08X", settings->GetName(), settings->DockId);
            ImGui::Text("In SettingsNodes:");
            for (int n = 0; n < dc->NodesSettings.Size; n++)
            {
                ImGuiDockNodeSettings* settings = &dc->NodesSettings[n];
                const char* selected_tab_name = NULL;
                if (settings->SelectedWindowId)
                {
                    if (ImGuiWindow* window = FindWindowByID(settings->SelectedWindowId))
                        selected_tab_name = window->Name;
                    else if (ImGuiWindowSettings* window_settings = FindWindowSettings(settings->SelectedWindowId))
                        selected_tab_name = window_settings->GetName();
                }
                ImGui::BulletText("Node %08X, Parent %08X, SelectedTab %08X ('%s')", settings->ID, settings->ParentNodeId, settings->SelectedWindowId, selected_tab_name ? selected_tab_name : settings->SelectedWindowId ? "N/A" : "");
            }
            ImGui::TreePop();
        }
#endif   

        if (TreeNode("SettingsIniData", "Settings unpacked data (.ini): %d bytes", g.SettingsIniData.size()))
        {
            InputTextMultiline("##Ini", (char*)(void*)g.SettingsIniData.c_str(), g.SettingsIniData.Buf.Size, ImVec2(-FLT_MIN, GetTextLineHeight() * 20), ImGuiInputTextFlags_ReadOnly);
            TreePop();
        }
        TreePop();
    }

    if (TreeNode("Internal state"))
    {
        const char* input_source_names[] = { "None", "Mouse", "Nav", "NavKeyboard", "NavGamepad" }; IM_ASSERT(IM_ARRAYSIZE(input_source_names) == ImGuiInputSource_COUNT);

        Text("WINDOWING");
        Indent();
        Text("HoveredWindow: '%s'", g.HoveredWindow ? g.HoveredWindow->Name : "NULL");
        Text("HoveredRootWindow: '%s'", g.HoveredRootWindow ? g.HoveredRootWindow->Name : "NULL");
        Text("HoveredWindowUnderMovingWindow: '%s'", g.HoveredWindowUnderMovingWindow ? g.HoveredWindowUnderMovingWindow->Name : "NULL");
        Text("HoveredDockNode: 0x%08X", g.HoveredDockNode ? g.HoveredDockNode->ID : 0);
        Text("MovingWindow: '%s'", g.MovingWindow ? g.MovingWindow->Name : "NULL");
        Text("MouseViewport: 0x%08X (UserHovered 0x%08X, LastHovered 0x%08X)", g.MouseViewport->ID, g.IO.MouseHoveredViewport, g.MouseLastHoveredViewport ? g.MouseLastHoveredViewport->ID : 0);
        Unindent();

        Text("ITEMS");
        Indent();
        Text("ActiveId: 0x%08X/0x%08X (%.2f sec), AllowOverlap: %d, Source: %s", g.ActiveId, g.ActiveIdPreviousFrame, g.ActiveIdTimer, g.ActiveIdAllowOverlap, input_source_names[g.ActiveIdSource]);
        Text("ActiveIdWindow: '%s'", g.ActiveIdWindow ? g.ActiveIdWindow->Name : "NULL");
        Text("HoveredId: 0x%08X/0x%08X (%.2f sec), AllowOverlap: %d", g.HoveredId, g.HoveredIdPreviousFrame, g.HoveredIdTimer, g.HoveredIdAllowOverlap);                     
        Text("DragDrop: %d, SourceId = 0x%08X, Payload \"%s\" (%d bytes)", g.DragDropActive, g.DragDropPayload.SourceId, g.DragDropPayload.DataType, g.DragDropPayload.DataSize);
        Unindent();

        Text("NAV,FOCUS");
        Indent();
        Text("NavWindow: '%s'", g.NavWindow ? g.NavWindow->Name : "NULL");
        Text("NavId: 0x%08X, NavLayer: %d", g.NavId, g.NavLayer);
        Text("NavInputSource: %s", input_source_names[g.NavInputSource]);
        Text("NavActive: %d, NavVisible: %d", g.IO.NavActive, g.IO.NavVisible);
        Text("NavActivateId: 0x%08X, NavInputId: 0x%08X", g.NavActivateId, g.NavInputId);
        Text("NavDisableHighlight: %d, NavDisableMouseHover: %d", g.NavDisableHighlight, g.NavDisableMouseHover);
        Text("NavFocusScopeId = 0x%08X", g.NavFocusScopeId);
        Text("NavWindowingTarget: '%s'", g.NavWindowingTarget ? g.NavWindowingTarget->Name : "NULL");
        Unindent();

        TreePop();
    }

    if (cfg->ShowWindowsRects || cfg->ShowWindowsBeginOrder)
    {
        for (int n = 0; n < g.Windows.Size; n++)
        {
            ImGuiWindow* window = g.Windows[n];
            if (!window->WasActive)
                continue;
            ImDrawList* draw_list = GetForegroundDrawList(window);
            if (cfg->ShowWindowsRects)
            {
                ImRect r = Funcs::GetWindowRect(window, cfg->ShowWindowsRectsType);
                draw_list->AddRect(r.Min, r.Max, IM_COL32(255, 0, 128, 255));
            }
            if (cfg->ShowWindowsBeginOrder && !(window->Flags & ImGuiWindowFlags_ChildWindow))
            {
                char buf[32];
                ImFormatString(buf, IM_ARRAYSIZE(buf), "%d", window->BeginOrderWithinContext);
                float font_size = GetFontSize();
                draw_list->AddRectFilled(window->Pos, window->Pos + ImVec2(font_size, font_size), IM_COL32(200, 100, 100, 255));
                draw_list->AddText(window->Pos, IM_COL32(255, 255, 255, 255), buf);
            }
        }
    }

#ifdef IMGUI_HAS_TABLE
    if (cfg->ShowTablesRects)
    {
        for (int table_n = 0; table_n < g.Tables.GetSize(); table_n++)
        {
            ImGuiTable* table = g.Tables.GetByIndex(table_n);
            if (table->LastFrameActive < g.FrameCount - 1)
                continue;
            ImDrawList* draw_list = GetForegroundDrawList(table->OuterWindow);
            if (cfg->ShowTablesRectsType >= TRT_ColumnsRect)
            {
                for (int column_n = 0; column_n < table->ColumnsCount; column_n++)
                {
                    ImRect r = Funcs::GetTableRect(table, cfg->ShowTablesRectsType, column_n);
                    ImU32 col = (table->HoveredColumnBody == column_n) ? IM_COL32(255, 255, 128, 255) : IM_COL32(255, 0, 128, 255);
                    float thickness = (table->HoveredColumnBody == column_n) ? 3.0f : 1.0f;
                    draw_list->AddRect(r.Min, r.Max, col, 0.0f, ~0, thickness);
                }
            }
            else
            {
                ImRect r = Funcs::GetTableRect(table, cfg->ShowTablesRectsType, -1);
                draw_list->AddRect(r.Min, r.Max, IM_COL32(255, 0, 128, 255));
            }
        }
    }
#endif   

#ifdef IMGUI_HAS_DOCK
    if (cfg->ShowDockingNodes && g.IO.KeyCtrl && g.HoveredDockNode)
    {
        char buf[64] = "";
        char* p = buf;
        ImGuiDockNode* node = g.HoveredDockNode;
        ImDrawList* overlay_draw_list = node->HostWindow ? GetForegroundDrawList(node->HostWindow) : GetForegroundDrawList((ImGuiViewportP*)GetMainViewport());
        p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "DockId: %X%s\n", node->ID, node->IsCentralNode() ? " *CentralNode*" : "");
        p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "WindowClass: %08X\n", node->WindowClass.ClassId);
        p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "Size: (%.0f, %.0f)\n", node->Size.x, node->Size.y);
        p += ImFormatString(p, buf + IM_ARRAYSIZE(buf) - p, "SizeRef: (%.0f, %.0f)\n", node->SizeRef.x, node->SizeRef.y);
        int depth = DockNodeGetDepth(node);
        overlay_draw_list->AddRect(node->Pos + ImVec2(3, 3) * (float)depth, node->Pos + node->Size - ImVec2(3, 3) * (float)depth, IM_COL32(200, 100, 100, 255));
        ImVec2 pos = node->Pos + ImVec2(3, 3) * (float)depth;
        overlay_draw_list->AddRectFilled(pos - ImVec2(1, 1), pos + CalcTextSize(buf) + ImVec2(1, 1), IM_COL32(200, 100, 100, 255));
        overlay_draw_list->AddText(NULL, 0.0f, pos, IM_COL32(255, 255, 255, 255), buf);
    }
#endif   

    End();
}

void ImGui::DebugNodeColumns(ImGuiOldColumns* columns)
{
    if (!TreeNode((void*)(uintptr_t)columns->ID, "Columns Id: 0x%08X, Count: %d, Flags: 0x%04X", columns->ID, columns->Count, columns->Flags))
        return;
    BulletText("Width: %.1f (MinX: %.1f, MaxX: %.1f)", columns->OffMaxX - columns->OffMinX, columns->OffMinX, columns->OffMaxX);
    for (int column_n = 0; column_n < columns->Columns.Size; column_n++)
        BulletText("Column %02d: OffsetNorm %.3f (= %.1f px)", column_n, columns->Columns[column_n].OffsetNorm, GetColumnOffsetFromNorm(columns, columns->Columns[column_n].OffsetNorm));
    TreePop();
}

void ImGui::DebugNodeDockNode(ImGuiDockNode* node, const char* label)
{
    ImGuiContext& g = *GImGui;
    const bool is_alive = (g.FrameCount - node->LastFrameAlive < 2);       
    const bool is_active = (g.FrameCount - node->LastFrameActive < 2);   
    if (!is_alive) { PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled)); }
    bool open;
    if (node->Windows.Size > 0)
        open = TreeNode((void*)(intptr_t)node->ID, "%s 0x%04X%s: %d windows (vis: '%s')", label, node->ID, node->IsVisible ? "" : " (hidden)", node->Windows.Size, node->VisibleWindow ? node->VisibleWindow->Name : "NULL");
    else
        open = TreeNode((void*)(intptr_t)node->ID, "%s 0x%04X%s: %s split (vis: '%s')", label, node->ID, node->IsVisible ? "" : " (hidden)", (node->SplitAxis == ImGuiAxis_X) ? "horizontal" : (node->SplitAxis == ImGuiAxis_Y) ? "vertical" : "n/a", node->VisibleWindow ? node->VisibleWindow->Name : "NULL");
    if (!is_alive) { PopStyleColor(); }
    if (is_active && IsItemHovered())
        if (ImGuiWindow* window = node->HostWindow ? node->HostWindow : node->VisibleWindow)
            GetForegroundDrawList(window)->AddRect(node->Pos, node->Pos + node->Size, IM_COL32(255, 255, 0, 255));
    if (open)
    {
        IM_ASSERT(node->ChildNodes[0] == NULL || node->ChildNodes[0]->ParentNode == node);
        IM_ASSERT(node->ChildNodes[1] == NULL || node->ChildNodes[1]->ParentNode == node);
        BulletText("Pos (%.0f,%.0f), Size (%.0f, %.0f) Ref (%.0f, %.0f)",
            node->Pos.x, node->Pos.y, node->Size.x, node->Size.y, node->SizeRef.x, node->SizeRef.y);
        DebugNodeWindow(node->HostWindow, "HostWindow");
        DebugNodeWindow(node->VisibleWindow, "VisibleWindow");
        BulletText("SelectedTabID: 0x%08X, LastFocusedNodeID: 0x%08X", node->SelectedTabId, node->LastFocusedNodeId);
        BulletText("Misc:%s%s%s%s%s",
            node->IsDockSpace() ? " IsDockSpace" : "",
            node->IsCentralNode() ? " IsCentralNode" : "",
            is_alive ? " IsAlive" : "", is_active ? " IsActive" : "",
            node->WantLockSizeOnce ? " WantLockSizeOnce" : "");
        if (TreeNode("flags", "LocalFlags: 0x%04X SharedFlags: 0x%04X", node->LocalFlags, node->SharedFlags))
        {
            CheckboxFlags("LocalFlags: NoDocking", &node->LocalFlags, ImGuiDockNodeFlags_NoDocking);
            CheckboxFlags("LocalFlags: NoSplit", &node->LocalFlags, ImGuiDockNodeFlags_NoSplit);
            CheckboxFlags("LocalFlags: NoResize", &node->LocalFlags, ImGuiDockNodeFlags_NoResize);
            CheckboxFlags("LocalFlags: NoResizeX", &node->LocalFlags, ImGuiDockNodeFlags_NoResizeX);
            CheckboxFlags("LocalFlags: NoResizeY", &node->LocalFlags, ImGuiDockNodeFlags_NoResizeY);
            CheckboxFlags("LocalFlags: NoTabBar", &node->LocalFlags, ImGuiDockNodeFlags_NoTabBar);
            CheckboxFlags("LocalFlags: HiddenTabBar", &node->LocalFlags, ImGuiDockNodeFlags_HiddenTabBar);
            CheckboxFlags("LocalFlags: NoWindowMenuButton", &node->LocalFlags, ImGuiDockNodeFlags_NoWindowMenuButton);
            CheckboxFlags("LocalFlags: NoCloseButton", &node->LocalFlags, ImGuiDockNodeFlags_NoCloseButton);
            TreePop();
        }
        if (node->ParentNode)
            DebugNodeDockNode(node->ParentNode, "ParentNode");
        if (node->ChildNodes[0])
            DebugNodeDockNode(node->ChildNodes[0], "Child[0]");
        if (node->ChildNodes[1])
            DebugNodeDockNode(node->ChildNodes[1], "Child[1]");
        if (node->TabBar)
            DebugNodeTabBar(node->TabBar, "TabBar");
        TreePop();
    }
}

void ImGui::DebugNodeDrawList(ImGuiWindow* window, ImGuiViewportP* viewport, const ImDrawList* draw_list, const char* label)
{
    ImGuiContext& g = *GImGui;
    ImGuiMetricsConfig* cfg = &g.DebugMetricsConfig;
    int cmd_count = draw_list->CmdBuffer.Size;
    if (cmd_count > 0 && draw_list->CmdBuffer.back().ElemCount == 0 && draw_list->CmdBuffer.back().UserCallback == NULL)
        cmd_count--;
    bool node_open = TreeNode(draw_list, "%s: '%s' %d vtx, %d indices, %d cmds", label, draw_list->_OwnerName ? draw_list->_OwnerName : "", draw_list->VtxBuffer.Size, draw_list->IdxBuffer.Size, cmd_count);
    if (draw_list == GetWindowDrawList())
    {
        SameLine();
        TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "CURRENTLY APPENDING");              
        if (node_open)
            TreePop();
        return;
    }

    ImDrawList* fg_draw_list = viewport ? GetForegroundDrawList(viewport) : NULL;         
    if (window && fg_draw_list && IsItemHovered())
        fg_draw_list->AddRect(window->Pos, window->Pos + window->Size, IM_COL32(255, 255, 0, 255));
    if (!node_open)
        return;

    if (window && !window->WasActive)
        TextDisabled("Warning: owning Window is inactive. This DrawList is not being rendered!");

    for (const ImDrawCmd* pcmd = draw_list->CmdBuffer.Data; pcmd < draw_list->CmdBuffer.Data + cmd_count; pcmd++)
    {
        if (pcmd->UserCallback)
        {
            BulletText("Callback %p, user_data %p", pcmd->UserCallback, pcmd->UserCallbackData);
            continue;
        }

        char buf[300];
        ImFormatString(buf, IM_ARRAYSIZE(buf), "DrawCmd:%5d tris, Tex 0x%p, ClipRect (%4.0f,%4.0f)-(%4.0f,%4.0f)",
            pcmd->ElemCount / 3, (void*)(intptr_t)pcmd->TextureId,
            pcmd->ClipRect.x, pcmd->ClipRect.y, pcmd->ClipRect.z, pcmd->ClipRect.w);
        bool pcmd_node_open = TreeNode((void*)(pcmd - draw_list->CmdBuffer.begin()), "%s", buf);
        if (IsItemHovered() && (cfg->ShowDrawCmdMesh || cfg->ShowDrawCmdBoundingBoxes) && fg_draw_list)
            DebugNodeDrawCmdShowMeshAndBoundingBox(fg_draw_list, draw_list, pcmd, cfg->ShowDrawCmdMesh, cfg->ShowDrawCmdBoundingBoxes);
        if (!pcmd_node_open)
            continue;

        const ImDrawIdx* idx_buffer = (draw_list->IdxBuffer.Size > 0) ? draw_list->IdxBuffer.Data : NULL;
        const ImDrawVert* vtx_buffer = draw_list->VtxBuffer.Data + pcmd->VtxOffset;
        float total_area = 0.0f;
        for (unsigned int idx_n = pcmd->IdxOffset; idx_n < pcmd->IdxOffset + pcmd->ElemCount; )
        {
            ImVec2 triangle[3];
            for (int n = 0; n < 3; n++, idx_n++)
                triangle[n] = vtx_buffer[idx_buffer ? idx_buffer[idx_n] : idx_n].pos;
            total_area += ImTriangleArea(triangle[0], triangle[1], triangle[2]);
        }

        ImFormatString(buf, IM_ARRAYSIZE(buf), "Mesh: ElemCount: %d, VtxOffset: +%d, IdxOffset: +%d, Area: ~%0.f px", pcmd->ElemCount, pcmd->VtxOffset, pcmd->IdxOffset, total_area);
        Selectable(buf);
        if (IsItemHovered() && fg_draw_list)
            DebugNodeDrawCmdShowMeshAndBoundingBox(fg_draw_list, draw_list, pcmd, true, false);

        ImGuiListClipper clipper;
        clipper.Begin(pcmd->ElemCount / 3);                   
        while (clipper.Step())
            for (int prim = clipper.DisplayStart, idx_i = pcmd->IdxOffset + clipper.DisplayStart * 3; prim < clipper.DisplayEnd; prim++)
            {
                char* buf_p = buf, * buf_end = buf + IM_ARRAYSIZE(buf);
                ImVec2 triangle[3];
                for (int n = 0; n < 3; n++, idx_i++)
                {
                    const ImDrawVert& v = vtx_buffer[idx_buffer ? idx_buffer[idx_i] : idx_i];
                    triangle[n] = v.pos;
                    buf_p += ImFormatString(buf_p, buf_end - buf_p, "%s %04d: pos (%8.2f,%8.2f), uv (%.6f,%.6f), col %08X\n",
                        (n == 0) ? "Vert:" : "     ", idx_i, v.pos.x, v.pos.y, v.uv.x, v.uv.y, v.col);
                }

                Selectable(buf, false);
                if (fg_draw_list && IsItemHovered())
                {
                    ImDrawListFlags backup_flags = fg_draw_list->Flags;
                    fg_draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;               
                    fg_draw_list->AddPolyline(triangle, 3, IM_COL32(255, 255, 0, 255), true, 1.0f);
                    fg_draw_list->Flags = backup_flags;
                }
            }
        TreePop();
    }
    TreePop();
}

void ImGui::DebugNodeDrawCmdShowMeshAndBoundingBox(ImDrawList* out_draw_list, const ImDrawList* draw_list, const ImDrawCmd* draw_cmd, bool show_mesh, bool show_aabb)
{
    IM_ASSERT(show_mesh || show_aabb);
    ImDrawIdx* idx_buffer = (draw_list->IdxBuffer.Size > 0) ? draw_list->IdxBuffer.Data : NULL;
    ImDrawVert* vtx_buffer = draw_list->VtxBuffer.Data + draw_cmd->VtxOffset;

    ImRect clip_rect = draw_cmd->ClipRect;
    ImRect vtxs_rect(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    ImDrawListFlags backup_flags = out_draw_list->Flags;
    out_draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;               
    for (unsigned int idx_n = draw_cmd->IdxOffset; idx_n < draw_cmd->IdxOffset + draw_cmd->ElemCount; )
    {
        ImVec2 triangle[3];
        for (int n = 0; n < 3; n++, idx_n++)
            vtxs_rect.Add((triangle[n] = vtx_buffer[idx_buffer ? idx_buffer[idx_n] : idx_n].pos));
        if (show_mesh)
            out_draw_list->AddPolyline(triangle, 3, IM_COL32(255, 255, 0, 255), true, 1.0f);     
    }
    if (show_aabb)
    {
        out_draw_list->AddRect(ImFloor(clip_rect.Min), ImFloor(clip_rect.Max), IM_COL32(255, 0, 255, 255));        
        out_draw_list->AddRect(ImFloor(vtxs_rect.Min), ImFloor(vtxs_rect.Max), IM_COL32(0, 255, 255, 255));       
    }
    out_draw_list->Flags = backup_flags;
}

void ImGui::DebugNodeStorage(ImGuiStorage* storage, const char* label)
{
    if (!TreeNode(label, "%s: %d entries, %d bytes", label, storage->Data.Size, storage->Data.size_in_bytes()))
        return;
    for (int n = 0; n < storage->Data.Size; n++)
    {
        const ImGuiStorage::ImGuiStoragePair& p = storage->Data[n];
        BulletText("Key 0x%08X Value { i: %d }", p.key, p.val_i);              
    }
    TreePop();
}

void ImGui::DebugNodeTabBar(ImGuiTabBar* tab_bar, const char* label)
{
    char buf[256];
    char* p = buf;
    const char* buf_end = buf + IM_ARRAYSIZE(buf);
    const bool is_active = (tab_bar->PrevFrameVisible >= GetFrameCount() - 2);
    p += ImFormatString(p, buf_end - p, "%s 0x%08X (%d tabs)%s", label, tab_bar->ID, tab_bar->Tabs.Size, is_active ? "" : " *Inactive*");
    p += ImFormatString(p, buf_end - p, "  { ");
    for (int tab_n = 0; tab_n < ImMin(tab_bar->Tabs.Size, 3); tab_n++)
    {
        ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
        p += ImFormatString(p, buf_end - p, "%s'%s'",
            tab_n > 0 ? ", " : "", (tab->Window || tab->NameOffset != -1) ? tab_bar->GetTabName(tab) : "???");
    }
    p += ImFormatString(p, buf_end - p, (tab_bar->Tabs.Size > 3) ? " ... }" : " } ");
    if (!is_active) { PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled)); }
    bool open = TreeNode(tab_bar, "%s", buf);
    if (!is_active) { PopStyleColor(); }
    if (is_active && IsItemHovered())
    {
        ImDrawList* draw_list = GetForegroundDrawList();
        draw_list->AddRect(tab_bar->BarRect.Min, tab_bar->BarRect.Max, IM_COL32(255, 255, 0, 255));
        draw_list->AddLine(ImVec2(tab_bar->ScrollingRectMinX, tab_bar->BarRect.Min.y), ImVec2(tab_bar->ScrollingRectMinX, tab_bar->BarRect.Max.y), IM_COL32(0, 255, 0, 255));
        draw_list->AddLine(ImVec2(tab_bar->ScrollingRectMaxX, tab_bar->BarRect.Min.y), ImVec2(tab_bar->ScrollingRectMaxX, tab_bar->BarRect.Max.y), IM_COL32(0, 255, 0, 255));
    }
    if (open)
    {
        for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
        {
            const ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
            PushID(tab);
            if (SmallButton("<")) { TabBarQueueReorder(tab_bar, tab, -1); } SameLine(0, 2);
            if (SmallButton(">")) { TabBarQueueReorder(tab_bar, tab, +1); } SameLine();
            Text("%02d%c Tab 0x%08X '%s' Offset: %.1f, Width: %.1f/%.1f",
                tab_n, (tab->ID == tab_bar->SelectedTabId) ? '*' : ' ', tab->ID, (tab->Window || tab->NameOffset != -1) ? tab_bar->GetTabName(tab) : "???", tab->Offset, tab->Width, tab->ContentWidth);
            PopID();
        }
        TreePop();
    }
}

void ImGui::DebugNodeViewport(ImGuiViewportP* viewport)
{
    SetNextItemOpen(true, ImGuiCond_Once);
    if (TreeNode((void*)(intptr_t)viewport->ID, "Viewport #%d, ID: 0x%08X, Parent: 0x%08X, Window: \"%s\"", viewport->Idx, viewport->ID, viewport->ParentViewportId, viewport->Window ? viewport->Window->Name : "N/A"))
    {
        ImGuiWindowFlags flags = viewport->Flags;
        BulletText("Main Pos: (%.0f,%.0f), Size: (%.0f,%.0f)\nWorkArea Offset Left: %.0f Top: %.0f, Right: %.0f, Bottom: %.0f\nMonitor: %d, DpiScale: %.0f%%",
            viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y,
            viewport->WorkOffsetMin.x, viewport->WorkOffsetMin.y, viewport->WorkOffsetMax.x, viewport->WorkOffsetMax.y,
            viewport->PlatformMonitor, viewport->DpiScale * 100.0f);
        if (viewport->Idx > 0) { SameLine(); if (SmallButton("Reset Pos")) { viewport->Pos = ImVec2(200,200); if (viewport->Window) viewport->Window->Pos = ImVec2(200,200); } }
        BulletText("Flags: 0x%04X =%s%s%s%s%s%s%s", viewport->Flags,
            (flags & ImGuiViewportFlags_CanHostOtherWindows) ? " CanHostOtherWindows" : "", (flags & ImGuiViewportFlags_NoDecoration) ? " NoDecoration" : "",
            (flags & ImGuiViewportFlags_NoFocusOnAppearing)  ? " NoFocusOnAppearing"  : "", (flags & ImGuiViewportFlags_NoInputs)     ? " NoInputs"     : "",
            (flags & ImGuiViewportFlags_NoRendererClear)     ? " NoRendererClear"     : "", (flags & ImGuiViewportFlags_Minimized)    ? " Minimized"    : "",
            (flags & ImGuiViewportFlags_NoAutoMerge)         ? " NoAutoMerge"         : "");
        for (int layer_i = 0; layer_i < IM_ARRAYSIZE(viewport->DrawDataBuilder.Layers); layer_i++)
            for (int draw_list_i = 0; draw_list_i < viewport->DrawDataBuilder.Layers[layer_i].Size; draw_list_i++)
                DebugNodeDrawList(NULL, viewport, viewport->DrawDataBuilder.Layers[layer_i][draw_list_i], "DrawList");
        TreePop();
    }
}

void ImGui::DebugNodeWindow(ImGuiWindow* window, const char* label)
{
    if (window == NULL)
    {
        BulletText("%s: NULL", label);
        return;
    }

    ImGuiContext& g = *GImGui;
    const bool is_active = window->WasActive;
    ImGuiTreeNodeFlags tree_node_flags = (window == g.NavWindow) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;
    if (!is_active) { PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled)); }
    const bool open = TreeNodeEx(label, tree_node_flags, "%s '%s'%s", label, window->Name, is_active ? "" : " *Inactive*");
    if (!is_active) { PopStyleColor(); }
    if (IsItemHovered() && is_active)
        GetForegroundDrawList(window)->AddRect(window->Pos, window->Pos + window->Size, IM_COL32(255, 255, 0, 255));
    if (!open)
        return;

    if (window->MemoryCompacted)
        TextDisabled("Note: some memory buffers have been compacted/freed.");

    ImGuiWindowFlags flags = window->Flags;
    DebugNodeDrawList(window, window->Viewport, window->DrawList, "DrawList");
    BulletText("Pos: (%.1f,%.1f), Size: (%.1f,%.1f), ContentSize (%.1f,%.1f)", window->Pos.x, window->Pos.y, window->Size.x, window->Size.y, window->ContentSize.x, window->ContentSize.y);
    BulletText("Flags: 0x%08X (%s%s%s%s%s%s%s%s%s..)", flags,
        (flags & ImGuiWindowFlags_ChildWindow)  ? "Child " : "",      (flags & ImGuiWindowFlags_Tooltip)     ? "Tooltip "   : "",  (flags & ImGuiWindowFlags_Popup) ? "Popup " : "",
        (flags & ImGuiWindowFlags_Modal)        ? "Modal " : "",      (flags & ImGuiWindowFlags_ChildMenu)   ? "ChildMenu " : "",  (flags & ImGuiWindowFlags_NoSavedSettings) ? "NoSavedSettings " : "",
        (flags & ImGuiWindowFlags_NoMouseInputs)? "NoMouseInputs":"", (flags & ImGuiWindowFlags_NoNavInputs) ? "NoNavInputs" : "", (flags & ImGuiWindowFlags_AlwaysAutoResize) ? "AlwaysAutoResize" : "");
    BulletText("WindowClassId: 0x%08X", window->WindowClass.ClassId);
    BulletText("Scroll: (%.2f/%.2f,%.2f/%.2f) Scrollbar:%s%s", window->Scroll.x, window->ScrollMax.x, window->Scroll.y, window->ScrollMax.y, window->ScrollbarX ? "X" : "", window->ScrollbarY ? "Y" : "");
    BulletText("Active: %d/%d, WriteAccessed: %d, BeginOrderWithinContext: %d", window->Active, window->WasActive, window->WriteAccessed, (window->Active || window->WasActive) ? window->BeginOrderWithinContext : -1);
    BulletText("Appearing: %d, Hidden: %d (CanSkip %d Cannot %d), SkipItems: %d", window->Appearing, window->Hidden, window->HiddenFramesCanSkipItems, window->HiddenFramesCannotSkipItems, window->SkipItems);
    BulletText("NavLastIds: 0x%08X,0x%08X, NavLayerActiveMask: %X", window->NavLastIds[0], window->NavLastIds[1], window->DC.NavLayerActiveMask);
    BulletText("NavLastChildNavWindow: %s", window->NavLastChildNavWindow ? window->NavLastChildNavWindow->Name : "NULL");
    if (!window->NavRectRel[0].IsInverted())
        BulletText("NavRectRel[0]: (%.1f,%.1f)(%.1f,%.1f)", window->NavRectRel[0].Min.x, window->NavRectRel[0].Min.y, window->NavRectRel[0].Max.x, window->NavRectRel[0].Max.y);
    else
        BulletText("NavRectRel[0]: <None>");
    BulletText("Viewport: %d%s, ViewportId: 0x%08X, ViewportPos: (%.1f,%.1f)", window->Viewport ? window->Viewport->Idx : -1, window->ViewportOwned ? " (Owned)" : "", window->ViewportId, window->ViewportPos.x, window->ViewportPos.y);
    BulletText("ViewportMonitor: %d", window->Viewport ? window->Viewport->PlatformMonitor : -1);
    BulletText("DockId: 0x%04X, DockOrder: %d, Act: %d, Vis: %d", window->DockId, window->DockOrder, window->DockIsActive, window->DockTabIsVisible);
    if (window->DockNode || window->DockNodeAsHost)
        DebugNodeDockNode(window->DockNodeAsHost ? window->DockNodeAsHost : window->DockNode, window->DockNodeAsHost ? "DockNodeAsHost" : "DockNode");
    if (window->RootWindow != window)       { DebugNodeWindow(window->RootWindow, "RootWindow"); }
    if (window->RootWindowDockStop != window->RootWindow) { DebugNodeWindow(window->RootWindowDockStop, "RootWindowDockStop"); }
    if (window->ParentWindow != NULL)       { DebugNodeWindow(window->ParentWindow, "ParentWindow"); }
    if (window->DC.ChildWindows.Size > 0)   { DebugNodeWindowsList(&window->DC.ChildWindows, "ChildWindows"); }
    if (window->ColumnsStorage.Size > 0 && TreeNode("Columns", "Columns sets (%d)", window->ColumnsStorage.Size))
    {
        for (int n = 0; n < window->ColumnsStorage.Size; n++)
            DebugNodeColumns(&window->ColumnsStorage[n]);
        TreePop();
    }
    DebugNodeStorage(&window->StateStorage, "Storage");
    TreePop();
}

void ImGui::DebugNodeWindowSettings(ImGuiWindowSettings* settings)
{
    Text("0x%08X \"%s\" Pos (%d,%d) Size (%d,%d) Collapsed=%d",
        settings->ID, settings->GetName(), settings->Pos.x, settings->Pos.y, settings->Size.x, settings->Size.y, settings->Collapsed);
}

void ImGui::DebugNodeWindowsList(ImVector<ImGuiWindow*>* windows, const char* label)
{
    if (!TreeNode(label, "%s (%d)", label, windows->Size))
        return;
    Text("(In front-to-back order:)");
    for (int i = windows->Size - 1; i >= 0; i--)     
    {
        PushID((*windows)[i]);
        DebugNodeWindow((*windows)[i], "Window");
        PopID();
    }
    TreePop();
}

#else

void ImGui::ShowMetricsWindow(bool*) {}
void ImGui::DebugNodeColumns(ImGuiOldColumns*) {}
void ImGui::DebugNodeDrawList(ImGuiWindow*, ImGuiViewportP*, const ImDrawList*, const char*) {}
void ImGui::DebugNodeDrawCmdShowMeshAndBoundingBox(ImDrawList*, const ImDrawList*, const ImDrawCmd*, bool, bool) {}
void ImGui::DebugNodeStorage(ImGuiStorage*, const char*) {}
void ImGui::DebugNodeTabBar(ImGuiTabBar*, const char*) {}
void ImGui::DebugNodeWindow(ImGuiWindow*, const char*) {}
void ImGui::DebugNodeWindowSettings(ImGuiWindowSettings*) {}
void ImGui::DebugNodeWindowsList(ImVector<ImGuiWindow*>*, const char*) {}
void ImGui::DebugNodeViewport(ImGuiViewportP*) {}

#endif

#ifdef IMGUI_INCLUDE_IMGUI_USER_INL
#include "imgui_user.inl"
#endif

#endif   
