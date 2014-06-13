precision highp float;
varying vec2 v_texCoord;        // texture coordinates
uniform sampler2D s_texture;    // texture sampler
uniform vec2 u_texDimensions;   // image/texture dimensions
uniform int u_pass;
// uniform int u_debug;

const float ZERO = 0.0;
const float ONE  = 1.0;
const float TWO  = 2.0;

/*
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function packs 2 16-bit short integer values into the 4 texture channels.
They then can be used in subsequent shader operations or by the client after a call to glReadPixels.

in vec2:  it is assumed that each element contains an integer in float representation

return vec4: RGBA value which contains the packed shorts with LSB first.

*/
vec4 pack2shorts(in vec2 shorts)
{
    shorts /= 256.0;
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
    return vec2(rgba.xz * 255.0 + 255.0*256.0 * rgba.yw);
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
    return (TWO*texCoord*u_texDimensions-ONE)/TWO;
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

void main()
{
    // initial scan
    if(u_pass == 0)
    {
        vec4 pixelCol = texture2D( s_texture, v_texCoord );
        vec2 curLabel = unpack2shorts(pixelCol);

        // Check if the current pixel is a root pixel of a certain spot
        float isRoot = float( all( equal(curLabel, tex2imgCoord(v_texCoord) + ONE ) ) );


        // count == 1.0 if curPixelCol is not root pixel or zero and 0.0 otherwise
        float count = ONE - step(ONE/256.0, length(pixelCol * isRoot ) );

        // Check if pixel to the left is zero
        vec2 coord = tex2imgCoord(v_texCoord);
        coord = img2texCoord( coord + vec2(-ONE, ZERO) );
        pixelCol = texture2D( s_texture, coord);
        curLabel = unpack2shorts(pixelCol);
        isRoot = float( all( equal(curLabel, tex2imgCoord(coord) + ONE ) ) );

        // Add 1.0 to count if pixel to the left is 0.0 or not root, make sure not to read outside of texture
        count += ( ONE - step(ONE/256.0, length(pixelCol * isRoot) ) ) * step(ZERO, coord.x);

        // Save the number of zero or non-root pixel from this and left neighbor pixel (0.0, 1.0 or 2.0)
        gl_FragColor = pack2shorts(vec2(count, ZERO));
    }
    else
    {
        vec4 pixelCol = texture2D( s_texture, v_texCoord );
        float count = unpack2shorts(pixelCol).x;

        vec2 coord = tex2imgCoord(v_texCoord) - vec2(exp2( float(u_pass) ), ZERO);
        coord =  img2texCoord( coord );
        pixelCol = texture2D( s_texture, coord );

        // Add the value 2^u_pass to the left (filter out values outside of texture range)
        count += unpack2shorts(pixelCol).x * step(ZERO, coord.x);

        gl_FragColor = pack2shorts(vec2(count, ZERO));
    }
}
