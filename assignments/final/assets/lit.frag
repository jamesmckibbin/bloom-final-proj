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
};

struct PointLight {    
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    float intensity;
};

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
    vec3 ambient  = _Material.Ka * light.ambient * vec3(texture(_MainTex, fs_in.TexCoord));
    vec3 diffuse  = _Material.Kd * light.diffuse * diffuseFactor * vec3(texture(_MainTex, fs_in.TexCoord));
    vec3 specular = specularFactor * vec3(texture(_MainTex, fs_in.TexCoord));
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

    vec3 ambient  = _Material.Ka * light.ambient  * vec3(texture(_MainTex, fs_in.TexCoord));
    vec3 diffuse  = _Material.Kd * light.diffuse  * light.intensity * diffuseFactor * vec3(texture(_MainTex, fs_in.TexCoord));
    vec3 specular = _Material.Ks * specularFactor * vec3(texture(_MainTex, fs_in.TexCoord));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

void main() {
	vec3 normal = normalize(fs_in.WorldNormal);
	vec3 toEye = normalize(_EyePos - fs_in.WorldPos);

	//vec3 lightColor = CalcDirLight(_GlobalLight, normal, toEye);
    vec3 lightColor = CalcPointLight(_Cube1Light, normal, fs_in.WorldPos, toEye);
    lightColor += CalcPointLight(_Cube2Light, normal, fs_in.WorldPos, toEye);

	FragColor = vec4(lightColor, 1.0);
}
