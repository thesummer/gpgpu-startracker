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
#include "esUtil.h"

#define WIDTH 2
#define HEIGHT 2

typedef struct
{
   // Handle to a program object
   GLuint programObject;

   // Attribute locations
   GLint  positionLoc;
   GLint  texCoordLoc;

   // Sampler location
   GLint samplerLoc;

   // Texture handle
   GLuint textureId;

   // Framebuffer
   GLuint fboId ;

   // Texture to attach to the frambuffer
   GLuint fboTexId;

} UserData;

///
// Create a simple 2x2 texture image with four different colors
//
GLuint CreateSimpleTexture2D( )
{
   // Texture object handle
   GLuint textureId;
   
   // 4x4 Image, 4 bytes per pixel (R, G, B, A)
   GLubyte pixels[WIDTH * HEIGHT * 4];

   printf("Pixels before rendering:\n");
   for (GLubyte i=0; i<(WIDTH*HEIGHT*4); i++)
   {
       pixels[i] = i;
       printf("%d\n", i);
   }


   // Use tightly packed data
   GL_CHECK( glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 ) );

   // Generate a texture object
   GL_CHECK( glGenTextures ( 1, &textureId ) );

   // Bind the texture object
   GL_CHECK( glBindTexture ( GL_TEXTURE_2D, textureId ) );

   // Load the texture
   GL_CHECK( glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels ) );

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
   esContext->userData = malloc(sizeof(UserData));	
   UserData *userData = esContext->userData;

   // Load the shaders and get a linked program object
   userData->programObject = esLoadProgramFromFile( vertShaderFile, fragShaderFile );

   // Get the attribute locations
   userData->positionLoc = glGetAttribLocation ( userData->programObject, "a_position" );
   userData->texCoordLoc = glGetAttribLocation ( userData->programObject, "a_texCoord" );
   
   // Get the sampler location
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );

   // Load the texture
   userData->textureId = CreateSimpleTexture2D ();

   // Create a framebuffer object
   GL_CHECK( glGenFramebuffers(1, &(userData->fboId)) );

   // Create a texture for the frambuffer
   GL_CHECK( glGenTextures(1, &(userData->fboTexId)) );
   GL_CHECK( glBindTexture(GL_TEXTURE_2D, userData->fboTexId) );
   GL_CHECK( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, esContext->width, esContext->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL) );

   GL_CHECK( glBindFramebuffer(GL_FRAMEBUFFER, userData->fboId) );
   GL_CHECK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                          userData->fboTexId, 0) );

   GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   if (status != GL_FRAMEBUFFER_COMPLETE)
   {
       printf("Framebuffer is not complete\n");
   }

   GL_CHECK( glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f ) );

   return GL_TRUE;
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
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
      
   // Set the viewport
   GL_CHECK( glViewport ( 0, 0, esContext->width, esContext->height ) );
   
   // Clear the color buffer
   GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );

   // Use the program object
   GL_CHECK( glUseProgram ( userData->programObject ) );

   // Load the vertex position
   GL_CHECK( glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT,
                           GL_FALSE, 5 * sizeof(GLfloat), vVertices ) );
   // Load the texture coordinate
   GL_CHECK( glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                           GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] ) );

   GL_CHECK( glEnableVertexAttribArray ( userData->positionLoc ) );
   GL_CHECK( glEnableVertexAttribArray ( userData->texCoordLoc ) );

   // Bind the texture
   GL_CHECK( glActiveTexture ( GL_TEXTURE0 ) );
   GL_CHECK( glBindTexture ( GL_TEXTURE_2D, userData->textureId ) );

   // Set the sampler texture unit to 0
   GL_CHECK( glUniform1i ( userData->samplerLoc, 0 ) );

   GL_CHECK( glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices ) );

}

///
// Cleanup
//
void ShutDown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   // Delete texture object
   GL_CHECK( glDeleteTextures ( 1, &userData->textureId ) );
   GL_CHECK( glDeleteFramebuffers(1, &userData->fboId) );
   GL_CHECK( glDeleteTextures(1, &userData->fboTexId) );

   // Delete program object
   GL_CHECK( glDeleteProgram ( userData->programObject ) );
	
   free(esContext->userData);
}

int main ( int argc, char *argv[] )
{
   ESContext esContext;
   UserData  userData;


   esInitContext ( &esContext );
   esContext.userData = &userData;

   esCreateWindow ( &esContext, "Simple Texture 2D", WIDTH, HEIGHT, ES_WINDOW_RGB | ES_WINDOW_ALPHA);

   if ( !Init ( &esContext, "vertShader.glsl", "fragShader.glsl" ) )
       return 0;


   Draw( &esContext);

   // Make the BYTE array, factor of 3 because it's RBG.
   GLubyte* pixels = malloc(4*esContext.width*esContext.height*sizeof(GLubyte));

   GL_CHECK( glReadPixels(0, 0, esContext.width, esContext.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );

   printf("Pixels after rendering:\n");
   for (GLubyte i=0; i<(WIDTH*HEIGHT*4); i++)
   {
       printf("%d\n", pixels[i]);
   }


//   // Convert to FreeImage format & save to file
//   FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, esContext.width, esContext.height, 4 * esContext.width, 32, 0, 0, 0, 0);
//   FreeImage_Save(FIF_BMP, image, "./test.bmp", 0);

//   // Free resources
//   FreeImage_Unload(image);
//   free(pixels);

//   glBindFramebuffer(GL_FRAMEBUFFER, 0);
//   eglSwapBuffers(esContext.eglDisplay, esContext.eglSurface);
//   Draw();
//   sleep(2);

   ShutDown ( &esContext );

   return 0;
}
