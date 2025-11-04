#include "audio_loader.h"
#include "cepch.h"

#include "core/logger.h"
#include "core/cememory.h"
#include "systems/resource_system.h"
#include "platform/file_system.h"

#include <miniaudio.h>
namespace caliope {

	ma_decoder decoder;

	bool audio_loader_load(std::string* file, resource* out_resource) {
		#define AUDIO_FORMATS_COUNT 3
		std::string formats[AUDIO_FORMATS_COUNT] = { ".mp3", ".wav", ".ogg"};
		std::string selected_format = "";
		for (int i = 0; i < AUDIO_FORMATS_COUNT; ++i) {
			if (file_system_exists(*file + formats[i])) {
				selected_format = formats[i];
				break;
			}
		}

		if (selected_format.empty()) {
			CE_LOG_ERROR("Audio %s not found", file->c_str());
			return false;
		}

		audio_clip_resource_data audio_resource;

		if (selected_format.compare("") != 0) {
			//ma_result result = ma_decoder_init_file(file->append(selected_format).c_str(), NULL, &decoder);
			// TODO: When OpenAl is implemented remove this, at the moment the buffer contains the name because miniaudio will be in charge of load from file (it needs to allocate and free memory blocks)
			// ma_decoder_init_file it could be used to load the raw data from the audio but the must exists in the memory and needs to be freed at the end of the application, WARNING if you override the decoder
			// the previous address of memory will not be freed(ma_decoder_uninit)!!!
			audio_resource.buffer = (void*)file->append(selected_format).c_str(); 
		}
		else {
			CE_LOG_ERROR("Unsupported audio file type.");
		}

		out_resource->data = audio_resource;
		out_resource->data_size = sizeof(audio_clip_resource_data);
		return true;
	}

	void audio_loader_unload(resource* resource) {
		resource->data.reset();
		resource->data_size = 0;
		resource->loader_name.clear();
	}

	resource_loader audio_resource_loader_create() {
		resource_loader loader;

		loader.type = RESOURCE_TYPE_AUDIO;
		loader.custom_type = std::string("");
		loader.resource_folder = std::string("audio/");
		loader.load = audio_loader_load;
		loader.unload = audio_loader_unload;

		return loader;
	}
}