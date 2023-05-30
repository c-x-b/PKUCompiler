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
    unique_ptr<BaseAST> add_exp;

    void Dump() const override {
        add_exp->Dump();
    }

    void GenKoopa(string & str) const override {
        add_exp->GenKoopa(str);
    }
};

class PrimaryExpAST : public BaseAST {
public:
    int tag;
    struct {
        unique_ptr<BaseAST> exp;
    } data0;
    struct {
        int number;
    } data1;

    void Dump() const override {
        switch(tag) {
        case 0:
            cout << "( ";
            data0.exp->Dump();
            cout << " )";
            break;
        case 1:
            cout << data1.number;
            break;
        }

    }

    void GenKoopa(string & str) const override {
        switch(tag) {
        case 0:
            data0.exp->GenKoopa(str);
            break;
        case 1:
            str += "%" + to_string(id++) + " = add 0, " + to_string(data1.number) + "\n";
            break;
        }
    }
};

class UnaryExpAST : public BaseAST {
public:
    int tag;
    struct {
        unique_ptr<BaseAST> primary_exp;      
    } data0;
    struct {
        unique_ptr<BaseAST> unary_op;
        unique_ptr<BaseAST> unary_exp;
    } data1;

    void Dump() const override {
        switch(tag) {
        case 0:
            data0.primary_exp->Dump();
            break;
        case 1:
            data1.unary_op->Dump();
            data1.unary_exp->Dump();
            break;
        }
    }

    void GenKoopa(string & str) const override {
        switch(tag) {
        case 0:
            data0.primary_exp->GenKoopa(str);
            break;
        case 1:
            data1.unary_exp->GenKoopa(str);
            data1.unary_op->GenKoopa(str);
            break;
        }
    }
};

class UnaryOpAST : public BaseAST {
public:
    enum UNARYOP
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

class MulExpAST : public BaseAST {
public:
    int tag;
    struct DATA0{
        unique_ptr<BaseAST> unary_exp;
    } data0;
    struct DATA1{
        unique_ptr<BaseAST> mul_exp;
        enum MULOP
        {
            OP_MUL,
            OP_DIV,
            OP_MOD
        } op;
        unique_ptr<BaseAST> unary_exp;
    } data1;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        switch(tag) {
        case 0:
            data0.unary_exp->GenKoopa(str);
            break;
        case 1:
            data1.unary_exp->GenKoopa(str);
            string r2 = "%" + to_string(id - 1);
            data1.mul_exp->GenKoopa(str);
            string binaryOP;
            switch(data1.op) {
            case DATA1::OP_MUL:
                binaryOP = "mul";
                break;
            case DATA1::OP_DIV:
                binaryOP = "div";
                break;
            case DATA1::OP_MOD:
                binaryOP = "mod";
                break;
            }
            str += "%" + to_string(id) + " = " + binaryOP + " %" + to_string(id - 1) + ", " + r2 + "\n";
            id++;
            break;
        }
    }
};

class AddExpAST : public BaseAST {
public:
    int tag;
    struct DATA0{
        unique_ptr<BaseAST> mul_exp;
    } data0;
    struct DATA1{
        unique_ptr<BaseAST> add_exp;
        enum ADDOP
        {
            OP_ADD,
            OP_MINUS
        } op;
        unique_ptr<BaseAST> mul_exp;
    } data1;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        switch(tag) {
        case 0:
            data0.mul_exp->GenKoopa(str);
            break;
        case 1:
            data1.mul_exp->GenKoopa(str);
            string r2 = "%" + to_string(id - 1);
            data1.add_exp->GenKoopa(str);
            string binaryOP;
            switch(data1.op) {
            case DATA1::OP_ADD:
                binaryOP = "add";
                break;
            case DATA1::OP_MINUS:
                binaryOP = "sub";
                break;
            }
            str += "%" + to_string(id) + " = " + binaryOP + " %" + to_string(id - 1) + ", " + r2 + "\n";
            id++;
            break;
        }
    }  
};