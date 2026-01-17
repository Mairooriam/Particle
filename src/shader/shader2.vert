#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 models[10];
    vec3 instanceColors[10];
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inPosition;
// layout(location = 1) in vec3 inColor;

layout(location = 0) out vec4 fragColor;  

void main() {
    mat4 model = ubo.models[gl_InstanceIndex];
    gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 0.0, 1.0);
    fragColor = vec4(ubo.instanceColors[gl_InstanceIndex], 1.0); 
}