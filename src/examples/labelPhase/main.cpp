#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <fstream>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "getTime.h"
#include"labelPhase.h"
#include "tga.h"

#ifdef _RPI
#include "bcm_host.h"
#endif

struct ESContext
{
   /// Window width
   GLint       width;

   /// Window height
   GLint       height;

   /// Window handle
   EGLNativeWindowType  hWnd;

   /// EGL display
   EGLDisplay  eglDisplay;

   /// EGL context
   EGLContext  eglContext;

   /// EGL surface
   EGLSurface  eglSurface;
} esContext;

int main()
{
#ifdef _RPI
    bcm_host_init();
#endif

    // Initialize esContext to 0
    esContext = {};

    LabelPhase labelPhase;

    // Read TGA-file
    TGA *tgaImage = 0;
    loadTgaImage(&tgaImage, &mImgData, "../test.tga");

    mWidth  = tgaImage->hdr.width;
    mHeight = tgaImage->hdr.height;

    labelPhase.mWidth  = mWidth;
    labelPhase.mHeight = mHeight;
    labelPhase.mTgaData = &mImgData;

    // initialize EGL-context
    initEGL(mWidth, mHeight);

    // initialize the 2 frambuffers for ping-pong method
    GL_CHECK( glGenFramebuffers(2, mFboId) );

    if(!labelPhase.initIndependent(mFboId, mUsedTexUnits) )
        exit(1);

    double labelTime;

    mLabelPhase.setupGeometry();
    labelTime = mLabelPhase.run();

    cout << "Label time: " << labelTime << endl;


#ifdef _RPI
    bcm_host_deinit();
#endif

    return 0;
}
