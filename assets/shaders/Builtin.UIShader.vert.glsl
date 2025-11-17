#version 450

//Attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inGeometryTexCoord;

// Uniforms
layout(binding = 0) uniform UniformBufferVertexObject {
	mat4 view;
	mat4 proj;
	vec3 view_position;
} ubo;

struct quad_properties {
	mat4 model;
	vec3 diffuse_color;
	vec2 texture_coordinates[4];
	uint id;
	uint diffuse_index;
};

layout(std430, binding = 2) readonly buffer quad_buffer{
	quad_properties quads[];
} quad_buffer_ssbo;

layout(location = 0) flat out struct data_transfer_flat{
	uint diffuse_index;
} out_data_transfer_flat;

layout(location = 5) out struct data_transfer{
	vec2 tex_coord;
	vec3 diffuse_color;
} out_data_transfer;


void main() {
	mat3 model_m3 = mat3(quad_buffer_ssbo.quads[gl_InstanceIndex].model);

	gl_Position = ubo.proj * ubo.view * quad_buffer_ssbo.quads[gl_InstanceIndex].model * vec4(inPosition, 1.0);

	out_data_transfer.tex_coord = quad_buffer_ssbo.quads[gl_InstanceIndex].texture_coordinates[gl_VertexIndex];
	out_data_transfer.diffuse_color = quad_buffer_ssbo.quads[gl_InstanceIndex].diffuse_color;
	
	out_data_transfer_flat.diffuse_index = quad_buffer_ssbo.quads[gl_InstanceIndex].diffuse_index;
}