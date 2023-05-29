#pragma once
#include <string>
#include <cassert>
#include <iostream>
#include "koopa.h"

class KoopaVisitor {
public:
    KoopaVisitor(std::string *target) { riscv = target; }

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
    std::string *riscv;

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
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                assert(false);
            }
        }
    }

    // 访问函数
    void Visit(const koopa_raw_function_t &func) {
        // 执行一些其他的必要操作
        *riscv += ".global " + std::string(func->name+1) + "\n";
        *riscv += std::string(func->name+1) + ":\n";
        // 访问所有基本块
        Visit(func->bbs);
    }

    // 访问基本块
    void Visit(const koopa_raw_basic_block_t &bb) {
        // 执行一些其他的必要操作
        // ...
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
            Visit(kind.data.ret);
            break;
            case KOOPA_RVT_INTEGER:
            // 访问 integer 指令
            Visit(kind.data.integer);
            break;
            default:
            // 其他类型暂时遇不到
            assert(false);
        }
    }

    //访问 return 指令
    void Visit(const koopa_raw_return_t &ret) {
        koopa_raw_value_t ret_value = ret.value;
        assert(ret_value->kind.tag == KOOPA_RVT_INTEGER);
        int32_t int_val = ret_value->kind.data.integer.value;
        *riscv += "li a0, " + std::to_string(int_val) + "\n";
        *riscv += "ret\n";
    }

    //访问 integer 指令
    void Visit(const koopa_raw_integer_t &integer) {

    }
};

void toRISCV(const std::string str, std::string *result) {
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
    std::cout << *result;

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
}