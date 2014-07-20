#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <fstream>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "ogles.h"
#include "phase.h"
#include "getTime.h"



#ifdef _DEBUG
    #define EGL_CHECK(stmt) do { \
            if(!stmt) { \
                checkEGLError(#stmt, __FILE__, __LINE__); \
            }\
        } while(0)
#else
#define EGL_CHECK(stmt) stmt
#endif

Ogles::Ogles(int width, int height)
    :mLabelPhase(width, height), mWidth(width), mHeight(height), mUsedTexUnits(0)
{
 /// TODO: update constructor
    // Initialize structs to 0
    esContext = {};

    // initialize EGL-context
    initEGL(width, height);

    // initialize the 2 frambuffers for ping-pong method
    GL_CHECK( glGenFramebuffers(2, mFboId) );

//    mLabelPhase.init(mFboId);
//    mReductionPhase.initIndependent(mFboId, mUsedTexUnits);
}

Ogles::~Ogles()
{
    // Clean up OpenGL objects
    GL_CHECK( glDeleteFramebuffers(2, mFboId) );
    // Clean up EGL-context
    EGL_CHECK ( eglMakeCurrent(esContext.eglDisplay , EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) );
    EGL_CHECK ( eglDestroyContext(esContext.eglDisplay, esContext.eglContext) );
    EGL_CHECK ( eglDestroySurface(esContext.eglDisplay, esContext.eglSurface) );
    EGL_CHECK ( eglTerminate(esContext.eglDisplay) );
}

Ogles::Ogles(std::string tgaFilename)
    :mLabelPhase(0, 0), mUsedTexUnits(0)
{
    // Initialize esContext to 0
    esContext = {};

    // Read TGA-file
    TGA *tgaImage = 0;
    loadTgaImage(&tgaImage, &mImgData, tgaFilename.c_str());

    mWidth  = tgaImage->hdr.width;
    mHeight = tgaImage->hdr.height;

    mLabelPhase.mWidth  = mWidth;
    mLabelPhase.mHeight = mHeight;
    mLabelPhase.mTgaData = &mImgData;

    // initialize EGL-context
    initEGL(mWidth, mHeight);

    // initialize the 2 frambuffers for ping-pong method
    GL_CHECK( glGenFramebuffers(2, mFboId) );

    if(!mLabelPhase.initIndependent(mFboId, mUsedTexUnits) )
        exit(1);

    mReductionPhase.mWidth   = mWidth;
    mReductionPhase.mHeight  = mHeight;
    mReductionPhase.mTgaData = &mImgData;
    if (!mReductionPhase.init(mFboId, mUsedTexUnits) )
        exit(1);
//    mReductionPhase.initIndependent(mFboId, mUsedTexUnits);

    mLookupPhase.mTexWidth  = mWidth;
    mLookupPhase.mTexHeight = mHeight;
    mLookupPhase.mTgaData   = &mImgData;
    mLookupPhase.mVertexWidth  = 16;
    mLookupPhase.mVertexHeight = mHeight;

   if (!mLookupPhase.init(mUsedTexUnits) )
       exit(1);

}

void Ogles::run()
{
    double startTime, endTime;
    double labelTime, reductionTime;
    double lookupTime;

    startTime = getRealTime();

    mLabelPhase.setupGeometry();
    labelTime = mLabelPhase.run();


    mReductionPhase.updateTextures(mLabelPhase.getLastTexture(), mLabelPhase.getLastTexUnit(),
                                   mLabelPhase.getFreeTexture(), mLabelPhase.getFreeTexUnit() );
//    mReductionPhase.setupGeometry();
    reductionTime = mReductionPhase.run();

    mLookupPhase.updateTextures(mReductionPhase.getLastTexture(), mReductionPhase.getLastTexUnit(),
                                mReductionPhase.getFreeTexture(), mReductionPhase.getFreeTexUnit() );
    mLookupPhase.setFbo(mReductionPhase.getFreeFbo());

    mLookupPhase.setupGeometry();
    lookupTime = mLookupPhase.run();

    endTime = getRealTime();

    cout << "Label time: " << labelTime << endl;
    cout << "Reduction time: " << reductionTime << endl;
    cout << "Lookup time: " << lookupTime << endl;
    cout << "Total time: " << (endTime-startTime)*1000 << endl;
}



int Ogles::initEGL(int width, int height)
{
    EGLDisplay eglDisplay;
    EGLConfig eglConfig;

    esContext.width  = width;
    esContext.height = height;

// Step 1 - Get the default display.
   eglDisplay = eglGetDisplay((EGLNativeDisplayType)0);
   EGL_CHECK( (eglDisplay != EGL_NO_DISPLAY) );

// Step 2 - Initialize EGL.
   EGL_CHECK(eglInitialize(eglDisplay, NULL, NULL) );

   EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2,
                                                          EGL_NONE };

// Step 3 - Make OpenGL ES the current API.
   EGL_CHECK( eglBindAPI(EGL_OPENGL_ES_API) );

// Step 4 - Specify the required configuration attributes.
   EGLint attribList[] = { EGL_SURFACE_TYPE   , EGL_PBUFFER_BIT,
                           EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                           EGL_NONE
                         };

// Step 5 - Find a config that matches all requirements.
   int iConfigs;
   EGL_CHECK( eglChooseConfig(eglDisplay, attribList, &eglConfig, 1, &iConfigs) );

   if (iConfigs != 1) {
       printf("Error: eglChooseConfig(): config not found.\n");
       return EGL_FALSE;
   }

// Step 6 - Create a surface to draw to.

   // Necessary to set EGL_WIDTH and EGL_HEIGHT to at least 1
   // if left default (0) the rpi gets into troubles
   const EGLint srfPbufferAttr[] =
   {
       EGL_WIDTH, 1,
       EGL_HEIGHT, 1,
       EGL_NONE
   };
   EGLSurface eglSurface;
   eglSurface = eglCreatePbufferSurface(eglDisplay, eglConfig, srfPbufferAttr);
   EGL_CHECK( (eglSurface != EGL_NO_SURFACE) );

// Step 7 - Create a context.
   EGLContext eglContext = eglCreateContext(eglDisplay, eglConfig, NULL, contextAttribs);
   EGL_CHECK( (eglContext != EGL_NO_CONTEXT) );

// Step 8 - Bind the context to the current thread
   EGL_CHECK( eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) );

   esContext.eglDisplay = eglDisplay;
   esContext.eglSurface = eglSurface;
   esContext.eglContext = eglContext;

   return EGL_TRUE;
}

int Ogles::loadTgaImage(TGA **image, TGAData *data, const char *filename)
{
    *image = TGAOpen(filename, "r");
    data->flags = TGA_IMAGE_DATA | TGA_RGB;

    if(!*image || (*image)->last != TGA_OK)
    {
        printf("Opening tga-file failed\n");
        return (*image)->last;
    }

    if(TGAReadImage(*image, data) != TGA_OK)
    {
        printf("Failed to read tga-file\n");
        return (*image)->last;
    }
    return TGA_OK;
}

void Ogles::checkEGLError(const char *stmt, const char *fname, int line)
{
    GLenum err = eglGetError();
    if (err != EGL_SUCCESS)
    {
        printf("EGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
        abort();
    }
}

