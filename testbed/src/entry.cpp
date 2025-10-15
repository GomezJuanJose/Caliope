#include <entry.h>

#include "program.h"

bool create_program(caliope::program_config& out_config) {
   
    out_config.name = "testbed";
    out_config.width = 1920;
    out_config.height = 1080;
    out_config.maximum_number_entities_per_frame = 10000;
    out_config.maximum_number_textures_per_frame = 400;


    out_config.initialize = initialize_testbed;
    out_config.update = update_testbed;
    out_config.resize = resize_testbed;
    out_config.shutdown = shutdown_testbed;

    return true;
}