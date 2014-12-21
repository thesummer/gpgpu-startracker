varying vec2 v_texCoord;        // texture coordinates
uniform sampler2D s_texture;    // texture sampler
uniform int u_pass;
uniform float u_factor;
uniform float u_threshold;      // threshold value for the threshold operation

/**
 * A simple 3x3 gaussian convolution filter, non-separated version
 * @author Sebastian Schaefer
 * @date 2012
 * @namespace GLSL::FILTER::BLUR
 * @class Gauss3x3
 */


void main()
{
    // First pass thresholding and initial labeling
    if (u_pass == 0)
    {
        vec4 curPixelCol = texture2D( s_texture, v_texCoord ).rrrr;
        // Threshold operation)
        curPixelCol = step(u_threshold, curPixelCol);

        gl_FragColor = pack2shorts( (tex2imgCoord(v_texCoord) + ONE ) * curPixelCol.xy);
    }
    // Second pass find neighbor with higest label
    else if (u_pass == 1)
    {
        vec4 curCol   = texture2D( s_texture, v_texCoord );
        vec2 curLabel = unpack2shorts(curCol);
        vec2 curCoord = tex2imgCoord(v_texCoord);
        float isZero  = step(ONE/256.0, length(curCol) );

        // Get neighbor pixel
        vec4 xValues, yValues;
        vec4 tempCol   = texture2D(s_texture, img2texCoord(curCoord + u_factor*vec2(ONE, 0.0)) );
        vec2 tempLabel = unpack2shorts(tempCol);
        xValues[0] = tempLabel.x;
        yValues[0] = tempLabel.y;


        tempCol = texture2D(s_texture, img2texCoord(curCoord + u_factor*vec2(-ONE, ONE)) );
        tempLabel  = unpack2shorts(tempCol);
        xValues[1] = tempLabel.x;
        yValues[1] = tempLabel.y;

        tempCol = texture2D(s_texture, img2texCoord(curCoord + u_factor*vec2(0.0, ONE)) );
        tempLabel = unpack2shorts(tempCol);
        xValues[2] = tempLabel.x;
        yValues[2] = tempLabel.y;

        tempCol = texture2D(s_texture, img2texCoord(curCoord + u_factor*vec2(ONE, ONE)) );
        tempLabel = unpack2shorts(tempCol);
        xValues[3] = tempLabel.x;
        yValues[3] = tempLabel.y;

        // Find max yValue from neighbors
        float maxY = max( max(yValues[0], yValues[1]), max(yValues[2], yValues[3]) );

        // Find the pair which belongs to the most bottom-right non-zero pixel
        // find most bottom pixels
        vec4 mask = step(maxY, yValues);

        // Set x-Values which are not in the bottom-most row to 0
        xValues *= mask;

        // Find max xValue from remaining xValues
        float maxX = max( max(xValues[0], xValues[1]), max(xValues[2], xValues[3]) );

        // Complete mask and compute final x- and y-coordinate of most bottom-right pixel
        mask *= step(maxX, xValues);
        maxX = dot(mask, xValues) / dot(mask, mask);
        maxY = dot(mask, yValues) / dot(mask, mask);

        // Compare the maximum of the neighbor pixels with current coord and assign the maximum label to current pixel
        // Floor seems to be necessary for the rpi, otherwise mask.xy = step(maxY, yValues.xy); yields a different result compared to intel
        xValues.xy = floor(vec2(maxX, curLabel.x)+0.5);
        yValues.xy = floor(vec2(maxY, curLabel.y)+0.5);

        maxY = max(yValues[0], yValues[1]);
        mask.xy = step(maxY, yValues.xy);

        xValues.xy *= mask.xy;
        maxX = floor(max(xValues[0], xValues[1]) +0.5);
        mask.xy *= step(maxX, xValues.xy);
        maxX = dot(mask.xy, xValues.xy) / dot(mask.xy, mask.xy);
        maxY = dot(mask.xy, yValues.xy) / dot(mask.xy, mask.xy);

        gl_FragColor = pack2shorts(vec2(maxX, maxY)) * isZero ;
    }
    else if (u_pass == -1)
    {
        // Copy mode
        gl_FragColor = texture2D( s_texture, v_texCoord );
    }
    else
    {
        vec2 curLabel = unpack2shorts( texture2D(s_texture, v_texCoord) );
        gl_FragColor  = texture2D(s_texture, img2texCoord(curLabel-ONE) );
    }
}
