#include "statsPhase.h"
#include <iostream>
using std::cout;
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

#define CENTROID_X_COORD   -1
#define CENTROID_Y_COORD   -2

#define OFFSET 10.0
#define OFFSET_Y 2
#define OFFSET_X 0
#define OFFSET_AREA ((size_t)OFFSET*sizeof(uint32_t))
#define OFFSET_LUMINANCE ((size_t)OFFSET*sizeof(uint32_t)+2)
#define OFFSET_SUM_X ((size_t)OFFSET*2*sizeof(uint32_t))
#define OFFSET_SUM_Y ((size_t)OFFSET*3*sizeof(uint32_t))

#include "getTime.h"

#define _DEBUG

StatsPhase::StatsPhase(int width, int height)
    : mVertFilename("../glsl/quad.vert"),
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
      mIndices { 0, 1, 2, 0, 2, 3 },
      mStatsAreaWidth(OFFSET*4),
      mStatsAreaHeight(height),
      mNumFillIterations(2)
{
    mProgFill.filename     = "../glsl/fillStage.frag";
    mProgCount.filename    = "../glsl/countStage.frag";
    mProgCentroid.filename = "../glsl/centroidStage.frag";
}

StatsPhase::~StatsPhase()
{
}

GLint StatsPhase::init(GLuint fbos[2], GLuint &bfUsedTextures)
{
    // Save the handles to the 2 framebuffersc
    mFboId[0] = fbos[0];
    mFboId[1] = fbos[1];

    // Initialize all OpenGL structures necessary for the
    // labeling phase here
    mRead  = 0;
    mWrite = 1;

    GL_CHECK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) );
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, 0) );

    // Setup the fillStage-program
    // Load the shaders and get a linked program object
    mProgFill.program = loadProgramFromFile( mVertFilename, mProgFill.filename);
    if (mProgFill.program == 0)
    {
        cerr << "Failed to generate Program object for fill stage of stats phase" << endl;
    }
    mProgFill.positionLoc = glGetAttribLocation ( mProgFill.program , "a_position" );
    mProgFill.texCoordLoc = glGetAttribLocation ( mProgFill.program , "a_texCoord" );
    GL_CHECK( glVertexAttribPointer ( mProgFill.positionLoc, 3, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), mVertices ) );
    GL_CHECK( glVertexAttribPointer ( mProgFill.texCoordLoc, 2, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), &mVertices[3] ) );

    mProgFill.s_labelLoc         = glGetUniformLocation( mProgFill.program , "s_label" );
    mProgFill.u_texDimLoc       = glGetUniformLocation ( mProgFill.program , "u_texDimensions" );
    mProgFill.u_passLoc         = glGetUniformLocation ( mProgFill.program , "u_pass" );
    mProgFill.u_factorLoc       = glGetUniformLocation ( mProgFill.program , "u_factor" );

    // Setup the centroiding stage-program
    mProgCentroid.program = loadProgramFromFile( mVertFilename, mProgCentroid.filename);
    if (mProgCentroid.program == 0)
    {
        cerr << "Failed to generate Program object for centroiding stage of stats phase" << endl;
    }
    mProgCentroid.positionLoc = glGetAttribLocation ( mProgCentroid.program , "a_position" );
    mProgCentroid.texCoordLoc = glGetAttribLocation ( mProgCentroid.program , "a_texCoord" );
    GL_CHECK( glVertexAttribPointer ( mProgCentroid.positionLoc, 3, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), mVertices ) );
    GL_CHECK( glVertexAttribPointer ( mProgCentroid.texCoordLoc, 2, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), &mVertices[3] ) );

    mProgCentroid.s_fillLoc   = glGetUniformLocation( mProgCentroid.program,  "s_fill" );
    mProgCentroid.s_labelLoc  = glGetUniformLocation( mProgCentroid.program,  "s_label" );
    mProgCentroid.s_resultLoc = glGetUniformLocation( mProgCentroid.program,  "s_result" );
    mProgCentroid.s_origLoc   = glGetUniformLocation( mProgCentroid.program,  "s_orig" );

    mProgCentroid.u_texDimLoc       = glGetUniformLocation ( mProgCentroid.program, "u_texDimensions" );
    mProgCentroid.u_passLoc         = glGetUniformLocation ( mProgCentroid.program, "u_pass" );
    mProgCentroid.u_stageLoc        = glGetUniformLocation ( mProgCentroid.program, "u_stage" );
    mProgCentroid.u_savingOffsetLoc = glGetUniformLocation ( mProgCentroid.program, "u_savingOffset" );
    mProgCentroid.u_factorLoc       = glGetUniformLocation ( mProgCentroid.program, "u_factor" );

    // Setup the count stage-progam
    mProgCount.program = loadProgramFromFile( mVertFilename, mProgCount.filename);
    if (mProgCount.program == 0)
    {
        cerr << "Failed to generate Program object for count stage of stats phase" << endl;
    }
    mProgCount.positionLoc = glGetAttribLocation ( mProgCount.program , "a_position" );
    mProgCount.texCoordLoc = glGetAttribLocation ( mProgCount.program , "a_texCoord" );
    GL_CHECK( glVertexAttribPointer ( mProgCount.positionLoc, 3, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), mVertices ) );
    GL_CHECK( glVertexAttribPointer ( mProgCount.texCoordLoc, 2, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), &mVertices[3] ) );

    mProgCount.s_fillLoc   = glGetUniformLocation( mProgCount.program,  "s_fill" );
    mProgCount.s_labelLoc  = glGetUniformLocation( mProgCount.program,  "s_label" );
    mProgCount.s_resultLoc = glGetUniformLocation( mProgCount.program,  "s_result" );
    mProgCount.s_origLoc   = glGetUniformLocation( mProgCentroid.program,  "s_orig" );

    mProgCount.u_texDimLoc       = glGetUniformLocation ( mProgCount.program, "u_texDimensions" );
    mProgCount.u_passLoc         = glGetUniformLocation ( mProgCount.program, "u_pass" );
    mProgCount.u_stageLoc        = glGetUniformLocation ( mProgCount.program, "u_stage" );
    mProgCount.u_savingOffsetLoc = glGetUniformLocation ( mProgCount.program, "u_savingOffset" );
    mProgCount.u_factorLoc       = glGetUniformLocation ( mProgCount.program, "u_factor" );

    // missing texture for ping-pong
    int i = 0;
    while( (1<<i) & bfUsedTextures) ++i;

    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexPiPoId[1]  = createSimpleTexture2D(mWidth, mHeight);
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_PIPO+1] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexPiPoId[1]) );

    return GL_TRUE;
}

GLint StatsPhase::initIndependent(GLuint fbos[], GLuint &bfUsedTextures)
{
    int i = 0;
    while( (1<<i) & bfUsedTextures) ++i;

    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexLabelId = createSimpleTexture2D(mWidth, mHeight, mImageLabel.data());
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_LABEL] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexLabelId) );

    i = 0;
    while( (1<<i) & bfUsedTextures) ++i;
    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexReducedId = createSimpleTexture2D(mWidth, mHeight, mImageReduced.data());
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_REDUCED] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexReducedId) );

    i = 0;
    while( (1<<i) & bfUsedTextures) ++i;
    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexOrigId = createSimpleTexture2D(mWidth, mHeight, mImageOrig.data());
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_ORIG] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexOrigId) );

    i = 0;
    while( (1<<i) & bfUsedTextures) ++i;

    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexFillId = createSimpleTexture2D(mWidth, mHeight);
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_FILL] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexFillId) );

    i = 0;
    while( (1<<i) & bfUsedTextures) ++i;

    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexPiPoId[0]  = createSimpleTexture2D(mWidth, mHeight);
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_PIPO+0] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexPiPoId[0]) );
    // Setup 2 Textures for Ping-Pong and
    // the program object
    return init(fbos, bfUsedTextures);
}

void StatsPhase::updateTextures(GLuint origTex, GLint origTexUnit,
                                GLuint labelTex, GLint labelTexUnit,
                                GLuint reducedTex, GLint reducedTexUnit,
                                GLuint freeTex, GLint freeTexUnit, GLuint freeTex2, GLint freeTexUnit2)
{
    mTexOrigId                 = origTex;
    mTextureUnits[TEX_ORIG]    = origTexUnit;

    mTexLabelId                = labelTex;
    mTextureUnits[TEX_LABEL]   = labelTexUnit;

    mTexReducedId              = reducedTex;
    mTextureUnits[TEX_REDUCED] = reducedTexUnit;

    mTexFillId                 = freeTex;
    mTextureUnits[TEX_FILL]    = freeTexUnit;

    mTexPiPoId[0]              = freeTex2;
    mTextureUnits[TEX_PIPO+0]  = freeTexUnit2;
}

void StatsPhase::setupGeometry()
{
    GL_CHECK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) );
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, 0) );
    // Set the viewport
    GL_CHECK( glViewport ( 0, 0, mWidth, mHeight ) );
}

double StatsPhase::run()
{
    double startTime, endTime;

    startTime = getRealTime();

    ///////////// --------- GENERAL SETUP ---------- ////////////////////

    GL_CHECK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) );
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, 0) );

    // Setup OpenGL
    // Attach the two PiPo-textures to the 2 fbos
    for(int i=0; i<2; i++)
    {
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[i]) );
        GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexPiPoId[i], 0) );
        CHECK_FBO();
    }

#ifdef _DEBUG
{
        char filename[50];
        sprintf(filename, "out0%03d.png", 0);
//        debugImage("Pixels before run:\n", filename);
}
#endif
    float factorX = 1.0, factorY = 1.0;

    fillStage(factorX, factorY);
    countStage(factorX, factorY, OFFSET);
    centroidingStage(factorX, factorY,CENTROID_X_COORD, 2*OFFSET);
    centroidingStage(factorX, factorY, CENTROID_Y_COORD, 3*OFFSET);

    factorX = -1.0;
    fillStage(factorX, factorY);
    countStage(factorX, factorY, OFFSET);
    centroidingStage(factorX, factorY,CENTROID_X_COORD, 2*OFFSET);
    centroidingStage(factorX, factorY, CENTROID_Y_COORD, 3*OFFSET);

    factorY = -1.0;
    fillStage(factorX, factorY);
    countStage(factorX, factorY, OFFSET);
    centroidingStage(factorX, factorY,CENTROID_X_COORD, 2*OFFSET);
    centroidingStage(factorX, factorY, CENTROID_Y_COORD, 3*OFFSET);

    factorX = 1.0;
    fillStage(factorX, factorY);
    countStage(factorX, factorY, OFFSET);
    centroidingStage(factorX, factorY,CENTROID_X_COORD, 2*OFFSET);
    centroidingStage(factorX, factorY, CENTROID_Y_COORD, 3*OFFSET);

    // Download the area of the final texture with the results back to program memory
    unsigned char data[4*mStatsAreaWidth*mStatsAreaHeight];
    GL_CHECK( glReadPixels(0, 0, mStatsAreaWidth, mStatsAreaHeight, GL_RGBA, GL_UNSIGNED_BYTE, data) );

    mSpots.clear();
    printf("Checking for spots %d x %d\n", mStatsAreaHeight, mStatsAreaWidth);
    for (unsigned j=0; j<mStatsAreaHeight; ++j)
    {
        for (unsigned i=0; i<mStatsAreaWidth/4; ++i)
        {
            int index = 4*(j*mStatsAreaWidth+ i);

            if (data[index] != 0)
            {
                Spot spot;
                GLushort sumLuminance = *(GLushort*) (data + index+ OFFSET_LUMINANCE);
                spot.area = *(GLushort*) (data + index+ OFFSET_AREA);
                if (spot.area > 2)
                {
                spot.x = convertSignedGl(*(GLuint*) (data + index+ OFFSET_SUM_X));
                spot.y = convertSignedGl(*(GLuint*) (data + index+ OFFSET_SUM_Y));
                printf("i: %4d j: %4d area: %2d\t x: %4d \t y: %4d \t sx: %f (0x%08x) \tsy: %f (0x%08x)\t lum: %d\n", i, j,
                       spot.area,
                       *(GLushort*) (data + index+ OFFSET_X)-1,
                       *(GLushort*) (data + index+ OFFSET_Y)-1,
                       spot.x,
                       *(GLuint*) (data + index+ OFFSET_SUM_X),
                       spot.y,
                       *(GLuint*) (data + index+ OFFSET_SUM_Y),
                       sumLuminance
                       );
                }
                spot.x = *(GLushort*) (data + index+ OFFSET_X)-1 - spot.x / sumLuminance;
                spot.y = *(GLushort*) (data + index+ OFFSET_Y)-1 - spot.y / sumLuminance;
                if (spot.area > 2)
                {
                    mSpots.push_back(spot);
                }
            }
        }
    }
//#ifdef _DEBUG
    {
        printf("Found %lu spots\n", mSpots.size());
        for (unsigned i=0; i<mSpots.size(); ++i)
        {
            printf ("i: %d  area: %d  x: %f  y: %f\n", i, mSpots[i].area, mSpots[i].x, mSpots[i].y);
        }
    }
//#endif
    printLabels(mStatsAreaWidth, mStatsAreaHeight, data);

    endTime = getRealTime();

    return (endTime-startTime)*1000;
}

void StatsPhase::releaseGlResources()
{
    GL_CHECK( glDeleteProgram(mProgFill.program) );
    GL_CHECK( glDeleteProgram(mProgCount.program) );
    GL_CHECK( glDeleteProgram(mProgCentroid.program) );
    GL_CHECK( glDeleteTextures(1, &mTexOrigId) );
    GL_CHECK( glDeleteTextures(2, mTexPiPoId) );
}

void StatsPhase::fillStage(float factorX, float factorY)
{

    GL_CHECK( glUseProgram (mProgFill.program) );

    GL_CHECK( glEnableVertexAttribArray ( mProgFill.positionLoc ) );
    GL_CHECK( glEnableVertexAttribArray ( mProgFill.texCoordLoc ) );

    // Bind the FBO to write to
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    GL_CHECK( glUniform2f ( mProgFill.u_texDimLoc, mWidth, mHeight) );
    // Set the sampler texture to use the texture containing the labels
    GL_CHECK( glUniform1i (mProgFill.s_labelLoc, mTextureUnits[TEX_LABEL] ) );
    // Set the pass index
    GL_CHECK( glUniform1i ( mProgFill.u_passLoc,  0) );
    GL_CHECK( glUniform2f ( mProgFill.u_factorLoc, factorX, factorY ) );
    // Draw scene
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
    std::swap(mRead, mWrite);

#ifdef _DEBUG
{
        char filename[50];
        char text[50];
        sprintf(filename, "outF%03d.png", 0);
        sprintf(text, "Pixels after pass %d:\n", 0);
        debugImage(text, filename);
}
#endif

    for(int i=1; i<mNumFillIterations; ++i)
    {
        // Bind the FBO to write to
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
        // Set the sampler texture to use the texture containing the labels
        GL_CHECK( glUniform1i ( mProgFill.s_labelLoc, mTextureUnits[TEX_PIPO+mRead] ) );
        // Set the pass index
        GL_CHECK( glUniform1i ( mProgFill.u_passLoc,  i) );
        //    GL_CHECK( glUniform1f ( u_factorLoc, u_factor) );
        // Draw scene
        GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
        std::swap(mRead, mWrite);

#ifdef _DEBUG
{
            char filename[50];
            char text[50];
            sprintf(filename, "outF%03d.png", i);
            sprintf(text, "Pixels after fill pass %d:\n", i);
            debugImage(text, filename);
}
#endif

    }

    // Save the texture with the results of the filling and use a new texture for PIPO
    std::swap(mTexPiPoId[mRead], mTexFillId);
    std::swap(mTextureUnits[TEX_FILL], mTextureUnits[TEX_PIPO+mRead]);
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mRead]) );
    GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexPiPoId[mRead], 0) );
    CHECK_FBO();

    GL_CHECK( glDisableVertexAttribArray ( mProgFill.positionLoc ) );
    GL_CHECK( glDisableVertexAttribArray ( mProgFill.texCoordLoc ) );
}

void StatsPhase::countStage(float factorX, float factorY, int offset)
{
    GL_CHECK( glUseProgram (mProgCount.program) );

    GL_CHECK( glEnableVertexAttribArray ( mProgCount.positionLoc ) );
    GL_CHECK( glEnableVertexAttribArray ( mProgCount.texCoordLoc ) );

    GL_CHECK( glUniform1i ( mProgCount.u_stageLoc , STAGE_COUNT) );
    GL_CHECK( glUniform2f ( mProgCount.u_texDimLoc, mWidth, mHeight) );
    GL_CHECK( glUniform2f ( mProgCount.u_factorLoc, factorX, factorY ) );
    // Bind the different sampler2D
    // Texture with the filled spots from previous stage (read only)
    GL_CHECK( glUniform1i ( mProgCount.s_fillLoc,  mTextureUnits[TEX_FILL] ) );
    GL_CHECK( glUniform1i ( mProgCount.s_origLoc,  mTextureUnits[TEX_ORIG] ) );
    // Texture with the labels from last phase (read only)
    GL_CHECK( glUniform1i ( mProgCount.s_labelLoc,  mTextureUnits[TEX_LABEL] ) );

    // Start with -1 because that is the initalization pass
    for (int i=-1; i<4   ; ++i)
    {
        // Bind the FBO to write to
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
        // Set the pass index
        GL_CHECK( glUniform1i ( mProgCount.u_passLoc,  i) );
        // Set the sampler texture to use the texture containing the labels
        GL_CHECK( glUniform1i ( mProgCount.s_resultLoc, mTextureUnits[TEX_PIPO+mRead] ) );

        // Draw scene
        GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
        std::swap(mRead, mWrite);

#ifdef _DEBUG
        {
            char filename[50];
            char text[50];
            sprintf(filename, "outC%03d.png", i+1);
            sprintf(text, "Pixels after count %d:\n", i);
            debugImage(text, filename);
        }
#endif

    }

    // Write the count result as a reduced table (with an offset so it won't interfere
    // with the table in texLabelId in the next step
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    GL_CHECK( glUniform1i ( mProgCount.u_stageLoc,  STAGE_SAVE ) );
    GL_CHECK( glUniform2f ( mProgCount.u_factorLoc, factorX, factorY ) );
    GL_CHECK( glUniform1f ( mProgCount.u_savingOffsetLoc, offset) );
    GL_CHECK( glUniform1i ( mProgCount.s_labelLoc,  mTextureUnits[TEX_REDUCED] ) );
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
    std::swap(mRead, mWrite);

#ifdef _DEBUG
    {
        char filename[50];
        sprintf(filename, "outS.png");
        debugImage("Pixels after save:\n", filename);
    }
#endif

    // Add/blend texture from previous step and mTexLabel together
    // This yields a texture which has the lookup table for root pixels in its first few
    // columns and the count values in the next few columns
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    GL_CHECK( glUniform1i ( mProgCount.u_stageLoc,  STAGE_BLEND ) );
    GL_CHECK( glUniform1i ( mProgCount.s_resultLoc, mTextureUnits[TEX_PIPO+mRead] ) );
    GL_CHECK( glUniform1i ( mProgCount.s_labelLoc,  mTextureUnits[TEX_REDUCED] ) );
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
    std::swap(mRead, mWrite);

#ifdef _DEBUG
    {
        char filename[50];
        sprintf(filename, "outM.png");
        debugImage("Pixels after merge:\n", filename);
    }
#endif

    // Save the result from the previous step into mTexLabel (by switching the texture
    // objects and the corresponding texture unit)
    std::swap(mTexPiPoId[mRead], mTexReducedId);
    std::swap(mTextureUnits[TEX_REDUCED], mTextureUnits[TEX_PIPO+mRead]);
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mRead]) );
    GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexPiPoId[mRead], 0) );
    CHECK_FBO();

    GL_CHECK( glDisableVertexAttribArray ( mProgCount.positionLoc ) );
    GL_CHECK( glDisableVertexAttribArray ( mProgCount.texCoordLoc ) );
}

void StatsPhase::centroidingStage(float factorX, float factorY, int coordinate, int offset)
{
    GL_CHECK( glUseProgram (mProgCentroid.program) );

    GL_CHECK( glEnableVertexAttribArray ( mProgCentroid.positionLoc ) );
    GL_CHECK( glEnableVertexAttribArray ( mProgCentroid.texCoordLoc ) );

    GL_CHECK( glUniform1i ( mProgCentroid.u_stageLoc , STAGE_CENTROIDING) );
    GL_CHECK( glUniform2f ( mProgCentroid.u_texDimLoc, mWidth, mHeight) );
    GL_CHECK( glUniform2f ( mProgCentroid.u_factorLoc, factorX, factorY ) );
    // Bind the different sampler2D
    // Texture with the filled spots from previous stage (read only)
    GL_CHECK( glUniform1i ( mProgCentroid.s_origLoc,  mTextureUnits[TEX_ORIG] ) );
    GL_CHECK( glUniform1i ( mProgCentroid.s_fillLoc,   mTextureUnits[TEX_FILL] ) );
    // Texture with the labels from last phase (read only)
    GL_CHECK( glUniform1i ( mProgCentroid.s_labelLoc,  mTextureUnits[TEX_LABEL] ) );

    // Initialization pass
    // Bind the FBO to write to
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    // Set the pass index
    GL_CHECK( glUniform1i ( mProgCentroid.u_passLoc,  coordinate) );
    // Set the sampler texture to use the texture containing the labels
    GL_CHECK( glUniform1i ( mProgCentroid.s_resultLoc, mTextureUnits[TEX_PIPO+mRead] ) );
    // Draw scene
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
    std::swap(mRead, mWrite);

#ifdef _DEBUG
    {
        char filename[50];
        sprintf(filename, "outS1.png");
        debugImage("Pixels after Init:\n", filename);
    }
#endif

    for (int i=0; i<4   ; ++i)
    {
        // Bind the FBO to write to
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
        // Set the pass index
        GL_CHECK( glUniform1i ( mProgCentroid.u_passLoc,  i) );
        // Set the sampler texture to use the texture containing the labels
        GL_CHECK( glUniform1i ( mProgCentroid.s_resultLoc, mTextureUnits[TEX_PIPO+mRead] ) );

        // Draw scene
        GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
        std::swap(mRead, mWrite);

#ifdef _DEBUG
        {
            char filename[50];
            char text[50];
            sprintf(filename, "outC%03d.png", i);
            sprintf(text, "Pixels after count %d:\n", i);
            debugImage(text, filename);
        }
#endif
    }
    // Write the centroiding result as a reduced table (with an offset so it won't interfere
    // with the table in texLabelId in the next step
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    GL_CHECK( glUniform1i ( mProgCentroid.u_stageLoc,  STAGE_SAVE ) );
    GL_CHECK( glUniform2f ( mProgCentroid.u_factorLoc, factorX, factorY ) );
    GL_CHECK( glUniform1f ( mProgCentroid.u_savingOffsetLoc, offset) );
    GL_CHECK( glUniform1i ( mProgCentroid.s_labelLoc,  mTextureUnits[TEX_REDUCED] ) );
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
    std::swap(mRead, mWrite);

#ifdef _DEBUG
    {
        char filename[50];
        sprintf(filename, "outS.png");
        debugImage("Pixels after save:\n", filename);
    }
#endif

    // Add/blend texture from previous step and mTexLabel together
    // This yields a texture which has the lookup table for root pixels in its first few
    // columns and the count values in the next few columns
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    GL_CHECK( glUniform1i ( mProgCentroid.u_stageLoc,  STAGE_BLEND ) );
    GL_CHECK( glUniform1i ( mProgCentroid.s_resultLoc, mTextureUnits[TEX_PIPO+mRead] ) );
    GL_CHECK( glUniform1i ( mProgCentroid.s_labelLoc,  mTextureUnits[TEX_REDUCED] ) );
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
    std::swap(mRead, mWrite);

#ifdef _DEBUG
    {
        char filename[50];
        sprintf(filename, "outM.png");
        debugImage("Pixels after merge:\n", filename);
    }
#endif

    // Save the result from the previous step into mTexLabel (by switching the texture
    // objects and the corresponding texture unit)
    std::swap(mTexPiPoId[mRead], mTexReducedId);
    std::swap(mTextureUnits[TEX_REDUCED], mTextureUnits[TEX_PIPO+mRead]);
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mRead]) );
    GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexPiPoId[mRead], 0) );
    CHECK_FBO();


    GL_CHECK( glDisableVertexAttribArray ( mProgCentroid.positionLoc ) );
    GL_CHECK( glDisableVertexAttribArray ( mProgCentroid.texCoordLoc ) );
}

void StatsPhase::debugImage(const char *text, const char *filename)
{
    CImg<unsigned char> image(4, mWidth, mHeight, 1, 0);
    GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, image.data()) );
    printf("%s", text);
    printLabels(mWidth, mHeight, image.data());
    writeImage(mWidth, mHeight, filename, image);
}
