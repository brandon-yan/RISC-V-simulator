#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <bitset>
#include "riscv.hpp"


int main() {
    //freopen("C:\\Users\\hanchong\\Desktop\\RISC-V\\riscv-testcases\\testcases\\bulgarian.data", "r", stdin);
    //freopen("C:\\Users\\hanchong\\Desktop\\RISC-V\\riscv-testcases\\testcases\\answer.txt", "w", stdout);
    int pos = 0, tmp;
    std::string buffer;
    while (std::getline(std::cin, buffer)) {
        //std::cout << pos << std::endl;
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
    //std::cout << "here1" << std::endl;
    while (1) {
        instcode ins;
        IF(ins);
        ID(ins);
        if(ins.inst == 0x0ff00513) break;
        EX(ins);
        MEM(ins);
        WB(ins);
    }
    std::cout << (((unsigned int)regi[10]) & 255u) << std::endl;
    return 0;
}
