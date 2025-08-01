#version 450

layout(location = 0) out float outColor;

layout(location = 0) flat in struct data_transfer_flat{
	uint ID;
	uint diffuse_index;
} in_data_transfer_flat;

layout(location = 2) in struct data_transfer{
	vec2 tex_coord;
} in_data_transfer;

layout(binding = 2) buffer ShaderStorageBufferObject{
    uint Selected_ID;
}ssbo;

layout(binding = 3) uniform sampler2D samplers[400];

void main(){
    vec4 diff_samp = texture(samplers[in_data_transfer_flat.diffuse_index], in_data_transfer.tex_coord);
    if(diff_samp.a == 0){
        discard;
    }
    //only needed for debugging to draw to color attachment
    outColor = in_data_transfer_flat.ID;
	ssbo.Selected_ID = in_data_transfer_flat.ID;

}
