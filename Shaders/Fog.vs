/******************************************************************************/
/*!
\file   Fog.vs
\author Hoh Shi Chao
\par    email: s.hoh\@digipen.edu
\par    DigiPen login: s.hoh
\par    Course: CS370
\date   23/4/2013
\brief  
  Setup for Real-Time Fog for Post-processing
*/
/******************************************************************************/

attribute vec3 aVertex;
attribute vec2 aTexCoord;

varying vec2 vTexCoord;
//varying vec3 vCameraEyePos;
varying vec3 vCameraViewVec;
varying vec3 vCameraPosition;
varying vec3 vFragmentVec;

uniform vec3 uCameraEyePos;
uniform vec3 uCameraViewVec;

uniform vec3 uNearTopLeft;
uniform vec3 uNearTopRight;
uniform vec3 uNearBottomLeft;
uniform vec3 uNearBottomRight;


bool Equal(float left, float right)
{
	if(abs(left - right) < 0.00001)
	{
		return true;
	}
	return false;
}

void main(void) 
{ 
  gl_Position = vec4(aVertex, 1.0);
  vTexCoord = aTexCoord;

  vCameraPosition = uCameraEyePos * -2.0;//gl_Position;
  vCameraViewVec = uCameraViewVec;

  if(Equal(aVertex.x, -1.0) && Equal(aVertex.y, 1.0))
  {
	vFragmentVec = uNearTopLeft;
  }
  else if(Equal(aVertex.x, 1.0) && Equal(aVertex.y, 1.0))
  {
	vFragmentVec = uNearTopRight;
  }
  else if(Equal(aVertex.x, -1.0) && Equal(aVertex.y, -1.0))
  {
	vFragmentVec = uNearBottomLeft;
  }
  else if(Equal(aVertex.x, 1.0) && Equal(aVertex.y, -1.0))
  {
	vFragmentVec = uNearBottomRight;
  }
  else
  {
	vFragmentVec = uCameraViewVec;
  }
}