uniform sampler2D uColorMap;  //full screen size texture
uniform sampler2D uPass0;
uniform sampler2D uPass1;
uniform sampler2D uPass2;
uniform sampler2D uPass3;
varying vec2 vTexCoord;
uniform float uCoeft1, uCoeft1x, uCoeft1y, uCoeft1z, uCoeft2; 

void main(void)
{
  vec4 t0 = texture2D(uColorMap, vTexCoord);
  vec3 t1 = texture2D(uPass0, vTexCoord).xyz;
  vec3 t2 = texture2D(uPass1, vTexCoord).xyz;
  vec3 t3 = texture2D(uPass2, vTexCoord).xyz;
  vec3 t4 = texture2D(uPass3, vTexCoord).xyz;    
  vec3 coeff = vec3(uCoeft1x, uCoeft1y, uCoeft1z);
    
  vec3 finalColor = uCoeft2 *  t0.xyz;
  
  finalColor += uCoeft1 * (coeff * t1);
  finalColor += uCoeft1 * (coeff * t2);
  finalColor += uCoeft1 * (coeff * t3);
  finalColor += uCoeft1 * (coeff * t4);
  
  gl_FragColor = vec4(finalColor, t0.a);
}

