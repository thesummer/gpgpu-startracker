#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <fstream>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "ogles.h"

#ifdef _DEBUG
    #define GL_CHECK(stmt) do { \
            stmt; \
            checkOpenGLError(#stmt, __FILE__, __LINE__); \
        } while (0)
#else
    #define GL_CHECK(stmt) stmt
#endif

#ifdef _DEBUG
    #define EGL_CHECK(stmt) do { \
            if(!stmt) { \
                checkEGLError(#stmt, __FILE__, __LINE__); \
            }\
        } while(0)
#else
#define EGL_CHECK(stmt) stmt
#endif

ogles::ogles(int width, int height)
{
    // Initialize esContext to 0
    esContext = {};

    // initialize EGL-context
}

ogles::ogles(std::string tgaFilename)
{
    // Initialize esContext to 0
    esContext = {};

    // Read TGA-file

    // initialize EGL-context
}



int ogles::initEGL(int width, int height)
{
    EGLDisplay eglDisplay;
    EGLConfig eglConfig;

    if ( esContext == NULL )
    {
       return GL_FALSE;
    }

    esContext->width  = width;
    esContext->height = height;

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
   EGLSurface eglSurface;
   eglSurface = eglCreatePbufferSurface(eglDisplay, eglConfig, NULL);
   EGL_CHECK( (eglSurface != EGL_NO_SURFACE) );

// Step 7 - Create a context.
   EGLContext eglContext = eglCreateContext(eglDisplay, eglConfig, NULL, contextAttribs);
   EGL_CHECK( (eglContext != EGL_NO_CONTEXT) );

// Step 8 - Bind the context to the current thread
   EGL_CHECK( eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) );

   esContext->eglDisplay = eglDisplay;
   esContext->eglSurface = eglSurface;
   esContext->eglContext = eglContext;

   return EGL_TRUE;
}

int ogles::initOGL()
{

}

GLuint ogles::loadProgramFromFile(const std::string vertShaderFile, const std::string fragShaderFile)
{
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;


    std::ifstream sourceFile(vertexShaderFile);
    std::string sourceString((std::istreambuf_iterator<char>(sourceFile)),
                              std::istreambuf_iterator<char>());

    sourceFile.close();

    // Load the vertex/fragment shaders
    vertexShader = loadShader( GL_VERTEX_SHADER, sourceString );
    if ( vertexShader == 0 )
       goto error;

    sourceFile.open(fragShaderFile);
    std::string sourceString((std::istreambuf_iterator<char>(sourceFile)),
                              std::istreambuf_iterator<char>());
    sourceFile.close();

    fragmentShader = loadShader(GL_FRAGMENT_SHADER, sourceString );
    if ( fragmentShader == 0 )
    {
       glDeleteShader( vertexShader );
       goto error;
    }

    // Create the program object
    programObject = glCreateProgram ( );
    if ( programObject == 0 )
       goto error;

    GL_CHECK( glAttachShader ( programObject, vertexShader ) );
    GL_CHECK( glAttachShader ( programObject, fragmentShader ) );

    // Link the program
    GL_CHECK( glLinkProgram ( programObject ) );

    // Check the link status
    GL_CHECK( glGetProgramiv ( programObject, GL_LINK_STATUS, &linked ) );

    if ( !linked )
    {
       GLint infoLen = 0;

       GL_CHECK( glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen ) );

       if ( infoLen > 1 )
       {
          char* infoLog = malloc (sizeof(char) * infoLen );

          GL_CHECK( glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog ) );
          cerr << "Error linking program:" << endl;
          cerr << infoLog << endl;

          free ( infoLog );
       }

       glDeleteProgram ( programObject );
       goto error;
    }

    // Free up no longer needed shader resources
    glDeleteShader ( vertexShader );
    glDeleteShader ( fragmentShader );

    return programObject;

 error:
    glDeleteShader ( vertexShader );
    glDeleteShader ( fragmentShader );
    return 0;
}

GLuint ogles::loadShader(GLenum type, const std::string &shaderSrc)
{
    GLuint shader;
    GLint compiled;

    // Create the shader object
    shader = glCreateShader ( type );

    if ( shader == 0 )
     return 0;

    // Load the shader source
    const GLchar * cstr = shaderSrc.c_str();
    GL_CHECK( glShaderSource ( shader, 1, &cstr, NULL ) );

    // Compile the shader
    GL_CHECK( glCompileShader ( shader ) );

    // Check the compile status
    GL_CHECK(glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled ) );

    if ( !compiled )
    {
       GLint infoLen = 0;

       GL_CHECK( glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen ) );

       if ( infoLen > 1 )
       {
          char* infoLog = malloc (sizeof(char) * infoLen );

          GL_CHECK( glGetShaderInfoLog ( shader, infoLen, NULL, infoLog ) );
          cerr << "Error compiling shader:" << endl;
          cerr << infoLog << endl;

          free ( infoLog );
       }

       GL_CHECK( glDeleteShader ( shader ) );
       return 0;
    }

    return shader;
}

void ogles::checkEGLError(const char *stmt, const char *fname, int line)
{
    GLenum err = eglGetError();
    if (err != EGL_SUCCESS)
    {
        printf("EGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
        abort();
    }
}

void ogles::checkOpenGLError(const char *stmt, const char *fname, int line)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
        abort();
    }
}
