#version 450

out vec4 FragColor;

in Surface{
	vec3 WorldPos;
	vec3 WorldNormal;
	vec2 TexCoord;
	mat3 TBN;
}fs_in;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {    
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 2
uniform PointLight pointLights[NR_POINT_LIGHTS];

struct Material{
	float Ka;
	float Kd;
	float Ks;
	float Shininess;
};

struct MiscSettings{
	bool NormalMap;
};

uniform sampler2D _MainTex;
uniform vec3 _EyePos;
uniform DirLight _GlobalLight;
uniform vec3 _LightDirection = vec3(0.0,-1.0,0.0);
uniform vec3 _LightColor = vec3(1.0);
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);

uniform vec3 _Cube1Position;
uniform vec3 _Cube1LightColor = vec3(1.0, 1.0, 0.0);
uniform vec3 _Cube2Position;
uniform vec3 _Cube2LightColor = vec3(0.0, 0.0, 1.0);

uniform Material _Material;
uniform MiscSettings _Settings;
uniform sampler2D _NormalMap;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 toEye, vec3 toLight)
{
    float diffuseFactor = max(dot(normal,toLight),0.0);
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;
	lightColor+=_AmbientColor * _Material.Ka;
	return lightColor;
}

void main(){

	_GlobalLight.direction = vec3(0.0, -1.0, 0.0);

	vec3 normal;
	vec3 toLight;
	vec3 toEye;

	if (_Settings.NormalMap) {
		normal = texture(_NormalMap, fs_in.TexCoord).rgb;
		normal = normalize(normal * 2.0 - 1.0);
		normal = normalize(fs_in.TBN * normal);
		toLight = fs_in.TBN * normalize(-_LightDirection);
		toEye  = fs_in.TBN * normalize(_EyePos - fs_in.WorldPos);
	} else {
		normal = normalize(fs_in.WorldNormal);
		toLight = normalize(-_LightDirection);
		toEye  = normalize(_EyePos - fs_in.WorldPos);
	}

	vec3 lightColor = CalcDirLight();

	vec3 objectColor = texture(_MainTex,fs_in.TexCoord).rgb;
	FragColor = vec4(objectColor * lightColor,1.0);
}
