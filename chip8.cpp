//
// Created by Johannes on 22.07.2022.
//

#define DEBUG_MODE

#include <cstdlib>
#include <ctime>
#include "SDL.h"
#include "chip8.h"

chip8::chip8() = default;

void chip8::init() {
    PC = 0x200; // Program counter starts at 0x200
    opcode = 0; // Reset current opcode
    I = 0; // Reset index register
    SP = 0; // Reset stack pointer

    memset(memory, 0, 4096); // Clear memory
    memset(V, 0, 16); // Clear registers
    memset(gfx, 0, 64 * 32); // Clear graphics memory
    memset(stack, 0, 16); // Clear stack
    memset(key, 0, 16); // Clear keypad

    // Load fontset
    for (int i = 0; i < 80; i++)
        memory[i] = chip8_fontset[i];

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
    FILE *fp = fopen(game, "rb");
    if (fp == nullptr) {
        printf("Could not open file %s\n", game);
        exit(42);
    }
    fread(&memory[0x200], 1, sizeof(memory) - 0x200, fp);
    fclose(fp);

#ifdef DEBUG_MODE
    printf("Game loaded\n");
#endif
}

void chip8::emulateCycle() {
    // Fetch Opcode
    opcode = memory[PC] << 8 | memory[PC + 1];

#ifdef DEBUG_MODE
    printf("PC: 0x%04x Op: 0x%04x\n", PC, opcode);
#endif

    // Decode Opcode
    switch (opcode & 0xF000) {
        case 0x0000: // 0xXXX ops
            switch (opcode & 0x00FF) {
                case 0x00E0: // 00E0: Clears the screen
                    for (unsigned char &i: gfx)
                        i = 0;
                    drawFlag = true;
                    PC += 2;
                    break;

                case 0x00EE: // 00EE: Returns from subroutine
                    PC = stack[--SP];
                    break;

                default:
                    fprintf(stderr, "Unknown opcode: 0x%x\n", opcode);
                    break;
            }
            break;

        case 0x1000: // 0x1NNN - Jump to address NNN
            PC = opcode & 0x0FFF;
            break;

        case 0x2000: // 0x2NNN - Call subroutine
            // Push current address onto the stack
            stack[SP++] = PC + 2;
            // Jump to address NNN
            PC = opcode & 0x0FFF;
            break;

        case 0x3000: // 0x3XNN - Skip next instruction if VX = NN
            V[(opcode >> 8) & 0x000F] == (opcode & 0x00FF) ? PC += 4 : PC += 2;
            break;

        case 0x4000: // 0x4XNN - Skip next instruction if VX != NN
            V[(opcode >> 8) & 0x000F] != (opcode & 0x00FF) ? PC += 4 : PC += 2;
            break;

        case 0x5000: // 0x5XY0 - Skip next instruction if VX = VY
            V[(opcode >> 8) & 0x000F] == V[(opcode & 0x00F0) >> 4] ? PC += 4 : PC += 2;
            break;

        case 0x6000: // 0x6XNN - Set VX to NN
            V[(opcode >> 8) & 0x000F] = opcode & 0x00FF;
            PC += 2;
            break;

        case 0x7000: // 0x7XNN - Add NN to VX
            V[(opcode >> 8) & 0x000F] += opcode & 0x00FF;
            PC += 2;
            break;

        case 0x8000: // 0x8XXX ops
            switch (opcode & 0x000F) {
                case 0x0000: // 8XY0: Set VX to VY
                    V[(opcode >> 8) & 0x000F] = V[(opcode >> 4) & 0x000F];
                    PC += 2;
                    break;
                case 0x0001: // 0x8XY1 - Set VX to VX | VY
                    V[(opcode >> 8) & 0x000F] |= V[(opcode >> 4) & 0x000F];
                    PC += 2;
                    break;

                case 0x0002: // 0x8XY2 - Set VX to VX & VY
                    V[(opcode >> 8) & 0x000F] &= V[(opcode >> 4) & 0x000F];
                    PC += 2;
                    break;

                case 0x0003: // 0x8XY3 - Set VX to VX ^ VY
                    V[(opcode >> 8) & 0x000F] ^= V[(opcode >> 4) & 0x000F];
                    PC += 2;
                    break;

                case 0x0004: // 0x8XY4 - Add VY to VX. Set VF to 1 if there's a carry, 0 if not
                    V[0xF] = (int) V[(opcode >> 8) & 0x000F] + (int) (0xFF - V[(opcode >> 4) & 0x000F]) > 255 ? 1 : 0;
                    V[(opcode >> 8) & 0x000F] += V[(opcode >> 4) & 0x000F];
                    PC += 2;
                    break;

                case 0x0005: // 0x8XY5 - Subtract VY from VX. Set VF to 0 if there's a borrow, 1 if not
                    V[0xF] = V[(opcode >> 8) & 0x000F] > V[(opcode >> 4) & 0x000F] ? 1 : 0;
                    V[(opcode >> 8) & 0x000F] -= V[(opcode >> 4) & 0x000F];
                    PC += 2;
                    break;

                case 0x0006: // 0x8XY6 - Store the least significant bit of VX in VF and then shift VX to the right by 1.
                    V[0xF] = V[(opcode >> 8) & 0x000F] & 0x1;
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    PC += 2;
                    break;

                case 0x0007: // 0x8XY7 - Set VX to VY minus VX. Set VF to 0 if there's a borrow, 1 if not
                    V[0xF] = V[(opcode >> 4) & 0x000F] > V[(opcode >> 8) & 0x000F] ? 1 : 0;
                    V[(opcode >> 8) & 0x000F] = V[(opcode >> 4) & 0x000F] - V[(opcode >> 8) & 0x000F];
                    PC += 2;
                    break;

                case 0x000E: // 0x8XYE - Store the most significant bit of VX in VF and then shift VX to the left by 1.
                    V[0xF] = (V[(opcode >> 8) & 0x000F] >> 7) & 0x1;
                    V[(opcode >> 8) & 0x000F] <<= 1;
                    PC += 2;
                    break;

                default:
                    fprintf(stderr, "Unknown opcode: 0x%x\n", opcode);
                    break;
            }
            break;

        case 0x9000: // 0x9XY0 - Skip next instruction if VX != VY
            V[(opcode >> 8) & 0x000F] != V[(opcode >> 4) & 0x000F] ? PC += 4 : PC += 2;
            break;

        case 0xA000: // ANNN: Set I to NNN
            I = opcode & 0x0FFF;
            PC += 2;
            break;

        case 0xB000: // BNNN: Jump to address NNN + V0
            PC = (opcode & 0x0FFF) + V[0];
            break;

        case 0xC000: // CXNN: Set VX to a random number with a mask of NN
            V[(opcode >> 8) & 0x000F] = (rand() % 256) & (opcode & 0x00FF);
            PC += 2;
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
            PC += 2;
        }
            break;

        case 0xE000: // 0xEXXX ops
            switch (opcode & 0x00FF) {
                case 0x009E: // 0xEX9E: Skip next instruction if key with the value of VX is pressed.
                    PC += key[V[(opcode >> 8) & 0x000F]] ? 4 : 2;
                    break;

                case 0x00A1: // 0xEXA1: Skip next instruction if key with the value of VX isn't pressed.
                    PC += !key[V[(opcode >> 8) & 0x000F]] ? 4 : 2;
                    break;

                default:
                    fprintf(stderr, "Unknown opcode: 0x%x\n", opcode);
                    break;
            }
            break;

        case 0xF000: // 0xFX07 - Set VX to the value of the delay timer
            switch (opcode & 0x00FF) {
                case 0x0007: // 0xFX07: Set VX to the value of the delay timer
                    V[(opcode >> 8) & 0x000F] = delay_timer;
                    PC += 2;
                    break;

                case 0x000A: // 0xFX0A - Wait for a key press, then store the value of the key in VX
                    for (int i = 0; i < 16; i++) {
                        if (key[i]) {
                            V[(opcode >> 8) & 0x000F] = i;
                            break;
                        }
                    }
                    PC += 2;
                    break;

                case 0x0015: // 0xFX15 - Set the delay timer to VX
                    delay_timer = V[(opcode >> 8) & 0x000F];
                    PC += 2;
                    break;

                case 0x0018: // 0xFX18 - Set the sound timer to VX
                    sound_timer = V[(opcode >> 8) & 0x000F];
                    PC += 2;
                    break;

                case 0x001E: // 0xFX1E - Add VX to I
                    V[0xF] = I + V[(opcode >> 8) & 0x000F] > 0xFFF ? 1 : 0;
                    PC += 2;
                    break;

                case 0x0029: // 0xFX29 - Set I to the location of the sprite for the character in VX
                    I = V[(opcode >> 8) & 0x000F] * 0x5;
                    PC += 2;
                    break;

                case 0x0033: // 0xFX33 - Store the Binary-coded decimal representation of VX in memory locations I, I+1, and I+2
                    memory[I] = (V[(opcode >> 8) & 0x000F] % 1000) / 100;
                    memory[I + 1] = (V[(opcode >> 8) & 0x000F] % 100) / 10;
                    memory[I + 2] = (V[(opcode >> 8) & 0x000F] % 10);
                    PC += 2;
                    break;

                case 0x0055: // 0xFX55 - Store V0 to VX in memory starting at address I
                    for (int i = 0; i <= ((opcode >> 8) & 0x000F); i++)
                        memory[I + i] = V[i];
                    I += ((opcode >> 8) & 0x000F) + 1;
                    PC += 2;
                    break;

                case 0x0065: // 0xFX65 - Load V0 to VX from memory starting at address I
                    for (int i = 0; i <= ((opcode >> 8) & 0x000F); i++)
                        V[i] = memory[I + i];
                    I += ((opcode >> 8) & 0x000F) + 1;
                    PC += 2;
                    break;
            }
            break;

        default:
            fprintf(stderr, "Unknown opcode: 0x%x\n", opcode);
            break;
    }

#ifdef DEBUG_MODE
 printState();
#endif
}

void chip8::tick(){
    // Update timers
    if (delay_timer > 0)
        delay_timer--;

    if (sound_timer > 0) {
        if (sound_timer == 1)
            printf("BEEP!\n");
        sound_timer--;
    }
}

void chip8::printState() {
    printf("------------------------------------------------------------------\n");
    printf("\n");

    printf("V0: 0x%02x  V4: 0x%02x  V8: 0x%02x  VC: 0x%02x\n",
           V[0], V[4], V[8], V[12]);
    printf("V1: 0x%02x  V5: 0x%02x  V9: 0x%02x  VD: 0x%02x\n",
           V[1], V[5], V[9], V[13]);
    printf("V2: 0x%02x  V6: 0x%02x  VA: 0x%02x  VE: 0x%02x\n",
           V[2], V[6], V[10], V[14]);
    printf("V3: 0x%02x  V7: 0x%02x  VB: 0x%02x  VF: 0x%02x\n",
           V[3], V[7], V[11], V[15]);

    printf("\n");
    printf("PC: 0x%04x\n", PC);
    printf("\n");
    printf("\n");
}

chip8::~chip8() = default;
