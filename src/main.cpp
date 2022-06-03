/*
 * adep
 *
 * main.cpp
 *
 * This file contains the entry point for the application.
 *
 */

#include "adept.h"

int main(int argc, char* argsv[])
{
    adept* app = adept::init();
    int retCode = app->run(argc, argsv);
    adept::deinit();
    
    return retCode;
}
