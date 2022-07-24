#define SDL_MAIN_HANDLED

#include <iostream>
#include "SDL.h"
#include "chip8.h"

chip8 chip8;
SDL_Event event;

void setupGraphics();

void drawGraphics();

void handleInput();

[[noreturn]] int main() {
    // Setup render system
    setupGraphics();

    // Initialize the chip8 system and load the game into the memory
    chip8.init();
    chip8.loadGame("roms/test_opcode.ch8");

    // Emulation loop
    for (;;) {
        // Emulate one cycle
        chip8.emulateCycle();

        // Handle input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            }
            handleInput();
        }

        // If the draw flag is set, update the screen
        if (chip8.drawFlag)
            drawGraphics();
    }
}

void drawGraphics() {
    // Clear the screen
    SDL_SetRenderDrawColor(chip8.renderer, 0, 0, 0, 255);
    SDL_RenderClear(chip8.renderer);

    // Draw the screen
    for (int i = 0; i < 64 * 32; i++) {
        if (chip8.gfx[i] == 0) {
            SDL_SetRenderDrawColor(chip8.renderer, 0, 0, 0, 255); // Draw black
        } else {
            SDL_SetRenderDrawColor(chip8.renderer, 255, 255, 255, 255); // Draw white
        }
        SDL_Rect rect = {i % 64 * 10, i / 64 * 10, 10, 10}; // Create a rectangle
        SDL_RenderFillRect(chip8.renderer, &rect); // Draw the rectangle
    }
    SDL_RenderPresent(chip8.renderer); // Update the screen
}

void setupGraphics() {
    SDL_SetMainReady();

    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }

    // Create window
    SDL_CreateWindowAndRenderer(640, 320, SDL_WINDOW_OPENGL, &chip8.window, &chip8.renderer);
    SDL_RenderClear(chip8.renderer);
    SDL_RenderPresent(chip8.renderer);
}

void handleInput() {
    switch (event.key.keysym.sym) {
        case SDLK_1:
            chip8.key[0x1] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_2:
            chip8.key[0x2] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_3:
            chip8.key[0x3] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_4:
            chip8.key[0xC] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_q:
            chip8.key[0x4] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_w:
            chip8.key[0x5] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_e:
            chip8.key[0x6] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_r:
            chip8.key[0xD] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_a:
            chip8.key[0x7] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_s:
            chip8.key[0x8] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_d:
            chip8.key[0x9] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_f:
            chip8.key[0xE] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_y:
            chip8.key[0xA] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_x:
            chip8.key[0x0] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_c:
            chip8.key[0xB] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_v:
            chip8.key[0xF] = event.type == SDL_KEYDOWN ? 1 : 0;
            break;
        case SDLK_ESCAPE:
            exit(0);
        default:
            break;
    }
}
