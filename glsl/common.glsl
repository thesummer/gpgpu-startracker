precision highp float;
precision highp sampler2D;

/*!
 * Common functions for packing and unpacking
 * of integer values and computation of texture and image coordinates.
 * @author Jan Sommer
 * @date 2014
 * @namespace GLSL::COMMON
 * @class CommonFunctions
 */

uniform vec2  u_texDimensions;   /*!< Dimensions of the image in pixels */
const float ZERO = 0.0;          /*!< Constant for 0.0 otherwise memory is reserved for every literal */
const float ONE  = 1.0;          /*!< Constant for 1.0 otherwise memory is reserved for every literal*/
const float TWO  = 2.0;          /*!< Constant for 2.0 otherwise memory is reserved for every literal*/
const float f256 = 256.0;        /*!< Constant for 256.0 otherwise memory is reserved for every literal */
const float f255 = 255.0;        /*!< Constant for 256.0 otherwise memory is reserved for every literal*/


/*!
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function packs 2 16-bit unsigned short integer values into the 4 texture channels.
They can be used in subsequent shader operations or by the client after a call to glReadPixels.

\param shorts  the two unsigned integer as float vectors

\return RGBA value which contains the packed shorts with LSB first.

*/
vec4 pack2shorts(in vec2 shorts)
{
    shorts = floor(shorts+0.5);
    const vec4 bitSh = vec4(ONE/(f256),
                            ONE/(f256 * f256),
                            ONE/(f256),
                            ONE/(f256 * f256) );
    vec4 comp = fract(vec4(shorts.xx, shorts.yy) * bitSh);
    return floor(comp * f256) /f255;
}

/*!
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function unpacks 2 16-bit short integer values from the 4 texture channels into 2 floats.

\param rgba RGBA value which contains 2 packed unsigned shorts with LSB first

\return float vector which will contain the 2 unpacked shorts

*/
vec2 unpack2shorts(in vec4 rgba)
{
    const highp vec2 bitSh = vec2(ONE, f256);
    vec4 rounded = floor((rgba * f255) + 0.5);
    return vec2(dot(rounded.xy, bitSh), dot(rounded.zw, bitSh));
}

/*!
  Assuming that the texture is 8-bit RGBA 32bits are available for packing.
  This function uses the first 24bits to pack a long integer (24bit)
  The last Byte is used to store the sign of the integer, hence in total
  a 25bit signed integer is packed.

  \param int32 the signed integer as float
  \result RGBA value with the packed integer representation
*/
vec4 packLong(in float int32)
{
    float sign = ONE - step(ZERO, int32);
    int32 = floor(abs(int32)+0.5);
    const vec3 bitSh = vec3(ONE/(f256),
                            ONE/(f256 * f256),
                            ONE/(f256 * f256 * f256) );
    vec4 comp;
    comp.xyz = fract( int32 * bitSh );
    comp.xyz = floor(comp.xyz*f256)/f255;
    comp.w = sign;

    // floor needed by the rpi
    return comp;
}

/*!
  Assuming that the texture is 8-bit RGBA 32bits are available for packing.
  This function uses the first 24bits to unpack a long integer (24bit)
  The last Byte stores the sign of the integer, hence in total
  a 25bit signed integer is unpacked.

  \param rgba the RGBA value which has the packed integer
  \return the signed integer as float

*/
float unpackLong(in vec4 rgba)
{
    float sign = step(0.5, rgba.w);
    vec3 rounded = floor((rgba.xyz * f255) + 0.5);
    const highp vec3 bitShifts = vec3(ONE,
                                f256,
                                f256 * f256);
    return floor(dot(rounded , bitShifts)+0.5) * (ONE-TWO*sign);
}

/*!
This function computes the image coordinates of the texture with the dimensions from u_texDimensions
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

\param texCoord vector with texture coordinates in [0,1], [0,1]
\return vector with corresponding image coordinates [0, width], [0, height]

*/
vec2 tex2imgCoord(in vec2 texCoord)
{
    return floor( (TWO*texCoord*u_texDimensions-ONE)/TWO+0.5 );
}


/*!
Reverse function of \ref tex2imgCoord. Takes image coordinates and
returns the  corresponding texture coordinates.

        tx = (2*px+1)/2dimX
        ty = (2*py+1)/2dimY

\param imgCoord image coordinates in [0, width], [0, height]
\return texture coordinates in [0,1], [0,1]
*/
vec2 img2texCoord(in vec2 imgCoord)
{
    return (TWO*imgCoord + ONE)/(TWO*u_texDimensions);
}
