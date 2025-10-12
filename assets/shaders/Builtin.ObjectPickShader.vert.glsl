#version 450

//Attributes
layout(location = 0) in vec3 inPosition;

// Uniforms
layout(binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
	vec4 ambient_color;
	vec3 view_position;
} ubo;

struct pick_sprite_properties {
	mat4 model;
	vec2 texture_coordinates[4];
	uint id;
	uint diffuse_index;
};

layout(std430, binding = 1) readonly buffer quad_buffer{
	pick_sprite_properties quads[];
} quad_pick_buffer_ssbo;


layout(location = 0) flat out struct data_transfer_flat{
	uint ID;
	uint diffuse_index;
} out_data_transfer_flat;

layout(location = 2) out struct data_transfer{
	vec2 tex_coord;
} out_data_transfer;

void main() {
	gl_Position = ubo.proj * ubo.view * quad_pick_buffer_ssbo.quads[gl_InstanceIndex].model * vec4(inPosition, 1.0);
	
	out_data_transfer_flat.ID = quad_pick_buffer_ssbo.quads[gl_InstanceIndex].id;
	out_data_transfer_flat.diffuse_index = quad_pick_buffer_ssbo.quads[gl_InstanceIndex].diffuse_index;

	out_data_transfer.tex_coord = quad_pick_buffer_ssbo.quads[gl_InstanceIndex].texture_coordinates[gl_VertexIndex];
}