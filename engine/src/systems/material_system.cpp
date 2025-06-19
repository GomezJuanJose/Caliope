#include "material_system.h"
#include "cepch.h"

#include "core/logger.h"

#include "loaders/resources_types.inl"
#include "systems/resource_system.h"
#include "systems/texture_system.h"

namespace caliope {

	typedef struct material_system_state{
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
		for (auto [key, value] : state_ptr->registered_materials) {
			destroy_material(value);
		}
		destroy_material(state_ptr->default_material);

		state_ptr->registered_materials.empty();
		state_ptr.reset();
	}

	std::shared_ptr<material> material_system_adquire(std::string& name) {
		if (state_ptr->registered_materials.find(name) == state_ptr->registered_materials.end()) {
			
			resource r;
			resource_system_load(name, RESOURCE_TYPE_MATERIAL, r);
			material_configuration mat_config = std::any_cast<material_configuration>(r.data);
			
			if (!load_material(mat_config)) {
				CE_LOG_ERROR("material_system_adquire couldnt adquire material");
				return false;
			}

		}
				
		return std::make_shared<material>(state_ptr->registered_materials[name]);
	}

	std::shared_ptr<material> material_system_adquire_from_config(material_configuration& material_config) {

		if (state_ptr->registered_materials.find(material_config.name) == state_ptr->registered_materials.end()) {

			if (!load_material(material_config)) {
				CE_LOG_ERROR("material_system_adquire couldnt adquire material");
				return false;
			}

		}

		return std::make_shared<material>(state_ptr->registered_materials[material_config.name]);
	}

	void material_system_release(std::string& name) {
		if (state_ptr->registered_materials.find(name) == state_ptr->registered_materials.end()) {
			destroy_material(state_ptr->registered_materials[name]);
			state_ptr->registered_materials.erase(name);
		}
	}

	std::shared_ptr<material> material_system_get_default() {
		return std::make_shared<material>(state_ptr->default_material);
	}

	bool load_material(material_configuration& mat_config) {
		material m;
		m.name = mat_config.name;
		m.type = mat_config.type;
		m.diffuse_color = mat_config.diffuse_color;
		m.diffuse_texture = texture_system_adquire(mat_config.diffuse_texture_name);

		state_ptr->registered_materials.insert({ m.name, m });

		return true;
	}

	void destroy_material(material& m) {
		m.name = "";
		m.type = MATERIAL_TYPE_UNDEFINED;
		m.diffuse_color = glm::vec3(0.0f);
		m.diffuse_texture.reset();
		m.diffuse_texture = nullptr;
	}

	void generate_default_material() {
		state_ptr->default_material.name = std::string("default");
		state_ptr->default_material.type = MATERIAL_TYPE_SCENE;
		state_ptr->default_material.diffuse_color = glm::vec3(1.0f);
		state_ptr->default_material.diffuse_texture = texture_system_get_default();	
	}
}