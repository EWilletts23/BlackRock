/*
 *  The Main GoGX Operating system Kernel
 *  Main script goes here
 *  Author: pradosh-arduino / helloImPR#6776
 *  Project: Started: 1/8/22 Github: 3/8/22
 */

// includes
#include <stdint.h>
#include <stddef.h>
#include <limine.h>

#include "strings/strings.h"

// Limine terminal 
static volatile struct limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0
};

struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0, .response = NULL
};

struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0, .response = NULL
};

// We need to halt if not limine will bootloop
static void done(void) {
    for (;;) {
    
    }
}

// Main start
void _start(void) {
    if (terminal_request.response == NULL || terminal_request.response->terminal_count < 1) done();

    print("Blackrock\n\n");

    print("Initialising Framebuffer...\n");

    struct limine_framebuffer_response *framebuffer_response = framebuffer_request.response;
    struct limine_framebuffer *framebuffer = framebuffer_response->framebuffers[0];

    print("Framebuffer Initialised\n\n");
    e9_printf("\x1b[41m\x1b[37mFramebuffer Info:\x1b[0m\x1b[37m\n");
    e9_printf("Address: %x\nWidth: %d\nHeight: %d\nPPSL: %d\nFramebuffer Count: %d\n", framebuffer->address, framebuffer->width, framebuffer->height, framebuffer->edid, framebuffer_response->framebuffer_count);

    ResetColours();
 
    done();
}

//Print Function
void print(char msg[], ...){
    struct limine_terminal *terminal = terminal_request.response->terminals[0];
    terminal_request.response->write(terminal, msg, strlen(msg));
}