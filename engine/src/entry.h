#pragma once

#include "./core/application.h"

// Externally-defined function to create a program
extern bool create_program();

/**
 * Main entry point of the program
 */
int main(void){

    if(!create_program()){
        return -1;
    }

    if (!caliope::application_create()) {
        return -1;
    }
    
    caliope::application_run();

    return 0;
}