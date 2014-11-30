
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
//uniform int u_debug;
//uniform float u_threshold;      // threshold value for the threshold operation

#define STAGE_FILL          0
#define STAGE_COUNT         1
#define STAGE_CENTROIDING   2
#define STAGE_BLEND         3
#define STAGE_SAVE          4

#define CENTROID_X_COORD   -1
#define CENTROID_Y_COORD   -2

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
            float twoPow = exp2( float(u_pass) );
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
            float luminance = texture2D( s_orig, v_texCoord ).r * 255.0 ;
            float area = float( all(equal(curLabel, curFill)) );
            gl_FragColor = pack2shorts( vec2( area, luminance  ) * step(ONE, curLabel) );
            return;
        }

        vec2 offset = clamp(-u_factor, ZERO, ONE);
        if( all(equal( curCoord+ONE - offset , curFill)) )
        {
            curLabel = curFill;
        }

        float twoPow = exp2( float(u_pass) );
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
        float curCount  = unpackSignedLong( texture2D( s_result, v_texCoord ).xyz );
        vec2  curLabel  = unpack2shorts( texture2D( s_label, v_texCoord ) );
        vec2  curFill   = unpack2shorts( texture2D( s_fill, v_texCoord ) );
        vec2  curCoord  = tex2imgCoord(v_texCoord);

        if(u_pass == CENTROID_X_COORD) // x-coordinate
        {
            float luminance = texture2D( s_orig, v_texCoord ).r * 255.0;
            float weightedCoord = (curLabel.x-ONE-curCoord.x) * luminance;
            gl_FragColor.xyz = packSignedLong( weightedCoord * step(ONE, curLabel.x) );
            gl_FragColor.w   = ZERO;
            return;
        }
        else if(u_pass == CENTROID_Y_COORD) // y-coordinate
        {
            float luminance = texture2D( s_orig, v_texCoord ).r * 255.0;
            float weightedCoord = (curLabel.y-ONE-curCoord.y) * luminance;
            gl_FragColor.xyz = packSignedLong( weightedCoord * step(ONE, curLabel.y) );
            gl_FragColor.w   = ZERO;
            return;
        }

        vec2 offset = clamp(-u_factor, ZERO, ONE);
        if( all(equal( curCoord+ONE - offset , curFill)) )
        {
            curLabel = curFill;
        }

        float twoPow = exp2( float(u_pass) );
        vec3 cornerX, cornerY, cornerXY;

        cornerX.xy  = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(twoPow, ZERO) ) ) );
        cornerY.xy  = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(ZERO, twoPow) ) ) );
        cornerXY.xy = unpack2shorts( texture2D( s_fill, img2texCoord( curCoord - u_factor * vec2(twoPow, twoPow) ) ) );

        cornerX.z  = unpackSignedLong( texture2D( s_result, img2texCoord( curCoord - u_factor * vec2(twoPow, ZERO) ) ).xyz );
        cornerY.z  = unpackSignedLong( texture2D( s_result, img2texCoord( curCoord - u_factor * vec2(ZERO, twoPow) ) ).xyz );
        cornerXY.z = unpackSignedLong( texture2D( s_result, img2texCoord( curCoord - u_factor * vec2(twoPow, twoPow))).xyz );

        float isEqual = float( all(equal(cornerX.xy, curLabel) ) );
        curCount += isEqual * cornerX.z;
        isEqual   = float( all(equal(cornerY.xy, curLabel) ) );
        curCount += isEqual * cornerY.z;
        isEqual   = float( all(equal(cornerXY.xy, curLabel) ) );
        curCount += isEqual * cornerXY.z;

        gl_FragColor.xyz = packSignedLong( curCount );
        gl_FragColor.w   = ZERO;
    }
    else if(u_stage == STAGE_BLEND)
    {
        if(u_pass == STAGE_COUNT) // Blend count values
        {
            vec2 result   = unpack2shorts( texture2D( s_result, v_texCoord ) );
            vec2 reduced  = unpack2shorts( texture2D( s_label, v_texCoord ) );

            gl_FragColor = pack2shorts( result + reduced);

        }
        else // Blend centroiding values
        {
            vec4 temp = texture2D( s_label, v_texCoord );
            float reduced = unpackSignedLong(temp.xyz);
            float result  = unpackSignedLong(texture2D( s_result, v_texCoord ).xyz);

            gl_FragColor.xyz = packSignedLong( result + reduced );
            gl_FragColor.w = temp.w;

//            vec2 test = vec2(40.0, 10200);
//            vec4 rgba = pack2shorts(test) ;
//            float l = unpackSignedLong(rgba.rgb);
//            gl_FragColor.rgb = packSignedLong(l);
//            gl_FragColor.a = ZERO; //rgba.a;
        }
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
