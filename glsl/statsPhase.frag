precision highp float;
varying vec2 v_texCoord;        // texture coordinates
uniform sampler2D s_fill;
uniform sampler2D s_label;
uniform sampler2D s_result;
uniform vec2 u_texDimensions;   // image/texture dimensions
uniform int u_pass;
uniform int u_stage;
uniform int u_savingOffset;

//uniform float u_factor;
//uniform int u_debug;
//uniform float u_threshold;      // threshold value for the threshold operation

#define STAGE_FILL  0
#define STAGE_COUNT 1
#define STAGE_COPY  2
#define SAVE_COUNT  3


const float ZERO = 0.0;
const float ONE  = 1.0;
const float TWO  = 2.0;

const float OUT  = 10000.0;

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
    const float bias = ONE/1024.0;
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

void main()
{

    if (u_stage == STAGE_FILL)
    {
        vec2 curLabel = unpack2shorts( texture2D( s_fill, v_texCoord ) );
        vec2 curCoord = tex2imgCoord(v_texCoord);
        // If curLabel is not 0 it means it already has a label assigned and no
        // further action has to be done
        if (curLabel != vec2(ZERO))
        {
            gl_FragColor = pack2shorts( curLabel );
        }
        else
        {
            // Sample corner pixels of a u_pass * u_pass sized square
            float twoPow = exp2( u_pass );
            vec3 rightLabel, bottomLabel, bottomRightLabel;

            rightLabel.xy       = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord + vec2(twoPow, ZERO) ) ) );
            bottomLabel.xy      = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord + vec2(ZERO, twoPow) ) ) );
            bottomRightLabel.xy = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord + vec2(twoPow, twoPow) ) ) );

            // Check if these pixels are zero
            float rIsZero  = ONE-step(ONE/256.0, length(rightLabel.xy) );
            float bIsZero  = ONE-step(ONE/256.0, length(bottomLabel.xy) );
            float brIsZero = ONE-step(ONE/256.0, length(bottomRightLabel.xy) );

            // Make sure zero-pixel will have the maximum distance
            rightLabel       += rIsZero  * OUT;
            bottomLabel      += bIsZero  * OUT;
            bottomRightLabel += brIsZero * OUT;

            rightLabel.z       =  distance(rightLabel.xy, curCoord);
            bottomLabel.z      =  distance(bottomLabel.xy, curCoord);
            bottomRightLabel.z =  distance(bottomRightLabel.xy, curCoord);


            // Find the label with the smallest distance to current pixel
            float result = min( rightLabel.z, bottomLabel.z );
            result = min(result, bottomRightLabel.z );

            vec3 mask = ONE - ( step(-result, -vec3(rightLabel.z, bottomLabel.z, bottomRightLabel.z)) );
            rightLabel       += mask[0] * OUT;
            bottomLabel      += mask[1] * OUT;
            bottomRightLabel += mask[2] * OUT;


            // In case 2 different labels have the same distance from the current Pixels choose the one with the
            // smallest y-coordinate (it shouldn't be too important anyways)
            float smallestY = min(rightLabel.y, bottomLabel.y);
            smallestY = min(smallestY, bottomRightLabel.y);

            mask = ( step(-smallestY, -vec3(rightLabel.y, bottomLabel.y, bottomRightLabel.y) ) );

            rightLabel       *= mask[0];
            bottomLabel      *= mask[1];
            bottomRightLabel *= mask[2];

            // If all 3 corner pixel were 0 result will have an unreasonable large value --> set it back to 0
            result = ( ONE-step(5000.0, result) );



            // Adding all xy-coordinates of the 3 corner labels will only add the ones which have the smallest distance
            // to the current pixel. Divide by length(mask) to correct for the cases where more than one pixel have
            // the same distance (hence same value), multiply by result to correct for the case where all 3 are zero
            gl_FragColor = pack2shorts( (rightLabel.xy+bottomLabel.xy+bottomRightLabel.xy) / dot(mask, mask) ) * result;
        }
    }
    else if(u_stage == STAGE_COUNT)
    {
        float curCount = unpack2shorts( texture2D( s_result, v_texCoord ) ).x;
        vec2  curLabel  = unpack2shorts( texture2D( s_label, v_texCoord ) );
        vec2  curFill   = unpack2shorts( texture2D( s_fill, v_texCoord ) );
        vec2  curCoord  = tex2imgCoord(v_texCoord);

        if(u_pass == -1)
        {
            curCount = float( all( equal(curLabel, curFill) ) );
            gl_FragColor = pack2shorts( vec2( equal(curLabel, curFill) ) * step(ONE, curLabel) );
            return;

        }

        float twoPow = exp2( u_pass );
        vec3 leftPixel, topPixel, topLeftPixel;

        leftPixel.xy       = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord - vec2(twoPow, ZERO) ) ) );
        topPixel.xy      = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord - vec2(ZERO, twoPow) ) ) );
        topLeftPixel.xy = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord - vec2(twoPow, twoPow) ) ) );

        leftPixel.z       = unpack2shorts( texture2D( s_result, img2texCoord( curCoord - vec2(twoPow, ZERO) ) ) ).x;
        topPixel.z      = unpack2shorts( texture2D( s_result, img2texCoord( curCoord - vec2(ZERO, twoPow) ) ) ).x;
        topLeftPixel.z = unpack2shorts( texture2D( s_result, img2texCoord( curCoord - vec2(twoPow, twoPow))) ).x;

        float isEqual = float( all(equal(leftPixel.xy, curLabel) ) );
        curCount += isEqual * leftPixel.z;
        isEqual = float( all(equal(topPixel.xy, curLabel) ) );
        curCount += isEqual * topPixel.z;
        isEqual = float( all(equal(topLeftPixel.xy, curLabel) ) );
        curCount += isEqual * topLeftPixel.z;

        gl_FragColor = pack2shorts( vec2(curCount, ZERO) );
    }
    else if(u_stage == STAGE_COPY)
    {
        gl_FragColor = texture2D( s_result, v_texCoord);
    }
    else if(u_stage == STAGE_SAVE)
    {
        vec2 curCoord = tex2imgCoord(v_texCoord);
        vec2 lookupLabel = texture2D( s_label, img2texCoord( curCoord - vec2(u_savingOffset, ZERO) ) );

        if(equal(lookupLabel, vec2(ZERO) ) )
        {
            gl_FragColor = vec4(ZERO);
            return
        }

        vec2 currentValue = unpack2shorts( texture2D( s_fill,   img2texCoord( lookupLabel ) ) );
        vec2 addValue     = unpack2shorts( texture2D( s_result, img2texCoord( lookupLabel ) ) );

        gl_FragColor = pack2shorts(currentValue + addValue);
    }
}
