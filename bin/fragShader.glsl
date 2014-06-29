precision highp float;
varying vec2 v_texCoord;        // texture coordinates
uniform sampler2D s_texture;    // texture sampler

void main()
{
    gl_FragColor = texture2D( s_texture, v_texCoord );
}
