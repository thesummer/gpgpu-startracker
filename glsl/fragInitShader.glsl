

precision highp float;
varying vec2 v_texCoord;        // texture coordinates
uniform sampler2D s_texture;    // texture sampler
uniform vec2 u_texDimensions;   // image/texture dimensions
uniform float u_threshold;      // threshold value for the threshold operation

/*
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function packs 2 16-bit short integer values into the 4 texture channels.
They then can be used in subsequent shader operations or by the client after a call to glReadPixels.

in vec2:  it is assumed that each element contains an integer in float representation

return vec4: RGBA value which contains the packed shorts.

*/
vec4 pack2shorts(in vec2 shorts)
{
    // Correct for rounding errors due to the raspberry's limited precision works at least between 0..2700
    const float bias = 1.0/1024.0;
    shorts = shorts/256.0 + bias;
    return vec4(floor(shorts)/255.0, fract(shorts)*256.0/255.0).zxwy;
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
    return floor((2.0*texCoord*u_texDimensions-1.0)/2.0+0.5);
}


/*
Shader will do the following steps for each pixel:
    1. Threshold with u_threshold
    2. pack image coordinates for into RGBA value for non-zero pixels

    It is assumed that the input texture is a gray scale RGBA-image (c,c,c,255)
*/
void main()
{
    // Compute image coordinates
    vec2 imgCoord = tex2imgCoord(v_texCoord);

    // Get the input pixel value
    gl_FragColor = texture2D( s_texture, v_texCoord );

    // Threshold operation
    gl_FragColor.a = gl_FragColor.r;
    gl_FragColor = step(u_threshold, gl_FragColor);

    // pack image coordinates for the non-zero pixels
    gl_FragColor = pack2shorts(imgCoord) * gl_FragColor;
}
