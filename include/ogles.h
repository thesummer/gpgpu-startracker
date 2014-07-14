#ifndef OGLES_H
#define OGLES_H

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include<string>
#include "phase.h"
#include "labelPhase.h"
#include "reductionPhase.h"
#include "lookupPhase.h"
#include "tga.h"

class Ogles
{
public:
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

// Phases:
    //1. LabelPhase
    LabelPhase mLabelPhase;
    //2. ReductionPhase
    ReductionPhase mReductionPhase;
    //3. Create a lookup table
    LookupPhase mLookupPhase;


    Ogles(int width, int height);
    virtual ~Ogles();
    Ogles(std::string tgaFilename);

    void run();

private:
    int initEGL(int width, int height);

    int loadTgaImage(TGA **image, TGAData *data, const char *filename);
    void checkEGLError(const char* stmt, const char* fname, int line);

    int mWidth;
    int mHeight;
    unsigned mUsedTexUnits;

    // Framebuffers
    TGAData mImgData;
    GLuint mFboId[2] ;

};

#endif // OGLES_H
