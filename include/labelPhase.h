#ifndef LABELPHASE_H
#define LABELPHASE_H

#include "CImg.h"
using namespace cimg_library;

#include <stdio.h>

#include "phase.h"
#include "getTime.h"

/*!
    \ingroup labeling
    @{
*/

/*!
 \brief Contains all handles and functions for the labeling phase

 This class is responsible for the thresholding of the initial image and the
 labeling of the remaining spots.
 A more elaborate explanation of the GLSL-algorithm can be found in \ref labeling.

 NOTE: The handles for shader-, texture-, fbo-objects etc. are public at the moment
 for easy debugging purposes. Probably would be a good idea to move them to
 private later.



*/
class LabelPhase: public Phase
{
public:
    // Vertex and fragment shader files
    const char * mVertFilename; /*!< Path to the vertex shader file */
    const char * mFragFilename; /*!< Path to the fragment shader file */

    // Handle to a program object
    GLuint mProgramObject; /*!< Handle to the program object */

    int mWidth; /*!< Width of the scene*/
    int mHeight; /*!< Height of the scene */

    // Attribute locations
    GLint  mPositionLoc; /*!< Handle for the attribute a_position*/
    GLint  mTexCoordLoc; /*!< Handle for the attribute a_texCoord */

    // Vertices
    GLfloat mVertices[20]; /*!< Vertex and texture coordinates for the plain quad*/
    GLushort mIndices[6]; /*!< Indices for the quad scene*/

    // Uniform locations
    GLint  u_texDimLoc; /*!< Handle to the uniform u_texDimensions */
    GLint  u_thresholdLoc; /*!< Handle to the uniform u_threshold */
    GLint  u_passLoc; /*!< Handle to the uniform u_pass*/
    GLint  u_factorLoc; /*!< Handle to the uniform u_factor*/

    // Uniform values
    float u_threshold; /*!< threshold value for the thresholding operation*/
    GLint u_pass; /*!< Number of the current iteration */
    GLint u_factor; /*!< determines if forward mask (1.0) or backward mask (-1.0)*/

    // Sampler location
    GLint mSamplerLoc; /*!< Handle to the sampler s_texture */

    // Texture handle
    /// TODO: image somewhere else?
    CImg<unsigned char> mImage;

    // Texture to attach to the frambuffers
    GLuint mTexOrigId; /*!< Handle to the texture which holdes the original image*/
    GLuint mTexPiPoId[2]; /*!< Handle to the two textures which are used for ping-pong-method*/
    GLuint mFboId[2]; /*!< Handles to the two FBOs for ping-pong method*/
    GLint  mTextureUnits[3]; /*!< Handles to the texture units for the above textures*/

    int mWrite; /*!< Holds the index of the FBO/texture which is written to */
    int mRead; /*!< Holds the index of the texture which is read from */

    /*!
     \brief Constructor

     Initializes the name of the vertex and fragment shader, vertex and
     texture coordinates of the vertices, the indices of the quad and
     the value for the threshold operation.

     \param width  Width of the scene
     \param height Height of the scene
    */
    LabelPhase(int width = 0, int height = 0);

    /*!
     \brief Destructor

    */
    virtual ~LabelPhase();

    /*!
     \brief Initializes the scene and all necessary objects (except for the texture holding the original image)

     Assumes that the texture and texture unit which holds the original image
     is set from the parent, before \ref run is called.

     \param fbos[] The 2 framebuffers necessary for rendering
     \param bfUsedTextures The bitfield to determine which texture units are already used
     \return GLint Returns GL_TRUE on success
    */
    GLint init(GLuint fbos[], GLuint &bfUsedTextures);

    /*!
     \brief Initializes the scene and all necessary objects

     This init-function is used if the phase should be run without the need
     of the results of any previous stages or input. The image data of the
     original image is copied from \ref mTgaData

     \param fbos[] The 2 framebuffers necessary for rendering
     \param bfUsedTextures The bitfield to determine which texture units are already used
     \return GLint Returns GL_TRUE on success
    */
    GLint initIndependent(GLuint fbos[], GLuint &bfUsedTextures);

    void updateOrigTexture();

    /*!
     \brief Return the handle to the texture holding the original image

     \return GLuint
    */
    GLuint getOrigTexture();

    /*!
     \brief Return the index of the texture unit the original texture is assinged to

     \return GLint
    */
    GLint getOrigTexUnit();


    /*!
     \brief Return the handle to the texture holding the result of the phase

     The last texture is the texture which holds the result of the last iteration
     of the labeling algorithm, i.e. it holds all identified spots with their
     assigned labels.

     \return GLuint
    */
    GLuint getLastTexture();

    /*!
     \brief Return the index of the texture unit the result texture is assinged to

     \return GLint
    */
    GLint getLastTexUnit();

    /*!
     \brief Return the handle to the texture which holds no important data and can be reused

     The free texture holds data from the iteration before the last one which
     served as input of the last labeling iteration. It is old data, therefore
     the texture and texture unit can be reused by subsequent phases.

     \return GLuint
    */
    GLuint getFreeTexture();

    /*!
     \brief Return the index of the texture unit the free texture is assinged to

     \return GLint
    */
    GLint getFreeTexUnit();

    /*!
     \brief Sets up the Viewport and the quad scene

    */
    void setupGeometry();

    /*!
     \brief Runs the labeling algorithm

     TODO: Cleanup the code and comments
     TODO: Write short explanation here?
     TODO: refere to the general explanation and GLSL docu

     \return double The time (in ms) the computation took
    */
    virtual double run();
};

/*!
    @}
*/

#endif // LABELPHASE_H
