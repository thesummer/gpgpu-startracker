precision highp float;
uniform vec2 u_texDimensions;   // image/texture dimensions
varying vec4 v_sourceCoord;

const float ONE = 1.0;
const float TWO = 2.0;

void main()
{
    gl_FragColor = v_sourceCoord;
}

