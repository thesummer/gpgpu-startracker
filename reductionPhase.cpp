#include <iostream>
using std::cerr;
using std::endl;

#include "reductionPhase.h"

ReductionPhase::ReductionPhase(int width, int height)
    :mVertFilename("vertShader.glsl"), mFragFilename("fragReductionShater.glsl"),
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

GLint ReductionPhase::init(GLuint fbos[], GLuint initTexture,
                           int numNewTextures, GLuint &bfUsedTextures)
{
    // Save the handles to the 2 framebuffers
    mFboId[0] = fbos[0];
    mFboId[1] = fbos[1];
    mInitTexId = initTexture;

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
         int i = 0;
         while( (1<<i) & bfUsedTextures) ++i;

         GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
         bfUsedTextures |= (1<<i);

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
    //
}

void ReductionPhase::run()
{
    //
}
