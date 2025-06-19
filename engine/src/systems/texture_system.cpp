#include "texture_system.h"
#include "cepch.h"
#include "core/logger.h"

#include "loaders/resources_types.inl"
#include "systems/resource_system.h"

#include "renderer/renderer_frontend.h"

namespace caliope {
	typedef struct texture_system_state {
		std::unordered_map<std::string, texture> registered_textures;

		texture default_texture;
	}texture_system_state;

	static std::unique_ptr<texture_system_state> state_ptr;
	
	bool load_texture(std::string& name, texture& t);
	void destroy_texture(texture& t);
	void generate_default_textures();


	bool texture_system_initialize() {

		state_ptr = std::make_unique<texture_system_state>();
		
		if (state_ptr == nullptr) {
			return false;
		}

		generate_default_textures();

		CE_LOG_INFO("Texture system initialized.");
		return true;
	}
	
	void texture_system_shutdown() {
		for (auto [key, value] : state_ptr->registered_textures) {
			destroy_texture(value);
		}
		destroy_texture(state_ptr->default_texture);

		state_ptr->registered_textures.empty();
		state_ptr.reset();
	}
	
	std::shared_ptr<texture> texture_system_adquire(std::string& name) {
		if (state_ptr->registered_textures.find(name) == state_ptr->registered_textures.end()) {
			texture t;
			if (!load_texture(name, t)) {
				CE_LOG_ERROR("texture_system_adquire failed to load texture %s", name.c_str());
				return false;
			}

			state_ptr->registered_textures.insert({ name, t });
		}

		return std::make_shared<texture>(state_ptr->registered_textures[name]);
	}
	
	void texture_system_release(std::string& name) {
		if (state_ptr->registered_textures.find(name) == state_ptr->registered_textures.end()) {
			destroy_texture(state_ptr->registered_textures[name]);
			state_ptr->registered_textures.erase(name);
		}
	}

	std::shared_ptr<texture> texture_system_get_default() {
		return std::make_shared<texture>(state_ptr->default_texture);
	}

	bool load_texture(std::string& name, texture& t) {
		resource r;
		if (!resource_system_load(name, RESOURCE_TYPE_IMAGE, r)) {
			return false;
		}
		image_resource_data image_data = std::any_cast<image_resource_data>(r.data);

		t.name = name;
		t.width = image_data.width;
		t.height = image_data.height;
		t.channel_count = image_data.channel_count;
		t.has_transparency = false;

		// Checks if the images contains alpha
		for (int i = 0; i < r.data_size; i+= image_data.channel_count) {
			uchar a = image_data.pixels[i + 3];
			if (a < 255) {
				t.has_transparency = true;
				break;
			}
		}
		renderer_create_texture(t);

		resource_system_unload(r);
		return true;
	}

	void destroy_texture(texture& t) {
		renderer_destroy_texture(t);
	}

	void generate_default_textures() {
		const uint texture_dimensions = 256;
		const uint texture_channels = 4;
		uchar pixels[262144]; // texture_dimensions * texture_dimensions

		for (uint row = 0; row < texture_dimensions; row++) {
			for (uint col = 0; col < texture_dimensions; col++) {
				uint index = (row * texture_dimensions) + col;
				uint index_bpp = index * texture_channels;
				if (row % 2) {
					if (col % 2) {
						pixels[index_bpp + 0] = 0;
						pixels[index_bpp + 1] = 0;
					}
				}
				else {
					if (!col % 2) {
						pixels[index_bpp + 0] = 0;
						pixels[index_bpp + 1] = 0;
					}
				}
			}
		}

		state_ptr->default_texture.name = std::string("default_texture");
		state_ptr->default_texture.width = texture_dimensions;
		state_ptr->default_texture.height = texture_dimensions;
		state_ptr->default_texture.channel_count = texture_channels;
		state_ptr->default_texture.has_transparency = false;
		
		renderer_create_texture(state_ptr->default_texture);
	}
}