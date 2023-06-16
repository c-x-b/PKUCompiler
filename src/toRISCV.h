#pragma once
#include <string>
#include <cassert>
#include <iostream>
#include <map>
#include <vector>
#include "koopa.h"

using namespace std;

class ArrayDimTable {
public:
    map<koopa_raw_value_t, vector<int>> table;

    ArrayDimTable() {
        table.clear();
    }

    bool check(const koopa_raw_value_t &ptr) {
        return (table.find(ptr) != table.end());
    }

    void insert(const koopa_raw_value_t &ptr, const vector<int> &dim) {
        assert(!check(ptr));
        table[ptr] = dim;
    }

    void get(const koopa_raw_value_t &ptr, vector<int> &dest) {
        assert(check(ptr));
        dest = table[ptr];
    }

    void clear() {
        table.clear();
    }
};

struct offsetData {
    int offset; // current offset
    int index;  // index of the array dimensions
    koopa_raw_value_t arr; // ptr of the array;
};

class ArrayOffsetTable {
public:
    map<koopa_raw_value_t, offsetData> table;

    ArrayOffsetTable() {
        table.clear();
    }

    bool check(const koopa_raw_value_t &ptr) {
        return (table.find(ptr) != table.end());
    }

    void insert(const koopa_raw_value_t &ptr, const offsetData &data) {
        assert(!check(ptr));
        table[ptr] = data;
    }

    offsetData get (const koopa_raw_value_t &ptr) {
        assert(check(ptr));
        return table[ptr];
    }

    void clear() {
        table.clear();
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
    int access(const koopa_raw_value_t &ptr, int space = 4) {
        if (check(ptr)) {
            return table[ptr];
        }
        else {
            table[ptr] = usedSpace;
            usedSpace += space;
            return (usedSpace - space);
        }
    }

    void clear() {
        table.clear();
        usedSpace = 0;
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

        // 访问所有全局变量
        Visit(program.values);
        // 访问所有函数
        Visit(program.funcs);
    }

private:
    string *riscv;
    ArrayDimTable globalArrTable;
    ArrayDimTable arrTable;
    StackTable stackTable;
    int stackSpace = 0;
    int paramStackSpace = 0;
    int raLoc = -1;
    ArrayOffsetTable offsetTable; // for array visit in getElemPtr and getPtr

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

    void AllocStack(const koopa_raw_function_t &func) {
        for (size_t i = 0; i < func->bbs.len;i++) {
            auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
            for (size_t i = 0; i < bb->insts.len; ++i) {
                koopa_raw_value_t inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[i]);
                if (inst->ty->tag==KOOPA_RTT_INT32) {
                    stackTable.access(inst);
                }
                else if (inst->ty->tag == KOOPA_RTT_POINTER) {
                    if (inst->kind.tag == KOOPA_RVT_ALLOC && inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
                        int arrSpace = 4;
                        auto base = inst->ty->data.pointer.base;
                        while (base->tag == KOOPA_RTT_ARRAY) {
                            arrSpace *= base->data.array.len;
                            base = base->data.array.base;
                        }
                        stackTable.access(inst, arrSpace);
                    }
                    else {
                        stackTable.access(inst);
                    }
                }
            }
        }
    }

    // 访问函数
    void Visit(const koopa_raw_function_t &func) {
        if (func->bbs.len==0)
            return;
        // 执行一些其他的必要操作
        *riscv += "  .text\n";
        *riscv += "  .globl " + string(func->name + 1) + "\n";
        *riscv += string(func->name+1) + ":\n";

        int varSpace = 0, raSpace = 0, maxParamNum = 0;
        for (size_t i = 0; i < func->bbs.len;i++) {
            auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
            for (size_t i = 0; i < bb->insts.len; ++i) {
                koopa_raw_value_t inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[i]);
                if (inst->ty->tag==KOOPA_RTT_INT32) {
                    varSpace += 4;
                }
                else if (inst->ty->tag == KOOPA_RTT_POINTER) {
                    if (inst->kind.tag == KOOPA_RVT_ALLOC && inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
                        int arrSpace = 4;
                        auto base = inst->ty->data.pointer.base;
                        vector<int> dim;
                        dim.clear();
                        while (base->tag == KOOPA_RTT_ARRAY) {
                            dim.push_back(base->data.array.len);
                            arrSpace *= base->data.array.len;
                            base = base->data.array.base;
                        }
                        varSpace += arrSpace;
                        arrTable.insert(inst, dim);
                    }
                    else if (inst->kind.tag == KOOPA_RVT_ALLOC && inst->ty->data.pointer.base->tag == KOOPA_RTT_POINTER) {
                        vector<int> dim;
                        dim.clear();
                        dim.push_back(1);
                        auto base = inst->ty->data.pointer.base->data.pointer.base;
                        while (base->tag == KOOPA_RTT_ARRAY) {
                            dim.push_back(base->data.array.len);
                            base = base->data.array.base;
                        }
                        varSpace += 4;
                        arrTable.insert(inst, dim);
                    }
                    else {
                        varSpace += 4;
                    }
                }
                if (inst->kind.tag == KOOPA_RVT_CALL) {
                    raSpace = 4;
                    maxParamNum = (inst->kind.data.call.args.len > maxParamNum) ? inst->kind.data.call.args.len : maxParamNum;
                }
            }
        }
        int paramSpace = (maxParamNum - 8) * 4;
        paramStackSpace = (paramSpace < 0) ? 0 : paramSpace;
        cout << "paramStackSpace " << paramStackSpace << endl;
        stackTable.usedSpace = paramStackSpace;
        int space = varSpace + raSpace + paramStackSpace;
        space = ((space - 4) / 16 + 1) * 16;
        *riscv += "li t0, -" + to_string(space) + "\n";
        *riscv += "add sp, sp, t0\n";
        stackSpace = space;
        if (raSpace==4) {
            raLoc = space - 4;
            *riscv += "li t3, " + to_string(raLoc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "sw ra, 0(t3)\n";
        }
        AllocStack(func);

        cout << "alloc done\n";

        // 访问所有基本块
        Visit(func->bbs);

        raLoc = -1;
        stackTable.clear();
        offsetTable.clear();
        arrTable.clear();
    }

    // 访问基本块
    void Visit(const koopa_raw_basic_block_t &bb) {
        // 执行一些其他的必要操作
        string name = string(bb->name + 1);
        if (name != "entry")
            *riscv += name + ":\n";
        else
            *riscv += "\n";
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
            break;
        case KOOPA_RVT_BRANCH:
            VisitBranch(kind.data.branch);
            break;
        case KOOPA_RVT_JUMP:
            VisitJump(kind.data.jump);
            break;
        case KOOPA_RVT_CALL:
            VisitCall(value);
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            VisitGlobalAlloc(value);
            break;
        case KOOPA_RVT_GET_ELEM_PTR:
            VisitGetElemPtr(value);
            break;
        case KOOPA_RVT_GET_PTR:
            VisitGetPtr(value);
            break;
        default:
        // 其他类型暂时遇不到
        assert(false);
        }
    }

    //访问 return 指令
    void VisitRet(const koopa_raw_return_t &ret) {
        koopa_raw_value_t ret_value = ret.value;
        if (ret_value != nullptr) {
            if (ret_value->kind.tag == KOOPA_RVT_INTEGER) {
                int32_t int_val = ret_value->kind.data.integer.value;
                *riscv += "li a0, " + to_string(int_val) + "\n";
            }
            else if (ret_value->kind.tag == KOOPA_RVT_BINARY) {
                int loc = stackTable.access(ret_value);
                if (loc>=2048) {
                    *riscv += "li t3, " + to_string(loc) + "\n";
                    *riscv += "add t3, sp, t3\n";
                    *riscv += "lw a0, 0(t3)\n";
                }
                else {
                    *riscv += "lw a0, " + to_string(loc) + "(sp)\n";
                }
            }
            else if (ret_value->kind.tag == KOOPA_RVT_LOAD) {
                int loc = stackTable.access(ret_value);
                if (loc>=2048) {
                    *riscv += "li t3, " + to_string(loc) + "\n";
                    *riscv += "add t3, sp, t3\n";
                    *riscv += "lw a0, 0(t3)\n";
                }
                else {
                    *riscv += "lw a0, " + to_string(loc) + "(sp)\n";
                }
            }
            else if (ret_value->kind.tag == KOOPA_RVT_CALL) {
                int loc = stackTable.access(ret_value);
                if (loc>=2048) {
                    *riscv += "li t3, " + to_string(loc) + "\n";
                    *riscv += "add t3, sp, t3\n";
                    *riscv += "lw a0, 0(t3)\n";
                }
                else {
                    *riscv += "lw a0, " + to_string(loc) + "(sp)\n";
                }
            }
        }
        if (raLoc > 0) {
            *riscv += "li t3, " + to_string(raLoc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "lw ra, 0(t3)\n";
        }
        *riscv += "li t0, " + to_string(stackSpace) + "\n";
        *riscv += "add sp, sp, t0\n";
        *riscv += "ret\n\n";
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
            if (loc >= 2048) {
                *riscv += "li t3, " + to_string(loc) + "\n";
                *riscv += "add t3, sp, t3\n";
                *riscv += "sw " + resultReg + ", 0(t3)\n\n";
            }
            else {
                *riscv += "sw " + resultReg + ", " + to_string(loc) + "(sp)\n\n";
            }
            return;
        }

        if (lhs_kind.tag == KOOPA_RVT_INTEGER) {
            int imm = lhs_kind.data.integer.value;
            //r1Reg = resultReg;
            *riscv += "li " + r1Reg + ", " + to_string(imm) + "\n";
        }
        else {
            int loc = stackTable.access(binary.lhs);
            if (loc >=2048) {
                *riscv += "li t3, " + to_string(loc) + "\n";
                *riscv += "add t3, sp, t3\n";
                *riscv += "lw " + r1Reg + ", 0(t3)\n";
            }
            else {
                *riscv += "lw " + r1Reg + ", " + to_string(loc) + "(sp)\n";
            }
        }
        if (rhs_kind.tag == KOOPA_RVT_INTEGER) {
            int imm = rhs_kind.data.integer.value;
            //r2Reg = resultReg;
            *riscv += "li " + r2Reg + ", " + to_string(imm) + "\n";
        }
        else {
            int loc = stackTable.access(binary.rhs);
            if (loc >=2048) {
                *riscv += "li t3, " + to_string(loc) + "\n";
                *riscv += "add t3, sp, t3\n";
                *riscv += "lw " + r2Reg + ", 0(t3)\n";
            }
            else {
                *riscv += "lw " + r2Reg + ", " + to_string(loc) + "(sp)\n";
            }
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
        if (loc >= 2048) {
            *riscv += "li t3, " + to_string(loc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "sw " + resultReg + ", 0(t3)\n\n";
        }
        else {
            *riscv += "sw " + resultReg + ", " + to_string(loc) + "(sp)\n\n";
        }
    }

    void VisitStore(const koopa_raw_store_t &store) {
        // may have bugs.
        if (store.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) { // func start, store params
            int pindex = store.value->kind.data.func_arg_ref.index; // start from 0
            int loc = stackTable.access(store.dest);
            if (pindex < 8) {
                string reg = "a" + to_string(pindex);
                if (loc >= 2048) {
                    *riscv += "li t3, " + to_string(loc) + "\n";
                    *riscv += "add t3, sp, t3\n";
                    *riscv += "sw " + reg + ", 0(t3)\n\n";
                }
                else {
                    *riscv += "sw " + reg + ", " + to_string(loc) + "(sp)\n\n";
                }
            }
            else {
                int loc2 = stackSpace + 4 * (pindex - 8);
                if (loc2 >= 2048) {
                    *riscv += "li t3, " + to_string(loc2) + "\n";
                    *riscv += "add t3, sp, t3\n";
                    *riscv += "lw t0, 0(t3)\n\n";
                }
                else {
                    *riscv += "lw t0, " + to_string(loc2) + "(sp)\n\n";
                }
                if (loc >= 2048) {
                    *riscv += "li t3, " + to_string(loc) + "\n";
                    *riscv += "add t3, sp, t3\n";
                    *riscv += "sw t0, 0(t3)\n\n";
                }
                else {
                    *riscv += "sw t0, " + to_string(loc) + "(sp)\n\n";
                }
            } 
        }
        else {
            if (store.value->kind.tag == KOOPA_RVT_INTEGER) {
                int val = store.value->kind.data.integer.value;
                *riscv += "li t0, " + to_string(val) + "\n";
            }
            else {
                int loc = stackTable.access(store.value);
                if (loc >= 2048) {
                    *riscv += "li t3, " + to_string(loc) + "\n";
                    *riscv += "add t3, sp, t3\n";
                    *riscv += "lw t0, 0(t3)\n";
                }
                else {
                    *riscv += "lw t0, " + to_string(loc) + "(sp)\n";
                }
            }

            if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
                *riscv += "la t3, " + string(store.dest->name + 1) + "\n";
                *riscv += "sw t0, 0(t3)\n\n";
            }
            else if (store.dest->kind.tag == KOOPA_RVT_ALLOC) {
                int loc = stackTable.access(store.dest);
                if (loc >= 2048) {
                    *riscv += "li t3, " + to_string(loc) + "\n";
                    *riscv += "add t3, sp, t3\n";
                    *riscv += "sw t0, 0(t3)\n\n";
                }
                else {
                    *riscv += "sw t0, " + to_string(loc) + "(sp)\n\n";
                }
            }
            else {
                int loc2 = stackTable.access(store.dest);
                if (loc2 >= 2048) {
                    *riscv += "li t3, " + to_string(loc2) + "\n";
                    *riscv += "add t3, sp, t3\n";
                    *riscv += "lw t3, 0(t3)\n";
                    *riscv += "sw t0, 0(t3)\n\n";
                }
                else {
                    *riscv += "lw t3, " + to_string(loc2) + "(sp)\n";
                    *riscv += "sw t0, 0(t3)\n\n";
                }
            }
        }
    }

    void VisitLoad(const koopa_raw_value_t &value) {
        koopa_raw_load_t load = value->kind.data.load;
        if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            string name = load.src->name + 1;
            *riscv += "la t0, " + name + "\n";
            *riscv += "lw t0, 0(t0)\n";
        }
        else if (load.src->kind.tag == KOOPA_RVT_ALLOC) {
            int loc1 = stackTable.access(load.src);
            if (loc1 >= 2048) {
                *riscv += "li t3, " + to_string(loc1) + "\n";
                *riscv += "add t3, sp, t3\n";
                *riscv += "lw t0, 0(t3)\n";
            }
            else {
                *riscv += "lw t0, " + to_string(loc1) + "(sp)\n";
            }
        }
        else {
            int loc1 = stackTable.access(load.src);
            if (loc1 >= 2048) {
                *riscv += "li t3, " + to_string(loc1) + "\n";
                *riscv += "add t3, sp, t3\n";
                *riscv += "lw t0, 0(t3)\n";
                *riscv += "lw t0, 0(t0)\n";
            }
            else {
                *riscv += "lw t0, " + to_string(loc1) + "(sp)\n";
                *riscv += "lw t0, 0(t0)\n";
            }
        }
        int loc2 = stackTable.access(value);
        if (loc2 >= 2048) {
            *riscv += "li t3, " + to_string(loc2) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "sw t0, 0(t3)\n\n";
        }
        else {
            *riscv += "sw t0, " + to_string(loc2) + "(sp)\n\n";
        }
    }

    void VisitBranch(const koopa_raw_branch_t &branch) {
        int loc = stackTable.access(branch.cond);
        if (loc >= 2048) {
            *riscv += "li t3, " + to_string(loc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "lw t0, 0(t3)\n";
        }
        else {
            *riscv += "lw t0, " + to_string(loc) + "(sp)\n";
        }
        *riscv += "bnez t0, " + string(branch.true_bb->name + 1) + "\n";
        *riscv += "j " + string(branch.false_bb->name + 1) + "\n";
        *riscv += "\n";
    }

    void VisitJump(const koopa_raw_jump_t &jump) {
        *riscv += "j " + string(jump.target->name + 1) + "\n";
        *riscv += "\n";
    }

    void VisitCall(const koopa_raw_value_t &value) {
        koopa_raw_call_t call = value->kind.data.call;
        bool hasType = (value->ty->tag != KOOPA_RTT_UNIT);
        cout << "call " << call.callee->name << ", arg num: " << call.args.len << endl;

        for (size_t i = 0; i < call.args.len;i++) {
            koopa_raw_value_t arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
            
            if (i < 8) {
                string reg = "a" + to_string(i);
                if (arg->kind.tag == KOOPA_RVT_INTEGER) {
                    *riscv += "li " + reg + ", " + to_string(arg->kind.data.integer.value) + "\n";
                }
                else {
                    int loc = stackTable.access(arg);
                    if (loc >= 2048) {
                        *riscv += "li t3, " + to_string(loc) + "\n";
                        *riscv += "add t3, sp, t3\n";
                        *riscv += "lw " + reg + ", 0(t3)\n";
                    }
                    else {
                        *riscv += "lw " + reg + ", " + to_string(loc) + "(sp)\n";
                    }
                }
            }
            else {
                string argLoc = to_string((i - 8) * 4);
                if (arg->kind.tag == KOOPA_RVT_INTEGER) {
                    *riscv += "li t0, " + to_string(arg->kind.data.integer.value) + "\n";
                    *riscv += "sw t0, " + argLoc +"(sp)\n";
                }
                else {
                    int loc = stackTable.access(arg);
                    if (loc >= 2048) {
                        *riscv += "li t3, " + to_string(loc) + "\n";
                        *riscv += "add t3, sp, t3\n";
                        *riscv += "lw t0, 0(t3)\n";
                    }
                    else {
                        *riscv += "lw t0, " + to_string(loc) + "(sp)\n";
                    }
                    *riscv += "sw t0, " + argLoc +"(sp)\n";
                }
            }
        }

        *riscv += "call " + string(call.callee->name + 1) + "\n";
        if (hasType) {
            int loc = stackTable.access(value);
            if (loc >= 2048) {
                *riscv += "li t3, " + to_string(loc) + "\n";
                *riscv += "add t3, sp, t3\n";
                *riscv += "sw a0, 0(t3)\n\n";
            }
            else {
                *riscv += "sw a0, " + to_string(loc) + "(sp)\n\n";
            }
        }
    }

    void GlobalAllocArrayDFS(const koopa_raw_slice_t &slices) {
        for (size_t i = 0; i < slices.len; i++) {
            koopa_raw_value_t inst = reinterpret_cast<koopa_raw_value_t>(slices.buffer[i]);
            if (inst->kind.tag == KOOPA_RVT_INTEGER) {
                *riscv += "  .word " + to_string(inst->kind.data.integer.value) + "\n";
            }
            else if (inst->kind.tag == KOOPA_RVT_AGGREGATE) {
                auto new_slices = inst->kind.data.aggregate.elems;
                GlobalAllocArrayDFS(new_slices);
            }
            else {
                assert(false);
            }
        }
    }

    void VisitGlobalAlloc(const koopa_raw_value_t &value) {
        koopa_raw_global_alloc_t global = value->kind.data.global_alloc;
        *riscv += "  .data\n";
        *riscv += "  .globl " + string(value->name + 1) + "\n";
        *riscv += string(value->name + 1) + ":\n";

        int space = 4;
        if (value->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
            vector<int> dim;
            auto base = value->ty->data.pointer.base;
            while (base->tag == KOOPA_RTT_ARRAY) {
                dim.push_back(base->data.array.len);
                space *= base->data.array.len;
                base = base->data.array.base;
            }
            globalArrTable.insert(value, dim);
        }

        if (global.init->kind.tag == KOOPA_RVT_ZERO_INIT) {
            *riscv += "  .zero " + to_string(space) + "\n\n";
        }
        else if (global.init->kind.tag == KOOPA_RVT_INTEGER) {
            *riscv += "  .word " + to_string(global.init->kind.data.integer.value) + "\n\n";
        }
        else if (global.init->kind.tag == KOOPA_RVT_AGGREGATE) {
            auto slices = global.init->kind.data.aggregate.elems;
            GlobalAllocArrayDFS(slices);
            *riscv += "\n";
        }
    }

    void VisitGetElemPtr(const koopa_raw_value_t &value) {
        koopa_raw_get_elem_ptr_t getElemPtr = value->kind.data.get_elem_ptr;
        int arrOffset;
        if (getElemPtr.src->kind.tag == KOOPA_RVT_ALLOC) {
            auto arrPtr = getElemPtr.src;
            vector<int> dim;
            arrTable.get(arrPtr, dim);
            arrOffset = 4;
            for (auto it = dim.begin() + 1; it != dim.end();it++) {
                arrOffset *= *it;
            }
            offsetTable.insert(value, offsetData{arrOffset, 1, arrPtr});
        }
        else if (getElemPtr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            auto arrPtr = getElemPtr.src;
            vector<int> dim;
            globalArrTable.get(arrPtr, dim);
            arrOffset = 4;
            for (auto it = dim.begin() + 1; it != dim.end();it++) {
                arrOffset *= *it;
            }
            offsetTable.insert(value, offsetData{arrOffset, 1, arrPtr});
        }
        else {
            offsetData data = offsetTable.get(getElemPtr.src);
            vector<int> dim;
            if (data.arr->kind.tag == KOOPA_RVT_ALLOC)
                arrTable.get(data.arr, dim);
            else if (data.arr->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
                globalArrTable.get(data.arr, dim);
            arrOffset = data.offset / dim[data.index];
            data.offset = arrOffset;
            data.index++;
            offsetTable.insert(value, data);
        }

        if (getElemPtr.src->kind.tag == KOOPA_RVT_ALLOC) {
            int loc = stackTable.access(getElemPtr.src);
            if (loc >= 2048) {
                *riscv += "li t0, " + to_string(loc) + "\n";
                *riscv += "add t0, sp, t0\n";
            }
            else {
                *riscv += "addi t0, sp, " + to_string(loc) + "\n";
            }
        }
        else if (getElemPtr.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
            string name = getElemPtr.src->name + 1;
            *riscv += "la t0, " + name + "\n";
        }
        else {
            int loc = stackTable.access(getElemPtr.src);
            if (loc >= 2048) {
                *riscv += "li t3, " + to_string(loc) + "\n";
                *riscv += "add t3, sp, t3\n";
                *riscv += "lw t0, 0(t3)\n";
            }
            else {
                *riscv += "lw t0, " + to_string(loc) + "(sp)\n";
            }
        }

        *riscv += "li t1, " + to_string(arrOffset) + "\n";

        if (getElemPtr.index->kind.tag == KOOPA_RVT_INTEGER) {
            int imm = getElemPtr.index->kind.data.integer.value;
            *riscv += "li t2, " + to_string(imm) + "\n";
        }
        else {
            int loc = stackTable.access(getElemPtr.index);
            if (loc >=2048) {
                *riscv += "li t3, " + to_string(loc) + "\n";
                *riscv += "add t3, sp, t3\n";
                *riscv += "lw t2, 0(t3)\n";
            }
            else {
                *riscv += "lw t2, " + to_string(loc) + "(sp)\n";
            }
        }

        *riscv += "mul t1, t1, t2\n";
        *riscv += "add t0, t0, t1\n";

        int loc = stackTable.access(value);
        if (loc >= 2048) {
            *riscv += "li t3, " + to_string(loc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "sw t0, 0(t3)\n\n";
        }
        else {
            *riscv += "sw t0, " + to_string(loc) + "(sp)\n\n";
        }
    }

    void VisitGetPtr(const koopa_raw_value_t &value) {
        koopa_raw_get_ptr_t getPtr = value->kind.data.get_ptr;

        int arrOffset;
        auto arrPtr = getPtr.src->kind.data.load.src;
        vector<int> dim;
        arrTable.get(arrPtr, dim);
        arrOffset = 4;
        for (auto it = dim.begin() + 1; it != dim.end();it++) {
            arrOffset *= *it;
        }
        offsetTable.insert(value, offsetData{arrOffset, 1, arrPtr});

        int loc = stackTable.access(getPtr.src);
        if (loc >= 2048) {
            *riscv += "li t3, " + to_string(loc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "lw t0, 0(t3)\n";
        }
        else {
            *riscv += "lw t0, " + to_string(loc) + "(sp)\n";
        }

        *riscv += "li t1, " + to_string(arrOffset) + "\n";

        if (getPtr.index->kind.tag == KOOPA_RVT_INTEGER) {
            int imm = getPtr.index->kind.data.integer.value;
            *riscv += "li t2, " + to_string(imm) + "\n";
        }
        else {
            int loc = stackTable.access(getPtr.index);
            if (loc >=2048) {
                *riscv += "li t3, " + to_string(loc) + "\n";
                *riscv += "add t3, sp, t3\n";
                *riscv += "lw t2, 0(t3)\n";
            }
            else {
                *riscv += "lw t2, " + to_string(loc) + "(sp)\n";
            }
        }

        *riscv += "mul t1, t1, t2\n";
        *riscv += "add t0, t0, t1\n";

        loc = stackTable.access(value);
        if (loc >= 2048) {
            *riscv += "li t3, " + to_string(loc) + "\n";
            *riscv += "add t3, sp, t3\n";
            *riscv += "sw t0, 0(t3)\n\n";
        }
        else {
            *riscv += "sw t0, " + to_string(loc) + "(sp)\n\n";
        }
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