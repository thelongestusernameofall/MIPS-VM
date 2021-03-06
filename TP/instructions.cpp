#include "instructions.h"
#include "regs.h"
#include "syscalls.h"
#include "memory.h"
#include <map>
#include <cstdio>
using namespace std;

// MIPS instruction parts
#define OPCODE(x)   (((x)&0xFC000000)>>26)
#define RS(x)       (((x)&0x3E00000)>>21)
#define RT(x)       (((x)&0x1F0000)>>16)
#define RD(x)       (((x)&0xF800)>>11)
#define SHAMT(x)    (((x)&0x7C0)>>6)
#define FUNCT(x)    ((x)&0x3F)
#define IMM(x)      ((x)&0xFFFF)
#define ADDR(x)     ((x)&0x3FFFFFF)

// HELPERS

// interpret immediate as a signed 16-bit integer (default: unsigned)
#define SIGN_IMM(x) ((int32_t(x)&0x7FFF) - (int32_t(x)&0x8000))

// INTRUCTOR DECLARATORS
#define DECL_R_INSTR(x) \
int x(int32_t rs, int32_t rt, int32_t rd, int32_t shamt)

#define DECL_I_INSTR(x) \
int x(int32_t rs, int32_t rt, int32_t imm)

#define DECL_J_INSTR(x) \
int x(int32_t addr)

// TODO: add floating-point variants

// INSTRUCTIONS
DECL_R_INSTR(add){
    int32_t a = R[rs];
    int32_t b = R[rt];
    R[rd] = a + b;
    return 0;
}

DECL_R_INSTR(sub){
    int32_t a = R[rs];
    int32_t b = R[rt];
    R[rd] = a - b;
    return 0;
}

DECL_R_INSTR(mult){
    int64_t a = (int32_t) R[rs];
    int64_t b = (int32_t) R[rt];
    int64_t c = a * b;
    HI = c >> 32;
    LO = c & 0xFFFFFFFF;
    return 0;
}

DECL_R_INSTR(multu){
    uint64_t a = R[rs];
    uint64_t b = R[rt];
    uint64_t c = a * b;
    HI = c >> 32;
    LO = c & 0xFFFFFFFF;
    return 0;
}

DECL_R_INSTR(div){
    int32_t a = R[rs];
    int32_t b = R[rt];
    try{
        HI = a % b;
        LO = a / b;
        return 0;
    }catch( ... ){
        return ST_ERROR;  // TODO: standarize: DISGUSTING !
    }

}

DECL_R_INSTR(divu){
    uint32_t a = R[rs];
    uint32_t b = R[rt];
    try{
        HI = a % b;
        LO = a / b;
        return 0;
    }catch( ... ){
        return ST_ERROR;  // TODO: standarize: DISGUSTING !
    }

}

DECL_R_INSTR(slt){
    int32_t a = R[rs];
    int32_t b = R[rt];
    R[rd] = (a < b);
    return 0;
}

DECL_R_INSTR(sltu){
    uint32_t a = R[rs];
    uint32_t b = R[rt];
    R[rd] = (a < b);
    return 0;
}

DECL_R_INSTR(sll){
    R[rd] =  R[rt] << shamt;
    return 0;
}

DECL_R_INSTR(srl){
    R[rd] =  R[rt] >> shamt;
    return 0;
}

DECL_R_INSTR(sra){
    int32_t a = R[rt];
    R[rd] = a >> shamt;
    return 0;
}

DECL_R_INSTR(jr){
    PC = R[rs];
    return ST_NOADVANCE;
}

DECL_R_INSTR(and_){ // and is a reserved C++ keyword
    R[rd] = R[rs] & R[rt];
    return 0;
}

DECL_R_INSTR(or_){ // or is a reserved C++ keyword
    R[rd] = R[rs] | R[rt];
    return 0;
}

DECL_R_INSTR(xor_){ // xor is a reserved C++ keyword
    R[rd] = R[rs] ^ R[rt];
    return 0;
}

DECL_R_INSTR(nor){
    R[rd] = ~(R[rs] | R[rt]);
    return 0;
}

DECL_R_INSTR(mfhi){
    R[rd] = HI;
    return 0;
}

DECL_R_INSTR(mthi){
    HI = R[rs];
    return 0;
}

DECL_R_INSTR(mtlo){
    LO = R[rs];
    return 0;
}

DECL_R_INSTR(mflo){
    R[rd] = LO;
    return 0;
}

DECL_I_INSTR(addi){
    int32_t a = R[rs];
    int32_t b = SIGN_IMM(imm);
    R[rt] = a + b;
    return 0;
}

DECL_I_INSTR(beq){
    if( R[rs] == R[rt] ){
        PC = PC + SIGN_IMM(imm) * 4;
        return ST_NOADVANCE;
    }
    return 0;
}

DECL_I_INSTR(bne){
    if( R[rs] != R[rt] ){
        PC = PC + SIGN_IMM(imm) * 4;
        return ST_NOADVANCE;
    }
    return 0;
}

DECL_I_INSTR(bltz_bgez){
    if( R[rt] == 0 ){
        if( (int32_t) R[rs] < 0 ){
            PC = PC + SIGN_IMM(imm) * 4;
            return ST_NOADVANCE;
        }
        return 0;
    }
    if( R[rt] == 1 ){
        if( (int32_t) R[rs] >= 0 ){
            PC = PC + SIGN_IMM(imm) * 4;
            return ST_NOADVANCE;
        }
        return 0;
    }
    return ST_ERROR;
}

DECL_I_INSTR(blez){
    if( (int32_t) R[rs] <= 0 ){
        PC = PC + SIGN_IMM(imm) * 4;
        return ST_NOADVANCE;
    }
    return 0;
}

DECL_I_INSTR(bgtz){
    if( (int32_t) R[rs] > 0 ){
        PC = PC + SIGN_IMM(imm) * 4;
        return ST_NOADVANCE;
    }
    return 0;
}

DECL_I_INSTR(addiu){
    int32_t a = R[rs];
    R[rt] = a + imm;
    return 0;
}

DECL_I_INSTR(slti){
    int32_t a = R[rs];
    int32_t b = SIGN_IMM(imm);
    R[rt] = (a < b);
    return 0;
}

DECL_I_INSTR(sltiu){
    uint32_t a = R[rs];
    uint32_t b = imm;
    R[rt] = (a < b);
    return 0;
}

DECL_I_INSTR(andi){ // zero-extending (MIPS convention)
    R[rt] = R[rs] & imm;
    return 0;
}

DECL_I_INSTR(ori){ // zero-extending (MIPS convention)
    R[rt] = R[rs] | imm;
    return 0;
}

DECL_I_INSTR(lui){
    R[rt] = imm << 16;
    return 0;
}

DECL_I_INSTR(lb){
    int32_t offset = SIGN_IMM(imm);
    int32_t v_addr = R[rs] + offset;
    int8_t *r_addr = static_cast<int8_t *>(resolve_addr( v_addr ));
    if( !r_addr )
        return ST_ERROR;
    R[rt] = static_cast<int32_t>(*r_addr);
    return 0;
}

DECL_I_INSTR(sb){
    int32_t offset = SIGN_IMM(imm);
    int32_t v_addr = R[rs] + offset;
    uint8_t *r_addr = static_cast<uint8_t *>(resolve_addr( v_addr ));
    if( !r_addr )
        return ST_ERROR;
    *r_addr = R[rt];
    return 0;
}

DECL_I_INSTR(sh){
    int32_t offset = SIGN_IMM(imm);
    int32_t v_addr = R[rs] + offset;
    uint16_t *r_addr = static_cast<uint16_t *>(resolve_addr( v_addr ));
    if( !r_addr )
        return ST_ERROR;
    *r_addr = R[rt];
    return 0;
}

DECL_I_INSTR(lh){
    int32_t offset = SIGN_IMM(imm);
    int32_t v_addr = R[rs] + offset;
    uint16_t *r_addr = static_cast<uint16_t *>(resolve_addr( v_addr ));
    if( !r_addr )
        return ST_ERROR;
    R[rt] = static_cast<uint16_t>(*r_addr);
    return 0;
}

DECL_I_INSTR(lbu){
    int32_t offset = SIGN_IMM(imm);
    int32_t v_addr = R[rs] + offset;
    uint8_t *r_addr = static_cast<uint8_t *>(resolve_addr( v_addr ));
    if( !r_addr )
        return ST_ERROR;
    R[rt] = static_cast<uint32_t>(*r_addr);
    return 0;
}

DECL_I_INSTR(lw){
    int32_t offset = SIGN_IMM(imm);
    int32_t v_addr = R[rs] + offset;
    uint32_t *r_addr = static_cast<uint32_t *>(resolve_addr( v_addr ));
    if( !r_addr )
        return ST_ERROR;
    R[rt] = *r_addr;
    return 0;
}

DECL_I_INSTR(sw){
    int32_t offset = SIGN_IMM(imm);
    int32_t v_addr = R[rs] + offset;
    uint32_t *r_addr = static_cast<uint32_t *>(resolve_addr( v_addr ));
    if( !r_addr ){
        return ST_ERROR;
	}
    *r_addr = R[rt];
    return 0;
}

DECL_J_INSTR(j){
    addr <<= 2;
    addr |= (PC + 4) & 0xF0000000;
    PC = addr;
    return ST_NOADVANCE;
}

DECL_J_INSTR(jal){
    R[REG_RA] = PC + 4;
    addr <<= 2;
    addr |= (PC + 4) & 0xF0000000;
    PC = addr;
    return ST_NOADVANCE;
}

// R-INSTRUCTION MAP
map<int, inst_r_t> R_funct = {
    {0x00, sll},
    {0x02, srl},
    {0x03, sra},
    {0x08, jr},
    {0x10, mfhi},
    {0x11, mthi},
    {0x12, mflo},
    {0x13, mtlo},
    {0x18, mult},
    {0x19, multu},
    {0x1A, div},
    {0x1B, divu},
    {0x20, add},
    {0x21, add}, // ADDU TODO: HANDLE OVERFLOW TRAP
    {0x22, sub},
    {0x23, sub}, // SUBU TODO: HANDLE OVERFLOW TRAP
    {0x24, and_},
    {0x25, or_},
    {0x26, xor_},
    {0x27, nor},
    {0x2A, slt},
    {0x2B, sltu}
};

// I-INSTRUCTION MAP
map<int, inst_i_t> I_opcodes = {
    {0x01, bltz_bgez},
    {0x04, beq},
    {0x05, bne},
    {0x06, blez},
    {0x07, bgtz},
    {0x08, addi},
    {0x09, addiu},
    {0x0A, slti},
    {0x0B, sltiu},
    {0x0C, andi},
    {0x0D, ori},
    {0x0F, lui},
    {0x20, lb},
    {0x21, lh},
    {0x23, lw},
    {0x24, lbu},
    {0x28, sb},
    {0x29, sh},
    {0x2B, sw}
};

// J-INSTRUCTION MAP
map<int, inst_j_t> J_opcodes = {
    {0x02, j},
    {0x03, jal}
};


// DECODE FUNCTION
int decode(instruction i){

    // check if it's a syscall
    if( i == 0xC )
        return syscall();
    
    unsigned oc = OPCODE(i);
    if( oc == 0x00 ){
        // R type instruction
        int32_t rs = RS(i);
        int32_t rt = RT(i);
        int32_t rd = RD(i);
        int32_t shamt = SHAMT(i);
        int32_t funct = FUNCT(i);
        auto operation = R_funct[funct];
        if( operation )
            return operation(rs, rt, rd, shamt);
        else{
            // not implement or invalid TODO: STANDARIZE THIS S·H·I·T
            printf("funct %u not found\n", funct);
            return ST_ERROR;
        }
    }
    
    if( auto operation = I_opcodes[oc] ){
        // I type instruction
        int32_t rs = RS(i);
        int32_t rt = RT(i);
        int32_t imm = IMM(i);
        return operation(rs, rt, imm);
    }
    
    if( auto operation = J_opcodes[oc] ){
        // J type instruction
        int32_t addr = ADDR(i);
        return operation(addr);        
    }
    
    // not implemented TODO: standarize
    printf("opcode %X not found\n", oc);
    return ST_ERROR;
}
