// Experimental Win32 direct OpenGL presenter for FBNeo Atomiswave/Flycast.
//
// This file is intentionally isolated from the generic video plugins.
// It is only active when FBNEO_AWAVE_VIDEO_MODE=direct_texture.
//
// Fix version notes:
// - Prepare() must be called before BurnDrvInit()/NaomiCoreInit().
// - DirectPresentEnabled() must NOT return true before the WGL presenter is ready.
// - Exit() clears OGL callbacks and destroys child HWND/HDC/HGLRC so repeated game
//   loads do not accumulate stale GL state.

#include "burner.h"
#include <GL/gl.h>
#include <string.h>

struct NaomiCoreHwVideoInfo {
    UINT32 size;
    UINT32 texture;
    UINT32 framebuffer;
    INT32 width;
    INT32 height;
    INT32 upsideDown;
    INT32 valid;
};

// Provided by src/burn/drv/atomiswave/awave_core.cpp / awave_core.h.
extern INT32 NaomiCoreGetHwVideoInfo(NaomiCoreHwVideoInfo* info);
extern INT32 NaomiCoreUsingHwDirectPresent();
extern void OGLSetContext(void* ptr);
extern void OGLSetMakeCurrent(void (*f)(void*));
extern void OGLSetDoneCurrent(void (*f)(void*));
extern void OGLSetSwapBuffers(void (*f)(void*));
extern void OGLSetUseFallbackContext(bool use);

struct AwaveWin32GLState {
    HWND parent;
    HWND child;
    HDC dc;
    HGLRC rc;
    bool callbacksInstalled;
    int lastW;
    int lastH;
};

static AwaveWin32GLState g_awaveGl = { NULL, NULL, NULL, NULL, false, 0, 0 };

static bool awave_env_is_direct_texture()
{
    const char* mode = getenv("FBNEO_AWAVE_VIDEO_MODE");
    return mode && (!_stricmp(mode, "direct_texture") || !_stricmp(mode, "direct") || !_stricmp(mode, "gl"));
}

static LRESULT CALLBACK awave_child_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

static ATOM awave_register_child_class()
{
    static ATOM atom = 0;
    if (atom) return atom;

    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = awave_child_proc;
    wc.hInstance = hAppInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = _T("FBNeoAwaveGLPresentWindow");
    atom = RegisterClass(&wc);
    return atom;
}

static void awave_make_current(void* ptr)
{
    AwaveWin32GLState* st = (AwaveWin32GLState*)ptr;
    if (st && st->dc && st->rc) {
        wglMakeCurrent(st->dc, st->rc);
    }
}

static void awave_done_current(void*)
{
    wglMakeCurrent(NULL, NULL);
}

static void awave_swap_buffers(void* ptr)
{
    AwaveWin32GLState* st = (AwaveWin32GLState*)ptr;
    if (st && st->dc) {
        SwapBuffers(st->dc);
    }
}

static void awave_resize_child(HWND parent)
{
    if (!g_awaveGl.child || !parent) return;

    RECT rc;
    GetClientRect(parent, &rc);
    const int w = rc.right - rc.left;
    const int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;

    if (w != g_awaveGl.lastW || h != g_awaveGl.lastH) {
        SetWindowPos(g_awaveGl.child, HWND_TOP, 0, 0, w, h, SWP_SHOWWINDOW);
        g_awaveGl.lastW = w;
        g_awaveGl.lastH = h;
    }
}

static void awave_clear_callbacks()
{
    // Put Atomiswave/Flycast GL bridge back into safe fallback state.
    // These setters accept NULL function pointers in the Plan-B atomiswave bridge.
    OGLSetMakeCurrent(NULL);
    OGLSetDoneCurrent(NULL);
    OGLSetSwapBuffers(NULL);
    OGLSetContext(NULL);
    OGLSetUseFallbackContext(true);
}

INT32 AwaveWin32Prepare(HWND hParent)
{
    if (!awave_env_is_direct_texture()) {
        return 0;
    }

    if (!hParent) {
        return 1;
    }

    if (g_awaveGl.rc && g_awaveGl.child) {
        awave_resize_child(hParent);
        return 0;
    }

    if (!awave_register_child_class()) {
        return 1;
    }

    g_awaveGl.parent = hParent;
    g_awaveGl.child = CreateWindowEx(
        0,
        _T("FBNeoAwaveGLPresentWindow"),
        _T(""),
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0, 64, 64,
        hParent,
        NULL,
        hAppInst,
        NULL);

    if (!g_awaveGl.child) {
        memset(&g_awaveGl, 0, sizeof(g_awaveGl));
        return 1;
    }

    g_awaveGl.dc = GetDC(g_awaveGl.child);
    if (!g_awaveGl.dc) {
        DestroyWindow(g_awaveGl.child);
        memset(&g_awaveGl, 0, sizeof(g_awaveGl));
        return 1;
    }

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    const int pf = ChoosePixelFormat(g_awaveGl.dc, &pfd);
    if (pf == 0 || !SetPixelFormat(g_awaveGl.dc, pf, &pfd)) {
        ReleaseDC(g_awaveGl.child, g_awaveGl.dc);
        DestroyWindow(g_awaveGl.child);
        memset(&g_awaveGl, 0, sizeof(g_awaveGl));
        return 1;
    }

    g_awaveGl.rc = wglCreateContext(g_awaveGl.dc);
    if (!g_awaveGl.rc) {
        ReleaseDC(g_awaveGl.child, g_awaveGl.dc);
        DestroyWindow(g_awaveGl.child);
        memset(&g_awaveGl, 0, sizeof(g_awaveGl));
        return 1;
    }

    awave_resize_child(hParent);

    // Critical: install the Win32 context before the Atomiswave driver initializes.
    // If Flycast uses its fallback hidden context, the texture id will not be
    // visible in this presenter context.
    OGLSetUseFallbackContext(false);
    OGLSetContext(&g_awaveGl);
    OGLSetMakeCurrent(awave_make_current);
    OGLSetDoneCurrent(awave_done_current);
    OGLSetSwapBuffers(awave_swap_buffers);
    g_awaveGl.callbacksInstalled = true;

    wglMakeCurrent(g_awaveGl.dc, g_awaveGl.rc);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    SwapBuffers(g_awaveGl.dc);
    wglMakeCurrent(NULL, NULL);

    bprintf(0, _T("AWAVE Win32 GL presenter prepared before driver init: child=%p dc=%p rc=%p\n"), g_awaveGl.child, g_awaveGl.dc, g_awaveGl.rc);
    return 0;
}

INT32 AwaveWin32DirectPresentEnabled()
{
    if (!awave_env_is_direct_texture()) {
        return 0;
    }

    if (!g_awaveGl.rc) {
        return 0;
    }

    return NaomiCoreUsingHwDirectPresent() ? 1 : 0;
}

INT32 AwaveWin32Frame(HWND hParent)
{
    if (!awave_env_is_direct_texture()) {
        return 1;
    }

    if (AwaveWin32Prepare(hParent)) {
        return 1;
    }

    if (!NaomiCoreUsingHwDirectPresent()) {
        return 1;
    }

    awave_resize_child(hParent);

    NaomiCoreHwVideoInfo info;
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);

    if (!NaomiCoreGetHwVideoInfo(&info) || !info.valid || info.texture == 0 || info.width <= 0 || info.height <= 0) {
        return 1;
    }

    if (!wglMakeCurrent(g_awaveGl.dc, g_awaveGl.rc)) {
        return 1;
    }

    RECT rc;
    GetClientRect(g_awaveGl.child, &rc);
    const int dstW = rc.right - rc.left;
    const int dstH = rc.bottom - rc.top;
    if (dstW <= 0 || dstH <= 0) {
        wglMakeCurrent(NULL, NULL);
        return 1;
    }

    glViewport(0, 0, dstW, dstH);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, (GLuint)info.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    const GLfloat t0 = info.upsideDown ? 1.0f : 0.0f;
    const GLfloat t1 = info.upsideDown ? 0.0f : 1.0f;

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, t0); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, t0); glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(1.0f, t1); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(0.0f, t1); glVertex2f(-1.0f,  1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    SwapBuffers(g_awaveGl.dc);
    wglMakeCurrent(NULL, NULL);
    return 0;
}

void AwaveWin32Exit()
{
    if (g_awaveGl.callbacksInstalled) {
        awave_clear_callbacks();
        g_awaveGl.callbacksInstalled = false;
    }

    if (g_awaveGl.rc) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_awaveGl.rc);
        g_awaveGl.rc = NULL;
    }

    if (g_awaveGl.child && g_awaveGl.dc) {
        ReleaseDC(g_awaveGl.child, g_awaveGl.dc);
        g_awaveGl.dc = NULL;
    }

    if (g_awaveGl.child) {
        DestroyWindow(g_awaveGl.child);
        g_awaveGl.child = NULL;
    }

    memset(&g_awaveGl, 0, sizeof(g_awaveGl));
    bprintf(0, _T("AWAVE Win32 GL presenter exited and callbacks cleared\n"));
}
