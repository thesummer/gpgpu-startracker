/*!
    \ingroup stats
    @{
*/

//////////////////////////////  BEGIN SHADER //////////////////////////

varying vec2 v_texCoord;        // texture coordinates
uniform sampler2D s_orig;
uniform sampler2D s_fill;
uniform sampler2D s_label;
uniform sampler2D s_result;
uniform int   u_pass;
uniform int   u_stage;
uniform float u_savingOffset;
uniform vec2  u_factor;

/*!
 * Last stage of the statistics computation
 *
 * @author Jan Sommer
 * @date 2014
 * @namespace GLSL
 * @class centroidingShader
 */

#define STAGE_CENTROIDING   2
#define STAGE_BLEND         3
#define STAGE_SAVE          4

#define CENTROID_X_COORD   -1
#define CENTROID_Y_COORD   -2

void main()
{
    if (u_stage == STAGE_CENTROIDING)
    {
        float curCount  = unpackLong( texture2D( s_result, v_texCoord ) );
        vec2  curLabel  = unpack2shorts( texture2D( s_label, v_texCoord ) );
        vec2  curFill   = unpack2shorts( texture2D( s_fill, v_texCoord ) );
        vec2  curCoord  = tex2imgCoord(v_texCoord);

        if(u_pass == CENTROID_X_COORD) // x-coordinate
        {
            float luminance = texture2D( s_orig, v_texCoord ).r * 255.0;
            float weightedCoord = (curLabel.x-ONE-curCoord.x) * luminance;
            gl_FragColor = packLong( weightedCoord * step(ONE, curLabel.x) );
            return;
        }
        else if(u_pass == CENTROID_Y_COORD) // y-coordinate
        {
            float luminance = texture2D( s_orig, v_texCoord ).r * 255.0;
            float weightedCoord = (curLabel.y-ONE-curCoord.y) * luminance;
            gl_FragColor = packLong( weightedCoord * step(ONE, curLabel.y) );
            return;
        }

        vec2 offset = clamp(-u_factor, ZERO, ONE);
        if( all(equal( curCoord+ONE - offset , curFill)) )
        {
            curLabel = curFill;
        }

        float twoPow = exp2( float(u_pass) );
        vec3 cornerX, cornerY, cornerXY;

        cornerX.xy  = unpack2shorts( BoundedTexture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(twoPow, ZERO) ) ) );
        cornerY.xy  = unpack2shorts( BoundedTexture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(ZERO, twoPow) ) ) );
        cornerXY.xy = unpack2shorts( BoundedTexture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(twoPow, twoPow) ) ) );

        cornerX.z  = unpackLong( BoundedTexture2D( s_result, img2texCoord( curCoord - u_factor * vec2(twoPow, ZERO) ) ) );
        cornerY.z  = unpackLong( BoundedTexture2D( s_result, img2texCoord( curCoord - u_factor * vec2(ZERO, twoPow) ) ) );
        cornerXY.z = unpackLong( BoundedTexture2D( s_result, img2texCoord( curCoord - u_factor * vec2(twoPow, twoPow))) );

        float isEqual = float( all(equal(cornerX.xy, curFill) ) );
        curCount += isEqual * cornerX.z;
        isEqual   = float( all(equal(cornerY.xy, curFill) ) );
        curCount += isEqual * cornerY.z;
        isEqual   = float( all(equal(cornerXY.xy, curFill) ) );
        curCount += isEqual * cornerXY.z;

        gl_FragColor = packLong( curCount );
    }
    else if(u_stage == STAGE_BLEND)
    {
        vec4 temp = texture2D( s_label, v_texCoord );
        float reduced = unpackLong(temp);
        float result  = unpackLong(texture2D( s_result, v_texCoord ));

        if( result == ZERO )
        {
            gl_FragColor = temp;
        }
        else
        {
            gl_FragColor = packLong( result + reduced );
        }
    }
    else if(u_stage == STAGE_SAVE)
    {
        vec2 coord = tex2imgCoord(v_texCoord) - vec2(u_savingOffset, ZERO);
        float outOfBounds = float( coord.x >= ZERO);
        vec2  lookupLabel = unpack2shorts (BoundedTexture2D( s_label, img2texCoord( coord ) ) );

        if( all(equal(lookupLabel, vec2(ZERO) )) )
        {
            gl_FragColor = vec4(ZERO);
            return;
        }

        float currCount = unpackLong(texture2D( s_label, v_texCoord ));
        vec2 offset = clamp(-u_factor, ZERO, ONE);
        gl_FragColor = BoundedTexture2D( s_result,   img2texCoord( lookupLabel - ONE + offset) ) * outOfBounds;

    }
}


/*!
    @}
*/
