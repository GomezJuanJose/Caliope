#include "shader_system.h"
#include "cepch.h"
#include "core/logger.h"

#include "loaders/resources_types.inl"
#include "systems/resource_system.h"

#include "renderer/renderer_frontend.h"

namespace caliope {
	#define BUILTIN_SHADER_NAME std::string("Builtin.SpriteShader")

	typedef struct shader_reference {
		shader shader;
		uint reference_count;
	} shader_reference;

	typedef struct shader_system_state {
		std::unordered_map<std::string, shader_reference> registered_shaders;

	}shader_system_state;

	static std::unique_ptr<shader_system_state> state_ptr;
	
	bool load_shader(shader_resource_data& shader_config);
	void destroy_shader(shader& s);


	bool shader_system_initialize() {

		state_ptr = std::make_unique<shader_system_state>();
		
		if (state_ptr == nullptr) {
			return false;
		}

		if (shader_system_adquire(BUILTIN_SHADER_NAME) == nullptr) {
			CE_LOG_ERROR("Could not load Builtin.SpriteShader as default shader. Shutting down");
			return false;
		}

		CE_LOG_INFO("Shader system initialized.");
		return true;
	}
	
	void shader_system_shutdown() {
		for (auto [key, value] : state_ptr->registered_shaders) {
			destroy_shader(value.shader);
		}

		state_ptr->registered_shaders.empty();
		state_ptr.reset();
		state_ptr = nullptr;
	}
	
	shader* shader_system_adquire(std::string& name) {
		if (state_ptr->registered_shaders.find(name) == state_ptr->registered_shaders.end()) {
			resource r;
			if (!resource_system_load(name, RESOURCE_TYPE_SHADER, r)) {
				CE_LOG_ERROR("shader_system_adquire couldnt load shader config file");
				return false;
			}
			shader_resource_data shader_config = std::any_cast<shader_resource_data>(r.data);
			resource_system_unload(r);

			if (!load_shader(shader_config)) {
				CE_LOG_ERROR("shader_system_create failed to load shader %s", name.c_str());
				return false;
			}
		}
		
		state_ptr->registered_shaders[name].reference_count++;
		return &state_ptr->registered_shaders[name].shader;
	}
	
	void shader_system_release(std::string& name) {
		if (state_ptr->registered_shaders.find(name) != state_ptr->registered_shaders.end()) {
			state_ptr->registered_shaders[name].reference_count--;

			if (state_ptr->registered_shaders[name].reference_count <= 0) {
				destroy_shader(state_ptr->registered_shaders[name].shader);
				state_ptr->registered_shaders.erase(name);
			}
		}
	}

	void shader_system_use(std::string& name) {
		if (state_ptr->registered_shaders.find(name) != state_ptr->registered_shaders.end()) {
			renderer_shader_use(state_ptr->registered_shaders[name].shader);
		}
	}

	bool load_shader(shader_resource_data& shader_config) {
		shader_reference sr;
		sr.reference_count = 0;
		sr.shader.name = shader_config.name;
		bool result = renderer_shader_create(shader_config, sr.shader);

		state_ptr->registered_shaders.insert({ shader_config.name, sr });

		return result;
	}

	void destroy_shader(shader& s) {
		renderer_shader_destroy(s);
	}

}