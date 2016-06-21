#ifndef STATSPHASE_H
#define STATSPHASE_H

#include "CImg.h"
using namespace cimg_library;
#include "phase.h"
#include <stdio.h>


/*!
    \ingroup stats
    @{
*/

/*!
 \brief Contains all handles and functions for the statistics computation of the star spots

 This class takes the result of the \ref labeling as input. It then computes
 the following statistics for each star spot:

    - Number of pixel
    - Weighted sum in x and y direction

  The results are added to the results of the \ref reduction.

*/
class StatsPhase: public Phase
{
public:
    const char * mVertFilename; /*!< Filename for the vertexShader, common for all phases */
    /*!
     \brief Struct which holds all handles for the filling stage

    */
    struct
    {
        std::string filename; /*!< Filename of the fragment shader */
        GLuint program; /*!< Handle to the program object */
        // Sampler location
        GLint s_labelLoc; /*!< Handle holding the texture with the results from \ref labeling */

        // Uniform locations
        GLint u_texDimLoc; /*!< Handle to the uniform u_texDimensions */
        GLint u_passLoc; /*!< Handle to the uniform u_pass*/
        GLint u_factorLoc; /*!< Handle to the uniform u_factor */

        // Attribute locations
        GLint  positionLoc; /*!< Handle for the attribute a_position*/
        GLint  texCoordLoc; /*!< Handle for the attribute a_texCoord */

    } mProgFill;

    /*!
     \brief Struct which holds all handles for the counting stage

    */
    struct
    {
        std::string filename; /*!< Filename of the fragment shader */
        GLuint program; /*!< Handle to the program object */
        // Sampler locations
        GLint s_labelLoc; /*!< Handle holding the texture with the results from \ref labeling */
        GLint s_fillLoc; /*!< Handle holding the texture with the results of the filling stage */
        GLint s_resultLoc; /*!< Handle holding the texture with the current intermediate results of the counting stage */
        GLint s_origLoc; /*!< Handle holding the texture with the original image */
        // Uniform locations
        GLint u_texDimLoc; /*!< Handle to the uniform u_texDimensions */
        GLint u_passLoc; /*!< Handle to the uniform u_pass*/
        GLint u_stageLoc; /*!< Handle to the uniform u_stage*/
        GLint u_savingOffsetLoc; /*!< Handle to the uniform u_savingOffset. Holds the column from where to write the results*/
        GLint u_factorLoc; /*!< Handle to the uniform u_factor */

        // Attribute locations
        GLint  positionLoc; /*!< Handle for the attribute a_position*/
        GLint  texCoordLoc; /*!< Handle for the attribute a_texCoord */
    } mProgCount;

    /*!
     \brief Struct which holds all handles for the centroiding stage

    */
    struct
    {
        std::string filename; /*!< Filename of the fragment shader */
        GLuint program; /*!< Handle to the program object */
        // Sampler locations
        GLint s_labelLoc; /*!< Handle holding the texture with the results from \ref labeling */
        GLint s_fillLoc; /*!< Handle holding the texture with the results of the filling stage */
        GLint s_resultLoc; /*!< Handle holding the texture with the current intermediate results of the centroiding stage */
        GLint s_origLoc; /*!< Handle holding the texture with the original image */
        // Uniform locations
        GLint u_texDimLoc; /*!< Handle to the uniform u_texDimensions */
        GLint u_passLoc; /*!< Handle to the uniform u_pass*/
        GLint u_stageLoc; /*!< Handle to the uniform u_stage*/
        GLint u_savingOffsetLoc; /*!< Handle to the uniform u_savingOffset. Holds the column from where to write the results*/
        GLint u_factorLoc; /*!< Handle to the uniform u_factor */
        // Attribute locations
        GLint  positionLoc; /*!< Handle for the attribute a_position*/
        GLint  texCoordLoc; /*!< Handle for the attribute a_texCoord */
    } mProgCentroid;

    int mWidth; /*!< Width of the scene*/
    int mHeight; /*!< Height of the scene*/

    // Vertices
    GLfloat  mVertices[20]; /*!< Vertex and texture coordinates for the plain quad*/
    GLushort mIndices[6]; /*!< Indices for the quad scene*/

    // Texture handle
    CImg<unsigned char>  mImageLabel; /*!< Buffer holding the data with result of labeling phase */
    CImg<unsigned char>  mImageReduced; /*!< Buffer holding the data with result of reduction phase*/
    CImg<unsigned char>  mImageOrig; /*!< Buffer holding the data of the original image*/

    // Texture to attach to the frambuffers
    GLuint mTexOrigId; /*!< Handle to the texture with the original image */
    GLuint mTexLabelId; /*!< Handle to the texture with the labeling results*/
    GLuint mTexReducedId; /*!< Handle to the texture with the reduction results*/
    GLuint mTexFillId; /*!< Handle to the texture with results from filling stage*/
    GLuint mTexPiPoId[2]; /*!< Handle to the two textures which are used for ping-pong-method*/
    GLuint mFboId[2]; /*!< Handles to the two FBOs for ping-pong method*/
    GLint  mTextureUnits[6]; /*!< Handles to the texture units for the above textures*/

    int mWrite; /*!< Holds the index of the FBO/texture which is written to */
    int mRead; /*!< Holds the index of the texture which is read from */

    unsigned mStatsAreaWidth;  /*!< Width of the area extracted at the end of the statistics computation */
    unsigned mStatsAreaHeight; /*!< Height of the area extracted at the end of the statistics computation */

    unsigned mNumFillIterations;  /*!< Sets the number of iteration in the filling stage (default is 2) */

    /*!
     \brief Constructor

     Initializes the name of the vertex and fragment shader, vertex and
     texture coordinates of the vertices, the indices of the quad and
     the value for the threshold operation.

     \param width  Width of the scene
     \param height Height of the scene
    */
    StatsPhase(int width = 0, int height = 0);

    /*!
     \brief Destructor

    */
    virtual ~StatsPhase();

    /*!
    \brief Initializes the scene and all necessary objects (except for the input textures)

    Assumes that the texture and texture unit which hold the original image,
    the result of the labeling phase the result of the reduction phase
    and for the root texture are set by the parent.
    I.e. that \ref updateTextures is called before \ref run is called.

    \param fbos[] The 2 framebuffers necessary for rendering
    \param bfUsedTextures The bitfield to determine which texture units are already used
    \return GLint Returns GL_TRUE on success
   */
    GLint init(GLuint fbos[], GLuint &bfUsedTextures);

    /*!
     \brief Initializes the scene and all necessary objects

     This init-function is used if the phase should be run without the need
     of the results of any previous stages as input. The input images with the
     labeling data are copied from the respective mTga* buffers

     \param fbos[] The 2 framebuffers necessary for rendering
     \param bfUsedTextures The bitfield to determine which texture units are already used
     \return GLint Returns GL_TRUE on success
    */
    GLint initIndependent(GLuint fbos[], GLuint &bfUsedTextures);

    /*!
     \brief Hand over the results of previous phases for further processing

     \param origTex      Texture holding the original image
     \param origTexUnit  Corresponding texture unit of \ref origTex
     \param labelTex     Texture holding the results of the labeling phase
     \param labelTexUnit Corresponding texture unit of \ref labelTex
     \param reducedTex   Texture holding the results of the reduction phase
     \param reducedTexUnit Corresponding texture unit of \ref reducedTex
     \param freeTex      Texture which is already allocated and free to use
     \param freeTexUnit  Corresponding texture unit of \ref freeTex
     \param freeTex2     Texture which is already allocated and free to use
     \param freeTexUnit2 Corresponding texture unit of \ref freeTex2
    */
    void updateTextures(GLuint origTex   , GLint origTexUnit,
                        GLuint labelTex  , GLint labelTexUnit,
                        GLuint reducedTex, GLint reducedTexUnit,
                        GLuint freeTex   , GLint freeTexUnit,
                        GLuint freeTex2   , GLint freeTexUnit2);

    /*!
     \brief Sets up the Viewport and the quad scene

    */
    void setupGeometry();

    /*!
     \brief Computes the statistics for each star spot

     Internally calls \ref fillStage, \ref countStage and
     \ref centroidingStage for each of the 4 directions and sums the
     results. The results are written into the same texture which already
     holds the results of the reduction phase and with the same layout
     but with an offset in x-direction

     TODO: Cleanup the code and comments
     TODO: Write short explanation here?
     TODO: refere to the general explanation and GLSL docu

     \return double The time (in ms) the computation took for the computation*/
    virtual double run();

    virtual void releaseGlResources();

private:
    /*!
     \brief Function taking care of the execution of the fill stage

     \param factorX X-direction of the filling process
     \param factorY Y-direction of the filling process
    */
    void fillStage(float factorX, float factorY);

    /*!
     \brief Function taking care of the execution of the counting stage

     \param factorX X-direction of the counting process
     \param factorY Y-direction of the counting process
     \param offset  Starting column to write the results into
    */
    void countStage(float factorX, float factorY, int offset);

    /*!
     \brief Function taking care of the execution of the centroiding stage

     \param factorX  X-direction of the centroiding process
     \param factorY  Y-direction of the centroiding process
     \param coordinate X- or Y-coordinate
     \param offset  Starting column to write the results into
    */
    void centroidingStage(float factorX, float factorY, int coordinate, int offset);
    void debugImage(const char *text, const char *filename);
};

/*!
    @}
*/


#endif // STATSPHASE_H
