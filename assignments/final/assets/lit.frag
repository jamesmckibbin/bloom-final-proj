#version 450

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor; 

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
};

struct PointLight {    
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    float constant;
    float linear;
    float quadratic;
    float intensity;
};

struct Material{
	float AmbientMod;
	float DiffuseMod;
	float SpecularMod;
	float Shininess;
};

struct MiscSettings{
	bool UseDirLight;
};

uniform sampler2D _MainTex;
uniform vec3 _EyePos;
uniform DirLight _GlobalLight;
uniform PointLight _Cube1Light;
uniform PointLight _Cube2Light;

uniform vec3 _Cube1Position;
uniform vec3 _Cube1LightColor = vec3(1.0, 1.0, 0.0);
uniform vec3 _Cube2Position;
uniform vec3 _Cube2LightColor = vec3(0.0, 0.0, 1.0);

uniform Material _Material;
uniform MiscSettings _Settings;
uniform sampler2D _NormalMap;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float specularFactor = pow(max(dot(viewDir, reflectDir), 0.0), _Material.Shininess);
    vec3 ambient  = _Material.AmbientMod * light.ambient * vec3(texture(_MainTex, fs_in.TexCoord));
    vec3 diffuse  = _Material.DiffuseMod * light.diffuse * diffuseFactor * vec3(texture(_MainTex, fs_in.TexCoord));
    vec3 specular = _Material.SpecularMod * specularFactor * vec3(texture(_MainTex, fs_in.TexCoord));
    return (ambient + diffuse + specularFactor);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 worldPos, vec3 toEye)
{
    vec3 lightDir = normalize(light.position - worldPos);
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
	vec3 h = normalize((light.position - worldPos) + toEye);
    float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);

    float distance = length(light.position - worldPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

    vec3 ambient  = _Material.AmbientMod * light.ambient  * vec3(texture(_MainTex, fs_in.TexCoord));
    vec3 diffuse  = _Material.DiffuseMod * light.diffuse  * light.intensity * diffuseFactor * vec3(texture(_MainTex, fs_in.TexCoord));
    vec3 specular = _Material.SpecularMod * specularFactor * vec3(texture(_MainTex, fs_in.TexCoord));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

void main() {
	vec3 normal = normalize(fs_in.WorldNormal);
	vec3 toEye = normalize(_EyePos - fs_in.WorldPos);

    vec3 lightColor;

    if (_Settings.UseDirLight) {
        lightColor += CalcDirLight(_GlobalLight, normal, toEye);
    }
    lightColor += CalcPointLight(_Cube1Light, normal, fs_in.WorldPos, toEye);
    lightColor += CalcPointLight(_Cube2Light, normal, fs_in.WorldPos, toEye);

	FragColor = vec4(lightColor, 1.0);
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.1)
        BrightColor = vec4(FragColor.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
