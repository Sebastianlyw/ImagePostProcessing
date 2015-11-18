/******************************************************************************/
/*!
\author Lim Ee Siang
\par    email: e.lim\@digipen.edu
\par    Course: CS370
\brief  

*/
/******************************************************************************/

uniform sampler2D uColorMap;
uniform sampler2D uShadowMap;

varying vec2 vTexCoord;

uniform float uMapSize;
uniform int uHalfSize;
uniform float uBlurCutoff;
uniform bool uInvert;
uniform bool uNaiveDOF;

void main(void)
{
  vec4 col = vec4(0.0, 0.0, 0.0, 1.0);
  
  float depth = texture2D(uShadowMap, vTexCoord).r;
  
  if(!uNaiveDOF || (uInvert? depth <= uBlurCutoff: depth >= uBlurCutoff))
  { 
    float weight = 1.0 / (uHalfSize * 2 + 1);
    for(int i = -uHalfSize; i <= uHalfSize; ++i)
    {
      vec2 coord = vTexCoord;
      coord.x += i * uMapSize;
      col += weight * texture2D(uColorMap, coord);
    }  
    
    gl_FragColor = col;   
  }  
  else
  {
    gl_FragColor = texture2D(uColorMap, vTexCoord); 
  }   
}