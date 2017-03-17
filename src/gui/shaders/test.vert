#version 130

in vec2 position;
out vec4 color;

void main()
{
    color = gl_Position;
    gl_Position = vec4(position, 0.0, 1.0);
}
