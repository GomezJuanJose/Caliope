#pragma once
#include "defines.h"
#include "loaders/resources_types.inl"



namespace caliope {
	struct resource_loader;

	resource_loader binary_resource_loader_create();
}