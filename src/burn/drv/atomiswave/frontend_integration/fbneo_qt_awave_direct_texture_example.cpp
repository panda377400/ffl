// Example integration snippet for FBNeo-QT / any OpenGL frontend.
// This is not meant to compile as a standalone file; paste the relevant pieces
// into the frontend renderer that owns the QOpenGLContext/window surface.

#include "awave_core.h"

// Option A: use an opaque frontend bridge object.
struct AwaveFrontendGlBridge {
    // QOpenGLContext* context;
    // QSurface* surface;
};

static void AwaveFrontendMakeCurrent(void* opaque)
{
    AwaveFrontendGlBridge* bridge = (AwaveFrontendGlBridge*)opaque;
    (void)bridge;
    // Qt example:
    // bridge->context->makeCurrent(bridge->surface);
}

static void AwaveFrontendDoneCurrent(void* opaque)
{
    AwaveFrontendGlBridge* bridge = (AwaveFrontendGlBridge*)opaque;
    (void)bridge;
    // Qt example:
    // bridge->context->doneCurrent();
}

void RegisterAwaveFrontendGlContext(AwaveFrontendGlBridge* bridge)
{
    OGLSetContext(bridge);
    OGLSetMakeCurrent(AwaveFrontendMakeCurrent);
    OGLSetDoneCurrent(AwaveFrontendDoneCurrent);
    OGLSetUseFallbackContext(false);
}

bool TryDrawAwaveDirectTexture()
{
    NaomiCoreHwVideoInfo info;
    if (NaomiCoreGetHwVideoInfo(&info) != 0 || !info.valid || info.texture == 0) {
        return false;
    }

    // The renderer should already have its GL context current here.
    // Draw info.texture as a GL_TEXTURE_2D fullscreen/viewport quad.
    // Flip vertically if info.upsideDown != 0.
    // Then skip the normal pBurnDraw/SoftFX upload for this frame.
    //
    // Minimal legacy GL idea:
    // glBindTexture(GL_TEXTURE_2D, info.texture);
    // draw textured quad covering the game viewport.

    return true;
}
