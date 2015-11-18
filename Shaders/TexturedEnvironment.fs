//Material properties
uniform sampler2D uColorMap, uEmissiveMap, uSpecularMap, uvNormalMap;

varying vec2 vTexCoord;
varying vec3 vNormal;
varying vec4 vViewPos;
varying vec4 vColor;
varying float vFogValue;

//Environment properties
uniform vec4 uFogColor;
uniform float uFogNear, uFogFar;
uniform vec4 uGlobalAmbient;

void main()
{
    float lodbias = 0.0;
    vec4 col = texture2D(uColorMap, vTexCoord, lodbias) * vColor;
    
    //Calculate fogValue (0.0 when fragment is closer than uFogNear, approaches 1.0 at uFogFar)
    //float fogValue = max(0.0, (length(vViewPos.xyz) - uFogNear) / (uFogFar - uFogNear));
    col.a *= 1.0 - ((gl_FragCoord.z / gl_FragCoord.w - uFogNear) / (uFogFar - uFogNear));	//This is to fade to skybox
    
    gl_FragColor = col;
}