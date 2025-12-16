#pragma once
#include "defines.h"
#include <glm/glm.hpp>
namespace caliope {

	struct texture;
	enum texture_filter;


	bool texture_system_initialize();
	void texture_system_shutdown();

	CE_API texture* texture_system_adquire(std::string& name);
	CE_API texture* texture_system_adquire_writeable(std::string& name, uint width, uint height, uchar channel_count, bool has_transparency);
	CE_API void texture_system_release(std::string& name);

	CE_API void texture_system_write_data(texture& t, uint offset, uint size, uchar* pixels);
	CE_API void texture_system_change_filter(std::string& name, texture_filter new_mag_filter, texture_filter new_min_filter);

	CE_API texture* texture_system_get_default_diffuse();
	CE_API texture* texture_system_get_default_specular();
	CE_API texture* texture_system_get_default_normal();

	/*
	 * @note Set left_bottom and right_top to 0 for the the whole texture
	 */
	CE_API std::array<glm::vec2,4> texture_system_calculate_custom_region_coordinates(texture& texture, glm::vec2 left_bottom_pixel, glm::vec2 right_top_pixel, bool invert_coordinates);
	// TODO: Implement the invert here too
	CE_API std::array<glm::vec2,4> texture_system_calculate_grid_region_coordinates(texture& texture, glm::vec2 grid_size, uint row_index, uint column_index);
}