/*!
    \ingroup labeling
    @{
*/
varying vec2 v_texCoord;        /*!< texture coordinates of the current pixel */
uniform sampler2D s_texture;    /*!< Sampler holding the input image */
uniform int u_pass;             /*!< The number of the pass this algorithm is in */
uniform float u_factor;         /*!< The factor for the displacement */
uniform float u_threshold;      /*!< Threshold value for the threshold operation */



/*!
 * A simple 3x3 gaussian convolution filter, non-separated version
 * @author Jan Sommer
 * @date 2014
 * @namespace GLSL
 * @class labelingShader
 */


#define STAGE_INITIAL_LABELING   0
#define STAGE_HIGHEST_LABEL      1


/*!
  \brief Main program of the labeling shader

  The labeling shader has basically 3 different stages:

  Initial labeling stage
  ----------------------

  The input image is the original greyscale image. The current shader reads
  the color of its corresponding pixel from the image and performs a thresholding
  operation with \ref u_threshold on it. If the pixel color is zero afterwards
  the gl_FragColor is set to ZERO.

  If not, the pixel color of all 8 neighboring pixel is read and thresholded.
  If any of these neighboring pixels is non-zero the current pixel is assigned an
  initial label with its image coordinates + ONE. This operation is to filter out
  all one-pixel spots and thereby greatly reducing the number of spots which have
  to be considered in subsequent steps.


*/
void main()
{
    // First pass thresholding and initial labeling
    if (u_pass == STAGE_INITIAL_LABELING)
    {
        float curPixelCol = texture2D( s_texture, v_texCoord ).r;
        // Threshold operation)
        curPixelCol = step(u_threshold, curPixelCol);

        // If the pixel color is 0 now, we are done
        if(curPixelCol == ZERO)
        {
            gl_FragColor = vec4(ZERO);
            return;
        }
        // else check all surrounding pixels are ZERO to remove lonely hot pixels

        // 1. Get the value of all 8 surrounding pixels
        vec2 imgCoord = tex2imgCoord(v_texCoord);
        vec4 forwardPixels;   // values of the pixels which are behind current pixel
        vec4 backwardPixels;  // values of the pixels which are before current pixel

        forwardPixels[0] = texture2D( s_texture, img2texCoord( imgCoord + vec2(ONE, ZERO) ) ).r;
        forwardPixels[1] = texture2D( s_texture, img2texCoord( imgCoord + vec2(-ONE, ONE) ) ).r;
        forwardPixels[2] = texture2D( s_texture, img2texCoord( imgCoord + vec2(ZERO, ONE) ) ).r;
        forwardPixels[3] = texture2D( s_texture, img2texCoord( imgCoord + vec2(ONE,  ONE) ) ).r;

        backwardPixels[0] = texture2D( s_texture, img2texCoord( imgCoord - vec2(ONE, ZERO) ) ).r;
        backwardPixels[1] = texture2D( s_texture, img2texCoord( imgCoord - vec2(-ONE, ONE) ) ).r;
        backwardPixels[2] = texture2D( s_texture, img2texCoord( imgCoord - vec2(ZERO, ONE) ) ).r;
        backwardPixels[3] = texture2D( s_texture, img2texCoord( imgCoord - vec2(ONE,  ONE) ) ).r;

        // Threshold the values of the neighboring pixels
        forwardPixels  = step(u_threshold, forwardPixels);
        backwardPixels = step(u_threshold, backwardPixels);

        // Check if any of the the neighboring pixels is not zero
        bool fwNonZero = any(bvec4(forwardPixels));
        bool bwNonZero = any(bvec4(backwardPixels));

        // Pack the imgCoord+ONE as labels if any of the neighboring pixels is not zero, else set gl_FragColor to zero
        gl_FragColor = pack2shorts( (tex2imgCoord(v_texCoord) + ONE ) * float(any(bvec2(fwNonZero, bwNonZero))) );
    }
    // Second pass find neighbor with higest label
    else if (u_pass == STAGE_HIGHEST_LABEL)
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
    else
    {
        vec2 curLabel = unpack2shorts( texture2D(s_texture, v_texCoord) );
        gl_FragColor  = texture2D(s_texture, img2texCoord(curLabel-ONE) );
    }
}

/*!
    @}
*/
