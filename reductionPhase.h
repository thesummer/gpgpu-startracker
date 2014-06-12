#ifndef REDUCTIONPHASE_H
#define REDUCTIONPHASE_H

#include "include/tga.h"
#include "phase.h"

class ReductionPhase : public Phase
{
public:

    // Vertex and fragment shader files
    const char * mVertFilename;
    const char * mFragFilename;

    // Handle to a program object
    GLuint mProgramObject;

    int mWidth;
    int mHeight;

    // Attribute locations
    GLint  mPositionLoc;
    GLint  mTexCoordLoc;

    // Vertices
    GLfloat mVertices[20];
    GLushort mIndices[6];

    // Uniform locations
    GLint  u_texDimLoc;
    GLint  u_passLoc;
    GLint  u_debugLoc;

    // Uniform values
    GLint u_pass;
    GLint u_debug;

//  Check is all members necessary
    // Sampler location
    GLint mSamplerLoc;

    // Texture handle
    GLuint mTextureId;
    /// TODO: tga somewhere else?
    TGA    *mTgaImage;
    TGAData *mTgaData;

// End Check
    // Texture to attach to the frambuffers
    GLuint mFboTexId[2];
    GLuint mInitTexId;
    GLuint mFboId[2];

    int mWrite;
    int mRead;

    ReductionPhase(int width = 0, int height = 0);
    GLint init(GLuint fbos[], GLuint initTexture, int numNewTextures, GLuint &bfUsedTextures);

    void setupGeometry();
    virtual void run();
};

#endif // REDUCTIONPHASE_H
