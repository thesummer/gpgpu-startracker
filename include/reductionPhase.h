#ifndef REDUCTIONPHASE_H
#define REDUCTIONPHASE_H

#include "tga.h"
#include "phase.h"

/*!
    \ingroup reduction
    @{
*/

/*!
 \brief Contains all handles and functions for the reduction phase

 This class takes the result of the \ref labeling as input. It then
 creates an intermediate texture which only contains the root pixel of each
 label. Then a reduction operation is performed first horizontally then
 vertically. In other words a (2-dimensional) list of all root pixels is
 created in the top-left corner of the resulting texture.
 A more elaborate explanation of the GLSL-algorithm can be found in \ref reduction..

 NOTE: The handles for shader-, texture-, fbo-objects etc. are public at the moment
 for easy debugging purposes. Probably would be a good idea to move them to
 private later.

*/
class ReductionPhase : public Phase
{
public:

    // Vertex and fragment shader files
    const char * mVertFilename; /*!< Path to the vertex shader file */
    const char * mFragFilename; /*!< Path to the fragment shader file */

    // Handle to a program object
    GLuint mProgramObject; /*!< Handle to the program object */

    int mWidth; /*!< Width of the scene*/
    int mHeight; /*!< Height of the scene*/

    // Attribute locations
    GLint  mPositionLoc; /*!< Handle for the attribute a_position*/
    GLint  mTexCoordLoc; /*!< Handle for the attribute a_texCoord */

    // Vertices
    GLfloat  mVertices[20]; /*!< Vertex and texture coordinates for the plain quad*/
    GLushort mIndices[6]; /*!< Indices for the quad scene*/

    // Uniform locations
    GLint  u_texDimLoc; /*!< Handle to the uniform u_texDimensions */
    GLint  u_passLoc; /*!< Handle to the uniform u_pass*/
    GLint  u_stageLoc; /*!< Handle to the uniform u_stage */
    GLint  u_directionLoc; /*!< Handle to the uniform u_direction*/

    // Uniform values
    GLint u_pass; /*!< Number of the current iteration */

//  Check is all members necessary
    // Sampler locations
    GLint s_reductionLoc; /*!< Handle to the sampler holding intermediate results*/
    GLint s_valuesLoc; /*!< Handle to sampler holding the label information */

    /// TODO: tga somewhere else?
    TGA    *mTgaImage; /*!< Handle for the loaded tga image */
    TGAData *mTgaData; /*!< Buffer holding the loaded image data */

// End Check
    // Texture to attach to the frambuffers
    GLuint mTexPiPoId[2]; /*!< Handle to the two textures which are used for ping-pong-method*/
    GLuint mTexLabelId; /*!< Handle to the texture which holds the labeling results*/
    GLuint mTexRootId; /*!< Handle to the texture which holds only the root pixels of the labels*/

    GLuint mFboId[2]; /*!< Handles to the two FBOs for ping-pong method*/
    GLint  mTextureUnits[4]; /*!< Handles to the texture units for the above textures*/

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
    ReductionPhase(int width = 0, int height = 0);

    /*!
     \brief Destructor

    */
    virtual ~ReductionPhase();

    /*!
     \brief Initializes the scene and all necessary objects (except for the input textures)

     Assumes that the texture and texture unit which hold the result of the
     labeling phase and for the root texture are set by the parent.
     I.e. that \ref updateTextures is called before \ref run is called.

     \param fbos[] The 2 framebuffers necessary for rendering
     \param bfUsedTextures The bitfield to determine which texture units are already used
     \return GLint Returns GL_TRUE on success
    */
    GLint init(GLuint fbos[], GLuint &bfUsedTextures);

    /*!
     \brief Initializes the scene and all necessary objects

     This init-function is used if the phase should be run without the need
     of the results of any previous stages or input. The input image with the
     labeling data is copied from \ref mTgaData

     \param fbos[] The 2 framebuffers necessary for rendering
     \param bfUsedTextures The bitfield to determine which texture units are already used
     \return GLint Returns GL_TRUE on success
    */
    GLint initIndependent(GLuint fbos[], GLuint &bfUsedTextures);

    /*!
     \brief Sets up the Viewport and the quad scene

    */
    void setupGeometry();

    /*!
     \brief Initializes the texure handles and units to previously allocated resources

     It is assumed that a previous phase (i.e. the \ref labeling) has already
     allocated VRAM for textures and texture units which can be reused in this
     phase as well. Only additional textures have to be allocated.

     \param labelTex  TextureId holding the handle to the texture with the labeling results
     \param labelTexUnit Holds the Id of the texture unit corresponding to labelTex
     \param freeTex   TextureId which holds a texture which can be safely overwritten
     \param freeTexUnit Holds the Id of the texture unit corresponding to freeTex
    */
    void updateTextures(GLuint labelTex, GLint labelTexUnit, GLuint freeTex, GLint freeTexUnit);

    /*!
     \brief Runs the reduction algorithm

     TODO: Cleanup the code and comments
     TODO: Write short explanation here?
     TODO: refere to the general explanation and GLSL docu

     \return double The time (in ms) the computation took
    */
    virtual double run();

    /*!
     \brief Return the handle to the texture holding the result of the phase

     The last texture is the texture which holds the result of the last iteration
     of the reduction algorithm, i.e. a texture which holds a 2D list of all
     root pixels in its top-left corner

     \return GLuint
    */
    GLint getLastTexture();

    /*!
     \brief Return the index of the texture unit the result texture is assinged to

     \return GLint
    */
    GLint getLastTexUnit();

    /*!
     \brief Return the handle to the texture which holds no important data and can be reused

     The free texture holds data from the iteration before the last one which
     served as input of the last reduction iteration. It is old data, therefore
     the texture and texture unit can be reused by subsequent phases.

     \return GLuint
    */
    GLint getFreeTexture();

    /*!
     \brief Return the index of the texture unit the free texture is assinged to

     \return GLint
    */
    GLint getFreeTexUnit();

    /*!
     \brief Return the handle to the 2nd texture which holds no important data and can be reused

     The free texture holds data from the iteration before the last one which
     served as input of the last reduction iteration. It is old data, therefore
     the texture and texture unit can be reused by subsequent phases.

     \return GLuint
    */
    GLint getFreeTexture2();

    /*!
     \brief Return the index of the 2nd texture unit the free texture is assinged to

     \return GLint
    */
    GLint getFreeTexUnit2();

private:
    /*!
     \brief Funtion doing the reduction stage

     \param length Length of the row or column to be reduced
    */
    void reduce(int length);
};

#endif // REDUCTIONPHASE_H
