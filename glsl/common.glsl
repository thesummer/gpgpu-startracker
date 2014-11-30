precision highp float;
precision highp sampler2D;
uniform vec2  u_texDimensions;   // image/texture dimensions

const float ZERO = 0.0;
const float ONE  = 1.0;
const float TWO  = 2.0;
const float bias = ONE/1024.0;
const float f256 = 256.0;
const float f255 = 255.0;


/*
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function packs 2 16-bit short integer values into the 4 texture channels.
They then can be used in subsequent shader operations or by the client after a call to glReadPixels.

in vec2:  it is assumed that each element contains an integer in float representation

return vec4: RGBA value which contains the packed shorts with LSB first.

*/
vec4 pack2shorts(in vec2 shorts)
{
    const vec4 bitSh = vec4(ONE/(f256),
                            ONE/(f256 * f256),
                            ONE/(f256),
                            ONE/(f256 * f256) );
    vec4 comp = fract(vec4(shorts.xx, shorts.yy) * bitSh);
    return floor(comp * f256) /f255;
}

/*
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function unpacks 2 16-bit short integer values from the 4 texture channels into 2 floats.

in vec4:  RGBA value to unpack with LSB first

return vec2: vector which will contain the 2 shorts

*/
vec2 unpack2shorts(in vec4 rgba)
{
    const highp vec2 bitSh = vec2(ONE, f256);
    vec4 rounded = floor((rgba * f255) + 0.5);
    return vec2(dot(rounded.xy, bitSh), dot(rounded.zw, bitSh));
}

vec4 packLong(in float uint32)
{
    float sign = ONE - step(ZERO, uint32);
    uint32 = floor(abs(uint32)+0.5);
    const vec3 bitSh = vec3(ONE/(f256),
                            ONE/(f256 * f256),
                            ONE/(f256 * f256 * f256) );
    vec4 comp;
    comp.xyz = fract( uint32 * bitSh );
    comp.xyz = floor(comp.xyz*f256)/f255;
    comp.w = sign;

    // floor needed by the rpi
    return comp;
}

float unpackLong(in vec4 rgba)
{
    float sign = step(0.5, rgba.w);
    vec3 rounded = floor((rgba.xyz * f255) + 0.5);
    const highp vec3 bitShifts = vec3(ONE,
                                f256,
                                f256 * f256);
    return floor(dot(rounded , bitShifts)+0.5) * (ONE-TWO*sign);
}

vec3 packSignedLong(in float int32)
{
    vec3 comp = packLong( abs(int32) );
    comp.z += (128.0/255.0) * step(0.5, -int32);
    return comp;
}

float unpackSignedLong(in vec3 rgb)
{
    float sign = step(0.5, rgb.z);
//    rgb.z -= sign* 128.0/255.0; doesn't work, floor needed by the rpi
    rgb.z = floor( (floor(rgb.z*f255+0.5) - 128.0 * sign+0.5))/f255;
    return unpackLong(rgb) * (ONE-TWO*sign) ;
}

float unpackSignedLong2(in vec3 rgb)
{
    float sign = step(0.5, rgb.z);
    rgb.z = floor( (floor(rgb.z*f255+0.5) - 128.0 * sign+0.5))/f255;

    float temp = unpackLong(rgb);
//    rgb.z = sign* 128.0/255.0; //doesn't work, floor needed by the rpi
//    rgb.z = floor( (floor(rgb.z*f255+0.5) - 128.0 * sign+0.5))/f255;
//    rgb.z = temp;
    return floor(unpackLong(rgb) * (ONE-TWO*sign)+0.5 ) ;
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
