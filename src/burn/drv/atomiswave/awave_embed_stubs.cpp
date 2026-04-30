#include "awave_wrap_config.h"
#include "types.h"
#include "stdclass.h"
#include "hw/pvr/Renderer_if.h"

#if defined(_WIN32)
#include <windows.h>
#include <GL/gl.h>
#endif

static void* g_oglContext = nullptr;
static void (*g_oglSwapBuffers)(void*) = nullptr;
static void (*g_oglMakeCurrent)(void*) = nullptr;
static void (*g_oglDoneCurrent)(void*) = nullptr;
static bool g_useDummyOglContext = false;

#if defined(_WIN32)
struct DummyOglContext {
	HWND hwnd = nullptr;
	HDC hdc = nullptr;
	HGLRC hglrc = nullptr;
};

static DummyOglContext g_dummyOglContext;
static ATOM g_dummyOglWindowClass = 0;

static const char* OGLDummyWindowClassName()
{
	return "FBNeoAwaveHiddenGLWindow";
}

static bool OGLRegisterDummyWindowClass()
{
	if (g_dummyOglWindowClass != 0) {
		return true;
	}

	WNDCLASSA wc = {};
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = DefWindowProcA;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.lpszClassName = OGLDummyWindowClassName();

	g_dummyOglWindowClass = RegisterClassA(&wc);
	if (g_dummyOglWindowClass == 0) {
		const DWORD err = GetLastError();
		if (err == ERROR_CLASS_ALREADY_EXISTS) {
			g_dummyOglWindowClass = 1;
			return true;
		}
		return false;
	}

	return true;
}

static bool OGLCreateDummyContext()
{
	if (g_dummyOglContext.hglrc != nullptr) {
		if (g_oglContext == nullptr) {
			g_oglContext = &g_dummyOglContext;
		}
		return true;
	}

	if (!OGLRegisterDummyWindowClass()) {
		return false;
	}

	HWND hwnd = CreateWindowA(OGLDummyWindowClassName(), "FBNeoAwaveHiddenGL", WS_POPUP,
		0, 0, 640, 480, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	if (hwnd == nullptr) {
		return false;
	}

	HDC hdc = GetDC(hwnd);
	if (hdc == nullptr) {
		DestroyWindow(hwnd);
		return false;
	}

	PIXELFORMATDESCRIPTOR pfd = {};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int pixelFormat = ChoosePixelFormat(hdc, &pfd);
	if (pixelFormat == 0 || !SetPixelFormat(hdc, pixelFormat, &pfd)) {
		ReleaseDC(hwnd, hdc);
		DestroyWindow(hwnd);
		return false;
	}

	HGLRC hglrc = wglCreateContext(hdc);
	if (hglrc == nullptr) {
		ReleaseDC(hwnd, hdc);
		DestroyWindow(hwnd);
		return false;
	}

	g_dummyOglContext.hwnd = hwnd;
	g_dummyOglContext.hdc = hdc;
	g_dummyOglContext.hglrc = hglrc;
	if (g_oglContext == nullptr) {
		g_oglContext = &g_dummyOglContext;
	}
	return true;
}

static void OGLDestroyDummyContext()
{
	if (g_dummyOglContext.hglrc == nullptr) {
		return;
	}

	if (wglGetCurrentContext() == g_dummyOglContext.hglrc) {
		wglMakeCurrent(nullptr, nullptr);
	}

	wglDeleteContext(g_dummyOglContext.hglrc);
	ReleaseDC(g_dummyOglContext.hwnd, g_dummyOglContext.hdc);
	DestroyWindow(g_dummyOglContext.hwnd);

	if (g_oglContext == &g_dummyOglContext) {
		g_oglContext = nullptr;
	}

	g_dummyOglContext = {};
}
#else
static bool OGLCreateDummyContext()
{
	return false;
}

static void OGLDestroyDummyContext()
{
}
#endif

#if !defined(HAVE_OPENGL) && !defined(HAVE_OPENGLES)
int screen_width = 640;
int screen_height = 480;
#else
extern int screen_width;
extern int screen_height;
DECL_ALIGN(32) u32 pixels[640 * 480];
#endif

#if defined(TARGET_NO_REC)
u8* CodeCache = nullptr;
#endif

#if !defined(HAVE_OPENGL) && !defined(HAVE_OPENGLES)
void rend_set_fb_scale(float, float)
{
}
#endif

void OGLSetContext(void* ptr)
{
	g_oglContext = ptr;
}

void OGLSetUseFallbackContext(bool use)
{
	g_useDummyOglContext = use;
}

void OGLSetSwapBuffers(void (*f)(void*))
{
	g_oglSwapBuffers = f;
}

void OGLSetMakeCurrent(void (*f)(void*))
{
	g_oglMakeCurrent = f;
}

void OGLSetDoneCurrent(void (*f)(void*))
{
	g_oglDoneCurrent = f;
}

void* OGLGetContext()
{
#if defined(_WIN32)
	if (g_useDummyOglContext && g_dummyOglContext.hglrc != nullptr) {
		return &g_dummyOglContext;
	}
#endif
	return g_oglContext;
}

void OGLMakeCurrentContext()
{
#if defined(_WIN32)
	if (g_useDummyOglContext && g_dummyOglContext.hglrc != nullptr) {
		wglMakeCurrent(g_dummyOglContext.hdc, g_dummyOglContext.hglrc);
		return;
	}
#endif
	if (g_oglMakeCurrent) {
		g_oglMakeCurrent(g_oglContext);
		return;
	}
}

void OGLDoneCurrentContext()
{
#if defined(_WIN32)
	if (g_useDummyOglContext && g_dummyOglContext.hglrc != nullptr) {
		wglMakeCurrent(nullptr, nullptr);
		return;
	}
#endif
	if (g_oglDoneCurrent) {
		g_oglDoneCurrent(g_oglContext);
		return;
	}
}

void OGLSwapCurrentContext()
{
#if defined(_WIN32)
	if (g_useDummyOglContext && g_dummyOglContext.hdc != nullptr) {
		return;
	}
#endif
	if (g_oglSwapBuffers) {
		g_oglSwapBuffers(g_oglContext);
		return;
	}
}

bool OGLCreateFallbackContext()
{
	return OGLCreateDummyContext();
}

void OGLDestroyFallbackContext()
{
	OGLDestroyDummyContext();
}

extern "C" void* OGLGetProcAddress(const char* name)
{
#if defined(_WIN32)
	if (name == nullptr) {
		return nullptr;
	}

	void* proc = reinterpret_cast<void*>(wglGetProcAddress(name));
	if (proc != nullptr && proc != reinterpret_cast<void*>(0x1) && proc != reinterpret_cast<void*>(0x2)
		&& proc != reinterpret_cast<void*>(0x3) && proc != reinterpret_cast<void*>(-1)) {
		return proc;
	}

	static HMODULE oglModule = LoadLibraryA("opengl32.dll");
	if (oglModule != nullptr) {
		return reinterpret_cast<void*>(GetProcAddress(oglModule, name));
	}
#else
	(void)name;
#endif
	return nullptr;
}

#if defined(TARGET_NO_REC)
void bm_Reset()
{
}

bool bm_RamWriteAccess(void*)
{
	return false;
}

bool BM_LockedWrite(u8*)
{
	return false;
}

void bm_RamWriteAccess(u32)
{
}

extern "C" void ngen_HandleException()
{
}
#endif

#if defined(TARGET_NO_THREADS)
cThread::cThread(ThreadEntryFP* function, void* prm) : Entry(function), param(prm), hThread(nullptr)
{
}

void cThread::Start()
{
}

void cThread::WaitToEnd()
{
	hThread = nullptr;
}
#endif

#if defined(TARGET_NO_THREADS)
bool reset_requested = false;
#endif

const char* flycast_retro_get_system_directory(void);

const char* retro_get_system_directory(void)
{
	return flycast_retro_get_system_directory();
}
