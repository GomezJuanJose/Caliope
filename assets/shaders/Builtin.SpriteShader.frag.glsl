#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in struct data_transfer{
	vec4 ambient;
	vec2 tex_coord;
	vec3 normal;
	vec4 tangent;
	vec3 view_position;
	vec3 frag_position;
	float shininess;// TODO: Do not upload in the vertex, upload directly to the fragment
} in_data_transfer;

//const int SAMP_DIFFUSE = 0;
//const int SAMP_SPECULAR = 1;
//const int SAMP_NORMAL = 2;
//layout(binding = 1) uniform sampler2D samplers[3];

mat3 TBN; // TODO: Move to vertex shader

// TODO: Move to a lighting system
struct point_light {
	vec3 position;
	vec4 color;
	float constant;
	float linear;
	float quadratic;
};

point_light p_light_0 = {
	vec3 (0.0, 0.0, 0.1),
	vec4 (1.0, 0.5, 0.0, 1.0),
	1.0,
	0.25,
	0.24
};

vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction);

void main(){
	vec3 normal = in_data_transfer.normal;
	vec3 tangent = in_data_transfer.tangent.xyz;
	tangent = (tangent - dot(tangent, normal) * normal);
	vec3 bitangent = cross(in_data_transfer.normal, in_data_transfer.tangent.xyz) * in_data_transfer.tangent.w;
	TBN = mat3(tangent, bitangent, normal);

	// Update the normal to use a sample from the normal map.
	vec3 local_normal = 2.0 * vec3(0.0, 0.0, 1.0) - 1.0;//texture(samplers[SAMP_NORMAL], in_data_transfer.tex_coord).rgb - 1.0;
	normal = normalize(TBN * local_normal);

	vec3 view_direction = normalize(in_data_transfer.view_position - in_data_transfer.frag_position);

	
	outColor = calculate_point_light(p_light_0, normal, in_data_transfer.frag_position, view_direction);// TODO: Do for N lights
}

vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction){
	vec3 light_direction = normalize(light.position - frag_position);
	float diff = max(dot(normal, light_direction), 0.0);

	vec3 reflect_direction = reflect(-light_direction, normal);
	float spec = pow(max(dot(view_direction, reflect_direction), 0.0), in_data_transfer.shininess);

	// Calculate attenuation or light falloff over distance.
	float distance = length(light.position - frag_position);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	vec3 ambient = vec3(in_data_transfer.ambient);
	vec3 diffuse = vec3(light.color * diff);
	vec3 specular = vec3(light.color * spec);

	vec4 diff_samp = vec4(1.0, 1.0, 1.0, 1.0);//texture(samplers[SAMP_DIFFUSE], in_data_transfer.tex_coord);
	diffuse *= diff_samp.xyz;
	ambient *= diff_samp.xyz;
	specular *= vec3(0.0, 0.0, 0.0); //vec3(texture(samplers[SAMP_SPECULAR], in_data_transfer.tex_coord).rgb);

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
	return vec4((ambient + diffuse + specular), diff_samp.a);
}