#version 450

out vec4 FragColor;

in vec2 UV;

uniform sampler2D _ScreenTexture;
uniform sampler2D _BloomBlur;
uniform float _Exposure;
uniform float _Gamma;

void main(){
    vec3 hdrColor = texture(_ScreenTexture, UV).rgb;
    vec3 bloomColor = texture(_BloomBlur, UV).rgb;
    hdrColor += bloomColor;
    vec3 mapped = vec3(1.0) - exp(-hdrColor * _Exposure);
    mapped = pow(mapped, vec3(1.0 / _Gamma));
    FragColor = vec4(mapped, 1.0);
}
