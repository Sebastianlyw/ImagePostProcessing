uniform sampler2D uColorMap;

varying vec2 vTexCoord;

void main (void)
{
  vec4  color = texture2D(uColorMap, vTexCoord);
  
  gl_FragColor = vec4(1.0 - color.r, 1.0 - color.g, 1.0 - color.b, color.a);
}