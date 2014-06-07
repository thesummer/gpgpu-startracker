#ifndef OGLES_H
#define OGLES_H

#include<string>
#include "include/tga.h"

class ogles
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


    // Struct which hold the Opengl handles for every stage
    // 1. Label stage:
    struct labData
    {
        // Handle to a program object
        GLuint programObject;

        // Attribute locations
        /// TODO: general shader?
        GLint  positionLoc;
        GLint  texCoordLoc;

        // Uniform locations
        GLint  u_texDimLoc;
        GLint  u_thresholdLoc;
        GLint  u_passLoc;
        GLint  u_debugLoc;
        GLint  u_factorLoc;

        // Uniform values
        float u_threshold;
        GLint u_pass;
        GLint u_debug;
        GLint u_factor;

        // Sampler location
        GLint samplerLoc;

        // Texture handle
        GLuint textureId;
        /// TODO: tga somewhere else?
        TGA    *tgaImage;
        TGAData *tgaData;

        // Framebuffer
        GLuint fboId[NUM_FBO] ;
        GLuint write;
        GLuint read;

        // Texture to attach to the frambuffer
        GLuint fboTexId[NUM_FBO];
    };

    ogles(int width, int height);
    ogles(std::string tgaFilename);

private:
    int initEGL(int width, int height);
    int initOGL();

    GLuint loadProgramFromFile(const std::string vertShaderFile,
                               const std::string fragShaderFile);

    GLuint loadShader(GLenum type, const std::string &shaderSrc);

    int loadTgaImage(TGA **image, TGAData *data, char *filename);
    void checkEGLError(const char* stmt, const char* fname, int line);
    void checkOpenGLError(const char* stmt, const char* fname, int line);

    int mWidth;
    int mHeight;

};

#endif // OGLES_H
