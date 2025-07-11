#include "geometry_system.h"
#include "cepch.h"

#include "core/logger.h"
#include "math/math_types.inl"
#include "loaders/resources_types.inl"
#include "renderer/renderer_frontend.h"

namespace caliope {
	typedef struct geometry_system_state {
		geometry quad;
	}geometry_system_state;

	std::unique_ptr<geometry_system_state> state_ptr;

	void generate_builtin_geometry();
	void calculate_tangents(std::vector<vertex>& vertices, std::vector<uint16>& indices);

	bool geometry_system_initialize() {
		state_ptr = std::make_unique<geometry_system_state>();

		if (state_ptr == nullptr) {
			return false;
		}

		generate_builtin_geometry();

		return true;
	}

	void geometry_system_shutdown() {
		renderer_geometry_destroy(state_ptr->quad);
		state_ptr.reset();
		state_ptr = 0;
	}

	geometry* geometry_system_get_quad() {
		return &state_ptr->quad;
	}

	void generate_builtin_geometry() {

		std::vector<vertex> vertices = {
			{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
			{{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
		};

		std::vector<uint16> indices = {
			0, 1, 2, 2, 1, 3
		};

		state_ptr->quad.index_count = indices.size();

		calculate_tangents(vertices, indices);
		renderer_geometry_create(state_ptr->quad, vertices, indices);
	}

	void calculate_tangents(std::vector<vertex>& vertices, std::vector<uint16>& indices) {
		for (uint i = 0; i < indices.size(); i += 3) {
			uint i0 = indices[i + 0];
			uint i1 = indices[i + 1];
			uint i2 = indices[i + 2];

			glm::vec3 deltaPos1 = vertices[i1].pos - vertices[i0].pos;
			glm::vec3 deltaPos2 = vertices[i2].pos - vertices[i0].pos;

			float deltaU1 = vertices[i1].tex_coord.x - vertices[i0].tex_coord.x;
			float deltaV1 = vertices[i1].tex_coord.y - vertices[i0].tex_coord.y;

			float deltaU2 = vertices[i2].tex_coord.x - vertices[i0].tex_coord.x;
			float deltaV2 = vertices[i2].tex_coord.y - vertices[i0].tex_coord.y;

			float fc = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);
			glm::vec3 tangent = glm::vec3(
				(fc * (deltaV2 * deltaPos1.x - deltaV1 * deltaPos2.x)),
				(fc * (deltaV2 * deltaPos1.y - deltaV1 * deltaPos2.y)),
				(fc * (deltaV2 * deltaPos1.z - deltaV1 * deltaPos2.z))
			);

			tangent = glm::normalize(tangent);

			float sx = deltaU1, sy = deltaU2;
			float tx = deltaV1, ty = deltaV2;
			float handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0 : 1.0f;

			glm::vec4 t4 = glm::vec4(tangent, handedness);
			vertices[i0].tangent = t4;
			vertices[i1].tangent = t4;
			vertices[i2].tangent = t4;
		}
	}
}