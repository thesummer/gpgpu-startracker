#include "labelPhase.h"

#include <iostream>
using std::cerr;
using std::endl;

#define TEX_ORIG   0
#define TEX_PIPO   1

#define STAGE_INITIAL_LABELING   0
#define STAGE_HIGHEST_LABEL      1


LabelPhase::LabelPhase(int width, int height)
    : mVertFilename("../glsl/quad.vert"), mFragFilename("../glsl/labelPhase.frag"),
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
      mIndices { 0, 1, 2, 0, 2, 3 }, u_threshold(64.3 / 255.0)
{
}

LabelPhase::~LabelPhase()
{
    GL_CHECK( glDeleteProgram(mProgramObject) );
    GL_CHECK( glDeleteTextures(1, &mTexOrigId) );
    GL_CHECK( glDeleteTextures(2, mTexPiPoId) );
}

GLint LabelPhase::init(GLuint fbos[2], GLuint &bfUsedTextures)
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
    mSamplerLoc     = glGetUniformLocation( mProgramObject,  "s_texture" );
    u_texDimLoc     = glGetUniformLocation ( mProgramObject, "u_texDimensions" );
    u_thresholdLoc  = glGetUniformLocation ( mProgramObject, "u_threshold" );
    u_passLoc       = glGetUniformLocation ( mProgramObject, "u_pass" );
    u_factorLoc     = glGetUniformLocation ( mProgramObject, "u_factor" );

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

    GL_CHECK( glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f ) );

    return GL_TRUE;
}

GLint LabelPhase::initIndependent(GLuint fbos[], GLuint &bfUsedTextures)
{
    int i = 0;
    while( (1<<i) & bfUsedTextures) ++i;

    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexOrigId = createSimpleTexture2D(mWidth, mHeight, mImage.data());
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_ORIG] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexOrigId) );

    // Setup 2 Textures for Ping-Pong and
    // the program object
    return init(fbos, bfUsedTextures);
}

GLuint LabelPhase::getOrigTexture()
{
    return mTexOrigId;
}

GLint LabelPhase::getOrigTexUnit()
{
    return mTextureUnits[TEX_ORIG];
}

GLuint LabelPhase::getLastTexture()
{
    return mTexPiPoId[mRead];
}

GLint LabelPhase::getLastTexUnit()
{
    return mTextureUnits[TEX_PIPO+mRead];
}

GLuint LabelPhase::getFreeTexture()
{
    return mTexPiPoId[mWrite];
}

GLint LabelPhase::getFreeTexUnit()
{
    return mTextureUnits[TEX_PIPO+mWrite];
}

void LabelPhase::setupGeometry()
{
    GL_CHECK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) );
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, 0) );
    // Set the viewport
    GL_CHECK( glViewport ( 0, 0, mWidth, mHeight ) );
    // Clear the color buffer
    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );
}

double LabelPhase::run()
{
    double startTime, endTime;

    startTime = getRealTime();

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
    GL_CHECK( glUniform1f ( u_thresholdLoc, u_threshold) );

    // Do the runs
    u_factor = -1.0;



    ///---------- 1. THRESHOLD AND INITIAL LABELING --------------------

    // Bind the FBO to write to
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    // Set the sampler texture to use the original image
    GL_CHECK( glUniform1i ( mSamplerLoc, mTextureUnits[TEX_ORIG] ) );
    // Set the pass index
    GL_CHECK( glUniform1i ( u_passLoc,  STAGE_INITIAL_LABELING) );
    GL_CHECK( glUniform1f ( u_factorLoc, u_factor) );
    // Draw scene
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
    std::swap(mRead, mWrite);

#ifdef _DEBUG
{
        CImg<unsigned char> image(4, mWidth, mHeight, 1, 0);
        GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, image.data()) );
        printf("Pixels after pass %d:\n", 0);
        printLabels(mWidth, mHeight, image.data());
        char filename[50];
        sprintf(filename, "outl%03d.png", 0);
        writeImage(mWidth, mHeight, filename, image);
}
#endif

    ///---------- 2. CONNECTED COMPONENT LABELING  --------------------

    for (int i = 1; i < logBase2(mHeight)+10; i++)
    {
        if( i%2 == 1)
        {
            u_pass = STAGE_HIGHEST_LABEL;
            u_factor *= -1.0;
        }
        else
        {
            u_pass = i;
        }

        // Bind the FBO to write to
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
        // Set the sampler texture unit to 0
        GL_CHECK( glUniform1i ( mSamplerLoc, mTextureUnits[TEX_PIPO+mRead] ) );
        // Set the pass index
        GL_CHECK( glUniform1i ( u_passLoc,  u_pass) );
        GL_CHECK( glUniform1f ( u_factorLoc, u_factor) );
        // Draw scene
        GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
        std::swap(mRead, mWrite);

#ifdef _DEBUG
{
        // Make the BYTE array, factor of 3 because it's RGBA.
        CImg<unsigned char> image(4, mWidth, mHeight, 1, 0);
        GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, image.data()) );
        printf("Pixels after pass %d:\n", i);
        printLabels(mWidth, mHeight, image.data());
        char filename[50];
        sprintf(filename, "outl%03d.png", i);
        writeImage(mWidth, mHeight, filename, image);
}
#endif

        // Switch read and write texture
    }

    GL_CHECK( glDisableVertexAttribArray ( mPositionLoc ) );
    GL_CHECK( glDisableVertexAttribArray ( mTexCoordLoc ) );

    endTime = getRealTime();

    return (endTime-startTime)*1000;
}

