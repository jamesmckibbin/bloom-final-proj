#version 450

out vec4 FragColor;

in vec2 UV;

uniform sampler2D _ScreenTexture;

void main(){
	vec3 color = texture(_ScreenTexture,UV).rgb;
	FragColor = vec4(color,1.0);
}
