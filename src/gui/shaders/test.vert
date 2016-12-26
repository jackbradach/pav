varying vec2 vTexCoord;

void main()
{
    vTexCoord = gl_MultiTexCoord0;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
