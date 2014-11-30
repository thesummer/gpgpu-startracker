
//////////////////////////////  BEGIN SHADER //////////////////////////

varying vec2 v_texCoord;        // texture coordinates
uniform highp sampler2D s_fill;
uniform int   u_pass;
uniform vec2  u_factor;

const float OUT  = 10000.0;

void main()
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
        float twoPow = floor(exp2( float(u_pass) )+0.5);
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

        cornerX  *= mask[0];
        cornerY  *= mask[1];
        cornerXY *= mask[2];

        // If all 3 corner pixel were 0 result will have an unreasonable large value --> set it back to 0
        result = ( ONE-step(5000.0, result) );



        // Adding all xy-coordinates of the 3 corner labels will only add the ones which have the smallest distance
        // to the current pixel. Divide by length(mask) to correct for the cases where more than one pixel have
        // the same distance (hence same value), multiply by result to correct for the case where all 3 are zero
        gl_FragColor = pack2shorts( (cornerX.xy+cornerY.xy+cornerXY.xy) / dot(mask, mask) ) * result;
    }
}
