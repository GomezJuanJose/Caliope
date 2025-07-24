#version 450

//Attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inTangent;
layout(location = 2) in vec2 inGeometryTexCoord;

// Uniforms
layout(binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
	vec4 ambient_color;
	vec3 view_position;
} ubo;

struct quad_properties {
	mat4 model;
	vec3 diffuse_color;
	vec2 texture_coordinates[4];
	uint diffuse_index;
	uint normal_index;
	uint specular_index;
	float shininess_intensity;
	float shininess_sharpness;
};

layout(std430, binding = 2) readonly buffer quad_buffer{
	quad_properties quads[];
} quad_buffer_ssbo;

layout(location = 0) flat out struct data_transfer_flat{
	float shininess_intensity;
	float shininess_sharpness;
	uint diffuse_index;
	uint normal_index;
	uint specular_index;
} out_data_transfer_flat;

layout(location = 5) out struct data_transfer{
	vec4 ambient;
	vec2 tex_coord;
	vec3 normal;
	vec4 tangent;
	vec3 diffuse_color;
	vec3 view_position;
	vec3 frag_position;
} out_data_transfer;


void main() {
	mat3 model_m3 = mat3(quad_buffer_ssbo.quads[gl_InstanceIndex].model);

	gl_Position = ubo.proj * ubo.view * quad_buffer_ssbo.quads[gl_InstanceIndex].model * vec4(inPosition, 1.0);

	out_data_transfer.ambient = ubo.ambient_color;
	out_data_transfer.tex_coord = quad_buffer_ssbo.quads[gl_InstanceIndex].texture_coordinates[gl_VertexIndex];
	out_data_transfer.normal = normalize(model_m3 * vec3(0, 0, 1));
	out_data_transfer.tangent = vec4(normalize(model_m3 * inTangent.xyz), inTangent.w);
	out_data_transfer.view_position = ubo.view_position;
	out_data_transfer.frag_position = vec3(quad_buffer_ssbo.quads[gl_InstanceIndex].model * vec4(inPosition, 1.0));
	out_data_transfer.diffuse_color = quad_buffer_ssbo.quads[gl_InstanceIndex].diffuse_color;
	
	out_data_transfer_flat.shininess_intensity = quad_buffer_ssbo.quads[gl_InstanceIndex].shininess_intensity;
	out_data_transfer_flat.shininess_sharpness = quad_buffer_ssbo.quads[gl_InstanceIndex].shininess_sharpness;
	out_data_transfer_flat.diffuse_index = quad_buffer_ssbo.quads[gl_InstanceIndex].diffuse_index;
	out_data_transfer_flat.normal_index = quad_buffer_ssbo.quads[gl_InstanceIndex].normal_index;
	out_data_transfer_flat.specular_index = quad_buffer_ssbo.quads[gl_InstanceIndex].specular_index;
}