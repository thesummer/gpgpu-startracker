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
 * Second stage of the statistics computation
 *
 * @author Jan Sommer
 * @date 2014
 * @namespace GLSL
 * @class countShader
 */

#define STAGE_COUNT         1
#define STAGE_BLEND         3
#define STAGE_SAVE          4

void main()
{
    if(u_stage == STAGE_COUNT)
    {
        vec2  curCount  = unpack2shorts( texture2D( s_result, v_texCoord ) );
        vec2  curLabel  = unpack2shorts( texture2D( s_label, v_texCoord ) );
        vec2  curFill   = unpack2shorts( texture2D( s_fill, v_texCoord ) );
        vec2  curCoord  = tex2imgCoord(v_texCoord);

        // Initialize the pixel. Copy the luminance value from the original image and
        // set the inital count to 1 if curLabel == curFill
        if(u_pass == -1)
        {
            float luminance = texture2D( s_orig, v_texCoord ).r * f255 ;
            float area = float( all(equal(curLabel, curFill)) );
            gl_FragColor = pack2shorts( vec2( area, luminance  ) * step(ONE, curLabel) );
            return;
        }

        // The whole statsPhase is run multiple times with different factors in x- and y-direction
        // However, to avoid overlabs of the tiles an offset of ONE has to be applied if factor is != (1.0, 1.0)
        // The following basically checks if the pixel next to it in the respective direction is the root pixel
        // and if yes assigns it's label to the current pixel (if it's not already the same)
        vec2 offset = clamp(-u_factor, ZERO, ONE);
        if( all(equal( curCoord+ONE - offset , curFill )) )
        {
            curLabel = curFill;
        }

        float twoPow = exp2( float(u_pass) );
        vec4 cornerX, cornerY, cornerXY;

        cornerX.xy  = unpack2shorts( BoundedTexture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(twoPow, ZERO) ) ) );
        cornerY.xy  = unpack2shorts( BoundedTexture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(ZERO, twoPow) ) ) );
        cornerXY.xy = unpack2shorts( BoundedTexture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(twoPow, twoPow) ) ) );

        cornerX.zw  = unpack2shorts( BoundedTexture2D( s_result, img2texCoord( curCoord - u_factor * vec2(twoPow, ZERO) ) ) );
        cornerY.zw  = unpack2shorts( BoundedTexture2D( s_result, img2texCoord( curCoord - u_factor * vec2(ZERO, twoPow) ) ) );
        cornerXY.zw = unpack2shorts( BoundedTexture2D( s_result, img2texCoord( curCoord - u_factor * vec2(twoPow, twoPow))) );

        float isEqual = float( all(equal(cornerX.xy, curFill) ) );
        curCount += isEqual * cornerX.zw;
        isEqual = float( all(equal(cornerY.xy, curFill) ) );
        curCount += isEqual * cornerY.zw;
        isEqual = float( all(equal(cornerXY.xy, curFill) ) );
        curCount += isEqual * cornerXY.zw;

        gl_FragColor = pack2shorts( curCount );
    }
    else if(u_stage == STAGE_BLEND)
    {
        vec4 texReduced = texture2D( s_label, v_texCoord );
        /* NOTE: There was a problem that unpacking a long int written to during the centroiding stage
         *       here would alter its value. Therefore u_factor now has the x1 and x2 bounds within the
         *       unpacking is safe. In other words only unpack the count values but do NOT unpack centroiding values
         */
        vec2 coord = tex2imgCoord(v_texCoord);
        if (coord.x < u_factor.x || coord.x >= u_factor.y)
        {
            // Only copy the value
            gl_FragColor = texReduced;
            return;
        }

        vec2 result   = unpack2shorts( texture2D( s_result, v_texCoord ) );
        vec2 reduced  = unpack2shorts( texReduced );

        gl_FragColor = pack2shorts( result + reduced);

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
        vec2 offset = clamp(-u_factor, ZERO, ONE);
        gl_FragColor = BoundedTexture2D( s_result,   img2texCoord( lookupLabel - ONE + offset) ) * outOfBounds;

    }
}

/*!
    @}
*/
