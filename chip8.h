//
// Created by Johannes on 22.07.2022.
//

#ifndef CHIP_8_EMULATOR_CHIP8_H
#define CHIP_8_EMULATOR_CHIP8_H


#include <stack>

class chip8 {
private:
    unsigned char memory[4096]; // Memory of the chip8 system
    unsigned char V[16]; // Registers V0-VF
    unsigned short stack[16]; // Stack of addresses
    unsigned short I; // Index register
    unsigned short PC; // Program counter
    unsigned short SP; // Stack pointer
    unsigned short opcode; // Current opcode
    unsigned char sound_timer; // Sound timer
    unsigned char delay_timer; // Delay timer

    // Keypad
    unsigned char chip8_fontset[80] = {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    }; // Fontset

public:
    chip8();

    ~chip8();

    void init();

    void loadGame(const char *game);

    void emulateCycle();

    void tick();

    void printState();

    bool drawFlag; // Draw flag
    unsigned char gfx[64 * 32]; // Graphics memory
    unsigned char key[16]; // Keypad
    SDL_Window *window; // Window
    SDL_Renderer *renderer; // Renderer
};


#endif //CHIP_8_EMULATOR_CHIP8_H
