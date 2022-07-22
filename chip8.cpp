//
// Created by Johannes on 22.07.2022.
//

//#define DEBUG_MODE

#include <cstdlib>
#include <ctime>
#include "SDL.h"
#include "chip8.h"

chip8::chip8() = default;

void chip8::init() {
    pc = 0x200; // Program counter starts at 0x200
    opcode = 0; // Reset current opcode
    I = 0; // Reset index register

    // Clear display
    for (unsigned char &i: gfx) {
        i = 0;
    }

    // Clear stack
    for (int i = 0; i < stack.size(); i++) {
        stack.pop();
    }

    // Clear registers V0-VF
    for (unsigned char &i: V) {
        i = 0;
    }

    // Clear memory
    for (unsigned char &i: memory) {
        i = 0;
    }

    // Load fontset
    for (int i = 0; i < 80; i++) {
        memory[i] = chip8_fontset[i];
    }

    // Reset timers
    sound_timer = 0;
    delay_timer = 0;

    // Clear screen once
    drawFlag = true;

    srand(time(nullptr));

#ifdef DEBUG_MODE
    printf("Chip8 emulator initialized\n");
#endif
}

void chip8::loadGame(const char *game) {
    // Load game into memory
    FILE *gameFile = fopen(game, "rb"); // Open the game file in binary mode
    if (gameFile == nullptr) {
        printf("Could not open game file %s\n", game);
        return;
    }

    fseek(gameFile, 0, SEEK_END); // Seek to the end of the file
    long gameSize = ftell(gameFile); // Get the size of the file
    rewind(gameFile); // Rewind the file pointer to the beginning of the file

    char *buffer = (char *) malloc(sizeof(char) * gameSize); // Allocate memory for the game
    if (buffer == nullptr) {
        printf("Could not allocate memory for game\n");
        return;
    }

    size_t result = fread(buffer, 1, gameSize, gameFile); // Read the game into the buffer
    if (result != gameSize) {
        printf("Could not read game file\n");
        return;
    }

    if ((4096 - 512) > gameSize) {
        for (int i = 0; i < gameSize; i++) {
            memory[i + 512] = buffer[i];
        }
    } else {
        printf("Game is too big to fit in memory\n");
        return;
    }

    fclose(gameFile); // Close the game file
    free(buffer); // Free the memory allocated for the game

#ifdef DEBUG_MODE
    printf("Game loaded\n");
#endif
}

void chip8::emulateCycle() {
    // Fetch Opcode
    opcode = memory[pc] << 8 | memory[pc + 1];

#ifdef DEBUG_MODE
    printf("Opcode: %x\n", opcode);
#endif

    // Decode Opcode
    switch (opcode & 0xF000) {
        case 0x0000: // 0xXXX ops
            switch (opcode & 0x000F) {
                case 0x0000: // 00E0: Clears the screen
                    for (unsigned char &i: gfx) {
                        i = 0;
                    }
                    drawFlag = true;
                    pc += 2;
                    break;
                case 0x000E: // 00EE: Returns from subroutine
                    pc = stack.top();
                    stack.pop();
                    break;
                default:
                    printf("Unknown opcode: %x\n", opcode);
                    break;
            }
            break;

        case 0x1000: // 0x1NNN - Jump to address NNN
            pc = opcode & 0x0FFF;
            break;

        case 0x2000: // 0x2NNN - Call subroutine
            // Push current address onto the stack
            stack.push(pc);
            // Jump to address NNN
            pc = opcode & 0x0FFF;
            break;

        case 0x3000: // 0x3XNN - Skip next instruction if VX = NN
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
                pc += 4;
            else
                pc += 2;
            break;

        case 0x4000: // 0x4XNN - Skip next instruction if VX != NN
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
                pc += 4;
            else
                pc += 2;
            break;

        case 0x5000: // 0x5XY0 - Skip next instruction if VX = VY
            if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc += 2;
            break;

        case 0x6000: // 0x6XNN - Set VX to NN
            V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
            pc += 2;
            break;

        case 0x7000: // 0x7XNN - Add NN to VX
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;

        case 0x8000: // 0x8XXX ops
            switch (opcode & 0x000F) {
                case 0x0000: // 8XY0: Set VX to VY
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                case 0x0001: // 0x8XY1 - Set VX to VX | VY
                    V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;

                case 0x0002: // 0x8XY2 - Set VX to VX & VY
                    V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;

                case 0x0003: // 0x8XY3 - Set VX to VX ^ VY
                    V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;

                case 0x0004: // 0x8XY4 - Add VY to VX. Set VF to 1 if there's a carry, 0 if not
                    if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
                        V[0xF] = 1; // carry
                    else
                        V[0xF] = 0;
                    V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;

                case 0x0005: // 0x8XY5 - Subtract VY from VX. Set VF to 0 if there's a borrow, 1 if not
                    if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
                        V[0xF] = 0; // borrow
                    else
                        V[0xF] = 1;
                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;

                case 0x0006: // 0x8XY6 - Store the least significant bit of VX in VF and then shift VX to the right by 1.
                    V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    pc += 2;
                    break;

                case 0x0007: // 0x8XY7 - Set VX to VY minus VX. Set VF to 0 if there's a borrow, 1 if not
                    if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
                        V[0xF] = 0; // borrow
                    else
                        V[0xF] = 1;
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;

                case 0x000E: // 0x8XYE - Store the most significant bit of VX in VF and then shift VX to the left by 1.
                    V[0xF] = (V[(opcode & 0x0F00) >> 8] & 0x80) >> 7;
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    pc += 2;
                    break;
                default:
                    printf("Unknown opcode: %x\n", opcode);
                    break;
            }
            break;

        case 0x9000: // 0x9XY0 - Skip next instruction if VX != VY
            if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc += 2;
            break;

        case 0xA000: // ANNN: Set I to NNN
            I = opcode & 0x0FFF;
            pc += 2;
            break;

        case 0xB000: // BNNN: Jump to address NNN + V0
            pc = (opcode & 0x0FFF) + V[0];
            break;

        case 0xC000: // CXNN: Set VX to a random number with a mask of NN
            V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
            pc += 2;
            break;

        case 0xD000: // 0xDXYN - Draw sprite at coordinate (VX, VY)
        {
            unsigned short x = V[(opcode & 0x0F00) >> 8];
            unsigned short y = V[(opcode & 0x00F0) >> 4];
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;

            V[0xF] = 0;
            for (int yline = 0; yline < height; yline++) {
                pixel = memory[I + yline];
                for (int xline = 0; xline < 8; xline++) {
                    if ((pixel & (0x80 >> xline)) != 0) {
                        if (gfx[(x + xline + ((y + yline) * 64))] == 1)
                            V[0xF] = 1;
                        gfx[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }

            drawFlag = true;
            pc += 2;
        }
            break;

        case 0xE000: // 0xEXXX ops
            switch (opcode & 0x00FF) {
                case 0x009E: // 0xEX9E: Skip next instruction if key with the value of VX is pressed.
                    if (key[V[(opcode & 0x0F00) >> 8]] != 0)
                        pc += 4;
                    else
                        pc += 2;
                    break;

                case 0x00A1: // 0xEXA1: Skip next instruction if key with the value of VX isn't pressed.
                    if (key[V[(opcode & 0x0F00) >> 8]] == 0)
                        pc += 4;
                    else
                        pc += 2;
                    break;
                default:
                    printf("Unknown opcode: %x\n", opcode);
                    break;
            }
            break;

        case 0xF000: // 0xFX07 - Set VX to the value of the delay timer
            switch (opcode & 0x00FF) {
                case 0x0007: // 0xFX07: Set VX to the value of the delay timer
                    V[(opcode & 0x0F00) >> 8] = delay_timer;
                    pc += 2;
                    break;
                case 0x000A: // 0xFX0A - Wait for a key press, then store the value of the key in VX
                    for (int i = 0; i < 16; i++) {
                        if (key[i] != 0) {
                            V[(opcode & 0x0F00) >> 8] = i;
                            break;
                        }
                    }
                    pc += 2;
                    break;

                case 0x0015: // 0xFX15 - Set the delay timer to VX
                    delay_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;

                case 0x0018: // 0xFX18 - Set the sound timer to VX
                    sound_timer = V[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;

                case 0x001E: // 0xFX1E - Add VX to I
                    if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    pc += 2;
                    break;

                case 0x0029: // 0xFX29 - Set I to the location of the sprite for the character in VX
                    I = V[(opcode & 0x0F00) >> 8] * 0x5;
                    pc += 2;
                    break;

                case 0x0033: // 0xFX33 - Store the Binary-coded decimal representation of VX in memory locations I, I+1, and I+2
                    memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
                    pc += 2;
                    break;

                case 0x0055: // 0xFX55 - Store V0 to VX in memory starting at address I
                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
                        memory[I + i] = V[i];
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                    break;

                case 0x0065: // 0xFX65 - Load V0 to VX from memory starting at address I
                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
                        V[i] = memory[I + i];
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                    break;
            }
            break;


        default:
            printf("Unknown opcode: %X\n", opcode);
            break;
    }

// Update timers
    if (delay_timer > 0)
        delay_timer--;

    if (sound_timer > 0) {
        if (sound_timer == 1)
            printf("BEEP!\n");
        sound_timer--;
    }
}

chip8::~chip8() = default;
