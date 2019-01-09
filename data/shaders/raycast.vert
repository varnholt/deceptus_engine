uniform vec2 light1;
varying vec4 lightpos;
varying vec4 pos;

void main()
{
   lightpos = gl_ModelViewProjectionMatrix * vec4(light1, 1.0, 1.0);
   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
   pos = gl_Position;
   gl_FrontColor = gl_Color;
}
