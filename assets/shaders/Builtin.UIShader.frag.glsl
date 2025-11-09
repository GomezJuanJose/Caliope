#version 450

layout(location = 0) out vec4 outColor;



layout(location = 0) flat in struct data_transfer_flat{
	uint diffuse_index;
} in_data_transfer_flat;

layout(location = 5) in struct data_transfer{
	vec2 tex_coord;
	vec3 diffuse_color;
} in_data_transfer;


layout(binding = 1) uniform sampler2D samplers[400];

void main(){
	vec4 diff_samp = texture(samplers[in_data_transfer_flat.diffuse_index], in_data_transfer.tex_coord);
	outColor =  diff_samp * vec4(in_data_transfer.diffuse_color.rgb, 1.0);
}
