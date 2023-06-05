#pragma once
#include <string>
#include <cassert>
#include <iostream>
#include <map>
#include "koopa.h"

using namespace std;

class RegAllocator {
public:
    koopa_raw_value_t allocatedReg[15]; // 0-6:t0-t6, 7-14: a0-a7
    int regLife[15];
    int allocatedNum;

    RegAllocator() {
        allocatedNum = 0;
        for (int i = 0; i < 15;i++) {
            allocatedReg[i] = nullptr;
            regLife[i] = 0;
        }
    }

    void GetRegUseIndex(int index, string &reg) {
        if (index<=6) {
            reg = "t" + to_string(index);
        }
        else {
            reg = 'a' + to_string(index - 7);
        }
        regLife[index]--;
        if (regLife[index]==0) {
            allocatedReg[index] = nullptr;
        }
    }

    // find the allocated register of %ptr and set it to %reg.
    // allocate one if not exist.
    void GetRegStr(const koopa_raw_value_t &ptr, string &reg, int life) {
        int firstFree = -1;
        for (int i = 0; i < 15;i++){
            if (allocatedReg[i] == ptr) {
                GetRegUseIndex(i, reg);
                return;
            }
            else if (firstFree==-1 && allocatedReg[i]==nullptr) {
                firstFree = i;
            }
        }

        assert(firstFree >= 0 && firstFree < 15);
        regLife[firstFree] = life + 1;
        GetRegUseIndex(firstFree, reg);
        allocatedReg[firstFree] = ptr;
    }
};

class StackTable {
public:
    map<koopa_raw_value_t, int> table;
    int usedSpace;

    StackTable() {
        table.clear();
        usedSpace = 0;
    }

    bool check(const koopa_raw_value_t &ptr) {
        return (table.find(ptr) != table.end());
    }

    // find the ptr in the map and return the int
    // if not exist, create one and return
    int access(const koopa_raw_value_t &ptr) {
        if (check(ptr)) {
            return table[ptr];
        }
        else {
            table[ptr] = usedSpace;
            usedSpace += 4;
            return (usedSpace - 4);
        }
    }
};

class KoopaVisitor {
public:
    KoopaVisitor(string *target) { 
        riscv = target;
    }

    // 访问 raw program
    void Visit(const koopa_raw_program_t &program) {
        // 执行一些其他的必要操作
        *riscv += ".text\n";
        // 访问所有全局变量
        Visit(program.values);
        // 访问所有函数
        Visit(program.funcs);
    }

private:
    string *riscv;
    RegAllocator regs;
    StackTable stackTable;
    int stackSpace = 0;

    // 访问 raw slice
    void Visit(const koopa_raw_slice_t &slice) {
        for (size_t i = 0; i < slice.len; ++i) {
            auto ptr = slice.buffer[i];
            // 根据 slice 的 kind 决定将 ptr 视作何种元素
            switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                // 访问函数
                Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                // 访问基本块
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
                // 访问指令
                Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            default:
                assert(false);
            }
        }
    }

    // 访问函数
    void Visit(const koopa_raw_function_t &func) {
        // 执行一些其他的必要操作
        *riscv += ".global " + string(func->name + 1) + "\n";
        *riscv += string(func->name+1) + ":\n";

        int space = 0;
        for (size_t i = 0; i < func->bbs.len;i++) {
            auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
            for (size_t i = 0; i < bb->insts.len; ++i) {
                koopa_raw_value_t inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[i]);
                if (inst->ty->tag!=KOOPA_RTT_UNIT) {
                    space += 4;
                }
            }
        }
        space = ((space - 4) / 16 + 1) * 16;
        *riscv += "li t0, -" + to_string(space) + "\n";
        *riscv += "add sp, sp, t0\n";
        stackSpace = space;

        // 访问所有基本块
        Visit(func->bbs);
    }

    // 访问基本块
    void Visit(const koopa_raw_basic_block_t &bb) {
        // 执行一些其他的必要操作
        *riscv += string(bb->name + 1) + ":\n";
        // 访问所有指令
        Visit(bb->insts);
        
    }

    // 访问指令
    void Visit(const koopa_raw_value_t &value) {
        // 根据指令类型判断后续需要如何访问
        const auto &kind = value->kind;
        switch (kind.tag) {
        case KOOPA_RVT_RETURN:
            // 访问 return 指令
            VisitRet(kind.data.ret);
            break;
        case KOOPA_RVT_INTEGER:
            // 访问 integer 指令
            VisitInt(kind.data.integer);
            break;
        case KOOPA_RVT_BINARY:
            // 访问 binary OP 指令
            VisitBinary(value);
            break;
        case KOOPA_RVT_STORE:
            VisitStore(kind.data.store);
            break;
        case KOOPA_RVT_LOAD:
            VisitLoad(value);
            break;
        case KOOPA_RVT_ALLOC:
            stackTable.access(value);
            cout << "alloc\n";
            break;
        case KOOPA_RVT_BRANCH:
            VisitBranch(kind.data.branch);
            break;
        case KOOPA_RVT_JUMP:
            VisitJump(kind.data.jump);
            break;
        default:
        // 其他类型暂时遇不到
        assert(false);
        }
    }

    //访问 return 指令
    void VisitRet(const koopa_raw_return_t &ret) {
        koopa_raw_value_t ret_value = ret.value;
        if (ret_value->kind.tag == KOOPA_RVT_INTEGER) {
            int32_t int_val = ret_value->kind.data.integer.value;
            *riscv += "li a0, " + to_string(int_val) + "\n";
            *riscv += "li t0, " + to_string(stackSpace) + "\n";
            *riscv += "add sp, sp, t0\n";
            *riscv += "ret\n";
        }
        else if (ret_value->kind.tag == KOOPA_RVT_BINARY) {
            int loc = stackTable.access(ret_value);
            *riscv += "li t3, " + to_string(loc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "lw a0, 0(t3)\n";
            *riscv += "li t0, " + to_string(stackSpace) + "\n";
            *riscv += "add sp, sp, t0\n";
            *riscv += "ret\n";
        }
        else if (ret_value->kind.tag == KOOPA_RVT_LOAD) {
            int loc = stackTable.access(ret_value);
            *riscv += "li t3, " + to_string(loc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "lw a0, 0(t3)\n";
            *riscv += "li t0, " + to_string(stackSpace) + "\n";
            *riscv += "add sp, sp, t0\n";
            *riscv += "ret\n";
        }
    }

    //访问 integer 指令
    void VisitInt(const koopa_raw_integer_t &integer) {

    }

    // 访问 binary OP 指令
    void VisitBinary(const koopa_raw_value_t &value) {
        koopa_raw_binary_t binary = value->kind.data.binary;
        koopa_raw_value_kind_t lhs_kind = binary.lhs->kind;
        koopa_raw_value_kind_t rhs_kind = binary.rhs->kind;
        string resultReg = "t0";
        //regs.GetRegStr(value, resultReg, 1);
        string r1Reg = "t1";
        string r2Reg = "t2";

        // constant
        if (lhs_kind.tag == KOOPA_RVT_INTEGER && rhs_kind.tag == KOOPA_RVT_INTEGER) {
            int imm;
            switch (binary.op) {
            case KOOPA_RBO_ADD:
                imm = lhs_kind.data.integer.value + rhs_kind.data.integer.value;
                break;
            case KOOPA_RBO_SUB:
                imm = lhs_kind.data.integer.value - rhs_kind.data.integer.value;
                break;
            case KOOPA_RBO_MUL:
                imm = lhs_kind.data.integer.value * rhs_kind.data.integer.value;
                break;
            case KOOPA_RBO_DIV:
                imm = lhs_kind.data.integer.value / rhs_kind.data.integer.value;
                break;
            case KOOPA_RBO_MOD:
                imm = lhs_kind.data.integer.value % rhs_kind.data.integer.value;
                break;
            case KOOPA_RBO_EQ:
                imm = (lhs_kind.data.integer.value == rhs_kind.data.integer.value);
                break;
            case KOOPA_RBO_NOT_EQ:
                imm = (lhs_kind.data.integer.value != rhs_kind.data.integer.value);
                break;
            case KOOPA_RBO_LT:
                imm = (lhs_kind.data.integer.value < rhs_kind.data.integer.value);
                break;
            case KOOPA_RBO_GT:
                imm = (lhs_kind.data.integer.value > rhs_kind.data.integer.value);
                break;
            case KOOPA_RBO_LE:
                imm = (lhs_kind.data.integer.value <= rhs_kind.data.integer.value);
                break;
            case KOOPA_RBO_GE:
                imm = (lhs_kind.data.integer.value >= rhs_kind.data.integer.value);
                break;
            case KOOPA_RBO_AND:
                imm = (lhs_kind.data.integer.value & rhs_kind.data.integer.value);
                break;
            case KOOPA_RBO_OR:  
                imm = (lhs_kind.data.integer.value | rhs_kind.data.integer.value);
                break;
            }
            *riscv += "li " + resultReg + ", " + to_string(imm) + "\n";
            int loc = stackTable.access(value);
            *riscv += "li t3, " + to_string(loc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "sw " + resultReg + ", 0(t3)\n\n";
            return;
        }

        if (lhs_kind.tag == KOOPA_RVT_INTEGER) {
            int imm = lhs_kind.data.integer.value;
            //r1Reg = resultReg;
            *riscv += "li " + r1Reg + ", " + to_string(imm) + "\n";
        }
        else {
            int loc = stackTable.access(binary.lhs);
            *riscv += "li t3, " + to_string(loc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "lw " + r1Reg + ", 0(t3)\n";
            //regs.GetRegStr(binary.lhs, r1Reg, 999);
        }
        if (rhs_kind.tag == KOOPA_RVT_INTEGER) {
            int imm = rhs_kind.data.integer.value;
            //r2Reg = resultReg;
            *riscv += "li " + r2Reg + ", " + to_string(imm) + "\n";
        }
        else {
            int loc = stackTable.access(binary.rhs);
            *riscv += "li t3, " + to_string(loc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "lw " + r2Reg + ", 0(t3)\n";
            //regs.GetRegStr(binary.rhs, r2Reg, 999);
        }
        
        switch (binary.op) {
        case KOOPA_RBO_ADD:
            *riscv += "add " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            break;
        case KOOPA_RBO_SUB:
            *riscv += "sub " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            break;
        case KOOPA_RBO_MUL:
            *riscv += "mul " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            break;
        case KOOPA_RBO_DIV:
            *riscv += "div " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            break;
        case KOOPA_RBO_MOD:
            *riscv += "rem " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            break;
        case KOOPA_RBO_EQ :
            *riscv += "xor " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            *riscv += "seqz " + resultReg + ", " + resultReg + "\n";
            break;
        case KOOPA_RBO_NOT_EQ:
            *riscv += "xor " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            *riscv += "snez " + resultReg + ", " + resultReg + "\n";
            break;
        case KOOPA_RBO_LT:
            *riscv += "slt " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            break;
        case KOOPA_RBO_GT:
            *riscv += "sgt " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            break;
        case KOOPA_RBO_LE:
            *riscv += "sgt " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            *riscv += "seqz " + resultReg + ", " + resultReg + "\n";
            break;
        case KOOPA_RBO_GE:
            *riscv += "slt " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            *riscv += "seqz " + resultReg + ", " + resultReg + "\n";
            break;
        case KOOPA_RBO_AND:
            *riscv += "and " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            break;
        case KOOPA_RBO_OR:
            *riscv += "or " + resultReg + ", " + r1Reg + ", " + r2Reg + "\n";
            break;
        }

        int loc = stackTable.access(value);
        *riscv += "li t3, " + to_string(loc) + "\n";
        *riscv += "add t3, sp, t3\n";
        *riscv += "sw " + resultReg + ", 0(t3)\n\n";
    }

    void VisitStore(const koopa_raw_store_t &store) {
        if (store.value->kind.tag == KOOPA_RVT_INTEGER) {
            int val = store.value->kind.data.integer.value;
            *riscv += "li t0, " + to_string(val) + "\n";
            int loc = stackTable.access(store.dest);
            *riscv += "li t3, " + to_string(loc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "sw t0, 0(t3)\n";
        }
        else {
            int loc1 = stackTable.access(store.value);
            *riscv += "li t3, " + to_string(loc1) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "lw t0, 0(t3)\n";
            int loc2 = stackTable.access(store.dest);
            *riscv += "li t3, " + to_string(loc2) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "sw t0, 0(t3)\n\n";
        }
    }

    void VisitLoad(const koopa_raw_value_t &value) {
        koopa_raw_load_t load = value->kind.data.load;
        int loc1 = stackTable.access(load.src);
        *riscv += "li t3, " + to_string(loc1) + "\n";
        *riscv += "add t3, sp, t3\n";
        *riscv += "lw t0, 0(t3)\n";
        int loc2 = stackTable.access(value);
        *riscv += "li t3, " + to_string(loc2) + "\n";
        *riscv += "add t3, sp, t3\n";
        *riscv += "sw t0, 0(t3)\n\n";
    }

    void VisitBranch(const koopa_raw_branch_t &branch) {
        int loc = stackTable.access(branch.cond);
        *riscv += "li t3, " + to_string(loc) + "\n";
        *riscv += "add t3, sp, t3\n";
        *riscv += "lw t0, 0(t3)\n";
        *riscv += "bnez t0, " + string(branch.true_bb->name + 1) + "\n";
        *riscv += "j " + string(branch.false_bb->name + 1) + "\n";
        *riscv += "\n";
    }

    void VisitJump(const koopa_raw_jump_t &jump) {
        *riscv += "j " + string(jump.target->name + 1) + "\n";
        *riscv += "\n";
    }
};

void toRISCV(const string str, string *result) {
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(str.c_str(), &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    KoopaVisitor visitor(result);
    visitor.Visit(raw);
    //cout << *result;

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
}