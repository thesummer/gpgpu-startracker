 
#include <iostream>
using std::cerr;
using std::endl;

#define TEX_ORIG   0
#define TEX_PIPO   1

#include "lookupPhase.h"

LookupPhase::LookupPhase(int width, int height)
    : mVertFilename("../glsl/lookUp.vert"), mFragFilename("../glsl/lookUp.frag"),
      mWidth(width), mHeight(height), mVertices{1.0, 1.0}
{
}

LookupPhase::~LookupPhase()
{
    GL_CHECK( glDeleteProgram(mProgramObject) );
    GL_CHECK( glDeleteTextures(1, &mTexOrigId) );
    GL_CHECK( glDeleteTextures(2, mTexPiPoId) );
}

GLint LookupPhase::init(GLuint fbos[2], GLuint &bfUsedTextures)
{
    // Save the handles to the 2 framebuffers
    mFboId[0] = fbos[0];
    mFboId[1] = fbos[1];

    // Initialize all OpenGL structures necessary for the
    // labeling phase here
    mRead  = 0;
    mWrite = 1;

    //Initialize VBO
    glGenBuffers(1, &mVboId);
    //Upload vertex data
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, mVboId) );
    GL_CHECK( glBufferData(GL_ARRAY_BUFFER, 1 * 2 * sizeof(float), mVertices, GL_STATIC_DRAW) );

    // Load the shaders and get a linked program object
    mProgramObject = loadProgramFromFile( mVertFilename, mFragFilename);
    if (mProgramObject == 0)
    {
        cerr << "Failed to generate Program object for label phase" << endl;
    }


     // Get the attribute locations
     mPositionLoc = glGetAttribLocation ( mProgramObject, "a_position" );
//     mTexCoordLoc = glGetAttribLocation ( mProgramObject, "a_texCoord" );

     // Get the sampler locations
//     mSamplerLoc     = glGetUniformLocation( mProgramObject, "s_texture" );
//     u_texDimLoc     = glGetUniformLocation ( mProgramObject, "u_texDimensions" );
//     u_passLoc       = glGetUniformLocation ( mProgramObject, "u_pass" );
//     u_debugLoc      = glGetUniformLocation ( mProgramObject, "u_debug" );
//     u_factorLoc     = glGetUniformLocation ( mProgramObject, "u_factor" );

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

GLint LookupPhase::initIndependent(GLuint fbos[], GLuint &bfUsedTextures)
{
//    int i = 0;
//    while( (1<<i) & bfUsedTextures) ++i;

//    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
//    mTexOrigId = createSimpleTexture2D(mWidth, mHeight, mTgaData->img_data);
//    bfUsedTextures |= (1<<i);
//    mTextureUnits[TEX_ORIG] = i;
//    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexOrigId) );

    // Setup 2 Textures for Ping-Pong and
    // the program object
    return init(fbos, bfUsedTextures);
}

GLint LookupPhase::getLastTexture()
{
    return mTexPiPoId[mRead];
}

GLint LookupPhase::getLastTexUnit()
{
    return mTextureUnits[TEX_PIPO+mRead];
}

GLint LookupPhase::getFreeTexture()
{
    return mTexPiPoId[mWrite];
}

GLint LookupPhase::getFreeTexUnit()
{
    return mTextureUnits[TEX_PIPO+mWrite];
}

void LookupPhase::setupGeometry()
{
    // Set the viewport
    GL_CHECK( glViewport ( 0, 0, mWidth, mHeight ) );
    // Clear the color buffer
    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );

    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, mVboId) );
    // Load the vertex position
    GL_CHECK( glEnableVertexAttribArray ( mPositionLoc ) );
    GL_CHECK( glVertexAttribPointer ( mPositionLoc, 2, GL_FLOAT,
                                      GL_FALSE, 0 * sizeof(GLfloat), 0) );
    // Load the texture coordinates
//    GL_CHECK( glVertexAttribPointer ( mTexCoordLoc, 2, GL_FLOAT,
//                                      GL_FALSE, 5 * sizeof(GLfloat), &mVertices[3] ) );
//    GL_CHECK( glEnableVertexAttribArray ( mTexCoordLoc ) );
}

double LookupPhase::run()
{
    double startTime, endTime;

    startTime = getRealTime();

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

    // Bind the FBO to write to
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId[mWrite]) );
    // Set the sampler texture to use the original image
//    GL_CHECK( glUniform1i ( mSamplerLoc, mTextureUnits[TEX_ORIG] ) );

    // Draw scene
    GL_CHECK( glDrawArrays( GL_POINTS, 0, 1) );

#ifdef _DEBUG
        // Make the BYTE array, factor of 3 because it's RGBA.
        GLubyte* pixels = new GLubyte[4*mWidth*mHeight];
        GL_CHECK( glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
        printf("Pixels after pass %d:\n", i);
        printLabels(mWidth, mHeight, pixels);
//        char filename[50];
//        sprintf(filename, "outl%03d.tga", i);
//        writeTgaImage(mWidth, mHeight, filename, pixels);
        delete [] pixels;
#endif

        // Switch read and write texture

    endTime = getRealTime();

    return (endTime-startTime)*1000;
}

