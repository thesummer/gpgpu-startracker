#ifndef LABELPHASE_H
#define LABELPHASE_H

#include <stdio.h>

#include "phase.h"
#include "include/tga.h"
#include "include/getTime.h"


class LabelPhase: public Phase
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
    /// TODO: general shader?
    GLint  mPositionLoc;
    GLint  mTexCoordLoc;

    // Vertices
    GLfloat mVertices[20];
    GLushort mIndices[6];

    // Uniform locations
    GLint  u_texDimLoc;
    GLint  u_thresholdLoc;
    GLint  u_passLoc;
    GLint  u_debugLoc;
    GLint  u_factorLoc;

    // Uniform values
    float u_threshold;
    GLint u_pass;
    GLint u_debug;
    GLint u_factor;

    // Sampler location
    GLint mSamplerLoc;

    // Texture handle
    GLuint mTextureId;
    /// TODO: tga somewhere else?
    TGA    *mTgaImage;
    TGAData *mTgaData;

    // Texture to attach to the frambuffers
    GLuint mFboTexId[2];
    GLuint mFboId[2];

    int mWrite;
    int mRead;

    LabelPhase(int width = 0, int height = 0);
    GLint init(GLuint fbos[]);

    void setupGeometry();

    virtual void run();
};

#endif // LABELPHASE_H
