varying vec2 v_texCoord;

void main()
{
    vec2 coord = tex2imgCoord(v_texCoord);
//    gl_FragColor = pack2shorts(vec2(coord.y, coord.x + coord.y * u_texDimensions.x) );

    vec4 temp = pack2shorts( vec2(coord.x + coord.y * u_texDimensions.x) );

//    vec2 scalar = unpack2shorts ( temp );

//    temp = pack2shorts(scalar);

    gl_FragColor = temp;

//    gl_FragColor.xyz = temp;
//    gl_FragColor.w   = ZERO;

//    gl_FragColor.xyz = packLong(scalar);
//    gl_FragColor.w = temp.w;

}
