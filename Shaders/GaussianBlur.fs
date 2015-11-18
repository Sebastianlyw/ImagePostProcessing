/******************************************************************************/
/*!
\file   GaussianBlur.fs
\author Hoh Shi Chao
\par    email: s.hoh\@digipen.edu
\par    DigiPen login: s.hoh
\par    Course: CS370
\date   3/1/2013
\brief  
  Simple Gaussian Blur
*/
/******************************************************************************/

#define KERNEL_SIZE 9

// Gaussian kernel
// 1 2 1
// 2 4 2
// 1 2 1	
float kernel[KERNEL_SIZE];

varying vec2 vTexCoord;

uniform sampler2D uColorMap;
uniform float uMapWidth;
uniform float uMapHeight;

float step_w = 1.0/uMapWidth;
float step_h = 1.0/uMapHeight;

vec2 offset[KERNEL_SIZE];

uniform float uRadius;

void main(void)
{
    int i = 0;
    vec4 sum = vec4(0.0);
   
   offset[0] = vec2(-step_w, -step_h);
   offset[1] = vec2(0.0, -step_h);
   offset[2] = vec2(step_w, -step_h);
   
   offset[3] = vec2(-step_w, 0.0);
   offset[4] = vec2(0.0, 0.0);
   offset[5] = vec2(step_w, 0.0);
   
   offset[6] = vec2(-step_w, step_h);
   offset[7] = vec2(0.0, step_h);
   offset[8] = vec2(step_w, step_h);
   
   kernel[0] = 1.0/16.0; 	kernel[1] = 2.0/16.0;	kernel[2] = 1.0/16.0;
   kernel[3] = 2.0/16.0;	kernel[4] = 4.0/16.0;	kernel[5] = 2.0/16.0;
   kernel[6] = 1.0/16.0;   	kernel[7] = 2.0/16.0;	kernel[8] = 1.0/16.0;
   
   for( i=0; i<KERNEL_SIZE; i++ )
   {
    vec4 tmp = texture2D(uColorMap, vTexCoord.xy + offset[i] * uRadius);
    sum += tmp * kernel[i];
   }

   gl_FragColor = sum;
}