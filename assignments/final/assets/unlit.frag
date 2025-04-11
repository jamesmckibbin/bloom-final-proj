#version 450

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor; 

in Surface{
	vec3 WorldPos;
	vec3 WorldNormal;
	vec2 TexCoord;
}fs_in;

uniform sampler2D _MainTex;

void main(){
	vec3 color = texture(_MainTex, fs_in.TexCoord).rgb;
	FragColor = vec4(color,1.0);
	float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 0.2)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}