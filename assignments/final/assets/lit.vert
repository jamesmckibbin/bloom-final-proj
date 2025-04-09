#version 450

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in vec3 vTangent; 

uniform mat4 _Model;
uniform mat4 _ViewProjection;

out Surface{
	vec3 WorldPos;
	vec3 WorldNormal;
	vec2 TexCoord;
	mat3 TBN;
}vs_out;

void main(){
	vs_out.WorldPos = vec3(_Model * vec4(vPos,1.0));
	vs_out.WorldNormal = transpose(inverse(mat3(_Model))) * vNormal;
	vs_out.TexCoord = vTexCoord;

	vec3 T = normalize(vec3(_Model * vec4(vTangent, 0.0)));
	vec3 N = normalize(vec3(_Model * vec4(vNormal, 0.0)));
	vec3 B = cross(N, T);
	vs_out.TBN = transpose(mat3(T, B, N));
	gl_Position = _ViewProjection * _Model * vec4(vPos,1.0);
}
