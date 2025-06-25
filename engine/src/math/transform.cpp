#include "transform.h"

namespace caliope {
	transform transform_create() {
		transform t;
		transform_set_position_rotation_scale(t, glm::vec3(0.0f), glm::quat(), glm::vec3(1.0f));
		t.local = glm::mat4(1.0f);
		t.parent = nullptr;
		return t;
	}

	transform transform_from_position(glm::vec3 postion) {
		transform t;
		transform_set_position_rotation_scale(t, postion, glm::quat(), glm::vec3(1.0f));
		t.local = glm::mat4(1.0f);
		t.parent = nullptr;
		return t;
	}

	transform transform_from_rotation(glm::quat rotation) {
		transform t;
		transform_set_position_rotation_scale(t, glm::vec3(0.0f), rotation, glm::vec3(1.0f));
		t.local = glm::mat4(1.0f);
		t.parent = nullptr;
		return t;
	}

	transform transform_from_position_rotation(glm::vec3 postion, glm::quat rotation) {
		transform t;
		transform_set_position_rotation_scale(t, postion, rotation, glm::vec3(1.0f));
		t.local = glm::mat4(1.0f);
		t.parent = nullptr;
		return t;
	}

	transform transform_from_position_rotation_scale(glm::vec3 postion, glm::quat rotation, glm::vec3 scale) {
		transform t;
		transform_set_position_rotation_scale(t, postion, rotation, scale);
		t.local = glm::mat4(1.0f);
		t.parent = nullptr;
		return t;
	}

	std::shared_ptr<transform> transform_get_parent(const transform& t){
		return t.parent;
	}

	void transform_set_parent(transform& t, const transform& parent) {
		t.parent = std::make_shared<transform>(parent);
	}

	glm::vec3 transform_get_position(const transform& t) {
		return t.position;
	}

	void transform_set_position(transform& t, glm::vec3 position) {
		t.position = position;
		t.is_dirty = true;
	}

	void transform_translate(transform& t, glm::vec3 translation) {
		t.position = t.position + translation;
		t.is_dirty = true;
	}

	glm::quat transform_get_rotaion(const transform& t) {
		return t.rotation;
	}

	void transform_set_rotation(transform& t, glm::quat rotation) {
		t.rotation = rotation;
		t.is_dirty = true;
	}

	void transform_rotate(transform& t, glm::quat rotation) {
		t.rotation = t.rotation * rotation;
		t.is_dirty = true;
	}

	glm::vec3 transform_get_scale(const transform& t) {
		return t.scale;
	}

	void transform_set_scale(transform& t, glm::vec3 scale) {
		t.scale = scale;
		t.is_dirty = true;
	}

	void transform_scalate(transform& t, glm::vec3 scalation) {
		t.scale = t.scale * scalation;
		t.is_dirty = true;
	}

	void transform_set_position_rotation(transform& t, glm::vec3 position, glm::quat rotation) {
		t.position = position;
		t.rotation = rotation;
		t.is_dirty = true;
	}

	void transform_set_position_rotation_scale(transform& t, glm::vec3 position, glm::quat rotation, glm::vec3 scale) {
		t.position = position;
		t.rotation = rotation;
		t.scale = scale;
		t.is_dirty = true;
	}

	void transform_translate_rotate(transform& t, glm::vec3 translation, glm::quat rotation) {
		t.position = t.position + translation;
		t.rotation = t.rotation * rotation;
		t.is_dirty = true;
	}

	glm::mat4 transform_get_local(transform& t) {
		if (t.is_dirty) {
			glm::mat4 tr = glm::toMat4(t.rotation) * glm::translate(glm::mat4(1.0f), t.position);
			tr = glm::scale(glm::mat4(1.0f), t.scale) * tr;
			t.local = tr;
			t.is_dirty = false;
		}

		return t.local;
	}

	glm::mat4 transform_get_world(transform& t) {
		glm::mat4 l = transform_get_local(t);
		if (t.parent) {
			glm::mat4 p = transform_get_world(*t.parent);
			return l * p;
		}

		return l;
	}
}