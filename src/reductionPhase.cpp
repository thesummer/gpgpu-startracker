#include <iostream>
using std::cerr;
using std::endl;

#define HORIZONTAL 0
#define VERTICAL   1

#define TEX_LABEL  0
#define TEX_ROOT   1
#define TEX_PIPO   2

#define MODE_RUNNING_SUM     0
#define MODE_BINARY_SEARCH   1
#define MODE_ROOT_INIT       2


#include "reductionPhase.h"
#include "getTime.h"

ReductionPhase::ReductionPhase(int width, int height)
    :mVertFilename("vertShader.glsl"), mFragFilename("fragReductionShader.glsl"),
      mWidth(width), mHeight(height),
      mVertices {-1.0f, -1.0f, 0.0f,  // Position 0
                  0.0f,  0.0f,        // TexCoord 0
                 -1.0f,  1.0f, 0.0f,  // Position 1
                  0.0f,  1.0f,        // TexCoord 1
                  1.0f,  1.0f, 0.0f,  // Position 2
                  1.0f,  1.0f,        // TexCoord 2
                  1.0f, -1.0f, 0.0f,  // Position 3
                  1.0f,  0.0f         // TexCoord 3
                },
      mIndices { 0, 1, 2, 0, 2, 3 }, u_debug(0)

{
}

ReductionPhase::~ReductionPhase()
{
    GL_CHECK( glDeleteProgram(mProgramObject) );
    GL_CHECK( glDeleteTextures(1, &mTexLabelId) );
    GL_CHECK( glDeleteTextures(1, &mTexRootId) );
    GL_CHECK( glDeleteTextures(2, mTexPiPoId) );
}

GLint ReductionPhase::init(GLuint fbos[], GLuint &bfUsedTextures)
{
    // Save the handles to the 2 framebuffers
    mFboId[0] = fbos[0];
    mFboId[1] = fbos[1];

    // Initialize all OpenGL structures necessary for the
    // labeling phase here
    mRead  = 0;
    mWrite = 1;

    // Load the shaders and get a linked program object
    mProgramObject = loadProgramFromFile( mVertFilename, mFragFilename);
    if (mProgramObject == 0)
    {
        cerr << "Failed to generate Program object for reduction phase" << endl;
        return GL_FALSE;
    }

     // Get the attribute locations
     mPositionLoc = glGetAttribLocation ( mProgramObject, "a_position" );
     mTexCoordLoc = glGetAttribLocation ( mProgramObject, "a_texCoord" );

     // Get the sampler locations
     s_reductionLoc  = glGetUniformLocation ( mProgramObject, "s_texture" );
     s_valuesLoc     = glGetUniformLocation ( mProgramObject, "s_values" );
     u_texDimLoc     = glGetUniformLocation ( mProgramObject, "u_texDimensions" );
     u_passLoc       = glGetUniformLocation ( mProgramObject, "u_pass" );
     u_debugLoc      = glGetUniformLocation ( mProgramObject, "u_debug" );
     u_stageLoc      = glGetUniformLocation ( mProgramObject, "u_stage" );
     u_directionLoc  = glGetUniformLocation ( mProgramObject, "u_direction" );

     // 3. and 4. texture for ping-pong
     for(int j=0; j<2; ++j)
     {
         int i = 0;
         while( (1<<i) & bfUsedTextures) ++i;

         GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
         mTexPiPoId[j]  = createSimpleTexture2D(mWidth, mHeight);
         bfUsedTextures |= (1<<i);
         mTextureUnits[TEX_PIPO+j] = i;
         GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexPiPoId[j]) );
     }

     GL_CHECK( glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f ) );

     return GL_TRUE;
}


GLint ReductionPhase::initIndependent(GLuint fbos[], GLuint &bfUsedTextures)
{
    // Create numNewTextures texture for the frambuffer and
    // bind the textures to the corresponding fbo

    /*
      * Create 4 textures:
      *  1. Texture for the Label image (read only)
      *  2. Texture which only contains the root pixels from 1.
      *  3.+4. 2 textures for the ping-pong of the algorithm
      */

    // 1. texture for Label image (read from tga-file)
    int i = 0;
    while( (1<<i) & bfUsedTextures) ++i;

    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexLabelId  = createSimpleTexture2D(mWidth, mHeight, mTgaData->img_data);
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_LABEL] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexLabelId) );

    // 2. texture for root pixels
    i = 0;
    while( (1<<i) & bfUsedTextures) ++i;
    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );

    mTexRootId = createSimpleTexture2D(mWidth, mHeight);
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_ROOT] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexRootId) );

    // 3. and 4. texture for ping-pong
    // and setup of the program object
    return init(fbos, bfUsedTextures);
}

void ReductionPhase::setupGeometry()
{
    // Set the viewport
    GL_CHECK( glViewport ( 0, 0, mWidth, mHeight ) );
    // Clear the color buffer
    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );
    // Load the vertex position
    GL_CHECK( glVertexAttribPointer ( mPositionLoc, 3, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), mVertices ) );
    // Load the texture coordinates
    GL_CHECK( glVertexAttribPointer ( mTexCoordLoc, 2, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), &mVertices[3] ) );
    GL_CHECK( glEnableVertexAttribArray ( mPositionLoc ) );
    GL_CHECK( glEnableVertexAttribArray ( mTexCoordLoc ) );
}

void ReductionPhase::updateTextures(GLuint labelTex, GLint labelTexUnit, GLuint freeTex, GLint freeTexUnit)
{
    // 1. texture for label image
    mTexLabelId = labelTex;
    mTextureUnits[TEX_LABEL] = labelTexUnit;

    // 2. Texture for root pixels
    mTexRootId = freeTex;
    mTextureUnits[TEX_ROOT] = freeTexUnit;
}

double ReductionPhase::run()
{
    /*
     *  5 stages:
     *      1. Copy mTexLabel into mTexRoot, but keep only root pixels (mode ROOT_INIT)
     *      2. Use the PiPo-textures to reduce mTexRoot horizontally
     *      3. Use the result of 2. as new mTexRoot (switch mTexRootId with last mTexPiPoId)
     *      4. Use the PiPo-textures to reduce mTexRoot vertically
     *TODO:(5. Render result into a smaller texture)
     *
     *      Result: A texture containing a compact list of all available labels
     */

    double startTime, endTime;
    startTime = getRealTime();

    // Use the program object
    GL_CHECK( glUseProgram ( mProgramObject ) );
    // Image dimensions do not change
    GL_CHECK( glUniform2f ( u_texDimLoc, mWidth, mHeight) );

    ///---------- 1. GENERATE ROOT-TEXTURE --------------------

    // Set the mode to ROOT_INIT
    GL_CHECK( glUniform1i ( u_stageLoc, MODE_ROOT_INIT ) );
    // Set the read only texture
    GL_CHECK( glUniform1i ( s_valuesLoc, mTextureUnits[TEX_LABEL] ) );
    // Just bind any texture (not used in this stage)
    GL_CHECK( glUniform1i ( s_reductionLoc, mTextureUnits[TEX_PIPO] ) );
    // Bind a frambuffer
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[0]) );
    // Attach mTexRoot to framebuffer
    GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexRootId, 0) );
    CHECK_FBO();

    // Draw scene
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );

#ifdef _DEBUG
    // Make the BYTE array, factor of 3 because it's RGBA.
    GLubyte* pixels = new GLubyte[4*mWidth*mHeight];

    GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
    printf("Pixels after root pass\n");
////    printLabels(mWidth, mHeight, pixels);
    char filename[50];
    sprintf(filename, "outRoot.tga");
    writeTgaImage(mWidth, mHeight, filename, pixels);
    delete [] pixels;
#endif



    ///---------- 2. REDUCE HORIZONTALLY --------------------


    // Attach the two PiPo-textures to the 2 fbos
    for(int i=0; i<2; i++)
    {
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[i]) );
        GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexPiPoId[i], 0) );
        CHECK_FBO();
    }

    // Bind the correct framebuffer
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    // Bind the mTexRoot to the s_values sampler for the complete run
    GL_CHECK( glUniform1i ( s_valuesLoc, mTextureUnits[TEX_ROOT] ) );

    GL_CHECK( glUniform1i ( u_directionLoc, HORIZONTAL) );


    reduce(mWidth);

#ifdef _DEBUG
//    // Make the BYTE array, factor of 3 because it's RGBA.
//    // GLubyte* pixels = new GLubyte[4*mWidth*mHeight];
    pixels = new GLubyte[4*mWidth*mHeight];
    GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
    printf("Pixels after horizontal pass\n");
////    printLabels(mWidth, mHeight, pixels);
//    char filename[50];
    sprintf(filename, "outHori.tga");
    writeTgaImage(mWidth, mHeight, filename, pixels);
    delete [] pixels;
#endif

    ///---------- 3. SWITCH RESULT WITH TEX_ROOT --------------------

    // Swap texture Ids and corresponding Texture Units
    std::swap(mTexRootId, mTexPiPoId[mRead]);
    std::swap(mTextureUnits[TEX_ROOT], mTextureUnits[TEX_PIPO+mRead]);
    // Update s_values Sampler with changed texture unit
    GL_CHECK( glUniform1i ( s_valuesLoc, mTextureUnits[TEX_ROOT] ) );

    // Attach the old mTexRoot to framebuffer
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mRead]) );
    GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexPiPoId[mRead], 0) );
    CHECK_FBO();

    ///---------- 4. REDUCE VERTICALLY --------------------


    GL_CHECK( glUniform1i ( u_directionLoc, VERTICAL) );
//    u_debug = 1;
    reduce(mHeight);

#ifdef _DEBUG
    // Make the BYTE array, factor of 3 because it's RGBA.
    // GLubyte* pixels = new GLubyte[4*mWidth*mHeight];
    pixels = new GLubyte[4*mWidth*mHeight];
    GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
    printf("Pixels after vertical pass\n");
//    printLabels(mWidth, mHeight, pixels);
//    char filename[50];
    sprintf(filename, "out.tga");
    writeTgaImage(mWidth, mHeight, filename, pixels);
    delete [] pixels;
#endif

    ///TODO: ---------- 5. RENDER RESULT INTO SMALL TEXTURE --------------------
    endTime = getRealTime();

    return (endTime-startTime)*1000;
}

void ReductionPhase::reduce(int length)
{
#ifdef _DEBUG
    // Make the BYTE array, factor of 3 because it's RGBA.
    GLubyte* pixels = new GLubyte[4*mWidth*mHeight];
#endif

    // First part is RUNNING_SUM
    GL_CHECK( glUniform1i ( u_stageLoc, MODE_RUNNING_SUM ) );

    // Do the runs

    for (int i = 0; i < logBase2(length); ++i)
    {
        u_pass = i;

        // Bind the FBO to write to
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
        // Set the sampler s_texture unit
        GL_CHECK( glUniform1i ( s_reductionLoc, mTextureUnits[TEX_PIPO+mRead] ) );

        // Set the pass index
        GL_CHECK( glUniform1i ( u_passLoc,  u_pass) );
        GL_CHECK( glUniform1i ( u_debugLoc, u_debug) );
        // Draw scene
        GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );

#ifdef _DEBUG
        GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
        printf("Pixels after pass %d:\n", i);
//        printLabels(mWidth, mHeight, pixels);
//        char filename[50];
//        sprintf(filename, "out%03d.tga", i);
//        writeRawTgaImage(mWidth, mHeight, filename, pixels);
#endif

        // Switch read and write texture
        std::swap(mRead, mWrite);
    }

    // Second part is BINARY_SEARCH
    GL_CHECK( glUniform1i ( u_stageLoc, MODE_BINARY_SEARCH) );

    for (int i = logBase2(length)-1; i >= 0; --i)
    {
        u_pass = i;

        // Bind the FBO to write to
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
        // Set the sampler texture unit to 0
        GL_CHECK( glUniform1i ( s_reductionLoc, mTextureUnits[TEX_PIPO+mRead] ) );
        // Set the pass index
        GL_CHECK( glUniform1i ( u_passLoc,  u_pass) );
        GL_CHECK( glUniform1i ( u_debugLoc, u_debug) );
        // Draw scene
        GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );

#ifdef _DEBUG
        GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
        printf("Pixels after pass %d:\n", i);
//        printLabels(mWidth, mHeight, pixels);
//        char filename[50];
//        sprintf(filename, "out%03d.tga", i);
//        writeRawTgaImage(mWidth, mHeight, filename, pixels);
#endif

        // Switch read and write texture
        std::swap(mRead, mWrite);
    }

#ifdef _DEBUG
    delete [] pixels;
#endif
}
