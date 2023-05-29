#pragma once
#include <string>
#include <memory>
#include <iostream>

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;

    virtual void GenKoopa(std::string & str) const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
  // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
    }

    void GenKoopa(std::string & str) const override {
        func_def->GenKoopa(str);
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override {
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << ", " << ident << ", ";
        block->Dump();
        std::cout << " }";
    }

    void GenKoopa(std::string & str) const override {
        str += "fun @" + ident + "(): ";
        func_type->GenKoopa(str);
        str += " ";
        block->GenKoopa(str);
    }
};

class FuncTypeAST : public BaseAST {
public:
    std::string type;

    void Dump() const override {
        std::cout << "FunTypeAst { " << type << " }";
    }

    void GenKoopa(std::string & str) const override {
        if (type == "int")
            str += "i32";
    }
};

class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " }";
    }

    void GenKoopa(std::string & str) const override {
        str += "{\n";
        str += "\%entry:\n";
        stmt->GenKoopa(str);
        str += "\n}";
    }   
};

class StmtAST : public BaseAST {
public:
    int number;

    void Dump() const override {
        std::cout << "StmtAST { " << number << " }";
    }

    void GenKoopa(std::string & str) const override {
        str += "ret " + std::to_string(number);
    }
};
