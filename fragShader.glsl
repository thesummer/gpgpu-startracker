

precision highp float;
varying vec2 v_texCoord;
uniform sampler2D s_texture;
uniform vec2 u_texDimensions;

/*


*/
vec4 pack2shorts(in vec2 shorts)
{
    shorts /= 256.0f;
    return vec4(floor(shorts)/255.0f, fract(shorts)*256.0f/255.0f).zxwy;
}

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
