attribute vec3 aVertex;
attribute vec3 aNormal; 
attribute vec2 aTexCoord;
attribute vec4 aVertexCol;

//Environment properties
uniform float uFogNear, uFogFar;
uniform mat4 uViewMtx;
uniform mat4 uProjMtx;

varying vec2 vTexCoord;
varying vec3 vNormal;
varying vec4 vViewPos;
varying vec4 vColor;
varying float vFogValue;

void main(void) 
{ 
  /////////////////////////////////////////////////////////
  //Transform position
  //World position
  vec4 worldPos;
  vec4 vert = vec4(aVertex, 1.0);
  worldPos.x = dot(gl_MultiTexCoord1, vert);
  worldPos.y = dot(gl_MultiTexCoord2, vert);
  worldPos.z = dot(gl_MultiTexCoord3, vert);
  worldPos.w = 1.0;
  
  //View Position
  vViewPos = uViewMtx * worldPos;
  
  //Screen Position
  gl_Position = uProjMtx * vViewPos;
  
  /////////////////////////////////////////////////////////
  //Transform Normals
  vec3 worldNormal;
  worldNormal.x = dot(gl_MultiTexCoord1.xyz, aNormal);
  worldNormal.y = dot(gl_MultiTexCoord2.xyz, aNormal);
  worldNormal.z = dot(gl_MultiTexCoord3.xyz, aNormal);
  worldNormal = normalize(worldNormal);
  
  vNormal = (uViewMtx * vec4(worldNormal, 0.0)).xyz;
  
  //Output texture coordinates
  vTexCoord = aTexCoord; 
  
  //Output colour
  vColor = gl_MultiTexCoord5 * aVertexCol;
  
  //Calculate fogValue (0.0 when fragment is closer than uFogNear, approaches 1.0 at uFogFar)
  //vFogValue = max(0.0, (length(vViewPos.xyz) - uFogNear) / (uFogFar - uFogNear));
}