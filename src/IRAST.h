#pragma once
#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <iostream>

using namespace std;

static int id = 0;
static int tableId = 0;
static int blockId = 0;
static bool hasRet = 0;
class BaseAST;

struct CalcResult {
    bool err;
    int result;
};

struct Symbol {
    int tag; // 0 const, 1 var
    union {
        int const_val;
        int var_id; // id, prevent duplicate names
    } data;

    Symbol(int _tag, int val) {
        if (_tag==0) {
            tag = 0;
            data.const_val = val;
        }
        else if (_tag==1) {
            tag = 1;
            data.var_id = val;
        }
    }
    Symbol() {
        tag = -1;
    }
};

class SymbolTable {
public:
    map<string, Symbol> table;
    int id;

    SymbolTable() { table.clear(); }
    bool check(const string &ident) {
        return (table.find(ident) != table.end());
    }
    void insert(const string &ident, Symbol sym) {
        //cout << "insert " << ident << endl;
        assert(!check(ident));
        if (sym.tag==1) {
            sym.data.var_id = id;
        }
        table[ident] = sym;
    }
    Symbol find(const string &ident) {
        assert(check(ident));
        return table[ident];
    }
};
struct SymbolTableNode {
    SymbolTable table;
    SymbolTableNode *parent;

    SymbolTable* findTable(const string &ident) {
        SymbolTableNode *ptr = this;
        while (ptr != nullptr) {
            if (ptr->table.check(ident))
                return &(ptr->table);
            ptr = ptr->parent;
        }
        return nullptr;
    }
};
static SymbolTableNode *current_node = nullptr;
//static SymbolTable symbol_table;


// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;

    virtual void GenKoopa(string & str) const = 0;

    virtual CalcResult Calc() {
        return CalcResult{1, 0};
    }
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
        cout << "CompUnitAST { \n";
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
        cout << "FuncDefAST { \n";
        func_type->Dump();
        cout << ", " << ident << ", ";
        block->Dump();
        cout << " }";
    }

    void GenKoopa(string & str) const override {
        str += "fun @" + ident + "(): ";
        func_type->GenKoopa(str);
        str += " {\n";
        str += "\%entry:\n";
        block->GenKoopa(str);
        str += "}\n";
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


class DeclAST : public BaseAST {
public:
    int tag;
    struct {
        unique_ptr<BaseAST> const_decl;
    } data0;
    struct {
        unique_ptr<BaseAST> var_decl;
    } data1;

    void Dump() const override {
        switch (tag) {
        case 0:
            cout << "const decl { \n";
            data0.const_decl->Dump();
            cout << "}";
            break;
        case 1:
            cout << "var decl { \n";
            cout << "}";
            break;
        }
    }

    void GenKoopa(string &str) const override {
        
        switch(tag) {
        case 0:
            data0.const_decl->GenKoopa(str);
            break;
        case 1:
            data1.var_decl->GenKoopa(str);
            break;
        }
    }
};

class ConstDeclAST : public BaseAST {
public:
    unique_ptr<BaseAST> b_type;
    unique_ptr<vector<unique_ptr<BaseAST>>> const_defs;

    void Dump() const override {
        cout << "ConstDeclAST { ";
        for (auto it = const_defs->begin(); it != const_defs->end();it++)
            (*it)->Dump();
        cout << " }";
    }

    void GenKoopa(string &str) const override {
        for (auto it = const_defs->begin(); it != const_defs->end();it++)
            (*it)->GenKoopa(str);
    }
};

class BTypeAST : public BaseAST {
public:
    string type;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {

    }
};

class ConstDefAST : public BaseAST {
public:
    string ident;
    unique_ptr<BaseAST> const_init_val;

    void Dump() const override {
        cout << "ConstDef { " << ident << " }; ";
    }

    void GenKoopa(string &str) const override {
        CalcResult result = const_init_val->Calc();
        if (!result.err) {
            Symbol sym(0, result.result);
            current_node->table.insert(ident, sym);
        }
        else 
            cout << "ConstInitVal Err!\n";
    }

    CalcResult Calc() override {
        return const_init_val->Calc();
    }
};

class ConstInitValAST : public BaseAST {
public:
    unique_ptr<BaseAST> const_exp;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {

    }

    CalcResult Calc() override {
        return const_exp->Calc();
    }
};

class VarDeclAST : public BaseAST {
public:
    unique_ptr<BaseAST> b_type;
    unique_ptr<vector<unique_ptr<BaseAST>>> var_defs;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        for (auto it = var_defs->begin(); it != var_defs->end();it++)
            (*it)->GenKoopa(str);
    }
};

class VarDefAST : public BaseAST {
public:
    int tag;
    struct {
        string ident;
    } data0;
    struct {
        string ident;
        unique_ptr<BaseAST> init_val;
    } data1;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        Symbol sym(1, 0);
        switch(tag) {
        case 0:
            current_node->table.insert(data0.ident, sym);
            str += "@" + data0.ident + "_" + to_string(current_node->table.id) + " = alloc i32\n";
            break;
        case 1:
            current_node->table.insert(data1.ident, sym);
            str += "@" + data1.ident + "_" + to_string(current_node->table.id) + " = alloc i32\n";
            data1.init_val->GenKoopa(str);
            str += "store %" + to_string(id - 1) + ", @" + data1.ident + "_" + to_string(current_node->table.id) + "\n";
            break;
        }
    }
};

class InitValAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        exp->GenKoopa(str);
    }
};

class BlockAST : public BaseAST {
public:
    unique_ptr<vector<unique_ptr<BaseAST>>> block_items;

    void Dump() const override {
        cout << "BlockAST { \n";
        for (auto it = block_items->begin(); it != block_items->end();it++)
            (*it)->Dump();
        cout << " }";
    }

    void GenKoopa(string & str) const override {
        //cout << "Block, Table " << tableId << endl;
        SymbolTableNode *node = new SymbolTableNode;
        node->parent = current_node;
        node->table.id = tableId++;
        current_node = node;
        for (auto it = block_items->begin(); it != block_items->end();it++)
            (*it)->GenKoopa(str);
        current_node = node->parent;
        //cout << "Block End, Table " << node->id << endl;
        delete node;
    }   
};

class BlockItemAST : public BaseAST {
public:
    int tag;
    struct {
        unique_ptr<BaseAST> decl;
    } data0;
    struct {
        unique_ptr<BaseAST> stmt;
    } data1;

    void Dump() const override {
        cout << "BlockItem ";
        if (tag==0) {
            //cout << "decl {\n";
            data0.decl->Dump();
            cout << " }";
        }
        else {
            //cout << "stmt {\n";
            data1.stmt->Dump();
            cout << " }";
        }
        cout << "; \n";
    }

    void GenKoopa(string &str) const override {
        if (hasRet) {
            return;
        }
        switch(tag) {
        case 0:
            data0.decl->GenKoopa(str);
            break;
        case 1:
            data1.stmt->GenKoopa(str);
            break;
        }
    }
};

class StmtAST : public BaseAST {
public:
    int tag;
    struct {
        unique_ptr<BaseAST> exp_or_none;
    } data0;
    struct {
        string l_val;
        unique_ptr<BaseAST> exp;
    } data1;
    struct {
        unique_ptr<BaseAST> exp_or_none;
    } data2;
    struct {
        unique_ptr<BaseAST> block;
    } data3;

    void Dump() const override {
        switch (tag) {
        case 0:
            cout << "StmtAST { ";
            data0.exp_or_none->Dump();
            cout << " }";
            break;
        case 1:
            
            break;
        case 2:

            break;
        case 3:

            break;
        }
    }

    void GenKoopa(string & str) const override {
        SymbolTable *table;
        switch (tag) {
        case 0:
            //cout << "Stmt, ret;" << endl;
            data0.exp_or_none->GenKoopa(str);
            str += "ret %" + to_string(id - 1) + "\n";
            hasRet = 1;
            break;
        case 1:
            //cout << "Stmt, lval = exp;" << endl;
            data1.exp->GenKoopa(str);
            table = current_node->findTable(data1.l_val);
            str += "store %" + to_string(id - 1) + ", @" + data1.l_val + "_" + to_string(table->id) + "\n";
            break;
        case 2:
            //cout << "Stmt, exp;" << endl;
            data2.exp_or_none->GenKoopa(str);
            break;
        case 3:
            //cout << "Stmt, block" << endl;
            data3.block->GenKoopa(str);
            break;
        }
    }
};

class ExpOrNoneAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    void Dump() const override {
        if (exp) {
            exp->Dump();
        }
    }

    void GenKoopa(string &str) const override {
        if (exp) {
            exp->GenKoopa(str);
        }
    }
};


class ExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> l_or_exp;

    void Dump() const override
    {
        l_or_exp->Dump();
    }

    void GenKoopa(string & str) const override
    {
        l_or_exp->GenKoopa(str);
    }

    CalcResult Calc() override
    {
        return l_or_exp->Calc();
    }
};

class LValAST : public BaseAST {
public:
    string ident;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        SymbolTable *table = current_node->findTable(ident);
        Symbol sym = table->find(ident);
        switch(sym.tag) {
        case 0:
            str += "%" + to_string(id++) + " = add 0, " + to_string(sym.data.const_val) + "\n";
            break;
        case 1:
            str += "%" + to_string(id++) + " = load @" + ident + "_" + to_string(table->id) + "\n";
            break;
        }
    }

    CalcResult Calc() override {
        SymbolTable *table = current_node->findTable(ident);
        if (!table || !table->check(ident))
            return CalcResult{1, 0};
        else {
            Symbol sym = table->find(ident);
            return CalcResult{0, sym.data.const_val};
        }
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
    struct {
        unique_ptr<BaseAST> l_val;
    } data2;

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
        case 2:
            data2.l_val->GenKoopa(str);
            break;
        }
    }

    CalcResult Calc() override {
        switch (tag) {
        case 0:
            return data0.exp->Calc();
        case 1:
            return CalcResult{0, data1.number};
        case 2:
            return data2.l_val->Calc();
        default:
            return CalcResult{1, 0};
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

    CalcResult Calc() override {
        switch(tag) {
        case 0:
            return data0.primary_exp->Calc();
        case 1:
            CalcResult t = data1.unary_exp->Calc();
            int op = data1.unary_op->Calc().result;
            int result = 0;
            if (op==0)
                result = t.result;
            else if (op==1)
                result = -t.result;
            else if (op==2)
                result = !t.result;
            return CalcResult{t.err, result};
        }
        return CalcResult{1, 0};
    }
};

class UnaryOpAST : public BaseAST {
public:
    enum UNARYOP
    {
        OP_ADD,
        OP_SUB,
        OP_NOT
    } op;

    void Dump() const override {
        switch(op) {
        case OP_ADD:
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
        case OP_ADD:
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

    CalcResult Calc() override {
        return CalcResult{1, op};
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

    CalcResult Calc() override {
        switch(tag) {
        case 0:
            return data0.unary_exp->Calc();
        case 1:
            CalcResult t1 = data1.mul_exp->Calc(), t2 = data1.unary_exp->Calc();
            int result = 0;
            if (data1.op==DATA1::OP_MUL)
                result = t1.result * t2.result;
            else if (data1.op==DATA1::OP_DIV)
                result = t1.result / t2.result;
            else if (data1.op==DATA1::OP_MOD)
                result = t1.result % t2.result;
            return CalcResult{t1.err || t2.err, result};
        }
        return CalcResult{1, 0};
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

    CalcResult Calc() override {
        switch(tag) {
        case 0:
            return data0.mul_exp->Calc();
        case 1:
            CalcResult t1 = data1.add_exp->Calc(), t2 = data1.mul_exp->Calc();
            int result = -1;
            if (data1.op==DATA1::OP_ADD)
                result = t1.result + t2.result;
            else if (data1.op==DATA1::OP_SUB)
                result = t1.result - t2.result;
            return CalcResult{t1.err || t2.err, result};
        }
        return CalcResult{1, 0};
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

    CalcResult Calc() override {
        switch(tag) {
        case 0:
            return data0.add_exp->Calc();
        case 1:
            CalcResult t1 = data1.rel_exp->Calc(), t2 = data1.add_exp->Calc();
            int result = 0;
            if (data1.comp==DATA1::COMP_LT)
                result = (t1.result < t2.result);
            else if (data1.comp==DATA1::COMP_GT)
                result = (t1.result > t2.result);
            else if (data1.comp==DATA1::COMP_LE)
                result = (t1.result <= t2.result);
            else if (data1.comp==DATA1::COMP_GE)
                result = (t1.result >= t2.result);
            return CalcResult{t1.err || t2.err, result};
        }
        return CalcResult{1, 0};
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

    CalcResult Calc() override {
        switch(tag) {
        case 0:
            return data0.rel_exp->Calc();
        case 1:
            CalcResult t1 = data1.eq_exp->Calc(), t2 = data1.rel_exp->Calc();
            int result = 0;
            if (data1.comp==DATA1::COMP_EQ)
                result = (t1.result == t2.result);
            else if (data1.comp==DATA1::COMP_NE)
                result = (t1.result != t2.result);
            return CalcResult{t1.err || t2.err, result};
        }
        return CalcResult{1, 0};
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

    CalcResult Calc() override {
        switch(tag) {
        case 0:
            return data0.eq_exp->Calc();
        case 1:
            CalcResult t1 = data1.l_and_exp->Calc(), t2 = data1.eq_exp->Calc();
            return CalcResult{t1.err || t2.err, t1.result && t2.result};
        }
        return CalcResult{1, 0};
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

    CalcResult Calc() override {
        switch(tag) {
        case 0:
            return data0.l_and_exp->Calc();
        case 1:
            CalcResult t1 = data1.l_or_exp->Calc(), t2 = data1.l_and_exp->Calc();
            return CalcResult{t1.err || t2.err, t1.result || t2.result};
        }
        return CalcResult{1, 0};
    }
};

class ConstExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {

    }

    CalcResult Calc() override {
        CalcResult tmp = exp->Calc();
        cout << tmp.err << " " << tmp.result << endl;
        return tmp;
    }
};