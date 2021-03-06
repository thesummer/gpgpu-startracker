 
#ifndef LOOKUPPHASE_H
#define LOOKUPPHASE_H
#include "CImg.h"
using namespace cimg_library;

#include "phase.h"
#include "getTime.h"
#include <stdio.h>

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
    CImg<unsigned char> mImage;

    // Texture to attach to the frambuffers
    GLuint mTexReducedId;
    GLuint mTexLookUpId;
    GLuint mFboId;
    GLint  mTextureUnits[2];

    LookupPhase(int texWidth = 0, int texHeight = 0, int vertexWidth = 1, int vertexHeight = 1);
    virtual ~LookupPhase();
    GLint init(GLuint &bfUsedTextures);
    GLint initIndependent(GLuint fbo, GLuint &bfUsedTextures);

    void setupGeometry();
    void updateTextures(GLuint reducedTex, GLint reducedTexUnit, GLuint freeTex, GLint freeTexUnit);
    void setFbo(GLuint newFbo);
    virtual double run();

    virtual void releaseGlResources();

};

#endif // LOOKUPPHASE_H
