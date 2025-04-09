#version 450

out vec4 FragColor;

in Surface{
	vec3 WorldPos;
	vec3 WorldNormal;
	vec2 TexCoord;
	mat3 TBN;
}fs_in;

uniform sampler2D _MainTex;
uniform vec3 _EyePos;
uniform vec3 _LightDirection = vec3(0.0,-1.0,0.0);
uniform vec3 _LightColor = vec3(1.0);
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);

struct Material{
	float Ka;
	float Kd;
	float Ks;
	float Shininess;
};

struct MiscSettings{
	bool NormalMap;
};

uniform Material _Material;
uniform MiscSettings _Settings;
uniform sampler2D _NormalMap;


void main(){
	vec3 normal;
	vec3 toLight;
	vec3 toEye;

	if (_Settings.NormalMap) {
		normal = texture(_NormalMap, fs_in.TexCoord).rgb;
		normal = normalize(normal * 2.0 - 1.0);
		normal = normalize(fs_in.TBN * normal);
		toLight = fs_in.TBN * -_LightDirection;
		toEye  = fs_in.TBN * normalize(_EyePos - fs_in.WorldPos);
	} else {
		normal = normalize(fs_in.WorldNormal);
		toLight = -_LightDirection;
		toEye  = normalize(_EyePos - fs_in.WorldPos);
	}

	float diffuseFactor = max(dot(normal,toLight),0.0);
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;
	lightColor+=_AmbientColor * _Material.Ka;
	vec3 objectColor = texture(_MainTex,fs_in.TexCoord).rgb;
	FragColor = vec4(objectColor * lightColor,1.0);
}
