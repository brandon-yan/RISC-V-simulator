//
// Created by hanchong on 2020/7/7.
//

#ifndef RISC_V_SIMULATOR_RISCV_HPP
#define RISC_V_SIMULATOR_RISCV_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <bitset>

enum Insttype {
    LUI, AUIPC,                                              //U-type
    JAL,                                                     //J-type
    JALR, LB, LH, LW, LBU, LHU,
    ADDI, SLTI,SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,     //I-type
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,        //R-type
    BEQ, BNE, BLT, BGE, BLTU, BGEU,                          //B-type
    SB, SH, SW,                                              //S-type
    NOP, START
};

unsigned char memory[0x20000];
int regi[32];
unsigned int pc = 0;
bool ifrun = true, ifmem = false, ifpredict = true;
int datacnt = 0, memcnt = 0;

int get_num(int inst, int l, int r) {
    int ret = ((inst >> l) & (1 << (r - l + 1)) - 1);
    return ret;
}
struct instcode {
    int inst = 0;
    int rs1 = -1;
    int rs2 = -1;
    int rd = -1;
    int imm = 0;
    int shamt = 0;
    int value = 0;
    int addr = 0;
    int inspc = 0;
    bool ifjump = false;
    Insttype type = START;
    void clear() {
        inst = 0, rs1 = -1, rs2 = -1, rd = -1, imm = 0, shamt = 0, value = 0, addr = 0, inspc = 0, ifjump = false, type = NOP;
    }

};
instcode IF_ID, ID_EX, EX_MEM, MEM_WB;

struct predictor {
    int count = 0;               //00 01 for notjump, 10 11 for jump
    bool ifjump = false;         //true for jump, false for notjump
    bool ifright(bool ifju) {
        if (count == 0) {
            if (ifju) {
                count = 1;
                ifjump = false;
                return false;
            }
            return true;
        }
        else if (count == 1) {
            if (ifju) {
                count = 3;
                ifjump = true;
                return false;
            }
            else {
                count = 0;
                ifjump = false;
                return true;
            }
        }
        else if (count == 2) {
            if (ifju) {
                count = 3;
                ifjump = true;
                return true;
            }
            else {
                count = 0;
                ifjump = false;
                return false;
            }
        }
        else if (count == 3) {
            if (!ifju) {
                count = 2;
                ifjump = true;
                return false;
            }
            return true;
        }
    };
}predi[100];

int get_U_imm (const int &inst) {
    int ret = get_num(inst, 12, 31);
    ret <<= 12;
    return ret;
};

int get_J_imm (const int &inst) {
    int ret = 0;
    if (inst >> 31) ret = (1 << 13) - 1;
    ret <<= 8;
    ret += get_num(inst, 12, 19);
    ret <<= 1;
    ret += get_num(inst, 20, 20);
    ret <<= 10;
    ret += get_num(inst, 21, 30);
    ret <<= 1;
    return ret;
};

int get_I_imm (const int &inst) {
    int ret = 0;
    if (inst >> 31) ret = (1 << 22) - 1;
    ret <<= 11;
    ret += get_num(inst, 20, 30);
    return ret;
};

int get_B_imm (const int &inst) {
    int ret = 0;
    if (inst >> 31) ret = (1 << 21) - 1;
    ret <<= 1;
    ret += get_num(inst, 7, 7);
    ret <<= 6;
    ret += get_num(inst, 25, 30);
    ret <<= 4;
    ret += get_num(inst, 8, 11);
    ret <<= 1;
    return ret;
};

int get_S_imm (const int &inst) {
    int ret = 0;
    if (inst >> 31) ret = (1 << 22) - 1;
    ret <<= 6;
    ret += get_num(inst, 25, 30);
    ret <<= 4;
    ret += get_num(inst, 8, 11);
    ret <<= 1;
    ret += get_num(inst, 7, 7);
    return ret;
};

void IF() {
    int inst = *((unsigned int*)(memory + pc));
    IF_ID.inspc = pc;
    pc += 4;
    IF_ID.inst = inst;
}

void ID(instcode obj) {
    if (!ifpredict) return;
    int inst = obj.inst;
    int opcode = inst & 127;
    int funct3 = get_num(inst, 12, 14);
    int funct7 = get_num(inst, 25, 31);
    if (IF_ID.type == NOP) {
        IF_ID.type = START;
        ID_EX.type = NOP;
        ID_EX.inst = 0;
        return;
    }
    switch (opcode) {
        case 55:
            obj.type = LUI;
            obj.rd = get_num(inst, 7, 11);
            obj.imm = get_U_imm(inst); break;
        case 23:
            obj.type = AUIPC;
            obj.rd = get_num(inst, 7, 11);
            obj.imm = get_U_imm(inst); break;
        case 111:
            obj.type = JAL;
            obj.rd = get_num(inst, 7, 11);
            obj.imm = get_J_imm(inst);
            pc = obj.inspc + obj.imm;
            ID_EX.ifjump = true; break;
        case 103:
            obj.type = JALR;
            obj.rd = get_num(inst, 7, 11);
            obj.imm = get_I_imm(inst);
            obj.rs1 = get_num(inst, 15, 19); break;
        case 99:
            switch (funct3) {
                case 0:
                    obj.type = BEQ;
                    obj.rs1 = get_num(inst, 15, 19);
                    obj.rs2 = get_num(inst, 20, 24);
                    obj.imm = get_B_imm(inst);
                    if (predi[obj.inspc & 63].ifjump) {
                        pc = obj.inspc + obj.imm;
                        ID_EX.ifjump = true;
                    } break;
                case 1:
                    obj.type = BNE;
                    obj.rs1 = get_num(inst, 15, 19);
                    obj.rs2 = get_num(inst, 20, 24);
                    obj.imm = get_B_imm(inst);
                    if (predi[obj.inspc & 63].ifjump) {
                        pc = obj.inspc + obj.imm;
                        ID_EX.ifjump = true;
                    } break;
                case 4:
                    obj.type = BLT;
                    obj.rs1 = get_num(inst, 15, 19);
                    obj.rs2 = get_num(inst, 20, 24);
                    obj.imm = get_B_imm(inst);
                    if (predi[obj.inspc & 63].ifjump) {
                        pc = obj.inspc + obj.imm;
                        ID_EX.ifjump = true;
                    } break;
                case 5:
                    obj.type = BGE;
                    obj.rs1 = get_num(inst, 15, 19);
                    obj.rs2 = get_num(inst, 20, 24);
                    obj.imm = get_B_imm(inst);
                    if (predi[obj.inspc & 63].ifjump) {
                        pc = obj.inspc + obj.imm;
                        ID_EX.ifjump = true;
                    } break;
                case 6:
                    obj.type = BLTU;
                    obj.rs1 = get_num(inst, 15, 19);
                    obj.rs2 = get_num(inst, 20, 24);
                    obj.imm = get_B_imm(inst);
                    if (predi[obj.inspc & 63].ifjump) {
                        pc = obj.inspc + obj.imm;
                        ID_EX.ifjump = true;
                    } break;
                case 7:
                    obj.type = BGEU;
                    obj.rs1 = get_num(inst, 15, 19);
                    obj.rs2 = get_num(inst, 20, 24);
                    obj.imm = get_B_imm(inst);
                    if (predi[obj.inspc & 63].ifjump) {
                        pc = obj.inspc + obj.imm;
                        ID_EX.ifjump = true;
                    } break;
            } break;
        case 3:
            switch (funct3) {
                case 0:
                    obj.type = LB;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19); break;
                case 1:
                    obj.type = LH;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19); break;
                case 2:
                    obj.type = LW;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19); break;
                case 4:
                    obj.type = LBU;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19); break;
                case 5:
                    obj.type = LHU;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19); break;
            } break;
        case 35:
            switch (funct3) {
                case 0:
                    obj.type = SB;
                    obj.rs1 = get_num(inst, 15, 19);
                    obj.rs2 = get_num(inst, 20, 24);
                    obj.imm = get_S_imm(inst); break;
                case 1:
                    obj.type = SH;
                    obj.rs1 = get_num(inst, 15, 19);
                    obj.rs2 = get_num(inst, 20, 24);
                    obj.imm = get_S_imm(inst); break;
                case 2:
                    obj.type = SW;
                    obj.rs1 = get_num(inst, 15, 19);
                    obj.rs2 = get_num(inst, 20, 24);
                    obj.imm = get_S_imm(inst); break;
            } break;
        case 19:
            switch (funct3) {
                case 0:
                    obj.type = ADDI;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19); break;
                case 2:
                    obj.type = SLTI;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19); break;
                case 3:
                    obj.type = SLTIU;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19); break;
                case 4:
                    obj.type = XORI;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19); break;
                case 6:
                    obj.type = ORI;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19); break;
                case 7:
                    obj.type = ANDI;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19); break;
                case 1:
                    obj.type = SLLI;
                    obj.rd = get_num(inst, 7, 11);
                    obj.imm = get_I_imm(inst);
                    obj.rs1 = get_num(inst, 15, 19);
                    obj.shamt = get_num(inst, 20, 24); break;
                case 5:
                    switch (funct7) {
                        case 0:
                            obj.type = SRLI;
                            obj.rd = get_num(inst, 7, 11);
                            obj.imm = get_I_imm(inst);
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.shamt = get_num(inst, 20, 24); break;
                        case 32:
                            obj.type = SRAI;
                            obj.rd = get_num(inst, 7, 11);
                            obj.imm = get_I_imm(inst);
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.shamt = get_num(inst, 20, 24); break;
                    } break;
            } break;
        case 51:
            switch (funct7) {
                case 0:
                    switch (funct3) {
                        case 0:
                            obj.type = ADD;
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.rs2 = get_num(inst, 20, 24);
                            obj.rd = get_num(inst, 7, 11); break;
                        case 1:
                            obj.type = SLL;
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.rs2 = get_num(inst, 20, 24);
                            obj.rd = get_num(inst, 7, 11); break;
                        case 2:
                            obj.type = SLT;
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.rs2 = get_num(inst, 20, 24);
                            obj.rd = get_num(inst, 7, 11); break;
                        case 3:
                            obj.type = SLTU;
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.rs2 = get_num(inst, 20, 24);
                            obj.rd = get_num(inst, 7, 11); break;
                        case 4:
                            obj.type = XOR;
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.rs2 = get_num(inst, 20, 24);
                            obj.rd = get_num(inst, 7, 11); break;
                        case 5:
                            obj.type = SRL;
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.rs2 = get_num(inst, 20, 24);
                            obj.rd = get_num(inst, 7, 11); break;
                        case 6:
                            obj.type = OR;
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.rs2 = get_num(inst, 20, 24);
                            obj.rd = get_num(inst, 7, 11); break;
                        case 7:
                            obj.type = AND;
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.rs2 = get_num(inst, 20, 24);
                            obj.rd = get_num(inst, 7, 11); break;
                    } break;
                case 32:
                    switch (funct3) {
                        case 0:
                            obj.type = SUB;
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.rs2 = get_num(inst, 20, 24);
                            obj.rd = get_num(inst, 7, 11); break;
                        case 5:
                            obj.type = SRA;
                            obj.rs1 = get_num(inst, 15, 19);
                            obj.rs2 = get_num(inst, 20, 24);
                            obj.rd = get_num(inst, 7, 11); break;
                    } break;
            } break;
    }
    ID_EX.inst = obj.inst, ID_EX.rs1 = obj.rs1, ID_EX.rs2 = obj.rs2, ID_EX.rd = obj.rd,
    ID_EX.shamt = obj.shamt, ID_EX.value = obj.value, ID_EX.addr = obj.addr, ID_EX.type = obj.type,
    ID_EX.inspc = obj.inspc, ID_EX.imm = obj.imm;
}

void EX(instcode obj) {
    switch (obj.type) {
        case NOP:
            obj.inst = 0;
            break;
        case START:
            break;
        case LUI:
            obj.value = obj.imm;
            break;
        case AUIPC:
            obj.value = obj.imm + obj.inspc;
            break;
        case JAL:
            obj.value = obj.inspc + 4;
//            pc = pc + obj.imm - 4;
//            EX_MEM.ifjump = true;
            break;
        case JALR:
            obj.value = obj.inspc + 4;
            pc = (regi[obj.rs1] + obj.imm) & ~1;
            EX_MEM.ifjump = true;
            break;
        case BEQ:
            if (regi[obj.rs1] == regi[obj.rs2]) {
//                EX_MEM.ifjump = true;
//                pc = pc + obj.imm - 4;
                ifpredict = predi[obj.inspc & 63].ifright(true);
                if (!ifpredict) pc = obj.inspc + obj.imm;
            }
            else {
                ifpredict = predi[obj.inspc & 63].ifright(false);
                if (!ifpredict) pc = obj.inspc + 4;
            } break;
        case BNE:
            if (regi[obj.rs1] != regi[obj.rs2]) {
//                EX_MEM.ifjump = true;
//                pc = pc + obj.imm - 4;
                ifpredict = predi[obj.inspc & 63].ifright(true);
                if (!ifpredict) pc = obj.inspc + obj.imm;
            }
            else {
                ifpredict = predi[obj.inspc & 63].ifright(false);
                if (!ifpredict) pc = obj.inspc + 4;
            } break;
        case BLT:
            if (regi[obj.rs1] < regi[obj.rs2]) {
//                EX_MEM.ifjump = true;
//                pc = pc + obj.imm - 4;
                ifpredict = predi[obj.inspc & 63].ifright(true);
                if (!ifpredict) pc = obj.inspc + obj.imm;
            }
            else {
                ifpredict = predi[obj.inspc & 63].ifright(false);
                if (!ifpredict) pc = obj.inspc + 4;
            } break;
        case BGE:
            if (regi[obj.rs1] >= regi[obj.rs2]) {
//                EX_MEM.ifjump = true;
//                pc = pc + obj.imm - 4;
                ifpredict = predi[obj.inspc & 63].ifright(true);
                if (!ifpredict) pc = obj.inspc + obj.imm;
            }
            else {
                ifpredict = predi[obj.inspc & 63].ifright(false);
                if (!ifpredict) pc = obj.inspc + 4;
            } break;
        case BLTU:
            if ((unsigned int)regi[obj.rs1] < (unsigned int)regi[obj.rs2]) {
//                EX_MEM.ifjump = true;
//                pc = pc + obj.imm - 4;
                ifpredict = predi[obj.inspc & 63].ifright(true);
                if (!ifpredict) pc = obj.inspc + obj.imm;
            }
            else {
                ifpredict = predi[obj.inspc & 63].ifright(false);
                if (!ifpredict) pc = obj.inspc + 4;
            } break;
        case BGEU:
            if ((unsigned int)regi[obj.rs1] >= (unsigned int)regi[obj.rs2]) {
//                EX_MEM.ifjump = true;
//                pc = pc + obj.imm - 4;
                ifpredict = predi[obj.inspc & 63].ifright(true);
                if (!ifpredict) pc = obj.inspc + obj.imm;
            }
            else {
                ifpredict = predi[obj.inspc & 63].ifright(false);
                if (!ifpredict) pc = obj.inspc + 4;
            } break;
        case LB:
        case LH:
        case LW:
        case LBU:
        case LHU:
        case SB:
        case SH:
        case SW:
            obj.addr = regi[obj.rs1] + obj.imm;
            break;
        case ADDI:
            obj.value = regi[obj.rs1] + obj.imm;
            break;
        case SLTI:
            obj.value = regi[obj.rs1] < obj.imm;
            break;
        case SLTIU:
            obj.value = (unsigned int)regi[obj.rs1] < (unsigned int)obj.imm;
            break;
        case XORI:
            obj.value = regi[obj.rs1] ^ obj.imm;
            break;
        case ORI:
            obj.value = regi[obj.rs1] | obj.imm;
            break;
        case ANDI:
            obj.value = regi[obj.rs1] & obj.imm;
            break;
        case SLLI:
            obj.value = regi[obj.rs1] << obj.shamt;
            break;
        case SRLI:
            obj.value = (unsigned int)regi[obj.rs1] >> obj.shamt;
            break;
        case SRAI:
            obj.value = regi[obj.rs1] >> obj.shamt;
            break;
        case ADD:
            obj.value = regi[obj.rs1] + regi[obj.rs2];
            break;
        case SUB:
            obj.value = regi[obj.rs1] - regi[obj.rs2];
            break;
        case SLL:
            obj.value = regi[obj.rs1] << regi[obj.rs2];
            break;
        case SLT:
            obj.value = regi[obj.rs1] < regi[obj.rs2];
            break;
        case SLTU:
            obj.value = (unsigned int)regi[obj.rs1] < (unsigned int)regi[obj.rs2];
            break;
        case XOR:
            obj.value = regi[obj.rs1] ^ regi[obj.rs2];
            break;
        case SRL:
            obj.value = (unsigned int)regi[obj.rs1] >> regi[obj.rs2];
            break;
        case SRA:
            obj.value = regi[obj.rs1] >> regi[obj.rs2];
            break;
        case OR:
            obj.value = regi[obj.rs1] | regi[obj.rs2];
            break;
        case AND:
            obj.value = regi[obj.rs1] & regi[obj.rs2];
            break;
    }
    EX_MEM.inst = obj.inst, EX_MEM.rs1 = obj.rs1, EX_MEM.rs2 = obj.rs2, EX_MEM.rd = obj.rd,
    EX_MEM.shamt = obj.shamt, EX_MEM.value = obj.value, EX_MEM.addr = obj.addr, EX_MEM.type = obj.type,
    EX_MEM.inspc = obj.inspc, EX_MEM.imm = obj.imm;
}

void MEM(instcode obj) {
    if (obj.type == LB || obj.type == LH || obj.type == LW || obj.type == LBU || obj.type == LHU || obj.type == SB || obj.type == SH || obj.type == SW) {
        ++memcnt;
        if (memcnt % 3) {
            ifmem = true;
            return;
        }
        else ifmem = false;
    }
    switch(obj.type) {
        case LB:
            obj.value = *((int8_t*)(memory + obj.addr));
            break;
        case LH:
            obj.value = *((int16_t*)(memory + obj.addr));
            break;
        case LW:
            obj.value = *((int32_t *)(memory + obj.addr));
            break;
        case LBU:
            obj.value = *((uint8_t*)(memory + obj.addr));
            break;
        case LHU:
            obj.value = *((uint16_t*)(memory + obj.addr));
            break;
        case SB:
            memcpy(memory + obj.addr, &regi[obj.rs2], 1);
            break;
        case SH:
            memcpy(memory + obj.addr, &regi[obj.rs2], 2);
            break;
        case SW:
            memcpy(memory + obj.addr, &regi[obj.rs2], 4);
            break;
        default:
            break;
    }
    MEM_WB = obj;
}

void WB(instcode obj) {
    switch (obj.type) {
        case LB:
        case LH:
        case LW:
        case LBU:
        case LHU:
        case ADD:
        case ADDI:
        case SUB:
        case LUI:
        case AUIPC:
        case XOR:
        case XORI:
        case OR:
        case ORI:
        case AND:
        case ANDI:
        case SLL:
        case SLLI:
        case SRL:
        case SRLI:
        case SRA:
        case SRAI:
        case SLT:
        case SLTI:
        case SLTU:
        case SLTIU:
        case JAL:
        case JALR:
            if (obj.rd != 0) regi[obj.rd] = obj.value;
            break;
        default:
            break;
    }
}
#endif //RISC_V_SIMULATOR_RISCV_HPP
