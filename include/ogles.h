#ifndef OGLES_H
#define OGLES_H
#include "CImg.h"
using namespace cimg_library;
#include "phase.h"
#include "labelPhase.h"
#include "reductionPhase.h"
#include "statsPhase.h"

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include<string>

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

     Minimal constructor Initialization has to be done manually afterwards

     \param width
     \param height
    */
    Ogles(int width = 0, int height = 0);

    /*!
     \brief Destructor

    */
    virtual ~Ogles();

    /*!
     \brief Constructor which creates object and loads inital file

     It creates and initializes the EGLContext the OpenGL-structures
     and the Phase-Objects. The file \param imageFilename is copied to a
     texture and will be used as a starting point for the computation.

     \param imageFilename Path to the file which is to be evaluated
    */
    Ogles(std::string imageFilename);

    /*!
     \brief Function which does all the computation

     Runs through all phases and makes sure each phase receives
     the necessary information of previous phases.

    */
    void extractSpots();

    void loadImageFromFile(std::string imageFilename, bool updateTexture = true);

    bool isInitialized();

private:
    void initialize();

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
     \brief Checks if the EGL error flag is set and prints an error message

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
    bool mIsInitialized;

    GLuint mFboId[2] ; /*!< Handles to the two framebuffer objects*/
};

#endif // OGLES_H
