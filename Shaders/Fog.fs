/******************************************************************************/
/*!
\file   Fog.fs
\author Hoh Shi Chao
\par    email: s.hoh\@digipen.edu
\par    DigiPen login: s.hoh
\par    Course: CS370
\date   23/4/2013
\brief  
  Real-Time Fog for Post-processing
*/
/******************************************************************************/

varying vec2 vTexCoord;
varying vec3 vCameraViewVec;
varying vec3 vCameraPosition;
varying vec3 vFragmentVec;

uniform sampler2D uColorMap;// previous color map, not original. input should be blurred image! uBlurredMap
uniform sampler2D uShadowMap; //Untouched depth buffer passed in as Shadow map

uniform float uCameraNear;
uniform float uCameraFar;

// should we use this to calculate worldfragment? or use the interpolated one from vs, vPosition?
uniform vec3 uCameraEyePos;

void main(void)
{
  // Read fragment depth from depth texture
  vec4 zbuffer = texture2D(uShadowMap, vTexCoord.st);

  // Unit vector in the direction of the camera to the fragment
  vec3 fragmentunitvec = normalize(vFragmentVec);
  // Unit vector in the direction of the camera
  vec3 viewunitvec = normalize(vCameraViewVec);

  // Perform scene depth reconstruction
  float p34 = (-2.0 * uCameraFar * uCameraNear) / (uCameraFar - uCameraNear);
  float p33 = (uCameraFar + uCameraNear) / (uCameraNear - uCameraFar);
  // Actual depth value
  float z = -p34 / (zbuffer.x + p33);

  // Fragment Euclidean distance (refer to above) //(through unit vector in the direction of fragment interpolated by vertex shader, and camera normalized view direction)
  float u = z / dot(fragmentunitvec, viewunitvec);
  // The 3D position of the fragment relative to the camera is obtained by
  // Real world 3D position of current fragment
  vec3 worldfragment = /*uCameraEyePos*/vCameraPosition + u * fragmentunitvec;	

  vec4 fragmentcolor = vec4(1,1,1,0);//vec4(texture2D(uColorMap, vTexCoord.st).xyz,0.0);

  // Evaluate fog integral
  float integral = 0.02 * -(exp(-vCameraPosition.y*0.01)-exp(-worldfragment.y*0.01)); // integral(e^(worldfragment.y))
  float F = u * integral / (vCameraPosition.y - worldfragment.y);

  // Compute alpha value
  fragmentcolor.a = 1.2 - exp(-F);

  gl_FragColor = fragmentcolor;
}