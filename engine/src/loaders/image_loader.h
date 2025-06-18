#pragma once
#include "defines.h"
#include "loaders/resources_types.inl"



namespace caliope {
	struct resource_loader;

	resource_loader image_resource_loader_create();
}