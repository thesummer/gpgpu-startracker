 
#ifndef LOOKUPPHASE_H
#define LOOKUPPHASE_H

#include <stdio.h>

#include "phase.h"
#include "tga.h"
#include "getTime.h"

class LookupPhase: public Phase
{
public:
    // Vertex and fragment shader files
    const char * mVertFilename;
    const char * mFragFilename;

    // Handle to a program object
    GLuint mProgramObject;

    int mTexWidth;
    int mTexHeight;
    int mVertexWidth;
    int mVertexHeight;

    // Attribute locations
    GLint  mPositionLoc;

    // Vertices
    GLfloat*  mVertices;
    GLuint    mVboId;
    GLuint    mNumVertices;

    // Uniform locations
    GLint  u_texDimLoc;
    GLint  u_debugLoc;

    // Uniform values
    GLint u_debug;

    // Sampler location
    GLint mSamplerLoc;

    // Texture handle
    /// TODO: tga somewhere else?
    TGA    *mTgaImage;
    TGAData *mTgaData;

    // Texture to attach to the frambuffers
    GLuint mTexReducedId;
    GLuint mTexLookUpId;
    GLuint mFboId;
    GLint  mTextureUnits[1];

    LookupPhase(int texWidth = 0, int texHeight = 0, int vertexWidth = 1, int vertexHeight = 1);
    virtual ~LookupPhase();
    GLint init(GLuint &bfUsedTextures);
    GLint initIndependent(GLuint fbo, GLuint &bfUsedTextures);

    void setupGeometry();

    virtual double run();
};

#endif // LOOKUPPHASE_H
