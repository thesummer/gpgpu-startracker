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

    #define CHECK_FBO() do {                                                                    \
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);                           \
            if (status != GL_FRAMEBUFFER_COMPLETE)                                              \
            {                                                                                   \
               printf("OpenGL error %08x, at %s:%i - Framebuffer is not complete\n", status, __FILE__, __LINE__);    \
               abort();                                                                         \
            }                                                                                   \
        } while (0)
#else
    #define GL_CHECK(stmt) stmt
    #define CHECK_FBO() ;
#endif


/*!
 \brief Abstract class which provides the general interface for the different OpenGL phases

 This class provides some general functions which are used by all child classes.
 The \ref run function as well as the constructors have to be implemented by
 all derived classes.

*/
class Phase
{
public:

    /*!
     \brief Function which does the computation

     Has to be implemented by every child class.
     Is called after all data structures are setup for the run.

     \return double Time the computation took in ms
    */
    virtual double run () = 0;

    /*!
     \brief Creates a 2d-texture and copies data to it

     The function will generate a OpenGL texture object of given width and
     height, bind a GL_TEXTURE_2D target to it and upload the RGBA values
     from data to it (if given). If data is NULL an uninitialized texture is
     created.

     \param width Width of the new texture
     \param height Height of the new texture
     \param data pointer to the data
     \param type Texture type, default: GL_RGBA
     \return GLuint The index of the newly created texture
    */
    static GLuint createSimpleTexture2D(GLsizei width, GLsizei height,
                                        GLubyte *data = NULL,
                                        GLint type = GL_RGBA);

    /*!
     \brief Loads a shader program from a shader file

     \param vertShaderFile Path to the file with the vertex shader
     \param fragShaderFile Path to the file with the fragment shader
     \return GLuint The index of the newly created shader program
    */
    static GLuint loadProgramFromFile(const std::string vertShaderFile,
                                      const std::string fragShaderFile);

    /*!
     \brief Creates and compiles a shader source

     \param type Either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
     \param shaderSrc   Source code of the shader
     \return GLuint     The index of the newly created shader object
    */
    static GLuint loadShader(GLenum type, const std::string &shaderSrc);

    /*!
     \brief Checks if the OpenGL error flag is set and prints an error message

     This function is mainly called by the debug macro GL_CHECK when
     compiled with _DEBUG flag set. It will check if the previous
     call to an GL-function was successful and if not print a error
     message with statement, filename and line number.

     \param stmt
     \param fname
     \param line
    */
    static void checkOpenGLError(const char* stmt, const char* fname, int line);

    /*!
     \brief Writes the pixel data into an tga-file

     It creates a new tga file of the given filename and writes the pixel data
     to it. The pixel data is put in a function before it is written to the file
     in order to create a false color image and make it easier to spot differences
     in neighboring pixels. If the original pixel values are to be stored,
     use \ref writeRawTgaImage instead.

     \param width Width of the image
     \param height Height of the image
     \param filename Name of the tga file to write to
     \param pixels data array with RGBA array data of length 4*width*height
     \return int TGA_OK on success
    */
    static int writeTgaImage(int width, int height, char *filename, GLubyte *pixels);
    /*!
     \brief Writes the raw pixel values into a tga-file

     It creates a new tga file of the given filename and writes the pixel data
     to it. The pixel data is not changed (in contrast to \ref writeTgaImage)
     before it is written to the file

     \param width Width of the image
     \param height Height of the image
     \param filename Name of the tga file to write to
     \param pixels data array with RGBA array data of length 4*width*height
     \return int TGA_OK on success
    */
    static int writeRawTgaImage(int width, int height, char *filename, GLubyte *pixels);

    /*!
     \brief Simple function to print the values of labels as formatted text

      This function is meant as debugging aid. After downloading the content
      of a FBO with glReadPixels, the encoded labels can be printed with this
      function. It converts the RGBA values into two unsigned shorts and prints
      their formatted values to stdout.

      NOTE: This is only meant to be used for small images

     \param width  Width of the image
     \param height Height of the image
     \param pixels Pixel data
    */
    void printLabels(int width, int height, GLubyte *pixels);
    /*!
     \brief Simple function to print the values of labels as formatted text

      This function is meant as debugging aid. After downloading the content
      of a FBO with glReadPixels, the encoded labels can be printed with this
      function. It converts the RGBA values into and signed integer and prints
      their formatted values to stdout.

      NOTE: This is only meant to be used for small images

     \param width  Width of the image
     \param height Height of the image
     \param pixels Pixel data
    */
    void printSignedLabels(int width, int height, GLubyte *pixels);

    /*!
     \brief Returns the logarithm of base 2 of n

     \param n
     \return int
    */
    static int logBase2(int n);

};

#endif // PHASE_H
