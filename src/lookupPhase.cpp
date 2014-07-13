 
#include <iostream>
using std::cerr;
using std::endl;

#define TEX_REDUCED 0

#include "lookupPhase.h"

LookupPhase::LookupPhase(int texWidth, int texHeight, int vertexWidth, int vertexHeight)
    : mVertFilename("../glsl/lookUp.vert"), mFragFilename("../glsl/lookUp.frag"),
      mTexWidth(texWidth), mTexHeight(texHeight),
      mVertexWidth(vertexWidth), mVertexHeight(vertexHeight)
{
    // Setup the Vertex positions
    mVertices = new GLfloat[2*mVertexWidth*mVertexHeight];
    for(int i=0; i<mVertexWidth; ++i)
    {
        for(int j=0; j<mVertexHeight; ++j)
        {
            int index = 2*(mVertexHeight*i + j);
            mVertices[index + 0] = (GLfloat) i;
            mVertices[index + 1] = (GLfloat) j;
        }

    }
    mNumVertices = mVertexHeight * mVertexWidth;
}

LookupPhase::~LookupPhase()
{
    GL_CHECK( glDeleteProgram(mProgramObject) );
    GL_CHECK( glDeleteTextures(1, &mTexReducedId) );
    GL_CHECK( glDeleteTextures(1, &mTexLookUpId) );
    GL_CHECK( glDeleteBuffers(1, &mVboId) );
}

GLint LookupPhase::init(GLuint &bfUsedTextures)
{
    // Initialize all OpenGL structures necessary for the
    // labeling phase here

    //Initialize VBO
    glGenBuffers(1, &mVboId);
    //Upload vertex data
    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, mVboId) );
    GL_CHECK( glBufferData(GL_ARRAY_BUFFER, mNumVertices * 2 * sizeof(float), mVertices, GL_STATIC_DRAW) );

    // Load the shaders and get a linked program object
    mProgramObject = loadProgramFromFile( mVertFilename, mFragFilename);
    if (mProgramObject == 0)
    {
        cerr << "Failed to generate Program object for label phase" << endl;
    }


     // Get the attribute locations
     mPositionLoc = glGetAttribLocation ( mProgramObject, "a_position" );

     // Get the sampler locations
     mSamplerLoc     = glGetUniformLocation( mProgramObject, "s_texture" );
     u_texDimLoc     = glGetUniformLocation ( mProgramObject, "u_texDimensions" );
//     u_debugLoc      = glGetUniformLocation ( mProgramObject, "u_debug" );

     GL_CHECK( glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f ) );

     return GL_TRUE;
}

GLint LookupPhase::initIndependent(GLuint fbo, GLuint &bfUsedTextures)
{
    mFboId = fbo;
    int i = 0;
    while( (1<<i) & bfUsedTextures) ++i;
    GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
    mTexLookUpId = createSimpleTexture2D(mTexWidth, mTexHeight, NULL);
    mTexReducedId = createSimpleTexture2D(mTexWidth, mTexHeight, mTgaData->img_data);
    bfUsedTextures |= (1<<i);
    mTextureUnits[TEX_REDUCED] = i;
    GL_CHECK( glBindTexture(GL_TEXTURE_2D, mTexReducedId) );

    return init(bfUsedTextures);
}

void LookupPhase::setupGeometry()
{
    // Set the viewport
    GL_CHECK( glViewport ( 0, 0, mTexWidth, mTexHeight ) );
    // Clear the color buffer
    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );

    GL_CHECK( glBindBuffer(GL_ARRAY_BUFFER, mVboId) );
    // Load the vertex position
    GL_CHECK( glEnableVertexAttribArray ( mPositionLoc ) );
    GL_CHECK( glVertexAttribPointer ( mPositionLoc, 2, GL_FLOAT,
                                      GL_FALSE, 0 * sizeof(GLfloat), 0) );
}

double LookupPhase::run()
{
    double startTime, endTime;

    startTime = getRealTime();

    // Setup OpenGL
    // Attach the texture to the fbo
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId) );
    GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexLookUpId, 0) );
    CHECK_FBO();


    // Use the program object
    GL_CHECK( glUseProgram ( mProgramObject ) );

    // Set the uniforms
    GL_CHECK( glUniform2f ( u_texDimLoc, mTexWidth, mTexHeight) );

    // Bind the FBO to write to
    GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, mFboId) );
    // Set the sampler texture to use the image with the reduced labels
    GL_CHECK( glUniform1i ( mSamplerLoc, mTextureUnits[TEX_REDUCED] ) );

    // Draw scene
    GL_CHECK( glDrawArrays( GL_POINTS, 0, mNumVertices) );

#ifdef _DEBUG
        // Make the BYTE array, factor of 3 because it's RGBA.
        GLubyte* pixels = new GLubyte[4*mTexWidth*mTexHeight];
        GL_CHECK( glReadPixels(0, 0, mTexWidth, mTexHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );
        printf("Pixels after pass:\n");
        printLabels(mTexWidth, mTexHeight, pixels);
//        char filename[50];
//        sprintf(filename, "outl.tga");
//        writeTgaImage(mTexWidth, mTexHeight, filename, pixels);
        delete [] pixels;
#endif

        // Switch read and write texture

    endTime = getRealTime();

    return (endTime-startTime)*1000;
}

