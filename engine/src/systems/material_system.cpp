#include "material_system.h"
#include "cepch.h"

#include "core/logger.h"

#include "loaders/resources_types.inl"
#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "systems/shader_system.h"

namespace caliope {

	typedef struct material_system_state {
		std::unordered_map<std::string, material> registered_materials;
		material default_material;
	}material_system_state;

	static std::unique_ptr<material_system_state> state_ptr;

	bool load_material(material_configuration& mat_config);
	void destroy_material(material& m);

	void generate_default_material();

	bool material_system_initialize() {
		state_ptr = std::make_unique<material_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		generate_default_material();

		CE_LOG_INFO("Material system_initialized.");
		return true;
	}
	void material_system_shutdown() {
		destroy_material(state_ptr->default_material);

		state_ptr->registered_materials.empty(); // Destroys all the materials and their pointers.
		state_ptr.reset();
		state_ptr = nullptr;
	}

	material* material_system_adquire(std::string& name) {
		if (state_ptr->registered_materials.find(name) == state_ptr->registered_materials.end()) {

			resource r;
			if(!resource_system_load(name, RESOURCE_TYPE_MATERIAL, r)){
				CE_LOG_ERROR("material_system_adquire couldnt load file material");
				return material_system_get_default();
			}
			material_configuration mat_config = std::any_cast<material_configuration>(r.data);
			resource_system_unload(r);


			if (!load_material(mat_config)) {
				CE_LOG_ERROR("material_system_adquire couldnt adquire material");
				return material_system_get_default();
			}

		}
				
		return &state_ptr->registered_materials[name];
	}

	material* material_system_adquire_from_config(material_configuration& material_config) {
		std::string string_material_name = std::string(material_config.name.data());

		if (state_ptr->registered_materials.find(string_material_name) == state_ptr->registered_materials.end()) {

			if (!load_material(material_config)) {
				CE_LOG_ERROR("material_system_adquire couldnt adquire material");
				return false;
			}

		}

		return &state_ptr->registered_materials[string_material_name];
	}

	void material_system_release(std::string& name) {
		if (state_ptr->registered_materials.find(name) == state_ptr->registered_materials.end()) {
			destroy_material(state_ptr->registered_materials[name]);
			state_ptr->registered_materials.erase(name);
		}
	}

	material* material_system_get_default() {
		return &state_ptr->default_material;
	}

	bool load_material(material_configuration& mat_config) {
		material m;
		m.name = std::string(mat_config.name.data());
		m.diffuse_color = mat_config.diffuse_color;
		m.shininess_intensity = mat_config.shininess_intensity;
		m.shininess_sharpness = mat_config.shininess_sharpness;
		m.shader = shader_system_adquire(std::string(mat_config.shader_name.data()));

		m.diffuse_texture = texture_system_adquire(std::string(mat_config.diffuse_texture_name.data()));
		m.specular_texture = texture_system_adquire(std::string(mat_config.specular_texture_name.data()));
		m.normal_texture = texture_system_adquire(std::string(mat_config.normal_texture_name.data()));

		state_ptr->registered_materials.insert({ m.name, m });

		return true;
	}

	void destroy_material(material& m) {
		state_ptr->registered_materials.erase(m.name);
		m.name = "";
		m.diffuse_color = glm::vec4(0.0f);
		m.shader.reset();
		m.shader = nullptr;
		m.diffuse_texture = nullptr;
		m.specular_texture = nullptr;
		m.normal_texture = nullptr;
	}

	void generate_default_material() {
		state_ptr->default_material.name = std::string("default");
		state_ptr->default_material.diffuse_color = glm::vec4(1.0f);
		state_ptr->default_material.shader = shader_system_adquire(std::string("Builtin.SpriteShader"));
		state_ptr->default_material.diffuse_texture = texture_system_get_default_diffuse();
		state_ptr->default_material.specular_texture = texture_system_get_default_specular();
		state_ptr->default_material.normal_texture = texture_system_get_default_normal();
	}
}