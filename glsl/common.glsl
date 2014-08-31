precision highp float;
precision highp sampler2D;
uniform vec2  u_texDimensions;   // image/texture dimensions

const float ZERO = 0.0;
const float ONE  = 1.0;
const float TWO  = 2.0;
const float bias = ONE/1024.0;

/*
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function packs 2 16-bit short integer values into the 4 texture channels.
They then can be used in subsequent shader operations or by the client after a call to glReadPixels.

in vec2:  it is assumed that each element contains an integer in float representation

return vec4: RGBA value which contains the packed shorts with LSB first.

*/
vec4 pack2shorts(in vec2 shorts)
{
    // Correct for rounding errors due to the raspberry's limited precision (works at least between 0..2700)
    shorts = shorts/256.0 + bias;
    return vec4(floor(shorts)/255.0, fract(shorts)*256.0/255.0).zxwy;
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
    return floor(vec2(rgba.xz * 255.0 + 255.0*256.0 * rgba.yw)+0.5);
}

vec4 pack2signed( in vec2 signed)
{
    // Build the bytes with pack2shorts and flip the MSbit if the sign is negative
    // No range checks are performed
    vec4 result = pack2shorts(signed);
    result.yw += 128.0 * step(ZERO, signed);
    return result;
}

vec2 unpack2signed(in vec4 rgba)
{
    // Correct the MSbit unpack using unpack2shorts and
    // apply the signs.
    vec2 signs = step(0.5, rgba.yw);
    rgba.yw -= 128.0/255.0 * signs;
    return unpack2shorts(rgba) * (ONE-2*signs) ;
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
    return floor( (TWO*texCoord*u_texDimensions-ONE)/TWO+0.5 );
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
    return (TWO*imgCoord + ONE)/(TWO*u_texDimensions);
}
 
