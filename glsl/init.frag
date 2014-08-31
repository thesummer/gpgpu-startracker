varying vec2 v_texCoord;        // texture coordinates
uniform sampler2D s_texture;    // texture sampler
uniform float u_threshold;      // threshold value for the threshold operation

/*
Shader will do the following steps for each pixel:
    1. Threshold with u_threshold
    2. pack image coordinates for into RGBA value for non-zero pixels

    It is assumed that the input texture is a gray scale RGBA-image (c,c,c,255)
*/
void main()
{
    // Compute image coordinates
    vec2 imgCoord = tex2imgCoord(v_texCoord);

    // Get the input pixel value
    gl_FragColor = texture2D( s_texture, v_texCoord );

    // Threshold operation
    gl_FragColor.a = gl_FragColor.r;
    gl_FragColor = step(u_threshold, gl_FragColor);

    // pack image coordinates for the non-zero pixels
    gl_FragColor = pack2shorts(imgCoord) * gl_FragColor;
}
