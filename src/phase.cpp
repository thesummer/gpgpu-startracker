#include <fstream>
#include <stdio.h>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "phase.h"
#include "tga.h"

GLuint Phase::createSimpleTexture2D(GLsizei width, GLsizei height, GLubyte *data, GLint type)
{
    // Texture object handle
    GLuint textureId;

    // Use tightly packed data
    GL_CHECK( glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 ) );

    // Generate a texture object
    GL_CHECK( glGenTextures ( 1, &textureId ) );

    // Bind the texture object
    GL_CHECK( glBindTexture ( GL_TEXTURE_2D, textureId ) );
    // Load the texture
    GL_CHECK( glTexImage2D ( GL_TEXTURE_2D, 0, type, width, height, 0, type, GL_UNSIGNED_BYTE, data) );

    // Set the filtering mode
    GL_CHECK( glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ) );
    GL_CHECK( glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST ) );
    GL_CHECK( glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
    GL_CHECK( glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );

    return textureId;
}

GLuint Phase::loadProgramFromFile(const std::string vertShaderFile, const std::string fragShaderFile)
{
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject = 0;
    GLint linked;
    std::string sourceString = "";
    std::string commonSource;

    std::ifstream sourceFile("common.glsl");

    if( !sourceFile.good() )
    {
        cerr << "Failed to open file: common.glsl" << endl;
        sourceFile.close();
        goto error;
    }
    else
    {
        commonSource = std::string((std::istreambuf_iterator<char>(sourceFile)),
                                               std::istreambuf_iterator<char>());
    }

    sourceFile.close();

    sourceFile.open(vertShaderFile);

    if(! sourceFile.good() )
    {
        cerr << "Failed to open vertex shader file: " << vertShaderFile << endl;
        sourceFile.close();
        goto error;
    }
    sourceString = std::string((std::istreambuf_iterator<char>(sourceFile)),
                                           std::istreambuf_iterator<char>());

    sourceFile.close();

    // Load the vertex/fragment shaders
    vertexShader = loadShader( GL_VERTEX_SHADER, commonSource + sourceString );
    if ( vertexShader == 0 )
    {
        cerr << "Failed to compile vertex shader!" << endl;
        goto error;
    }
    sourceFile.open(fragShaderFile);
    if(! sourceFile.good() )
    {
        cerr << "Failed to open fragment shader file: " << fragShaderFile << endl;
        sourceFile.close();
        goto error;
    }
    sourceString = std::string((std::istreambuf_iterator<char>(sourceFile)),
                              std::istreambuf_iterator<char>());
    sourceFile.close();

    fragmentShader = loadShader(GL_FRAGMENT_SHADER, commonSource + sourceString );
    if ( fragmentShader == 0 )
    {
        cerr << "Failed to compile fragment shader" << endl;
        goto error;
    }

    // Create the program object
    programObject = glCreateProgram ( );
    if ( programObject == 0 )
       goto error;

    GL_CHECK( glAttachShader ( programObject, vertexShader ) );
    GL_CHECK( glAttachShader ( programObject, fragmentShader ) );

    // Link the program
    GL_CHECK( glLinkProgram ( programObject ) );

    // Check the link status
    GL_CHECK( glGetProgramiv ( programObject, GL_LINK_STATUS, &linked ) );

    if ( !linked )
    {
       GLint infoLen = 0;

       GL_CHECK( glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen ) );

       if ( infoLen > 1 )
       {
          char* infoLog = (char*) malloc (sizeof(char) * infoLen );

          GL_CHECK( glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog ) );
          cerr << "Error linking program:" << endl;
          cerr << infoLog << endl;

          free ( infoLog );
       }

       glDeleteProgram ( programObject );
       goto error;
    }

    // Free up no longer needed shader resources
    glDeleteShader ( vertexShader );
    glDeleteShader ( fragmentShader );

    return programObject;

 error:
    glDeleteShader ( vertexShader );
    glDeleteShader ( fragmentShader );
    return 0;
}

GLuint Phase::loadShader(GLenum type, const std::string &shaderSrc)
{
    GLuint shader;
    GLint compiled;

    // Create the shader object
    shader = glCreateShader ( type );

    if ( shader == 0 )
     return 0;

    // Load the shader source
    const GLchar * cstr = shaderSrc.c_str();
    GL_CHECK( glShaderSource ( shader, 1, &cstr, NULL ) );

    // Compile the shader
    GL_CHECK( glCompileShader ( shader ) );

    // Check the compile status
    GL_CHECK(glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled ) );

    if ( !compiled )
    {
       GLint infoLen = 0;

       GL_CHECK( glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen ) );

       if ( infoLen > 1 )
       {
          char* infoLog = (char*) malloc (sizeof(char) * infoLen );

          GL_CHECK( glGetShaderInfoLog ( shader, infoLen, NULL, infoLog ) );
          cerr << "Error compiling shader:" << endl;
          cerr << infoLog << endl;

          free ( infoLog );
       }

       GL_CHECK( glDeleteShader ( shader ) );
       return 0;
    }

    return shader;
}

void Phase::checkOpenGLError(const char *stmt, const char *fname, int line)
{
    GLenum err = glGetError();
    bool terminate = false;
    while (err != GL_NO_ERROR)
    {
        printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
        terminate = true;
        err = glGetError();
    }
    if(terminate) abort();
}

int Phase::writeTgaImage(int width, int height, char *filename, GLubyte *pixels)
{
    TGA *image = TGAOpen(filename, "w");
    if(!image || image->last != TGA_OK)
    {
        printf("Opening tga-file failed\n");
        return image->last;
    }

    image->hdr.height = height;
    image->hdr.width  = width;
    image->hdr.alpha  = 8;
    image->hdr.depth  = 32;
    image->hdr.img_t  = 2;
    image->hdr.id_len = 0;
    image->hdr.vert   = 1;
    image->hdr.horz   = 1;
    image->hdr.x      = 0;
    image->hdr.y      = 0;
    image->hdr.map_entry = 0;
    image->hdr.map_first = 0;
    image->hdr.map_len   = 0;
    image->hdr.map_t     = 0;

    TGAData data = {0, 0, 0, TGA_IMAGE_DATA | TGA_RGB};
    data.img_data = (tbyte*) malloc(4*sizeof(char) * width * height);

    for(int i=0; i<(4*width*height); i+=4)
    {
        unsigned test = *(unsigned*) &pixels[i];
        if( test == 0 )
        {
            data.img_data[i+0] = 0;
            data.img_data[i+1] = 0;
            data.img_data[i+2] = 0;
            data.img_data[i+3] = 255;
        }
        else
        {
            data.img_data[i+0] = ( (5*pixels[i+0] + 7*pixels[i+1] + 1) * (pixels[i+2] + pixels[i+3]+1) )%256;
            data.img_data[i+1] = ( (3*pixels[i+2] + 2*pixels[i+1] + 1) * (pixels[i+0] + pixels[i+3]+1) )%256;
            data.img_data[i+2] = ( pixels[i+0] + pixels[i+1] + pixels[i+2] + pixels[i+3])%256;
            data.img_data[i+3] = 255;
        }
    }

    if (TGAWriteImage(image, &data) != TGA_OK)
    {
        printf("Writing tga-file failed\n");
        return image->last;
    }
    // Add footer for tga 2.0 files
    fseek(image->fd, 0, SEEK_END);
    fwrite("\0\0\0\0\0\0\0\0TRUEVISION-XFILE.\0", 1, 26, image->fd);
    free(data.img_data);
    TGAClose(image);

    return 0;
}

int Phase::writeRawTgaImage(int width, int height, char *filename, GLubyte *pixels)
{
    TGA *image = TGAOpen(filename, "w");
    if(!image || image->last != TGA_OK)
    {
        printf("Opening tga-file failed\n");
        return image->last;
    }

    image->hdr.height = height;
    image->hdr.width  = width;
    image->hdr.alpha  = 8;
    image->hdr.depth  = 32;
    image->hdr.img_t  = 2;
    image->hdr.id_len = 0;
    image->hdr.vert   = 1;
    image->hdr.horz   = 1;
    image->hdr.x      = 0;
    image->hdr.y      = 0;
    image->hdr.map_entry = 0;
    image->hdr.map_first = 0;
    image->hdr.map_len   = 0;
    image->hdr.map_t     = 0;

    TGAData data = {0, 0, 0, TGA_IMAGE_DATA | TGA_RGB};
    data.img_data = pixels;

    if (TGAWriteImage(image, &data) != TGA_OK)
    {
        printf("Writing tga-file failed\n");
        return image->last;
    }
    // Add footer for tga 2.0 files
    fseek(image->fd, 0, SEEK_END);
    fwrite("\0\0\0\0\0\0\0\0TRUEVISION-XFILE.\0", 1, 26, image->fd);
    TGAClose(image);

    return 0;
}

void Phase::printLabels(int width, int height, GLubyte *pixels)
{
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            int index = 4*(i*width + j);
//            printf("%04x %04x | ", *(GLushort*) (pixels+index), *(GLushort*) (pixels+index+2) );
            printf("%4u %4u | ", *(GLushort*) (pixels+index), *(GLushort*) (pixels+index+2) );
        }
        printf("\n");
    }
}

void Phase::printSignedLabels(int width, int height, GLubyte *pixels)
{
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            int index = 4*(i*width + j);
            unsigned val1 = *(unsigned*) (pixels+index);
            int      sVal1;
            if(val1 & 0xFF000000)
            {
                val1 &= ~(0xFF000000);
//                val1 *= -1;
                sVal1 = -val1;
            }
            else
            {
               sVal1 = val1;
            }

//            printf("\t%08x\t | ",val1);
            printf(" %8d | ", sVal1);
        }
        printf("\n");
    }
}

int Phase::logBase2(int n)
{
    int ret = 0;
    while (n >= 1)
    {
        ret++;
        n = n >> 1;
    }

    return ret;
};

