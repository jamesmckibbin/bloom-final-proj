#version 450

out vec2 TexCoords;

vec4 vertices[3] = {
	vec4(-1,-1,0,0),
	vec4(3,-1,2,0),
	vec4(-1,3,0,2)
};

void main() {
	TexCoords = vertices[gl_VertexID].zw;
	gl_Position = vec4(vertices[gl_VertexID].xy,0,1);
}