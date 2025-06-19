#pragma once
#include "math_types.inl"

namespace caliope {
	CE_API transform transform_create();
	CE_API transform transform_from_position(glm::vec3 postion);
	CE_API transform transform_from_rotation(glm::quat rotation);
	CE_API transform transform_from_position_rotation(glm::vec3 postion, glm::quat rotation);
	CE_API transform transform_from_position_rotation_scale(glm::vec3 postion, glm::quat rotation, glm::vec3 scale);

	CE_API std::shared_ptr<transform> transform_get_parent(const transform& t);
	CE_API void transform_set_parent(transform& t, const transform& parent);

	CE_API glm::vec3 transform_get_position(const transform& t);
	CE_API void transform_set_position(transform& t, glm::vec3 position);
	CE_API void transform_translate(transform& t, glm::vec3 translation);

	CE_API glm::quat transform_get_rotaion(const transform& t);
	CE_API void transform_set_rotation(transform& t, glm::quat rotation);
	CE_API void transform_rotate(transform& t, glm::quat rotation);

	CE_API glm::vec3 transform_get_scale(const transform& t);
	CE_API void transform_set_scale(transform& t, glm::vec3 scale);
	CE_API void transform_scalate(transform& t, glm::vec3 scalation);

	CE_API void transform_set_position_rotation(transform& t, glm::vec3 position, glm::quat rotation);
	CE_API void transform_set_position_rotation_scale(transform& t, glm::vec3 position, glm::quat rotation, glm::vec3 scale);
	CE_API void transform_translate_rotate(transform& t, glm::vec3 translation, glm::quat rotation);

	CE_API glm::mat4 transform_get_local(transform& t);
	CE_API glm::mat4 transform_get_world(transform& t);

}