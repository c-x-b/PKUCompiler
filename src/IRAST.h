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

class TemplateAST : public BaseAST {
public:


    void Dump() const override {

    }

    void GenKoopa(string &str) const override {

    }
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
    unique_ptr<BaseAST> l_or_exp;

    void Dump() const override {
        l_or_exp->Dump();
    }

    void GenKoopa(string & str) const override {
        l_or_exp->GenKoopa(str);
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
        OP_SUB,
        OP_NOT
    } op;

    void Dump() const override {
        switch(op) {
        case OP_PLUS:
            cout << "+";
            break;
        case OP_SUB:
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
        case OP_SUB:
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
            data1.mul_exp->GenKoopa(str);
            string r1 = "%" + to_string(id - 1);
            data1.unary_exp->GenKoopa(str);
            string binaryOP;
            switch(data1.op) {
            case DATA1::OP_MUL:
                binaryOP = "mul ";
                break;
            case DATA1::OP_DIV:
                binaryOP = "div ";
                break;
            case DATA1::OP_MOD:
                binaryOP = "mod ";
                break;
            }
            str += "%" + to_string(id) + " = " + binaryOP + r1 + ", %" + to_string(id - 1) + "\n";
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
            OP_SUB
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
            data1.add_exp->GenKoopa(str);
            string r1 = "%" + to_string(id - 1);
            data1.mul_exp->GenKoopa(str);
            string binaryOP;
            switch(data1.op) {
            case DATA1::OP_ADD:
                binaryOP = "add ";
                break;
            case DATA1::OP_SUB:
                binaryOP = "sub ";
                break;
            }
            str += "%" + to_string(id) + " = " + binaryOP + r1 + ", %" + to_string(id - 1) + "\n";
            id++;
            break;
        }
    }  
};

class RelExpAST : public BaseAST {
public:
    int tag;
    struct DATA0 {
        unique_ptr<BaseAST> add_exp;
    } data0;
    struct DATA1 {
        unique_ptr<BaseAST> rel_exp;
        enum COMP
        {
            COMP_LT,
            COMP_GT,
            COMP_LE,
            COMP_GE
        } comp;
        unique_ptr<BaseAST> add_exp;
    } data1;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        switch(tag) {
        case 0:
            data0.add_exp->GenKoopa(str);
            break;
        case 1:
            data1.rel_exp->GenKoopa(str);
            string r1 = "%" + to_string(id - 1);
            data1.add_exp->GenKoopa(str);
            string binaryOP;
            switch(data1.comp) {
            case DATA1::COMP_LT:
                binaryOP = "lt ";
                break;
            case DATA1::COMP_GT:
                binaryOP = "gt ";
                break;
            case DATA1::COMP_LE:
                binaryOP = "le ";
                break;
            case DATA1::COMP_GE:
                binaryOP = "ge ";
                break;
            }
            str += "%" + to_string(id) + " = " + binaryOP + r1 + ", %" + to_string(id - 1) + "\n";
            id++;
            break;
        }
    }
};

class EqExpAST : public BaseAST {
public:
    int tag;
    struct DATA0 {
        unique_ptr<BaseAST> rel_exp;
    } data0;
    struct DATA1 {
        unique_ptr<BaseAST> eq_exp;
        enum COMP
        {
            COMP_EQ,
            COMP_NE,
        } comp;
        unique_ptr<BaseAST> rel_exp;
    } data1;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        switch(tag) {
        case 0:
            data0.rel_exp->GenKoopa(str);
            break;
        case 1:
            data1.eq_exp->GenKoopa(str);
            string r1 = "%" + to_string(id - 1);
            data1.rel_exp->GenKoopa(str);
            string binaryOP;
            switch(data1.comp) {
            case DATA1::COMP_EQ:
                binaryOP = "eq ";
                break;
            case DATA1::COMP_NE:
                binaryOP = "ne ";
                break;
            }
            str += "%" + to_string(id) + " = " + binaryOP + r1 + ", %" + to_string(id - 1) + "\n";
            id++;
            break;
        }
    }
};

class LAndExpAST : public BaseAST {
public:
    int tag;
    struct DATA0 {
        unique_ptr<BaseAST> eq_exp;
    } data0;
    struct DATA1 {
        unique_ptr<BaseAST> l_and_exp;
        unique_ptr<BaseAST> eq_exp;
    } data1;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        switch(tag) {
        case 0:
            data0.eq_exp->GenKoopa(str);
            break;
        case 1:
            data1.l_and_exp->GenKoopa(str);
            string r1 = "%" + to_string(id - 1);
            data1.eq_exp->GenKoopa(str);
            string r2 = "%" + to_string(id - 1);
            /*  r2 = ne 0, r0
                r3 = ne 0, r1
                r4 = and r2, r3
            */
            str += "%" + to_string(id++) + " = ne 0, " + r1 + "\n";
            str += "%" + to_string(id++) + " = ne 0, " + r2 + "\n";
            str += "%" + to_string(id) + " = and %" + to_string(id - 2) + ", %" + to_string(id - 1) + "\n";
            id++;
            break;
        }
    }
};

class LOrExpAST : public BaseAST {
public:
    int tag;
    struct DATA0 {
        unique_ptr<BaseAST> l_and_exp;
    } data0;
    struct DATA1 {
        unique_ptr<BaseAST> l_or_exp;
        unique_ptr<BaseAST> l_and_exp;
    } data1;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        switch(tag) {
        case 0:
            data0.l_and_exp->GenKoopa(str);
            break;
        case 1:
            data1.l_or_exp->GenKoopa(str);
            string r1 = "%" + to_string(id - 1);
            data1.l_and_exp->GenKoopa(str);
            /*  r2 = or r0, r1
                r3 = ne 0, r2   */   
            str += "%" + to_string(id) + " = or " + r1 + ", %" + to_string(id - 1) + "\n";
            id++;
            str += "%" + to_string(id) + " = ne 0, %" + to_string(id - 1) + "\n";
            id++;
            break;
        }
    }
};