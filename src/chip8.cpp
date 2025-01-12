#include "chip8.hpp"
#include "constants.hpp"

#include <fstream>
#include <vector>
#include <iostream>
#include <filesystem>

// Constructor
Chip8::Chip8() {
    // Reset pointers and counters
    pc = 0x200; 
    opcode = 0;
    I = 0;
    sp = 0;


    // TODO:  Clear display	
    
    clearStack();
    clearRegisters();
    clearMemory();
    loadFontSet();

    // TODO: Reset timers
}

void Chip8::clearStack() {
    for (int i=0; i < 16; i++) {
        stack[i] = 0;
    }
}

void Chip8::clearRegisters() {
     for (int i=0; i < 16; i++) {
        V[i] = 0;
    }
}

void Chip8::clearMemory() {
    for (int i=0; i < 4096; i++) {
        memory[i] = 0;
    }
}

void Chip8::loadFontSet() {
    // first 80 bytes of memory stores default sprites
    for (int i=0; i < 80; i++) {
        memory[i] = c8const::chip8_fontset[i];
    }
}

// If this does not work, can try to read to a buffer first then transfer data to memory
void Chip8::loadROM(const std::string& message) {
    try {
        // Construct the filename and path 
        const std::string fileName = "../../games/" + message + ".ch8"; // TODO: Provide better path?
        const std::filesystem::path inputFilePath{fileName};

        std::cout << "Attempting to load ROM from: " << inputFilePath << std::endl;

        // Check if file exists before attempting operations
        if (!std::filesystem::exists(inputFilePath)) {
            throw std::runtime_error("File does not exist: " + fileName);
        }

        // Get file size (might throw filesystem_error)
        const auto length = std::filesystem::file_size(inputFilePath);
        // TODO: DENOTE PROGRAM START ADDRESS AS CONSTANT
        std::size_t offset = 512;
        

        if (length > sizeof(memory) - offset) {
            throw std::runtime_error("Not enough space in memory!");
        }

        // Read input file as binary
        std::ifstream inputFile(inputFilePath, std::ios::binary);
        if (!inputFile) {
            throw std::runtime_error("Failed to open file: " + fileName);
        }

        // Read file content
        if (!inputFile.read(reinterpret_cast<char*>(memory + offset), length)) {
            throw std::runtime_error("Failed to read file: " + fileName);
        }

        std::cout << "success! \n";

    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error(std::string("Filesystem error: ") + e.what());
    }
}

void Chip8::emulationCycle() {
    // Opcode variables
    // nnn or addr: 12-bit, lowest 12 bit of instruction
    // n: 4-bit, lowest 4 bit of instruction
    // x: 4-bit, Lowest 4 bit of high byte of instruction
    // y: 4-bit, Upper 4 bit of low byte of instruction
    // kk: 8-bit, Lowest 8 bits of instruction
        
    fetchOpcode();
    decodeOpcode();

    // TODO: Update Timer
}

void Chip8::fetchOpcode() {
    opcode = memory[pc] << 8 | memory[pc+1];
}

void Chip8::decodeOpcode() {
    // First nibble determines category of instruction
    // Categories:
    // 0: System instruction
    // 1: Jump
    // 2: Subroutine call
    // 3: Skip if equal
    // 4: Skip if not equal
    // 5: Skip if register equal
    // 6: Load constant
    // 7: Add constant
    // 8: Arithmetic and logic
    // 9: Skip if register not equal
    // A: Load index
    // B: Jump with offset
    // C: Random
    // D: Draw  
    // E: Key input instruction
    // F: Misc instructions
    unsigned short opCategory = opcode & 0xF000;
    unsigned short opInstruction = opcode & 0x0FFF;

    switch(opCategory) {
        case 0x0000:
            executeSI(opInstruction);
        break;

        case 0x1000:
            executeJMP(opInstruction);
        break;

        case 0x2000:
            executeSRC(opInstruction);
        break;

        case 0x3000:
            executeSE(opInstruction);
        break;

        case 0x4000:
            executeSNE(opInstruction);
        break;

        case 0x5000:
            executeSRE(opInstruction);
        break;

        case 0x6000:
            executeLC(opInstruction);
        break;

        case 0x7000:
            executeAC(opInstruction);
        break;

        case 0x8000:
            executeAL(opInstruction);
        break;

        case 0x9000:
            executeSRNE(opInstruction);
        break;

        case 0xA000:
            executeLI(opInstruction);
        break;

        case 0xB000:
            executeJMPOFF(opInstruction);
        break;

        case 0xC000:
            executeRAN(opInstruction);
        break;

        case 0xD000:
            executeDR(opInstruction);
        break;

        case 0xE000:
            executeKII(opInstruction);
        break;

        case 0xF000:
            executeMISC(opInstruction);
        break;
    }

}

void Chip8::executeSI(unsigned short opInstruction) {
    switch(opInstruction & 0x00FF) {
        // CLS - 00E0 : CLear display
        case 0x00EE:
            // TODO : Clear display
        break;

        // RET - 00EE : Return from subroutine
        case 0x00E0:
            pc = stack[sp];
            sp -= 1;
        break;

        default:
            printf("Unknown opcode 0: 0x%X\n", opcode);
    }
}

void Chip8::executeJMP(unsigned short opInstruction) {
    // JP addr - 1nnn : Jump to address nnn
    pc = opInstruction & 0x0FFF;
}

void Chip8::executeSRC(unsigned short opInstruction) {
    // CALL addr - 2nnn : Call subroutine at nnn
    sp += 1;
    stack[sp] = pc;
    pc = opInstruction & 0x0FFF;
}

void Chip8::executeSE(unsigned short opInstruction) {
    // SE Vx, byte - 3xkk : Skip next instruction if Vx = kk
    unsigned char x = (opInstruction & 0x0F00) >> 8;
    unsigned char kk = (opInstruction & 0X00FF);

    if (V[x] == kk) {
        pc += 2;
    }
}

void Chip8::executeSNE(unsigned short opInstruction) {
    // SNE Vx, byte - 4xkk : Skip next instruction if Vx != kk
    unsigned char x = (opInstruction & 0x0F00) >> 8;
    unsigned char kk = (opInstruction & 0X00FF);

    if (V[x] != kk) {
        pc += 2;
    }
}

void Chip8::executeSRE(unsigned short opInstruction) {
    // SE Vx, Vy - 5xy0 : Skip next instruction if Vx = Vy
    unsigned char x = (opInstruction & 0x0F00) >> 8;
    unsigned char y = (opInstruction & 0X00F0) >> 4;

    if (V[x] == V[y]) {
        pc += 2;
    }
}

void Chip8::executeLC(unsigned short opInstruction) {
    // LD Vx, byte - 6xkk : Put value kk into register Vx
    unsigned char x = (opInstruction & 0x0F00) >> 8;
    unsigned char kk = opInstruction & 0X00FF;

    V[x] = kk;
    pc += 2;
}

void Chip8::executeAC(unsigned short opInstruction) {
    // ADD Vx, byte - 7xkk : Adds value kk  value of register Vx, store in Vx
    unsigned char x = (opInstruction & 0x0F00) >> 8;
    unsigned char kk = opInstruction & 0X00FF;

    V[x] += kk;
    pc += 2;
}

void Chip8::executeAL(unsigned short opInstruction) {
    unsigned char x = (opInstruction & 0x0F00) >> 8;
    unsigned char y = (opInstruction & 0X00F0) >> 4;
    unsigned short sum;

    switch (opInstruction & 0x000F) {
        case 0x0000:
            // LD Vx, Vy - 8xy0 : Stores the value of register Vy in register Vx
            V[x] += V[y];
        break;

        case 0x0001:
            // OR Vx, Vy - 8xy1 : Performs bitwise OR on Vx and Vy, stores result in Vx
            V[x] |= V[y];
        break;

        case 0x0002:
            // AND Vx, Vy - 8xy2 : Performs bitwise AND on Vx and Vy, stores result in Vx
            V[x] &= V[y];
        break;

        case 0x0003:
            // AND Vx, Vy - 8xy3 : Performs bitwise XOR on Vx and Vy, stores result in Vx
            V[x] ^= V[y];
        break;

        case 0x0004:
            // ADD Vx, Vy - 8xy4 : Performs addition on Vx and Vy, 
            // If the result is greater than 8 bits, VF set to 1, otherwise 0. 
            // Only lowest 8 bits of the result are kept, and stored in Vx.
            sum = V[x] + V[y];

            if (sum > 0xFF) {
                V[0XF] = 1;
            } else {
                V[0XF] = 0;
            }

            V[x] = sum & 0XFF;
        break;

        case 0x0005:
            // SUB Vx, Vy - 8xy5 : If Vx > Vy, then VF is set to 1, otherwise 0. 
            // Vy is then subtracted from Vx, and stored in Vx.

            if (V[x] > V[y]) {
                V[0XF] = 1;
            } else {
                V[0XF] = 0;
            }

            V[x] -= V[y];
        break;

        case 0x0006:
            // SHR Vx {, Vy} - 8xy6 : If the least-significant bit of Vx is 1, then VF is set to 1, 
            // otherwise 0. Then Vx is divided by 2.

            if ((V[x] & 0x01) == 0x01) {
                V[0xF] = 1;
            } else {
                V[0xF] = 0;
            }

            V[x] >>= 1;
        break;

        case 0x0007:
            // SUBN Vx, Vy - 8xy7 : If Vy > Vx, then VF is set to 1, otherwise 0.
            // Then Vx is subtracted from Vy, and the results stored in Vx.

            if (V[y] > V[x]) {
                V[0xF] = 1;
            } else {
                V[0xF] = 0;
            }

            V[x] = V[y] - V[x];
        break;  

        case 0x000E:
            // SHL Vx {, Vy} - 8xyE : If the most-significant bit of Vx is 1, 
            // then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.

            if ((V[x] & 0x80) == 0x80) {
                V[0xF] = 1;
            } else {
                V[0xF] = 0;
            }

            V[x] <<= 1;
        break;

        default:
            printf("Unknown opcode 8: 0x%X\n", opcode);
    }

    pc += 2;
}

void Chip8::executeSRNE(unsigned short opInstruction) {
    // SNE Vx, Vy - 9xy0 : The values of Vx and Vy are compared, 
    // and if they are not equal, the program counter is increased by 2.

    unsigned char x = (opInstruction & 0x0F00) >> 8;
    unsigned char y = (opInstruction & 0X00F0) >> 4;

    if (V[x] != V[y]) {
        pc += 2;
    }
}

void Chip8::executeLI(unsigned short opInstruction) {
    // LD I, addr - Annn : The value of register I is set to nnn.

    I = opInstruction & 0x0FFF;
    pc += 2;
}

void Chip8::executeJMPOFF(unsigned short opInstruction) {
    // JP V0, addr - Bnnn : The program counter is set to nnn plus the value of V0.

    pc = (opInstruction & 0x0FFF) + V[0];
}

void Chip8::executeRAN(unsigned short opInstruction) {
    // RND Vx, byte - Cxkk : The interpreter generates a random number from 0 to 255, 
    // which is then ANDed with the value kk. The results are stored in Vx. 
    unsigned char x = (opInstruction & 0x0F00) >> 8;
    unsigned char kk = opInstruction & 0X00FF;

    auto random = rand() % 256;
    V[x] = random & kk;
    pc += 2;

}

// TODO
void Chip8::executeDR(unsigned short opInstruction) {

}

// TODO
void Chip8::executeKII(unsigned short opInstruction) {

}

// TODO
void Chip8::executeMISC(unsigned short opInstruction) {

}

