#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <bitset>
#include "riscv.hpp"


int main() {
//    freopen("C:\\Users\\hanchong\\Desktop\\RISC-V\\riscv-testcases\\testcases\\heart.data", "r", stdin);
//    freopen("C:\\Users\\hanchong\\Desktop\\RISC-V\\riscv-testcases\\testcases\\answer.txt", "w", stdout);
    int pos = 0, tmp;
    std::string buffer;
    while (std::getline(std::cin, buffer)) {
        if (buffer[0] == '@') {
            std::stringstream s1;
            s1 << std::hex << buffer.substr(1);
            s1 >> pos;
        }
        else {
            std::stringstream s2;
            s2 << std::hex << buffer;
            while (s2 >> tmp) memory[pos++] = (unsigned char)tmp;
        }
    }
//    while (1) {
//        instcode ins;
//        IF(ins);
//        ID(ins);
//        if(ins.inst == 0x0ff00513) break;
//        EX(ins);
//        MEM(ins);
//        WB(ins);
//    }
    while (MEM_WB.inst != 0x0ff00513) {
        if (datacnt == 1) {
            ifrun = true;
            datacnt = 0;
        }
        else if (EX_MEM.type != NOP) {
            if (ID_EX.type == JALR || ID_EX.type == LB || ID_EX.type == LH || ID_EX.type == LW || ID_EX.type == LBU || ID_EX.type == LHU || ID_EX.type == SB || ID_EX.type == SH || ID_EX.type == SW || ID_EX.type == ADDI || ID_EX.type == SLTI || ID_EX.type == SLTIU || ID_EX.type == XORI || ID_EX.type == ORI || ID_EX.type == ANDI || ID_EX.type == SLLI || ID_EX.type == SRLI || ID_EX.type == SRAI)
                if (ID_EX.rs1 == EX_MEM.rd) {
                    ifrun = false;
                    ++datacnt;
                }
            if (ID_EX.type == BEQ || ID_EX.type == BNE || ID_EX.type == BLT || ID_EX.type == BGE || ID_EX.type == BLTU || ID_EX.type == BGEU || ID_EX.type == ADD || ID_EX.type == SUB || ID_EX.type == SLL || ID_EX.type == SLT || ID_EX.type == SLTU || ID_EX.type == XOR || ID_EX.type == SRL || ID_EX.type == SRA || ID_EX.type == OR || ID_EX.type == AND)
                if (ID_EX.rs1 == EX_MEM.rd || ID_EX.rs2 == EX_MEM.rd) {
                    ifrun = false;
                    ++datacnt;
                }
        }
        if (!ifmem) WB(MEM_WB);
        MEM(EX_MEM);
        if (ifrun && (!ifmem)) EX(ID_EX);
        if (ifrun && (!ifmem)) ID(IF_ID);
        if (ifrun && (!ifmem)) IF();

        if (EX_MEM.ifjump || (!ifpredict)) {
            EX_MEM.ifjump = false;
            ifpredict = true;
            pc = IF_ID.inspc;
            IF_ID.clear();
            ID_EX.clear();
        }
        if (ID_EX.ifjump) {
            ID_EX.ifjump = false;
            pc = IF_ID.inspc;
            IF_ID.clear();
        }
    }
    std::cout << (((unsigned int)regi[10]) & 255u) << std::endl;
    return 0;
}
