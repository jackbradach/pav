uniform sampler2D txt;
varying vec2 txt_xy;

void main(void)
{
   gl_FragColor = texture2D(txt, txt_xy).bgra;
}
