#pragma once

#include "core/application.h"
#include "program_types.h"

// Externally-defined function to create a program
extern bool create_program(caliope::program_config& out_config);

/**
 * Main entry point of the program
 */
int main(void){

    caliope::program_config config;

    if(!create_program(config)){
        return -1;
    }

    if (!caliope::application_create(config)) {
        return -1;
    }
    
    caliope::application_run();

    return 0;
}