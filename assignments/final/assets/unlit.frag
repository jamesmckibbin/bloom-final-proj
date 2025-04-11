#version 450

out vec4 FragColor;

in Surface{
	vec3 WorldPos;
	vec3 WorldNormal;
	vec2 TexCoord;
}fs_in;

uniform sampler2D _MainTex;

void main(){
	vec3 color = texture(_MainTex, fs_in.TexCoord).rgb;
	FragColor = vec4(color,1.0);
}