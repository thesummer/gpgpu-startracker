

precision highp float;
varying vec2 v_texCoord;        // texture coordinates
uniform sampler2D s_texture;    // texture sampler
uniform vec2 u_texDimensions;   // image/texture dimensions
uniform int    u_forward;       // forward or backward mask operation

/*
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function packs 2 16-bit short integer values into the 4 texture channels.
They then can be used in subsequent shader operations or by the client after a call to glReadPixels.

in vec2:  it is assumed that each element contains an integer in float representation

return vec4: RGBA value which contains the packed shorts with LSB first.

*/
vec4 pack2shorts(in vec2 shorts)
{
    shorts /= 256.0f;
    return vec4(floor(shorts)/255.0f, fract(shorts)*256.0f/255.0f).zxwy;
}

/*
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function unpacks 2 16-bit short integer values from the 4 texture channels into 2 floats.

in vec4:  RGBA value to unpack with LSB first

return vec2: vector which will contain the 2 shorts

*/
vec2 unpack2shorts(in vec4 rgba)
{
    // LSB * 255 + MSB * 255*256
    return vec2(rgba.xz * 255.0f + 255.0f*256.0f * rgba.yw);
}

/*
This functions computes the image coordinates of the texture with the dimensions from u_texDimensions
Example:

For an 8 pixel texture (8x1) the texture coordinates are in [0.0, 1.0].
glNearest will yield texture coordinates which are  in the center of each pixel (e.g. 1/16, 3/16 etc).

 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
 ^   ^   ^   ^   ^   ^   ^   ^   ^
0.0  |   |   |   |   |   |   |  1.0
 |   |   |   |   |   |   |   |   |
0/8 1/8 2/8 3/8 4/8 5/8 6/8 7/8 8/8

Therefore, in order to get the pixel coordinates the following formular is used:

px = (2*tx*dimX-1)/2
py = (2*ty*dimY-1)/2

in vec2: vector with texture coordinates
return vec2: vecture with image coordinates

*/
vec2 tex2imgCoord(in vec2 texCoord)
{
    return (2.0f*texCoord*u_texDimensions-1.0f)/2.0f;
}


/*
Reverse function of tex2imgCoord. Takes image coordinates and
return the  corresponding texture coordinates.

tx = (2*px+1)/2dimX
ty = (2*py+1)/2dimY

in vec2: image coordinates in [0, width], [0, height]
return vec2: texture coordinates in [0,1], [0,1]
*/
vec2 img2texCoord(in vec2 imgCoord)
{
    return (2.0f*imgCoord + 1.0f)/(2.0f*u_texDimensions);
}


void getForwardMask(in vec2 imgCoord, out vec4 outX, out vec4 outY)
{

    vec4 tempRGBA  = texture2D( s_texture, img2texCoord(imgCoord + vec2(1.0f, 0.0f) )  );
    vec2 tempLabel = unpack2shorts(tempRGBA);
    outX.x = tempLabel.x;
    outY.x = tempLabel.y;

    vec4 tempRGBA  = texture2D( s_texture, img2texCoord(imgCoord + vec2(-1.0f, 1.0f) )  );
    vec2 tempLabel = unpack2shorts(tempRGBA);
    outX.y = tempLabel.x;
    outY.y = tempLabel.y;

    vec4 tempRGBA  = texture2D( s_texture, img2texCoord(imgCoord + vec2(0.0f, 1.0f) )  );
    vec2 tempLabel = unpack2shorts(tempRGBA);
    outX.z = tempLabel.x;
    outY.z = tempLabel.y;

    vec4 tempRGBA  = texture2D( s_texture, img2texCoord(imgCoord + vec2(1.0f, 1.0f) )  );
    vec2 tempLabel = unpack2shorts(tempRGBA);
    outX.w = tempLabel.x;
    outY.w = tempLabel.y;
}


void getBackwardMask(in vec2 imgCoord, out vec4 outX, out vec4 outY)
{

    vec4 tempRGBA  = texture2D( s_texture, img2texCoord(imgCoord - vec2(1.0f, 0.0f) )  );
    vec2 tempLabel = unpack2shorts(tempRGBA);
    outX.x = tempLabel.x;
    outY.x = tempLabel.y;

    vec4 tempRGBA  = texture2D( s_texture, img2texCoord(imgCoord - vec2(-1.0f, 1.0f) )  );
    vec2 tempLabel = unpack2shorts(tempRGBA);
    outX.y = tempLabel.x;
    outY.y = tempLabel.y;

    vec4 tempRGBA  = texture2D( s_texture, img2texCoord(imgCoord - vec2(0.0f, 1.0f) )  );
    vec2 tempLabel = unpack2shorts(tempRGBA);
    outX.z = tempLabel.x;
    outY.z = tempLabel.y;

    vec4 tempRGBA  = texture2D( s_texture, img2texCoord(imgCoord - vec2(1.0f, 1.0f) )  );
    vec2 tempLabel = unpack2shorts(tempRGBA);
    outX.w = tempLabel.x;
    outY.w = tempLabel.y;
}

/*
Shader will do the following steps for each pixel:
    1. Get current pixel label
    2. get labels of forward or backward mask
    3. search for the smallest label
    4. assign smallest label to pixel

    It is assumed that the input texture is a gray scale RGBA-image (c,c,c,255)
*/
void main()
{
    // Compute image coordinates
    vec2 imgCoord = tex2imgCoord(v_texCoord);

    // Get the input pixel value
    vec4 currentLabel = texture2D( s_texture, v_texCoord );
    vec4 xLabels, yLabels;

    if(u_forward == 1)
    {
        getForwardMask(imgCoord, xLabels, yLabels);
    }
    else
    {
        getBackwardMask(imgCoord, xLabels, yLabels);
    }

    // pack image coordinates for the non-zero pixels
    gl_FragColor = pack2shorts(imgCoord) * gl_FragColor;
}
