#pragma once
#include <string>
#include <memory>
#include <iostream>

using namespace std;

static int id = 0;

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;

    virtual void GenKoopa(string & str) const = 0;
};


class CompUnitAST : public BaseAST {
public:
  // 用智能指针管理对象
    unique_ptr<BaseAST> func_def;

    void Dump() const override {
        cout << "CompUnitAST { ";
        func_def->Dump();
        cout << " }";
    }

    void GenKoopa(string & str) const override {
        func_def->GenKoopa(str);
    }
};


class FuncDefAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> block;

    void Dump() const override {
        cout << "FuncDefAST { ";
        func_type->Dump();
        cout << ", " << ident << ", ";
        block->Dump();
        cout << " }";
    }

    void GenKoopa(string & str) const override {
        str += "fun @" + ident + "(): ";
        func_type->GenKoopa(str);
        str += " ";
        block->GenKoopa(str);
    }
};

class FuncTypeAST : public BaseAST {
public:
    string type;

    void Dump() const override {
        cout << "FunTypeAst { " << type << " }";
    }

    void GenKoopa(string & str) const override {
        if (type == "int")
            str += "i32";
    }
};

class BlockAST : public BaseAST {
public:
    unique_ptr<BaseAST> stmt;

    void Dump() const override {
        cout << "BlockAST { ";
        stmt->Dump();
        cout << " }";
    }

    void GenKoopa(string & str) const override {
        str += "{\n";
        str += "\%entry:\n";
        stmt->GenKoopa(str);
        str += "}";
    }   
};

class StmtAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    void Dump() const override {
        cout << "StmtAST { ";
        exp->Dump();
        cout << " }";
    }

    void GenKoopa(string & str) const override {
        exp->GenKoopa(str);
        str += "ret %" + to_string(id - 1) + "\n";
    }
};

class ExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> unary_exp;

    void Dump() const override {
        unary_exp->Dump();
    }

    void GenKoopa(string & str) const override {
        unary_exp->GenKoopa(str);
    }
};

class PrimaryExpAST : public BaseAST {
public:
    int tag;
    struct {
        // 0 
        unique_ptr<BaseAST> exp;
        // 1
        int number;
    } data;

    void Dump() const override {
        switch(tag) {
        case 0:
            cout << "( ";
            data.exp->Dump();
            cout << " )";
            break;
        case 1:
            cout << data.number;
            break;
        }

    }

    void GenKoopa(string & str) const override {
        switch(tag) {
        case 0:
            data.exp->GenKoopa(str);
            break;
        case 1:
            str += "%" + to_string(id++) + " = add 0, " + to_string(data.number) + "\n";
            break;
        }
    }
};

class UnaryExpAST : public BaseAST {
public:
    int tag;
    struct {
        // 0
        unique_ptr<BaseAST> primary_exp;
        // 1
        unique_ptr<BaseAST> unary_op;
        unique_ptr<BaseAST> unary_exp;
    } data;

    void Dump() const override {
        switch(tag) {
        case 0:
            data.primary_exp->Dump();
            break;
        case 1:
            data.unary_op->Dump();
            data.unary_exp->Dump();
            break;
        }
    }

    void GenKoopa(string & str) const override {
        switch(tag) {
        case 0:
            data.primary_exp->GenKoopa(str);
            break;
        case 1:
            data.unary_exp->GenKoopa(str);
            data.unary_op->GenKoopa(str);
            break;
        }
    }
};

class UnaryOpAST : public BaseAST {
public:
    enum OP
    {
        OP_PLUS,
        OP_MINUS,
        OP_NOT
    } op;

    void Dump() const override {
        switch(op) {
        case OP_PLUS:
            cout << "+";
            break;
        case OP_MINUS:
            cout << "-";
            break;
        case OP_NOT:
            cout << "!";
            break;
        default:
            cout << "ERROR";
        }
    }

    void GenKoopa(string & str) const override {
        switch(op) {
        case OP_PLUS:
            // do nothing
            break;
        case OP_MINUS:
            str += "%" + to_string(id) + " = sub 0, %" + to_string(id - 1) + "\n";
            id++;
            break;
        case OP_NOT:
            str += "%" + to_string(id) + " = eq 0, %" + to_string(id - 1) + "\n";
            id++;
            break;
        }
    }
};