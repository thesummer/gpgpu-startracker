#ifndef OGLES_H
#define OGLES_H

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include<string>
#include "phase.h"
#include "labelPhase.h"
#include "reductionPhase.h"
#include "statsPhase.h"
#include "tga.h"

/*!
 \brief Class which manages all the OpenGL handling for gpulabeling

  This class takes care of creating the necessary EGLContext, the initialization of all
  OpenGL structures and the correct order of execution of the different Phases.

*/
class Ogles
{
public:
    /*!
     \brief Keeps all handles necessary to create an EGLContext

     Follows the OpenGL ES2.0 programming guide by Aaftab Munshi,
     Dan Ginsburg, Dave Shreiner.

    */
    struct ESContext
    {
       GLint       width; /*!< Width of the window surface */

       GLint       height; /*!< Height of the window surface*/

       EGLNativeWindowType  hWnd; /*!< Handle for the EGLWindow*/

       EGLDisplay  eglDisplay; /*!< Handle to the EGLDisplay*/

       // EGL context
       EGLContext  eglContext; /*!< Handle to the EGLContext*/

       // EGL surface
       EGLSurface  eglSurface; /*!< Handle to the EGLSurface*/
    } esContext; /*!< TODO */

// Phases:
    //1. LabelPhase
    LabelPhase mLabelPhase; /*!< Object which takes care of thresholding and labeling of the Image*/
    //2. ReductionPhase
    ReductionPhase mReductionPhase; /*!< Object which creates a list of all identified spots*/
    //3. Compute the statistics of the labels
    StatsPhase mStatsPhase; /*!< Object which computes the statistics for each identified spot*/

    /*!
     \brief Constructor

     Hasn't been used in a long time, so it probably won't work atm.
     TODO: Either update the constructor or remove it.

     \param width
     \param height
    */
    Ogles(int width, int height);

    /*!
     \brief Destructor

    */
    virtual ~Ogles();

    /*!
     \brief Constructor which creates object and loads inital file

     It creates and initializes the EGLContext the OpenGL-structures
     and the Phase-Objects. The file \param tgaFilename is copied to a
     texture and will be used as a starting point for the computation.

     \param tgaFilename Path to the file which is to be evaluated
    */
    Ogles(std::string tgaFilename);

    /*!
     \brief Function which does all the computation

     Runs through all phases and makes sure each phase receives
     the necessary information of previous phases.

    */
    void run();

private:
    /*!
     \brief Initializes an EGLContext

     It takes care of all the step necessary to create a valid EGLContext.
     The Context is needed to call any OpenGL-functions.
     Because the GPU is only used for computation the Context is created without
     a display (EGL_NO_DISPLAY). It is necessary to create a pbuffer surface
     instead. As only Framebuffer Objects and textures are used for memory
     transfers between CPU and GPU the pbuffer size is set to 1x1 pixel.

     \param width
     \param height
     \return int Returns EGL_TRUE on success and EGL_FALSE otherwise
    */
    int initEGL(int width, int height);

    /*!
     \brief Loads a tgaImage from file into the TGAData structure

     \param image Handle to the TGA-structure to be created
     \param data  Returns pointer to the actual image data
     \param filename Name of the tga file to load
     \return int Returns TGA_OK on success and the error code otherwise
    */
    int loadTgaImage(TGA **image, TGAData *data, const char *filename);

    /*!
     \brief Checks the EGL error flag is set and prints an error message

     This function is mainly called by the debug macro EGL_CHECK when
     compiled with _DEBUG flag set. It will check if the previous
     call to an EGL-function was successful and if not print a error
     message with statement, filename and line number.

     \param stmt
     \param fname
     \param line
    */
    void checkEGLError(const char* stmt, const char* fname, int line);

    int mWidth; /*!< Width of the scene */
    int mHeight; /*!< Height of the scene*/
    unsigned mUsedTexUnits; /*!< Bitfield of used texture units */

    GLuint mFboId[2] ; /*!< Handles to the two framebuffer objects*/
    TGAData mImgData; /*!< Handle to the data from the loaded tga image*/

};

#endif // OGLES_H
