#version 120

uniform sampler2D fbo_texture;
uniform float coefficients[144];  // 144 = 12 x 12
uniform float offset;
uniform int ksize; // kernel size : 12 x 12

varying vec2 f_texCoord;

void main(void)
{
    float d = 0.1;      
    float left = f_texCoord.s - offset - offset;
    float top = f_texCoord.t - offset - offset;
    vec2 tc = vec2(left, top);
    vec4 c = vec4(0, 0, 0, 0);
	int sizesquare = ksize * ksize;


	for(int i = 0; i < sizesquare; ++i)
	{
	
		if( mod(i,ksize)  != ksize-1)
		{
			c += coefficients[i] * texture2D(fbo_texture, tc); tc.x += offset;
		}
		else
		{
			c += coefficients[i] * texture2D(fbo_texture, tc); tc.y += offset;
			tc.x = left;
		}

	}

		gl_FragColor = c;	
}

