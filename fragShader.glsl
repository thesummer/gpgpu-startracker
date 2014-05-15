

precision highp float;
varying vec2 v_texCoord;
uniform sampler2D s_texture;
uniform vec2 u_texDimensions;

/*
Assuming that the texture is 8-bit RGBA 32bits are available for packing.
This function packs 2 16-bit short integer values into the 4 texture channels.
They then can be used in subsequent shader operations or by the client after a call to glReadPixels.

in vec2:  it is assumed that each element contains an integer in float representation

return vec4: RGBA value which contains the packed shorts.

*/
vec4 pack2shorts(in vec2 shorts)
{
    shorts /= 256.0f;
    return vec4(floor(shorts)/255.0f, fract(shorts)*256.0f/255.0f).zxwy;
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
    return (2.0f*texCoord*u_texDimensions-1.0f)/2.0f;
}

void main()
{
    vec2 imgCoord = tex2imgCoord(v_texCoord);
    gl_FragColor = pack2shorts( imgCoord);
//    gl_FragColor = texture2D( s_texture, v_texCoord );
}
