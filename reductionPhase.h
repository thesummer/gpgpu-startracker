#ifndef REDUCTIONPHASE_H
#define REDUCTIONPHASE_H

#include "include/tga.h"
#include "phase.h"

#define HORIZONTAL 0
#define VERTICAL   1

#define TEX_LABEL  0
#define TEX_ROOT   1
#define TEX_PIPO   2

#define MODE_RUNNING_SUM     0
#define MODE_BINARY_SEARCH   1
#define MODE_ROOT_INIT       2

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
    GLint  u_stageLoc;
    GLint  u_directionLoc;

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
    GLuint mTexPiPoId[2];
    GLuint mTexLabelId;
    GLuint mTexRootId;

    GLuint mFboId[2];
    GLint  mTextureUnits[4];

    int mWrite;
    int mRead;
    int mReadOnly;

    ReductionPhase(int width = 0, int height = 0);
    GLint init(GLuint fbos[], bool independent, GLuint &bfUsedTextures);

    void setupGeometry();
    virtual void run();

private:
    void reduce(int length);
};

#endif // REDUCTIONPHASE_H
