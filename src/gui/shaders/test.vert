#version 130
varying vec2 txt_xy;

void main()
{
    txt_xy = gl_MultiTexCoord0.xy;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
