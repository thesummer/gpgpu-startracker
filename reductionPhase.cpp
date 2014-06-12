#include <iostream>
using std::cerr;
using std::endl;

#include "reductionPhase.h"
#include "include/getTime.h"

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
      mIndices { 0, 1, 2, 0, 2, 3 }

{
}

GLint ReductionPhase::init(GLuint fbos[], int numNewTextures, GLuint &bfUsedTextures)
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
    }

     // Get the attribute locations
     mPositionLoc = glGetAttribLocation ( mProgramObject, "a_position" );
     mTexCoordLoc = glGetAttribLocation ( mProgramObject, "a_texCoord" );

     // Get the sampler locations
     mSamplerLoc     = glGetUniformLocation( mProgramObject, "s_texture" );
     u_texDimLoc     = glGetUniformLocation ( mProgramObject, "u_texDimensions" );
     u_passLoc       = glGetUniformLocation ( mProgramObject, "u_pass" );
     u_debugLoc      = glGetUniformLocation ( mProgramObject, "u_debug" );

     // Create numNewTextures texture for the frambuffer and
     // bind the textures to the corresponding fbo
     if(numNewTextures > 1)
     {
         mFboTexId[0]  = createSimpleTexture2D(mWidth, mHeight, mTgaData->img_data);
         GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[0]) );
         int i = 0;
         while( (1<<i) & bfUsedTextures) ++i;

         GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
         bfUsedTextures |= (1<<i);
         mTextureUnits[0] = i;
         GL_CHECK( glBindTexture(GL_TEXTURE_2D, mFboTexId[0]) );
         GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFboTexId[0], 0) );
         GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
         if (status != GL_FRAMEBUFFER_COMPLETE)
         {
             printf("Framebuffer is not complete with %08x\n", status);
         }
     }

     if (numNewTextures > 0)
     {
         mFboTexId[1]  = createSimpleTexture2D(mWidth, mHeight);
         GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[1]) );
         int i = 0;
         while( (1<<i) & bfUsedTextures) ++i;

         GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
         bfUsedTextures |= (1<<i);
         mTextureUnits[1] = i;

         GL_CHECK( glBindTexture(GL_TEXTURE_2D, mFboTexId[1]) );
         GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFboTexId[1], 0) );
         GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
         if (status != GL_FRAMEBUFFER_COMPLETE)
         {
             printf("Framebuffer is not complete with %08x\n", status);
         }
     }

     GL_CHECK( glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f ) );

     return GL_TRUE;
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

void ReductionPhase::run()
{
    // Setup OpenGL

    // Use the program object
    GL_CHECK( glUseProgram ( mProgramObject ) );

    // Set the uniforms
    GL_CHECK( glUniform2f ( u_texDimLoc, mWidth, mHeight) );

    // Do the runs

    // Make the BYTE array, factor of 3 because it's RGBA.
    GLubyte* pixels = new GLubyte[4*mWidth*mHeight];

    double startTime, endTime;

    startTime = getRealTime();

    for (int i = 0; i < logBase2(mWidth); ++i)
    {
        u_pass = i;
        u_debug = 0;

        // Bind the FBO to write to
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
        // Set the sampler texture unit to 0
        GL_CHECK( glUniform1i ( mSamplerLoc, mTextureUnits[mRead] ) );
        // Set the pass index
        GL_CHECK( glUniform1i ( u_passLoc,  u_pass) );
        GL_CHECK( glUniform1i ( u_debugLoc, u_debug) );
        // Draw scene
        GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, mIndices ) );

#ifdef _DEBUG
        GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
        printf("Pixels after pass %d:\n", i);
        printLabels(mWidth, mHeight, pixels);
        char filename[50];
        sprintf(filename, "out%03d.tga", i);
        writeTgaImage(mWidth, mHeight, filename, pixels);
#endif

        // Switch read and write texture
        mRead  = 1 - mRead;
        mWrite = 1 - mWrite;
    }
    endTime = getRealTime();

    printf("Time: %f ms\n", (endTime-startTime)*1000);

    delete [] pixels;
}
