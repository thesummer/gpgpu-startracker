//
// Book:      OpenGL(R) ES 2.0 Programming Guide
// Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
// ISBN-10:   0321502795
// ISBN-13:   9780321502797
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780321563835
//            http://www.opengles-book.com
//

// Simple_Texture2D.c
//
//    This is a simple example that draws a quad with a 2D
//    texture image. The purpose of this example is to demonstrate 
//    the basics of 2D texturing
//
#include <stdlib.h>
#include <stdio.h>
#include <FreeImage.h>
#include "include/tga.h"
#include "esUtil.h"

#define NUM_FBO 2
#define NUM_TEX 2

typedef struct
{
    // Handle to a program object
    GLuint programObject;

    // Attribute locations
    GLint  positionLoc;
    GLint  texCoordLoc;

    // Uniform locations
    GLint  u_texDimLoc;
    GLint  u_thresholdLoc;
    GLint  u_passLoc;

    // Uniform values
    float u_threshold;
    GLint u_pass;
    // Sampler location
    GLint samplerLoc;

    // Texture handle
    GLuint textureId;
    TGA    *tgaImage;
    TGAData *tgaData;

    // Framebuffer
    GLuint fboId[NUM_FBO] ;
    GLuint write;
    GLuint read;

    // Texture to attach to the frambuffer
    GLuint fboTexId[NUM_FBO];

} UserData;


void createRuns(ESContext * esContext);

///
// Create a simple 2x2 texture image with four different colors
//
GLuint CreateSimpleTexture2D(TGA *image, tbyte *imgData)
{
    // Texture object handle
    GLuint textureId;

    TGAHeader *header = &image->hdr;

    // Use tightly packed data
    GL_CHECK( glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 ) );

    // Generate a texture object
    GL_CHECK( glGenTextures ( 1, &textureId ) );

    // Bind the texture object
    GL_CHECK( glBindTexture ( GL_TEXTURE_2D, textureId ) );
    // Load the texture
    GL_CHECK( glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, header->width, header->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData) );

    // Set the filtering mode
    GL_CHECK( glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ) );
    GL_CHECK( glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST ) );

    return textureId;
}


///
// Initialize the shader and program object
//
int Init ( ESContext *esContext, const char* vertShaderFile, const char* fragShaderFile)
{
    UserData *userData = esContext->userData;

    // Load the shaders and get a linked program object
    userData->programObject = esLoadProgramFromFile( vertShaderFile, fragShaderFile );

    // Get the attribute locations
    userData->positionLoc = glGetAttribLocation ( userData->programObject, "a_position" );
    userData->texCoordLoc = glGetAttribLocation ( userData->programObject, "a_texCoord" );

    // Get the sampler location
    userData->samplerLoc      = glGetUniformLocation ( userData->programObject, "s_texture" );
    userData->u_texDimLoc     = glGetUniformLocation ( userData->programObject, "u_texDimensions" );
    userData->u_thresholdLoc  = glGetUniformLocation ( userData->programObject, "u_threshold" );
    userData->u_passLoc       = glGetUniformLocation ( userData->programObject, "u_pass" );


    // Create a framebuffer object
    GL_CHECK( glGenFramebuffers(NUM_FBO, userData->fboId) );

    // Create a texture for the frambuffer
    userData->fboTexId[userData->read]  = CreateSimpleTexture2D(userData->tgaImage, userData->tgaData->img_data);
    userData->fboTexId[userData->write] = CreateSimpleTexture2D(userData->tgaImage, NULL);

    // Bind the textures to the corresponding fbo
    for(int i=0; i<NUM_FBO; ++i)
    {
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, userData->fboId[i]) );
        GL_CHECK( glActiveTexture( GL_TEXTURE0 + i) );
        GL_CHECK( glBindTexture(GL_TEXTURE_2D, userData->fboTexId[i]) );
        GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, userData->fboTexId[i], 0) );
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            printf("Framebuffer is not complete\n");
        }
    }

    GL_CHECK( glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f ) );

    return GL_TRUE;
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw ( ESContext *esContext )
{
//    UserData *userData = esContext->userData;
}

///
// Cleanup
//
void ShutDown ( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    // Delete texture object
    GL_CHECK( glDeleteTextures ( 1, &userData->textureId ) );
    GL_CHECK( glDeleteFramebuffers(NUM_FBO, userData->fboId) );
    GL_CHECK( glDeleteTextures(NUM_FBO, userData->fboTexId) );

    // Delete program object
    GL_CHECK( glDeleteProgram ( userData->programObject ) );

    // Free memory of the image data
    if(userData->tgaData->img_data != NULL)
        free(userData->tgaData->img_data);
    if(userData->tgaData->cmap != NULL)
        free(userData->tgaData->cmap);
    if(userData->tgaData->img_id != NULL)
        free(userData->tgaData->img_id);

    TGAClose (userData->tgaImage);

    //   free(esContext->userData); not necessary
}

void printLabels(TGAHeader *header, GLubyte *pixels)
{
    for(int i=0; i<header->height; i++)
    {
        for(int j=0; j<header->width; j++)
        {
            int index = 4*(i*header->width + j);
            printf("%4d %4d | ", *(GLushort*) (pixels+index), *(GLushort*) (pixels+index+2) );
        }
        printf("\n");
    }
}

int main ( int argc, char *argv[] )
{

    TGA *tgaImage = TGAOpen("test.tga", "r");
    TGAData tgaData = {0, 0, 0, TGA_IMAGE_DATA | TGA_RGB};

    ESContext esContext;
    UserData  userData;

    esInitContext ( &esContext );
    esContext.userData = &userData;
    userData.read  = 0;
    userData.write = 1;


    if(!tgaImage || tgaImage->last != TGA_OK)
    {
        printf("Opening tga-file failed\n");
        return 1;
    }

    if(TGAReadImage(tgaImage, &tgaData) == TGA_OK)
    {
        printf("tga-file successfully read\n");
    }

    TGAHeader *header = &tgaImage->hdr;
    printf("image dimensions:\n width: %d\t height:%d\t depth:%d bpp\n",
           header->width, header->height, header->depth);

    userData.tgaImage = tgaImage;
    userData.tgaData  = &tgaData;
    userData.u_threshold = 128 / 255.0;

    esCreateWindow ( &esContext, "Simple Texture 2D", header->width, header->height, ES_WINDOW_RGB | ES_WINDOW_ALPHA);
    if ( !Init ( &esContext, "vertShader.glsl", "fragCreateRuns.glsl" ) )
        return 0;

    printf("Pixels before rendering:\n");
    tbyte *img = tgaData.img_data;
    for(int i=0; i<header->height; i++)
    {
        for(int j=0; j<header->width; j++)
        {
            int index = 4*(i*header->width + j);
            printf("%3d %3d %3d %3d | ", img[index], img[index+1], img[index+2], img[index+3]);
        }
        printf("\n");
    }

    createRuns(&esContext);

    ShutDown ( &esContext );

    return 0;
}

int logBase2(int n)
{
    int ret = 0;
    while (n >= 1)
    {
        ret++;
        n = n >> 1;
    }

    return ret;
}

void createRuns(ESContext * esContext)
{
    UserData *userData = esContext->userData;

    // Setup geometry
    GLfloat vVertices[] = { -1.0f, -1.0f, 0.0f,  // Position 0
                            0.0f,  0.0f,        // TexCoord 0
                            -1.0f,  1.0f, 0.0f,  // Position 1
                            0.0f,  1.0f,        // TexCoord 1
                            1.0f,  1.0f, 0.0f,  // Position 2
                            1.0f,  1.0f,        // TexCoord 2
                            1.0f, -1.0f, 0.0f,  // Position 3
                            1.0f,  0.0f         // TexCoord 3
                          };
    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

    // Setup OpenGL

    // Set the viewport
    GL_CHECK( glViewport ( 0, 0, esContext->width, esContext->height ) );
    // Clear the color buffer
    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );
    // Use the program object
    GL_CHECK( glUseProgram ( userData->programObject ) );
    // Load the vertex position
    GL_CHECK( glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), vVertices ) );
    // Load the texture coordinates
    GL_CHECK( glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] ) );
    GL_CHECK( glEnableVertexAttribArray ( userData->positionLoc ) );
    GL_CHECK( glEnableVertexAttribArray ( userData->texCoordLoc ) );

    // Set the uniforms
    GL_CHECK( glUniform2f ( userData->u_texDimLoc, userData->tgaImage->hdr.width, userData->tgaImage->hdr.height) );
    GL_CHECK( glUniform1f ( userData->u_thresholdLoc, userData->u_threshold) );

    // Do the runs

    // Make the BYTE array, factor of 3 because it's RGBA.
    GLubyte* pixels = malloc(4*esContext->width*esContext->height*sizeof(GLubyte));

    for (int i = 0; i < logBase2(esContext->height); i++)
    {
        userData->u_pass = i;

        // Bind the FBO to write to
        GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, userData->fboId[userData->write]) );
        // Set the sampler texture unit to 0
        GL_CHECK( glUniform1i ( userData->samplerLoc, userData->read ) );
        // Set the pass index
        GL_CHECK( glUniform1i ( userData->u_passLoc, userData->u_pass) );
        // Draw scene
        GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices ) );

        GL_CHECK( glReadPixels(0, 0, esContext->width, esContext->height, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );

        printf("Pixels after pass %d:\n", userData->u_pass);
        printLabels(&userData->tgaImage->hdr, pixels);

        // Switch read and write texture
        userData->read  = 1 - userData->read;
        userData->write = 1 - userData->write;
    }

    free(pixels);
}
