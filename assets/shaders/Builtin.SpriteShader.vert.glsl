#version 450

//Attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;

// Uniforms
layout(binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
	vec4 ambient_color;
	vec3 view_position;
	float shininess;
} ubo;

struct quad_properties {
	mat4 model;
};

layout(std140, binding = 2) readonly buffer quad_buffer{
	quad_properties quads[];
} quad_buffer_ssbo;

layout(location = 0) out struct data_transfer{
	vec4 ambient;
	vec2 tex_coord;
	vec3 normal;
	vec4 tangent;
	vec3 view_position;
	vec3 frag_position;
	float shininess;
} out_data_transfer;

void main() {
	mat3 model_m3 = mat3(quad_buffer_ssbo.quads[gl_InstanceIndex].model);

	gl_Position = ubo.proj * ubo.view * quad_buffer_ssbo.quads[gl_InstanceIndex].model * vec4(inPosition, 1.0);

	out_data_transfer.ambient = ubo.ambient_color;
	out_data_transfer.tex_coord = inTexCoord;
	out_data_transfer.normal = normalize(model_m3 * inNormal);
	out_data_transfer.tangent = vec4(normalize(model_m3 * inTangent.xyz), inTangent.w);
	out_data_transfer.view_position = ubo.view_position;
	out_data_transfer.frag_position = vec3(quad_buffer_ssbo.quads[gl_InstanceIndex].model * vec4(inPosition, 1.0));
	out_data_transfer.shininess = ubo.shininess;
}