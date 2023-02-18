
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

struct limine_kernel_address_request kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0, .response = NULL
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

void* startRAMAddr = NULL;

void* quickMalloc(uint64_t size) {
    if (startRAMAddr == NULL) {
        e9_printf("> Cannot malloc from NULL!");
        done();
    }

    void* temp = startRAMAddr;
    startRAMAddr = (void*)((uint64_t)startRAMAddr + size);
    return temp;
}

void print(char msg[], ...){
    struct limine_terminal *terminal = terminal_request.response->terminals[0];
    terminal_request.response->write(terminal, msg, strlen(msg));
}

typedef struct Framebuffer {
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


void PutChar(Framebuffer* framebuffer, PSF1_FONT* psf1_font, unsigned int colour, char chr, unsigned int xOff, unsigned int yOff){
    unsigned int* pixPtr = (unsigned int*)framebuffer->BaseAddress;
    char* fontPtr = psf1_font->glyphBuffer + (chr * psf1_font->psf1_Header->charsize);

    for (unsigned long y = yOff; y < yOff + 16; y++){
        for (unsigned long x = xOff; x < xOff + 8; x++){
            if ((*fontPtr & (0b10000000 >> (x - xOff))) > 0){
                *(unsigned int*)(pixPtr + x + (y * framebuffer->PixelsPerScanLine)) = colour;

            }
        }

        fontPtr++;
    }
}


// We need to halt if not limine will bootloop
void done(void) {
    print("\n\nSystem Halted");
    for (;;) {
    
    }
}

/*
--------------------------
KERNEL SCRIPT
--------------------------
*/

void _start(Framebuffer* framebuffer, PSF1_FONT* psf1_font) {
    if (terminal_request.response == NULL || terminal_request.response->terminal_count < 1) {
        done();
    }

    limine_print = write_shim;

    print("Blackrock\n\n");


    // INITALISE FRAMEBUFFER
    
    print("Initialising Framebuffer...\n");

    struct limine_framebuffer_response *framebuffer_response = framebuffer_request.response;
    struct limine_framebuffer *lime_framebuffer = framebuffer_response->framebuffers[0];

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        print("\nError!\n");
        print("Framebuffer = NULL\n");
        done();
    } else {
        print("Framebuffer Initialised\n\n");
    }

    lime_framebuffer->bpp = 4;

    Framebuffer fb;
    fb.BaseAddress = lime_framebuffer->address;
    fb.Width = lime_framebuffer->width;
    fb.Height = lime_framebuffer->height;
    fb.PixelsPerScanLine = lime_framebuffer->pitch / 4;
    fb.BufferSize = lime_framebuffer->height * lime_framebuffer->pitch;
    fb.BufferCount = framebuffer_response->framebuffer_count;
    fb.BytesPerPixel = lime_framebuffer->bpp;

    // SHOW FRAMEBUFFER INFO

    e9_printf("\x1b[41m\x1b[37mFramebuffer Info:\x1b[0m\x1b[37m\nAddress: %x\nWidth: %d\nHeight: %d\nPPSL: %d\nFramebuffer Count: %d\nBPP: %d\n", fb.BaseAddress, fb.Width, fb.Height, fb.PixelsPerScanLine, fb.BufferCount, fb.BytesPerPixel);

    unsigned int y = 200;

    for (unsigned int x = 0; x < fb.Width / 2 * fb.BytesPerPixel; x+=fb.BytesPerPixel) {
        *(unsigned int*)(x + (y * fb.PixelsPerScanLine * fb.BytesPerPixel) + fb.BaseAddress) = 0xff54043b;
    }

    // INITIALISE MEMMAP

    if (memmap_request.response == NULL) {
        print("\nError!\n");
        e9_printf("Memory Map = NULL");
        done();
    }

    void* freeMemoryStart = NULL;
    uint64_t freeMemorySize = 0;

    if (kernel_address_request.response == NULL) {
        e9_printf("Kernel address not passed");
        done();
    }

    struct limine_kernel_address_response *ka_response = kernel_address_request.response;


    e9_printf("\x1b[41m\x1b[37mKernel Info:\x1b[0m\x1b[37m");
    e9_printf("Kernel address feature, revision %d", ka_response->revision);
    e9_printf("Physical base: %x", ka_response->physical_base);
    e9_printf("Virtual base: %x", ka_response->virtual_base);

    void* kernelStart = (void*)ka_response->physical_base;
    void* kernelStartVirtual = (void*)ka_response->virtual_base;
    uint64_t kernelSize = 1;

    struct limine_memmap_response *memmap_response = memmap_request.response;
    //e9_printf("Memory map feature, revision %d", memmap_response->revision);
    //e9_printf("%d memory map entries", memmap_response->entry_count);

    for (size_t i = 0; i < memmap_response->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap_response->entries[i];
        //e9_printf("> %x->%x %s", e->base, e->base + e->length, get_memmap_type(e->type));
        if (e->type == LIMINE_MEMMAP_USABLE)
        {
            //e9_printf("%x->%x %s  (%d %d %d %d bytes)", e->base, e->base + e->length, get_memmap_type(e->type), (e->length / 1000000000), (e->length / 1000000) % 1000, (e->length / 1000) % 1000, e->length % 1000);
            if (e->length > freeMemorySize)
            {
                freeMemoryStart = (void*)e->base;
                freeMemorySize = e->length;
            }
        }
        else if (e->base == (uint64_t)kernelStart)
        {
            kernelSize = e->length;
        }
    }
    if (freeMemoryStart == NULL) {
        print("\nError!\n");
        print("Memory = NULL\n");
        e9_printf("No valid Memory space found for OS!\n");
        done();
    }

    startRAMAddr = freeMemoryStart;
    
    if (module_request.response == NULL) {
        print("\nError!\n");
        print("Module = NULL\n");
        done();
    }

    // LOAD PSF1_FONT
    e9_printf("");
    e9_printf("Initialising PSF Font...");

    PSF1_FONT psf1_Font;
    {
        const char* fontPath = "fonts/zap-light16.psf";

        e9_printf("Locating PSF Font: \"%s\"", fontPath);

        struct limine_file* file = getFile(fontPath);

        if (file == NULL){
            e9_printf("\nError!\n");
            e9_printf("Failed to Locate Font: \"%s\"", fontPath);
            done();
        } else {
            e9_print("PSF Font Located\n");
        }

        psf1_Font.psf1_Header = (PSF1_HEADER*)file->address;
        if (psf1_Font.psf1_Header->magic[0] != 0x36 || psf1_Font.psf1_Header->magic[1] != 0x04) {
            e9_printf("Font Header Invalid");
            done();
        }

        psf1_Font.glyphBuffer = (void*)((uint64_t)file->address + sizeof(PSF1_HEADER));

        e9_printf("PSF Font Initialised");

    }

    {
        uint64_t mallocDiff = (uint64_t)startRAMAddr - (uint64_t)freeMemoryStart;
        
        e9_printf("");
        e9_printf("\x1b[41m\x1b[37mRAM Info:\x1b[0m\x1b[37m");
        e9_printf("%x (NOW) - %x (START) = %d bytes malloced", (uint64_t)startRAMAddr, (uint64_t)freeMemoryStart, mallocDiff);
        e9_printf("");

        //freeMemStart = startRAMAddr;
        freeMemorySize -= mallocDiff;
    }

    e9_printf("Kernel Start: %x (Size: %d Bytes)", (uint64_t)kernelStart, kernelSize);
    e9_printf("");
    e9_printf("%d MB of RAM free. (Starts at %x)", freeMemorySize / 1000000, (uint64_t)freeMemoryStart);
    e9_printf("");
    e9_printf("Boot calls initiated!");
    
    bootTest(fb, &psf1_Font, startRAMAddr, freeMemoryStart, freeMemorySize, kernelStart, kernelSize, kernelStartVirtual);

    done();
}