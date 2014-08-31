#include <iostream>
using std::cerr;
using std::endl;

#define TEX_ORIG    0
#define TEX_REDUCED 1
#define TEX_LABEL   2
#define TEX_FILL    3
#define TEX_PIPO    4

#define STAGE_FILL          0
#define STAGE_COUNT         1
#define STAGE_CENTROIDING   2
#define STAGE_BLEND         3
#define STAGE_SAVE          4


#define OFFSET 3.0
#define SIZE 1

#include "statsPhase.h"

StatsPhase::StatsPhase(int width, int height)
    : mVertFilename("../glsl/quad.vert"), mFragFilename("../glsl/statsPhase.frag"),
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
      mIndices { 0, 1, 2, 0, 2, 3 }
{
}

StatsPhase::~StatsPhase()
{
    GL_CHECK( glDeleteProgram(mProgramObject) );
//    GL_CHECK( glDeleteTextures(1, &mTexOrigId) );
    GL_CHECK( glDeleteTextures(2, mTexPiPoId) );
}

GLint StatsPhase::init(GLuint fbos[2], GLuint &bfUsedTextures)
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
        cerr << "Failed to generate Program object for label phase" << endl;
    }


    GL_CHECK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) );
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, 0) );
    // Get the attribute locations
    mPositionLoc = glGetAttribLocation ( mProgramObject, "a_position" );
    mTexCoordLoc = glGetAttribLocation ( mProgramObject, "a_texCoord" );

    // Get the sampler locations
    s_fillLoc         = glGetUniformLocation( mProgramObject,  "s_fill" );
    s_labelLoc        = glGetUniformLocation( mProgramObject,  "s_label" );
    s_resultLoc       = glGetUniformLocation( mProgramObject,  "s_result" );
    s_originalLoc     = glGetUniformLocation( mProgramObject,  "s_original" );

    // Get uniform locations
    u_texDimLoc       = glGetUniformLocation ( mProgramObject, "u_texDimensions" );
    u_passLoc         = glGetUniformLocation ( mProgramObject, "u_pass" );
    u_stageLoc        = glGetUniformLocation ( mProgramObject, "u_stage" );
    u_savingOffsetLoc = glGetUniformLocation ( mProgramObject, "u_savingOffset" );
    u_factorLoc       = glGetUniformLocation ( mProgramObject, "u_factor" );

//    u_debugLoc      = glGetUniformLocation ( mProgramObject, "u_debug" );
//    u_factorLoc     = glGetUniformLocation ( mProgramObject, "u_factor" );

    // 2. and 3. texture for ping-pong
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

    int i = 0;
    while( (1<<i) & bfUsedTextures) ++i;

    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexFillId = createSimpleTexture2D(mWidth, mHeight);
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_FILL] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexFillId) );

    GL_CHECK( glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f ) );

    return GL_TRUE;
}

GLint StatsPhase::initIndependent(GLuint fbos[], GLuint &bfUsedTextures)
{
    int i = 0;
    while( (1<<i) & bfUsedTextures) ++i;

    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexLabelId = createSimpleTexture2D(mWidth, mHeight, mTgaLabel->img_data);
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_LABEL] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexLabelId) );

    i = 0;
    while( (1<<i) & bfUsedTextures) ++i;
    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexReducedId = createSimpleTexture2D(mWidth, mHeight, mTgaReduced->img_data);
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_REDUCED] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexReducedId) );

    // Setup 2 Textures for Ping-Pong and
    // the program object
    return init(fbos, bfUsedTextures);
}

//GLint StatsPhase::getLastTexture()
//{
//    return mTexPiPoId[mRead];
//}

//GLint StatsPhase::getLastTexUnit()
//{
//    return mTextureUnits[TEX_PIPO+mRead];
//}

//GLint StatsPhase::getFreeTexture()
//{
//    return mTexPiPoId[mWrite];
//}

//GLint StatsPhase::getFreeTexUnit()
//{
//    return mTextureUnits[TEX_PIPO+mWrite];
//}

void StatsPhase::setupGeometry()
{
    GL_CHECK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) );
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, 0) );
    // Set the viewport
    GL_CHECK( glViewport ( 0, 0, mWidth, mHeight ) );
    // Clear the color buffer
    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );
}

double StatsPhase::run()
{
    double startTime, endTime;

    startTime = getRealTime();

    ///////////// --------- GENERAL SETUP ---------- ////////////////////

    GL_CHECK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) );
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, 0) );
    // Load the texture coordinates
    GL_CHECK( glVertexAttribPointer ( mPositionLoc, 3, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), mVertices ) );
    GL_CHECK( glVertexAttribPointer ( mTexCoordLoc, 2, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), &mVertices[3] ) );
    // Load the vertex position
    GL_CHECK( glEnableVertexAttribArray ( mPositionLoc ) );
    GL_CHECK( glEnableVertexAttribArray ( mTexCoordLoc ) );

    // Setup OpenGL
    // Attach the two PiPo-textures to the 2 fbos
    for(int i=0; i<2; i++)
    {
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[i]) );
        GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexPiPoId[i], 0) );
        CHECK_FBO();
    }

    // Use the program object
    GL_CHECK( glUseProgram ( mProgramObject ) );

    // Set the uniforms
    GL_CHECK( glUniform2f ( u_texDimLoc, mWidth, mHeight) );

    // Do the runs
//    u_factor = -1.0;


#ifdef _DEBUG
    {
        // Make the BYTE array, factor of 3 because it's RGBA.
        printf("Pixels before run:\n");
//        printLabels(mWidth, mHeight, mTgaData->img_data);
        char filename[50];
        sprintf(filename, "out0%03d.tga", 0);
        writeTgaImage(mWidth, mHeight, filename, mTgaLabel->img_data);
    }
#endif
    float factorX = 1.0, factorY = 1.0;

    fillStage(factorX, factorY);
    countStage();
    saveStage(factorX, factorX, OFFSET);

    factorX = -1.0;
    fillStage(factorX, factorY);
    countStage();
    saveStage(factorX, factorY, OFFSET);

    factorY = -1.0;
    fillStage(factorX, factorY);
    countStage();
    saveStage(factorX, factorY, OFFSET);

    factorX = 1.0;
    fillStage(factorX, factorY);
    countStage();
    saveStage(factorX, factorY, OFFSET);

    GL_CHECK( glDisableVertexAttribArray ( mPositionLoc ) );
    GL_CHECK( glDisableVertexAttribArray ( mTexCoordLoc ) );

    endTime = getRealTime();

    return (endTime-startTime)*1000;
}

void StatsPhase::fillStage(float factorX, float factorY)
{
    GL_CHECK( glUniform1i ( u_stageLoc , STAGE_FILL) );
    // Bind the FBO to write to
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    // Set the sampler texture to use the texture containing the labels
    GL_CHECK( glUniform1i ( s_fillLoc, mTextureUnits[TEX_LABEL] ) );
    // Set the pass index
    GL_CHECK( glUniform1i ( u_passLoc,  0) );
    GL_CHECK( glUniform2f ( u_factorLoc, factorX, factorY ) );
    //    GL_CHECK( glUniform1f ( u_factorLoc, u_factor) );
    // Draw scene
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
    std::swap(mRead, mWrite);

#ifdef _DEBUG
    {
        // Make the BYTE array, factor of 3 because it's RGBA.
        GLubyte* pixels = new GLubyte[4*mWidth*mHeight];
        GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
        printf("Pixels after pass %d:\n", 0);
        printLabels(mWidth, mHeight, pixels);
        char filename[50];
        sprintf(filename, "outF%03d.tga", 0);
        writeTgaImage(mWidth, mHeight, filename, pixels);
        delete [] pixels;
    }
#endif

    for(int i=1; i<SIZE; ++i)
    {
        // Bind the FBO to write to
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
        // Set the sampler texture to use the texture containing the labels
        GL_CHECK( glUniform1i ( s_fillLoc, mTextureUnits[TEX_PIPO+mRead] ) );
        // Set the pass index
        GL_CHECK( glUniform1i ( u_passLoc,  i) );
        //    GL_CHECK( glUniform1f ( u_factorLoc, u_factor) );
        // Draw scene
        GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
        std::swap(mRead, mWrite);

#ifdef _DEBUG
        {
            // Make the BYTE array, factor of 3 because it's RGBA.
            GLubyte* pixels = new GLubyte[4*mWidth*mHeight];
            GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
            printf("Pixels after pass %d:\n", i);
            //            printLabels(mWidth, mHeight, pixels);
            char filename[50];
            sprintf(filename, "outF%03d.tga", i);
            writeTgaImage(mWidth, mHeight, filename, pixels);
            delete [] pixels;
        }
#endif

    }
}

void StatsPhase::countStage()
{
    GL_CHECK( glUniform1i ( u_stageLoc , STAGE_COUNT) );
    // Save the texture with the results of the filling and use a new texture for PIPO
    std::swap(mTexPiPoId[mRead], mTexFillId);
    std::swap(mTextureUnits[TEX_FILL], mTextureUnits[TEX_PIPO+mRead]);
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mRead]) );
    GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexPiPoId[mRead], 0) );
    CHECK_FBO();

    // Bind the different sampler2D
    // Texture with the filled spots from previous stage (read only)
    GL_CHECK( glUniform1i ( s_fillLoc,   mTextureUnits[TEX_FILL] ) );
    // Texture with the labels from last phase (read only)
    GL_CHECK( glUniform1i ( s_labelLoc,  mTextureUnits[TEX_LABEL] ) );

    // Start with -1 because that is the initalization pass
    for (int i=-1; i<4   ; ++i)
    {
        // Bind the FBO to write to
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
        // Set the pass index
        GL_CHECK( glUniform1i ( u_passLoc,  i) );
        // Set the sampler texture to use the texture containing the labels
        GL_CHECK( glUniform1i ( s_resultLoc, mTextureUnits[TEX_PIPO+mRead] ) );

        // Draw scene
        GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
        std::swap(mRead, mWrite);

#ifdef _DEBUG
        {
            // Make the BYTE array, factor of 3 because it's RGBA.
            GLubyte* pixels = new GLubyte[4*mWidth*mHeight];
            GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
            printf("Pixels after count %d:\n", i);
            printLabels(mWidth, mHeight, pixels);
            char filename[50];
            sprintf(filename, "outC%03d.tga", i);
            writeTgaImage(mWidth, mHeight, filename, pixels);
            delete [] pixels;
        }
#endif

    }
}

void StatsPhase::saveStage(float factorX, float factorY, float savingOffset)
{
    // Write the count result as a reduced table (with an offset so it won't interfere
    // with the table in texLabelId in the next step
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    GL_CHECK( glUniform1i ( u_stageLoc,  STAGE_SAVE ) );
    GL_CHECK( glUniform2f ( u_factorLoc, factorX, factorY ) );
    GL_CHECK( glUniform1f ( u_savingOffsetLoc, savingOffset) );
    GL_CHECK( glUniform1i ( s_labelLoc,  mTextureUnits[TEX_REDUCED] ) );
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
    std::swap(mRead, mWrite);

#ifdef _DEBUG
    {
        // Make the BYTE array, factor of 3 because it's RGBA.
        GLubyte* pixels = new GLubyte[4*mWidth*mHeight];
        GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
        printf("Pixels after save:\n");
        printLabels(mWidth, mHeight, pixels);
        char filename[50];
        sprintf(filename, "outS.tga");
        writeTgaImage(mWidth, mHeight, filename, pixels);
        delete [] pixels;
    }
#endif

    // Add/blend texture from previous step and mTexLabel together
    // This yields a texture which has the lookup table for root pixels in its first few
    // columns and the count values in the next few columns
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    GL_CHECK( glUniform1i ( u_stageLoc,  STAGE_BLEND ) );
    GL_CHECK( glUniform1i ( s_resultLoc, mTextureUnits[TEX_PIPO+mRead] ) );
    GL_CHECK( glUniform1i ( s_labelLoc,  mTextureUnits[TEX_REDUCED] ) );
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
    std::swap(mRead, mWrite);

#ifdef _DEBUG
    {
        // Make the BYTE array, factor of 3 because it's RGBA.
        GLubyte* pixels = new GLubyte[4*mWidth*mHeight];
        GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
        printf("Pixels after merge:\n");
        printLabels(mWidth, mHeight, pixels);
        char filename[50];
        sprintf(filename, "outM.tga");
        writeTgaImage(mWidth, mHeight, filename, pixels);
        delete [] pixels;
    }
#endif

    // Save the result from the previous step into mTexLabel (by switching the texture
    // objects and the corresponding texture unit)
    std::swap(mTexPiPoId[mRead], mTexReducedId);
    std::swap(mTextureUnits[TEX_REDUCED], mTextureUnits[TEX_PIPO+mRead]);
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mRead]) );
    GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexPiPoId[mRead], 0) );
    CHECK_FBO();
}

