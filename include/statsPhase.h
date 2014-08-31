#ifndef LABELPHASE_H
#define LABELPHASE_H

#include <stdio.h>

#include "phase.h"
#include "tga.h"
#include "getTime.h"

class StatsPhase: public Phase
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
    GLint u_texDimLoc;
    GLint u_passLoc;
    GLint u_stageLoc;
    GLint u_savingOffsetLoc;
    GLint u_factorLoc;
//    GLint  u_debugLoc;

    // Uniform values
    GLint u_pass;
//    GLint u_debug;
//    GLint u_factor;

    // Sampler location
    GLint s_labelLoc;
    GLint s_fillLoc;
    GLint s_resultLoc;
    GLint s_origLoc;

    // Texture handle
    /// TODO: tga somewhere else?
    TGA    *mTgaImage;
    TGAData *mTgaLabel;
    TGAData *mTgaReduced;
    TGAData *mTgaOrig;

    // Texture to attach to the frambuffers
    GLuint mTexOrigId;
    GLuint mTexLabelId;
    GLuint mTexReducedId;
    GLuint mTexFillId;
    GLuint mTexPiPoId[2];
    GLuint mFboId[2];
    GLint  mTextureUnits[6];

    int mWrite;
    int mRead;

    StatsPhase(int width = 0, int height = 0);
    virtual ~StatsPhase();
    GLint init(GLuint fbos[], GLuint &bfUsedTextures);
    GLint initIndependent(GLuint fbos[], GLuint &bfUsedTextures);

//    GLint getLastTexture();
//    GLint getLastTexUnit();
//    GLint getFreeTexture();
//    GLint getFreeTexUnit();

    void setupGeometry();

    virtual double run();

private:
    void fillStage(float factorX, float factorY);
    void countStage();
    void saveStage(float factorX, float factorY, float savingOffset);
};

#endif // LABELPHASE_H
