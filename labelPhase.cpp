#include <iostream>
using std::cerr;
using std::endl;

#define TEX_ORIG   0
#define TEX_PIPO   1

#include "labelPhase.h"

LabelPhase::LabelPhase(int width, int height)
    : mVertFilename("vertShader.glsl"), mFragFilename("fragCreateRuns.glsl"),
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
      mIndices { 0, 1, 2, 0, 2, 3 }, u_threshold(64 / 255.0)
{
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


     // Get the attribute locations
     mPositionLoc = glGetAttribLocation ( mProgramObject, "a_position" );
     mTexCoordLoc = glGetAttribLocation ( mProgramObject, "a_texCoord" );

     // Get the sampler locations
     mSamplerLoc     = glGetUniformLocation( mProgramObject, "s_texture" );
     u_texDimLoc     = glGetUniformLocation ( mProgramObject, "u_texDimensions" );
     u_thresholdLoc  = glGetUniformLocation ( mProgramObject, "u_threshold" );
     u_passLoc       = glGetUniformLocation ( mProgramObject, "u_pass" );
     u_debugLoc      = glGetUniformLocation ( mProgramObject, "u_debug" );
     u_factorLoc     = glGetUniformLocation ( mProgramObject, "u_factor" );
     // Create a texture for the frambuffer
//     mTexPiPoId[mRead]  = createSimpleTexture2D(mWidth, mHeight, mTgaData->img_data);
//     mTexPiPoId[mWrite] = createSimpleTexture2D(mWidth, mHeight);


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
    mTexOrigId = createSimpleTexture2D(mWidth, mHeight, mTgaData->img_data);
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_ORIG] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexOrigId) );

    // Setup 2 Textures for Ping-Pong and
    // the program object
    return init(fbos, bfUsedTextures);
}

GLint LabelPhase::getLastTexture()
{
    return mTexPiPoId[mRead];
}

GLint LabelPhase::getLastTexUnit()
{
    return mTextureUnits[TEX_PIPO+mRead];
}

GLint LabelPhase::getFreeTexture()
{
    return mTexPiPoId[mWrite];
}

GLint LabelPhase::getFreeTexUnit()
{
    return mTextureUnits[TEX_PIPO+mWrite];
}

void LabelPhase::setupGeometry()
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

void LabelPhase::run()
{
    // Setup OpenGL
    // Attach the two PiPo-textures to the 2 fbos
    for(int i=0; i<2; i++)
    {
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[i]) );
        GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexPiPoId[i], 0) );
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            printf("Framebuffer is not complete with %08x\n", status);
        }
    }

    // Use the program object
    GL_CHECK( glUseProgram ( mProgramObject ) );

    // Set the uniforms
    GL_CHECK( glUniform2f ( u_texDimLoc, mWidth, mHeight) );
    GL_CHECK( glUniform1f ( u_thresholdLoc, u_threshold) );

    // Do the runs
    u_factor = -1.0;

    double startTime, endTime;

    startTime = getRealTime();


    ///---------- 1. THRESHOLD AND INITIAL LABELING --------------------

    // Bind the FBO to write to
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    // Set the sampler texture to use the original image
    GL_CHECK( glUniform1i ( mSamplerLoc, mTextureUnits[TEX_ORIG] ) );
    // Set the pass index
    GL_CHECK( glUniform1i ( u_passLoc,  0) );
    GL_CHECK( glUniform1f ( u_factorLoc, u_factor) );
    // Draw scene
    GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
    std::swap(mRead, mWrite);

    ///---------- 2. CONNECTED COMPONENT LABELING  --------------------

    for (int i = 1; i < logBase2(mHeight)+10; i++)
    {
        if( i%2 == 1)
        {
            u_pass = 1;
            u_factor *= -1.0;
            u_debug = 0;
        }
        else
        {
            u_pass = i;
            u_debug = 0;
        }


        // Bind the FBO to write to
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
        // Set the sampler texture unit to 0
        GL_CHECK( glUniform1i ( mSamplerLoc, mTextureUnits[TEX_PIPO+mRead] ) );
        // Set the pass index
        GL_CHECK( glUniform1i ( u_passLoc,  u_pass) );
        GL_CHECK( glUniform1i ( u_debugLoc, u_debug) );
        GL_CHECK( glUniform1f ( u_factorLoc, u_factor) );
        // Draw scene
        GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );
        std::swap(mRead, mWrite);

#ifdef _DEBUG
        // Make the BYTE array, factor of 3 because it's RGBA.
        GLubyte* pixels = new GLubyte[4*mWidth*mHeight];
        GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
        printf("Pixels after pass %d:\n", i);
//        printLabels(mWidth, mHeight, pixels);
        char filename[50];
        sprintf(filename, "raw%03d.tga", i);
        writeRawTgaImage(mWidth, mHeight, filename, pixels);
        delete [] pixels;
#endif

        // Switch read and write texture
    }
    endTime = getRealTime();

    printf("Time: %f ms\n", (endTime-startTime)*1000);
}

