precision highp float;
varying vec2 v_texCoord;        // texture coordinates
uniform sampler2D s_texture;    // texture sampler
uniform sampler2D s_values;
uniform vec2 u_texDimensions;   // image/texture dimensions
uniform int u_stage;            // different stages
uniform int u_pass;
uniform int u_direction;
uniform int u_debug;

#define RUNNING_SUM     0
#define BINARY_SEARCH   1
#define ROOT_INIT       2

#define HORIZONTAL      0
#define VERTICAL        1

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

void runningSum()
{
    vec2 coord ;
    float withinBounds;
    if(u_direction == HORIZONTAL)
    {
        coord = tex2imgCoord(v_texCoord) - vec2(exp2( float(u_pass) ), ZERO);
        withinBounds = step(ZERO, coord.x);
    }
    else // VERTICAL
    {
        coord = tex2imgCoord(v_texCoord) - vec2( ZERO, exp2( float(u_pass) ) );
        withinBounds = step(ZERO, coord.y);
    }

    coord = img2texCoord( coord );

    if(u_pass == 0)
    {
        vec4 pixelCol = texture2D( s_values, v_texCoord );
        vec2 curLabel = unpack2shorts(pixelCol);

        // count == 1.0 if curPixelCol is not root pixel or zero and 0.0 otherwise
        float count = ONE - step(ONE/256.0, length(pixelCol) );

        // Check if pixel to the left is zero
        pixelCol = texture2D( s_values, coord);
        curLabel = unpack2shorts(pixelCol);

        // Add 1.0 to count if pixel to the left is 0.0 or not root, make sure not to read outside of texture
        count += ( ONE - step(ONE/256.0, length(pixelCol) ) ) * withinBounds;

        // Save the number of zero or non-root pixel from this and left neighbor pixel (0.0, 1.0 or 2.0)
        gl_FragColor = pack2shorts(vec2(count, ZERO));
    }
    else if(u_pass > 0)
    // Add up the zeroes for all pixels to the left
    {
        vec4 pixelCol = texture2D( s_texture, v_texCoord );
        float count = unpack2shorts(pixelCol).x;

        pixelCol = texture2D( s_texture, coord );

        // Add the value 2^u_pass to the left (filter out values outside of texture range)
        count += unpack2shorts(pixelCol).x * withinBounds;
        gl_FragColor = pack2shorts(vec2(count, exp2( float(u_pass)-ONE )) );

    }
}

void binarySearch()
{
    // 1. Get the last guess for the current texel
    vec2 current = unpack2shorts(texture2D(s_texture, v_texCoord ) );
    float lastGuess = current.y;
    vec2  coord;
    if(u_direction == HORIZONTAL)
        coord = tex2imgCoord(v_texCoord) + vec2(lastGuess, ZERO);
    else // VERTICAL
        coord = tex2imgCoord(v_texCoord) + vec2(ZERO, lastGuess);

    if(u_pass > 0)
    // gather search after scan
    {
        /*
           Find the texel which which running sum is equal its distance to the current texel
           by using a binary search:
        */
        //   2. Get the value of the current guess
        coord = img2texCoord(coord);
        float guess = unpack2shorts( texture2D(s_texture, coord ) ).x;
        vec2  value = unpack2shorts( texture2D(s_values,  coord ) );

        // Version with if:
        float outGuess;

        if(guess > lastGuess)
        {
            outGuess = lastGuess + exp2(float(u_pass)-ONE);
        }
        else if(guess == lastGuess && length(value) > ZERO)
        {
            outGuess = lastGuess;
        }
        else
        {
            outGuess = lastGuess - exp2(float(u_pass)-ONE);
        }

        gl_FragColor = pack2shorts( vec2(current.x, outGuess) );

        // Version without if:
//        float factor = TWO * float(guess > lastGuess) - ONE + float(guess==lastGuess) * step(ONE/256.0, length(value) ) ;
//        gl_FragColor = pack2shorts( vec2( current.x, lastGuess+factor*exp2(-float(u_pass)-TWO) ) );



    }
    else if(u_pass == 0)
    {
        coord = img2texCoord(coord);

        float guess = unpack2shorts( texture2D(s_texture, coord ) ).x;
        vec2  value = unpack2shorts( texture2D(s_values,  coord ) );

        // Version with if:
        float outGuess;

        if(guess > lastGuess)
        {
            outGuess = lastGuess + ONE;
        }
        else if(guess == lastGuess && length(value) > ZERO)
        {
            outGuess = lastGuess;
        }
        else
        {
            outGuess = lastGuess - ONE;
        }

        // Final assignment
        if(u_direction == HORIZONTAL)
        {
            coord = tex2imgCoord(v_texCoord) + vec2(outGuess, ZERO);
        }
        else // VERTICAL
        {
            coord = tex2imgCoord(v_texCoord) + vec2(ZERO, outGuess);
        }
        float withinBounds = float( (u_texDimensions[u_direction]-coord[u_direction]) > ZERO );
        coord = img2texCoord(coord);

        gl_FragColor = texture2D(s_values, coord) * withinBounds;
//        gl_FragColor = vec4(withinBounds);
    }
}

void main()
{
    if(u_stage == RUNNING_SUM)
    {
        runningSum();
    }

    else if(u_stage == BINARY_SEARCH)
    {
        binarySearch();
    }
    else if(u_stage == ROOT_INIT)
    {
        vec4 curColor = texture2D(s_values, v_texCoord );
        bool isRoot = all( equal( unpack2shorts(curColor), tex2imgCoord(v_texCoord) + ONE ) );
        gl_FragColor = curColor * float(isRoot);
    }
}
