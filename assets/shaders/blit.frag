#version 450 core

layout(push_constant) uniform HDRSettings {
    float Exposure;
    float Gamma;
};

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler textureSampler;
layout(set = 0, binding = 1) uniform texture2D albedoTexture;
layout(set = 0, binding = 2) uniform texture2D depthTexture;

layout(location = 0) in struct {
    vec2 texcoord;
} v_in;

void main() {
    vec3 color = texture(sampler2D(albedoTexture, textureSampler), v_in.texcoord).rgb;

    color = 1.0f - exp(-color * Exposure);
    color = pow(color, vec3(1.0f / Gamma));
    out_color = vec4(color, 1.0f);
}