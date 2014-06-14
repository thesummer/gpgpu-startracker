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
    GLfloat  mVertices[20];
    GLushort mIndices[6];

    // Uniform locations
    GLint  u_texDimLoc;
    GLint  u_passLoc;
    GLint  u_debugLoc;

    // Uniform values
    GLint u_pass;
    GLint u_debug;

//  Check is all members necessary
    // Sampler locations
    GLint s_reductionLoc;
    GLint s_valuesLoc;

    // Texture handle
    GLuint mTextureId;
    /// TODO: tga somewhere else?
    TGA    *mTgaImage;
    TGAData *mTgaData;

// End Check
    // Texture to attach to the frambuffers
    GLuint mFboTexId[2];
    GLuint mReadOnlyTex;
    GLuint mFboId[2];
    GLint  mTextureUnits[3];

    int mWrite;
    int mRead;
    int mReadOnly;

    ReductionPhase(int width = 0, int height = 0);
    GLint init(GLuint fbos[], bool independent, GLuint &bfUsedTextures);

    void setupGeometry();
    virtual void run();
};

#endif // REDUCTIONPHASE_H
