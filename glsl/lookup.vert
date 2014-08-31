uniform sampler2D s_texture;

attribute vec2 a_position;
// attribute vec2 a_texCoord;
//varying vec2 v_texCoord;
varying vec4 v_sourceCoord;

const vec4  OUT  = vec4(-1000.0, -1000.0, ZERO, ZERO);

void main()
{
    // PointSize needs to be set
    gl_PointSize = ONE;

    vec2 uv = img2texCoord(a_position);
    // Get coordinates to scatter the vertex to (compensate for the added 1.0)
    vec2 scatterCoord = unpack2shorts( texture2D(s_texture, uv) ) - ONE;
    // If the pixels is zero move it out of the viewport
    float isZero = step(0.0, -length(scatterCoord) ) ;

    // Convert image coordinates into the [-1.0, 1.0] space
    scatterCoord = img2texCoord(scatterCoord)*TWO - ONE;

    // Set the new vertex position to the scatter destination
    // Filter out vertices where the texel was zero (just move them out of the viewport)
    gl_Position = vec4(scatterCoord, ZERO, ONE) + isZero * OUT;

    // Hand the original coordinates of this vertex to the fragment shader
    // add one in order to distinguish from a zero-pixel
    v_sourceCoord = pack2shorts(a_position + ONE);
}

