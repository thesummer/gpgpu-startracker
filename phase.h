#ifndef PHASE_H
#define PHASE_H

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <string>

#ifdef _DEBUG
    #define GL_CHECK(stmt) do { \
            stmt; \
            Phase::checkOpenGLError(#stmt, __FILE__, __LINE__); \
        } while (0)
#else
    #define GL_CHECK(stmt) stmt
#endif


class Phase
{
public:

    virtual void run () = 0;

    static GLuint createSimpleTexture2D(GLsizei width, GLsizei height,
                                        GLubyte *data = NULL,
                                        GLint type = GL_RGBA);

    static GLuint loadProgramFromFile(const std::string vertShaderFile,
                                      const std::string fragShaderFile);

    static GLuint loadShader(GLenum type, const std::string &shaderSrc);

    static void checkOpenGLError(const char* stmt, const char* fname, int line);

    static int writeTgaImage(int width, int height, char *filename, GLubyte *pixels);
    static int writeRawTgaImage(int width, int height, char *filename, GLubyte *pixels);

    void printLabels(int width, int height, GLubyte *pixels);

    static int logBase2(int n);
};

#endif // PHASE_H
