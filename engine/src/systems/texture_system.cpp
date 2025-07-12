#include "texture_system.h"
#include "cepch.h"
#include "core/logger.h"

#include "loaders/resources_types.inl"
#include "systems/resource_system.h"

#include "renderer/renderer_frontend.h"

namespace caliope {
	typedef struct texture_system_state {
		std::unordered_map<std::string, texture> registered_textures;

		texture default_diffuse_texture;
		texture default_specular_texture;
		texture default_normal_texture;
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
		destroy_texture(state_ptr->default_diffuse_texture);
		destroy_texture(state_ptr->default_specular_texture);
		destroy_texture(state_ptr->default_normal_texture);

		state_ptr->registered_textures.empty();
		state_ptr.reset();
		state_ptr = nullptr;
	}
	
	texture* texture_system_adquire(std::string& name) {
		if (name == "") {
			return nullptr;
		}

		if (state_ptr->registered_textures.find(name) == state_ptr->registered_textures.end()) {
			texture t;
			if (!load_texture(name, t)) {
				CE_LOG_ERROR("texture_system_adquire failed to load texture %s", name.c_str());
				return false;
			}

			state_ptr->registered_textures.insert({ name, t });
		}

		return &state_ptr->registered_textures[name];
	}
	
	void texture_system_release(std::string& name) {
		if (state_ptr->registered_textures.find(name) == state_ptr->registered_textures.end()) {
			destroy_texture(state_ptr->registered_textures[name]);
			state_ptr->registered_textures.erase(name);
		}
	}


	texture* texture_system_get_default_diffuse() {
		return &state_ptr->default_diffuse_texture;
	}

	texture* texture_system_get_default_specular() {
		return &state_ptr->default_specular_texture;
	}

	texture* texture_system_get_default_normal() {
		return &state_ptr->default_normal_texture;
	}
	
	bool load_texture(std::string& name, texture& t) {
		resource r;
		if (!resource_system_load(name, RESOURCE_TYPE_IMAGE, r)) {
			return false;
		}
		image_resource_data image_data = std::any_cast<image_resource_data>(r.data);

		t.name = name;
		t.id = 0;
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
		renderer_texture_create(t, image_data.pixels);

		resource_system_unload(r);
		return true;
	}

	void destroy_texture(texture& t) {
		renderer_texture_destroy(t);
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
		state_ptr->default_diffuse_texture.name = std::string("default_texture");
		state_ptr->default_diffuse_texture.id = 0;
		state_ptr->default_diffuse_texture.width = texture_dimensions;
		state_ptr->default_diffuse_texture.height = texture_dimensions;
		state_ptr->default_diffuse_texture.channel_count = texture_channels;
		state_ptr->default_diffuse_texture.has_transparency = false;
		renderer_texture_create(state_ptr->default_diffuse_texture, pixels);


		std::array<uchar, 16 * 16 * 4> spec_pixels = { 0 };
		state_ptr->default_specular_texture.name = std::string("default_spec");
		state_ptr->default_specular_texture.id = 0;
		state_ptr->default_specular_texture.width = 16;
		state_ptr->default_specular_texture.height = 16;
		state_ptr->default_specular_texture.channel_count = texture_channels;
		state_ptr->default_specular_texture.has_transparency = false;
		renderer_texture_create(state_ptr->default_specular_texture, spec_pixels.data());


		std::array<uchar, 16 * 16 * 4> normal_pixels = { 0 };
		for (uint row = 0; row < 16; row++) {
			for (uint col = 0; col < 16; col++) {
				uint index = (row * 16) + col;
				uint index_bpp = index * texture_channels;
				normal_pixels[index_bpp + 0] = 128;
				normal_pixels[index_bpp + 1] = 128;
				normal_pixels[index_bpp + 2] = 255;
				normal_pixels[index_bpp + 3] = 255;
			}
		}
		state_ptr->default_normal_texture.name = std::string("default_normal");
		state_ptr->default_normal_texture.id = 0;
		state_ptr->default_normal_texture.width = 16;
		state_ptr->default_normal_texture.height = 16;
		state_ptr->default_normal_texture.channel_count = texture_channels;
		state_ptr->default_normal_texture.has_transparency = false;
		renderer_texture_create(state_ptr->default_normal_texture, normal_pixels.data());
	}
}