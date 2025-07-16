#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) flat in struct data_transfer_flat{
	float shininess_intensity;
	float shininess_sharpness;
	uint diffuse_index;
	uint normal_index;
	uint specular_index;
} in_data_transfer_flat;

layout(location = 5) in struct data_transfer{
	vec4 ambient;
	vec2 tex_coord;
	vec3 normal;
	vec4 tangent;
	vec3 diffuse_color;
	vec3 view_position;
	vec3 frag_position;
} in_data_transfer;


layout(binding = 1) uniform sampler2D samplers[40];

mat3 TBN; // TODO: Move to vertex shader

// TODO: Move to a lighting system
struct point_light {
	vec3 position;
	vec4 color;
	float radius;
	float constant;
	float linear;
	float quadratic;
};

point_light p_light_0 = {	// Also the color describes the instensity of the light
	vec3 (0.0, 0.0, 1.0), // Always put the light at 1 to avoid problems with the lighting calculations with the surface of the quads, if the z value of the lights and quads are the same the light will do rare results
	vec4 (0.4, 0.2, 0.0, 1.0),
	1.0,
	1.0,
    0.35,
    0.44
};

vec4 calculate_point_light(point_light light, vec3 material_diffuse_color, vec3 normal, vec3 frag_position, vec3 view_direction);

void main(){
	vec3 normal = in_data_transfer.normal;
	vec3 tangent = in_data_transfer.tangent.xyz;
	tangent = (tangent - dot(tangent, normal) * normal);
	vec3 bitangent = cross(in_data_transfer.normal, in_data_transfer.tangent.xyz) * in_data_transfer.tangent.w;
	TBN = mat3(tangent, bitangent, normal);

	// Update the normal to use a sample from the normal map.
	vec3 local_normal = 2.0 * texture(samplers[int(in_data_transfer_flat.normal_index)], in_data_transfer.tex_coord).rgb - 1.0;
	normal = normalize(TBN * local_normal);

	vec3 view_direction = normalize(in_data_transfer.view_position - in_data_transfer.frag_position);

	
	outColor = calculate_point_light(p_light_0, in_data_transfer.diffuse_color, normal, in_data_transfer.frag_position, view_direction);// TODO: Do for N lights and in case of 0 lights only use ambient or a default one big
}

vec4 calculate_point_light(point_light light, vec3 material_diffuse_color, vec3 normal, vec3 frag_position, vec3 view_direction){
	vec3 light_direction = normalize(light.position - frag_position);
	float diff = max(dot(normal, light_direction), 0.0);

	vec3 reflect_direction = reflect(-light_direction, normal);
	float spec = pow(max(dot(view_direction, reflect_direction), 0.0), in_data_transfer_flat.shininess_sharpness);

	// Calculate attenuation or light falloff over distance.
	float distance = length(light.position - frag_position);
	float distance_normalized = distance / light.radius;
	float attenuation = 1.0 / (light.constant + light.linear * distance_normalized + light.quadratic * (distance_normalized * distance_normalized));

	vec3 ambient = vec3(in_data_transfer.ambient.rgb);
	vec3 diffuse = light.color.rgb * diff;
	vec3 specular = light.color.rgb * spec * in_data_transfer_flat.shininess_intensity;

	vec4 diff_samp = texture(samplers[in_data_transfer_flat.diffuse_index], in_data_transfer.tex_coord);
	diff_samp.rgb *= material_diffuse_color.rgb;
	diffuse *= diff_samp.rgb;
	ambient *= diff_samp.rgb;
	specular *= texture(samplers[in_data_transfer_flat.specular_index], in_data_transfer.tex_coord).rgb;

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
	return vec4(ambient + diffuse + specular, diff_samp.a);
}