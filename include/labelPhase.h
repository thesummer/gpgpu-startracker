#ifndef LABELPHASE_H
#define LABELPHASE_H

#include <stdio.h>

#include "phase.h"
#include "tga.h"
#include "getTime.h"

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
    /// TODO: tga somewhere else?
    TGA    *mTgaImage;
    TGAData *mTgaData;

    // Texture to attach to the frambuffers
    GLuint mTexOrigId;
    GLuint mTexPiPoId[2];
    GLuint mFboId[2];
    GLint  mTextureUnits[3];

    int mWrite;
    int mRead;

    LabelPhase(int width = 0, int height = 0);
    virtual ~LabelPhase();
    GLint init(GLuint fbos[], GLuint &bfUsedTextures);
    GLint initIndependent(GLuint fbos[], GLuint &bfUsedTextures);

    GLint getLastTexture();
    GLint getLastTexUnit();
    GLint getFreeTexture();
    GLint getFreeTexUnit();

    void setupGeometry();

    virtual double run();
};

#endif // LABELPHASE_H
