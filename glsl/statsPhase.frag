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



//////////////////////////////  BEGEIN SHADER //////////////////////////

varying vec2 v_texCoord;        // texture coordinates
uniform sampler2D s_orig;
uniform sampler2D s_fill;
uniform sampler2D s_label;
uniform sampler2D s_result;
uniform int   u_pass;
uniform int   u_stage;
uniform float u_savingOffset;
uniform vec2  u_factor;
//uniform int u_debug;
//uniform float u_threshold;      // threshold value for the threshold operation

#define STAGE_FILL          0
#define STAGE_COUNT         1
#define STAGE_CENTROIDING   2
#define STAGE_BLEND         3
#define STAGE_SAVE          4

const float OUT  = 10000.0;

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
            vec3 cornerX, cornerY, cornerXY;

            cornerX.xy  = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord + u_factor * vec2(twoPow, ZERO) ) ) );
            cornerY.xy  = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord + u_factor * vec2(ZERO, twoPow) ) ) );
            cornerXY.xy = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord + u_factor * vec2(twoPow, twoPow) ) ) );

            // Check if these pixels are zero
            float xIsZero  = ONE-step(ONE/256.0, length(cornerX.xy) );
            float yIsZero  = ONE-step(ONE/256.0, length(cornerY.xy) );
            float xyIsZero = ONE-step(ONE/256.0, length(cornerXY.xy) );

            // Make sure zero-pixel will have the maximum distance
            cornerX  += xIsZero  * OUT;
            cornerY  += yIsZero  * OUT;
            cornerXY += xyIsZero * OUT;

            cornerX.z  =  distance(cornerX.xy, curCoord);
            cornerY.z  =  distance(cornerY.xy, curCoord);
            cornerXY.z =  distance(cornerXY.xy, curCoord);


            // Find the label with the smallest distance to current pixel
            float result = min( cornerX.z, cornerY.z );
            result = min(result, cornerXY.z );

            vec3 mask = ONE - ( step(-result, -vec3(cornerX.z, cornerY.z, cornerXY.z)) );
            cornerX  += mask[0] * OUT;
            cornerY  += mask[1] * OUT;
            cornerXY += mask[2] * OUT;


            // In case 2 different labels have the same distance from the current Pixels choose the one with the
            // smallest y-coordinate (it shouldn't be too important anyways)
            float smallestY = min(cornerX.y, cornerY.y);
            smallestY = min(smallestY, cornerXY.y);

            mask = ( step(-smallestY, -vec3(cornerX.y, cornerY.y, cornerXY.y) ) );

            cornerX       *= mask[0];
            cornerY      *= mask[1];
            cornerXY *= mask[2];

            // If all 3 corner pixel were 0 result will have an unreasonable large value --> set it back to 0
            result = ( ONE-step(5000.0, result) );



            // Adding all xy-coordinates of the 3 corner labels will only add the ones which have the smallest distance
            // to the current pixel. Divide by length(mask) to correct for the cases where more than one pixel have
            // the same distance (hence same value), multiply by result to correct for the case where all 3 are zero
            gl_FragColor = pack2shorts( (cornerX.xy+cornerY.xy+cornerXY.xy) / dot(mask, mask) ) * result;
        }
    }
    else if(u_stage == STAGE_COUNT)
    {
        vec2  curCount  = unpack2shorts( texture2D( s_result, v_texCoord ) );
        vec2  curLabel  = unpack2shorts( texture2D( s_label, v_texCoord ) );
        vec2  curFill   = unpack2shorts( texture2D( s_fill, v_texCoord ) );
        vec2  curCoord  = tex2imgCoord(v_texCoord);

        if(u_pass == -1)
        {
            float luminance = texture2D( s_orig, v_texCoord ).r * 255.0;
            float area = float( all(equal(curLabel, curFill)) );
            gl_FragColor = pack2shorts( vec2( area, luminance  ) * step(ONE, curLabel) );
            return;
        }

        vec2 offset = clamp(-u_factor, ZERO, ONE);
        if( all(equal( curCoord+ONE - offset , curFill)) )
        {
            curLabel = curFill;
        }

        float twoPow = exp2( u_pass );
        vec4 cornerX, cornerY, cornerXY;

        cornerX.xy  = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(twoPow, ZERO) ) ) );
        cornerY.xy  = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(ZERO, twoPow) ) ) );
        cornerXY.xy = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(twoPow, twoPow) ) ) );

        cornerX.zw  = unpack2shorts( texture2D( s_result, img2texCoord( curCoord - u_factor * vec2(twoPow, ZERO) ) ) );
        cornerY.zw  = unpack2shorts( texture2D( s_result, img2texCoord( curCoord - u_factor * vec2(ZERO, twoPow) ) ) );
        cornerXY.zw = unpack2shorts( texture2D( s_result, img2texCoord( curCoord - u_factor * vec2(twoPow, twoPow))) );

        float isEqual = float( all(equal(cornerX.xy, curLabel) ) );
        curCount += isEqual * cornerX.zw;
        isEqual = float( all(equal(cornerY.xy, curLabel) ) );
        curCount += isEqual * cornerY.zw;
        isEqual = float( all(equal(cornerXY.xy, curLabel) ) );
        curCount += isEqual * cornerXY.zw;

        gl_FragColor = pack2shorts( curCount );
    }
    else if (u_stage == STAGE_CENTROIDING)
    {
        vec2  curCount  = unpack2shorts( texture2D( s_result, v_texCoord ) );
        vec2  curLabel  = unpack2shorts( texture2D( s_label, v_texCoord ) );
        vec2  curFill   = unpack2shorts( texture2D( s_fill, v_texCoord ) );
        vec2  curCoord  = tex2imgCoord(v_texCoord);

        if(u_pass == -1)
        {
            float luminance = texture2D( s_orig, v_texCoord ).r * 255.0;
            vec2 weightedCoord = (curLabel-ONE-curCoord) * luminance;
            gl_FragColor = pack2shorts( weightedCoord * step(ONE, curLabel) );
            return;
        }

        vec2 offset = clamp(-u_factor, ZERO, ONE);
        if( all(equal( curCoord+ONE - offset , curFill)) )
        {
            curLabel = curFill;
        }

        float twoPow = exp2( u_pass );
        vec4 cornerX, cornerY, cornerXY;

        cornerX.xy  = unpack2signed( texture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(twoPow, ZERO) ) ) );
        cornerY.xy  = unpack2signed( texture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(ZERO, twoPow) ) ) );
        cornerXY.xy = unpack2signed( texture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(twoPow, twoPow) ) ) );

        cornerX.zw  = unpack2signed( texture2D( s_result, img2texCoord( curCoord - u_factor * vec2(twoPow, ZERO) ) ) );
        cornerY.zw  = unpack2signed( texture2D( s_result, img2texCoord( curCoord - u_factor * vec2(ZERO, twoPow) ) ) );
        cornerXY.zw = unpack2signed( texture2D( s_result, img2texCoord( curCoord - u_factor * vec2(twoPow, twoPow))) );

        float isEqual = float( all(equal(cornerX.xy, curLabel) ) );
        curCount += isEqual * cornerX.zw;
        isEqual = float( all(equal(cornerY.xy, curLabel) ) );
        curCount += isEqual * cornerY.zw;
        isEqual = float( all(equal(cornerXY.xy, curLabel) ) );
        curCount += isEqual * cornerXY.zw;

        gl_FragColor = pack2signed( curCount );
    }
    else if(u_stage == STAGE_BLEND)
    {
        vec2 result   = unpack2shorts( texture2D( s_result, v_texCoord ) );
        vec2 reduced  = unpack2shorts( texture2D( s_label, v_texCoord ) );
        gl_FragColor = pack2shorts( result + reduced);
    }
    else if(u_stage == STAGE_SAVE)
    {
        vec2 coord = tex2imgCoord(v_texCoord) - vec2(u_savingOffset, ZERO);
        float outOfBounds = float( coord.x >= ZERO);
        vec2  lookupLabel = unpack2shorts (texture2D( s_label, img2texCoord( coord ) ) );

        if( all(equal(lookupLabel, vec2(ZERO) )) )
        {
            gl_FragColor = vec4(ZERO);
            return;
        }
        vec2 offset = clamp(-u_factor, ZERO, ONE);
        gl_FragColor = texture2D( s_result,   img2texCoord( lookupLabel - ONE + offset) ) * outOfBounds;

    }
}
