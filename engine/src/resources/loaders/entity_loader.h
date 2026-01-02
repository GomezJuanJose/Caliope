#pragma once
#include "defines.h"
#include "resources/resources_types.inl"



namespace caliope {
	struct resource_loader;

	resource_loader scene_resource_loader_create();
	resource_loader ui_layout_resource_loader_create();
}