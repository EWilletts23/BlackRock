
/*
--------------------------
INCLUDE FILES
--------------------------
*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "limine.h"

#include "strings/strings.h"
#include "graphics/e9print.h"
#include "rendering/framebuffer.h"

/*
--------------------------
LIMINE BOOTLOADER REQUESTS
--------------------------
*/

static volatile struct limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0
};

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

/*
--------------------------
SYSTEM CALLS
--------------------------
*/

static void write_shim(const char *s, uint64_t l) {
    struct limine_terminal *terminal = terminal_request.response->terminals[0];

    terminal_request.response->write(terminal, s, l);
}

void print(char msg[], ...){
    struct limine_terminal *terminal = terminal_request.response->terminals[0];
    terminal_request.response->write(terminal, msg, strlen(msg));
}

typedef struct Framebuffer{
	void* BaseAddress;
	size_t BufferSize;
	unsigned int Width;
	unsigned int Height;
	unsigned int PixelsPerScanLine;
    unsigned int BufferCount;
    unsigned int BytesPerPixel;
};

bool checkStringEndsWith(const char* str, const char* end) {
    const char* _str = str;
    const char* _end = end;

    while(*str != 0) {
        str++;
    }
        
    str--;

    while(*end != 0) {
        end++;
    }
        
    end--;

    while (true) {
        if (*str != *end) { 
            return false;
        }

        str--;
        end--;

        if (end == _end || (str == _str && end == _end)) {
            return true;
        }

        if (str == _str){
            return false;
        }
    }
    return true;
}

struct limine_file* getFile(const char* name) {
    struct limine_module_response *module_response = module_request.response;
    for (size_t i = 0; i < module_response->module_count; i++) {
        struct limine_file *f = module_response->modules[i];
        if (checkStringEndsWith(f->path, name))
            return f;
    }
    return NULL;
}

#define PSF1_MAGIC0 0x36;
#define PSF1_MAGIC1 0x04;

typedef struct {
    unsigned char magic[2];
    unsigned char mode;
    unsigned char charsize;
} PSF1_HEADER;

typedef struct
{
    PSF1_HEADER* psf1_Header;
    void* glyphBuffer;
} PSF1_FONT;


// We need to halt if not limine will bootloop
static void done(void) {
    print("\n\nSystem Halted");
    for (;;) {
    
    }
}

/*
--------------------------
KERNEL SCRIPT
--------------------------
*/

void _start(void) {
    if (terminal_request.response == NULL || terminal_request.response->terminal_count < 1) {
        done();
    }

    limine_print = write_shim;

    print("Blackrock\n\n");


    // INITALISE FRAMEBUFFER
    
    print("Initialising Framebuffer...\n");

    struct limine_framebuffer_response *framebuffer_response = framebuffer_request.response;
    struct limine_framebuffer *framebuffer = framebuffer_response->framebuffers[0];

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        print("\nError!\n");
        print("Framebuffer = NULL\n");
        done();
    } else {
        print("Framebuffer Initialised\n\n");
    }

    framebuffer->bpp = 4;

    Framebuffer fb;
    fb.BaseAddress = framebuffer->address;
    fb.Width = framebuffer->width;
    fb.Height = framebuffer->height;
    fb.PixelsPerScanLine = framebuffer->pitch / 4;
    fb.BufferSize = framebuffer->height * framebuffer->pitch;
    fb.BufferCount = framebuffer_response->framebuffer_count;
    fb.BytesPerPixel = framebuffer->bpp;


    // SHOW FRAMEBUFFER INFO

    e9_printf("\x1b[41m\x1b[37mFramebuffer Info:\x1b[0m\x1b[37m\nAddress: %x\nWidth: %d\nHeight: %d\nPPSL: %d\nFramebuffer Count: %d\nBPP: %d", fb.BaseAddress, fb.Width, fb.Height, fb.PixelsPerScanLine, fb.BufferCount, fb.BytesPerPixel);

    unsigned int y = 50;

    for (unsigned int x = 0; x < fb.Width / 2 * fb.BytesPerPixel; x+=fb.BytesPerPixel) {
        *(unsigned int*)(x + (y * fb.PixelsPerScanLine * fb.BytesPerPixel) + fb.BaseAddress) = 0xff54043b;
    }

    // LOAD PSF1_FONT

    PSF1_FONT psf1_Font;
    
    {
        const char* fontPath = "fonts/zap-light16.psf";
        struct limine_file* file = getFile(fontPath);
        if (file == NULL){
            e9_printf("Failed to get Font \"%s\"!", fontPath);
            done();
        }

        psf1_Font.psf1_Header = (PSF1_HEADER*)file->address;
        if (psf1_Font.psf1_Header->magic[0] != 0x36 || psf1_Font.psf1_Header->magic[1] != 0x04) {
            e9_printf("Font Header Invalid");
            done();
        }

        psf1_Font.glyphBuffer = (void*)((uint64_t)file->address + sizeof(PSF1_HEADER));

        e9_printf("Font Loaded");
    }

    done();
}