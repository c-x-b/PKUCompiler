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
static bool withinIntFunc = 0;
static bool withinIf = 0;
static string *curWhileEntry = nullptr;
static string *curWhileEnd = nullptr;
static vector<int> arrayDimensions;
class BaseAST;

struct CalcResult {
    bool err;
    bool array;
    int result;
    vector<int> *ptr;

    CalcResult(bool _err) { 
        err = _err;
        array = 0;
        result = 0;
        ptr = nullptr;
    }
    CalcResult(bool _err, int _result) {
        err = _err;
        array = 0;
        result = _result;
        ptr = nullptr;
    }
    CalcResult(bool _err, bool _array, int _result) {
        err = _err;
        array = _array;
        result = _result;
        ptr = new vector<int>;
    }
};

struct Symbol {
    int tag; // 0 const, 1 var, 2 func, 3 array
    union {
        int const_val;
        int var_id; // id, prevent duplicate names
        int func_has_ret; // 1 if func has ret
        int array_occupy; // no meaning
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
        else if (_tag==2) {
            tag = 2;
            data.func_has_ret = val;
        }
        else if (_tag==3) {
            tag = 3;
            data.array_occupy = val;
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
    SymbolTable* findRootTable() {
        SymbolTableNode *ptr = this;
        while (ptr->parent != nullptr) {
            ptr = ptr->parent;
        }
        return &(ptr->table);
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
        return CalcResult(true);
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
    unique_ptr<vector<unique_ptr<BaseAST>>> comp_units;

    void GenLibFuncKoopa(string & str) const {
        Symbol hasRet(2, 1);
        Symbol withoutRet(2, 0);

        str += "decl @getint(): i32\n";
        current_node->table.insert("getint", hasRet);
        str += "decl @getch(): i32\n";
        current_node->table.insert("getch", hasRet);
        str += "decl @getarray(*i32): i32\n";
        current_node->table.insert("getarray", hasRet);
        str += "decl @putint(i32)\n";
        current_node->table.insert("putint", withoutRet);
        str += "decl @putch(i32)\n";
        current_node->table.insert("putch", withoutRet);
        str += "decl @putarray(i32, *i32)\n";
        current_node->table.insert("putarray", withoutRet);
        str += "decl @starttime()\n";
        current_node->table.insert("starttime", withoutRet);
        str += "decl @stoptime()\n";
        current_node->table.insert("stoptime", withoutRet);
        str += "\n";
    }

    void Dump() const override {
        cout << "CompUnitAST { \n";
        for (auto it = comp_units->begin(); it != comp_units->end();it++) {
            (*it)->Dump();
        }
        cout << " }";
    }

    void GenKoopa(string & str) const override {
        SymbolTableNode *node = new SymbolTableNode;
        node->parent = current_node;
        node->table.id = tableId++;
        current_node = node;
        GenLibFuncKoopa(str);
        for (auto it = comp_units->begin(); it != comp_units->end();it++) {
            (*it)->GenKoopa(str);
        }
        current_node = node->parent;
        delete node;
    }
};


class FuncDefAST : public BaseAST {
public:
    int tag;
    struct {
        string func_type;
        string ident;
        unique_ptr<BaseAST> block;
    } data0;
    struct {
        string func_type;
        string ident;
        unique_ptr<vector<unique_ptr<BaseAST>>> func_f_params;
        unique_ptr<BaseAST> block;
    } data1;

    void GenParamsAllocKoopa(string &str) const {
        SymbolTableNode *ParamNode = current_node;
        map<string, Symbol> *ParamTable = &(ParamNode->table.table);
        for (auto it = ParamTable->begin(); it != ParamTable->end();it++) {
            string ident = it->first;
            cout << ident << endl;
            string id = ident + "_" + to_string(ParamNode->table.id);
            str += "@" + id + " = alloc i32\n";
            str += "store @" + ident + ", @" + id + "\n";
        }
    }

    void Dump() const override {
        // cout << "FuncDefAST { \n";
        // func_type->Dump();
        // cout << ", " << ident << ", ";
        // block->Dump();
        // cout << " }";
    }

    void GenKoopa(string & str) const override {
        //cout << "tag: " << tag << endl;
        Symbol sym(2, 0);
        switch(tag) {
        case 0:
            //cout << "GenKoopa: " << data0.ident << endl;
            sym.data.func_has_ret = (data0.func_type == "int") ? 1 : 0;
            current_node->table.insert(data0.ident, sym);
            str += "fun @" + data0.ident + "()";
            if (data0.func_type=="int") {
                str += ": i32";
                withinIntFunc = 1;
            }
            str += " {\n";
            str += "\%entry:\n";
            data0.block->GenKoopa(str);
            if (!hasRet) {
                if (withinIntFunc)
                    str += "ret 0\n";
                else 
                    str += "ret\n";
            }
            hasRet = 0;
            withinIntFunc = 0;
            str += "}\n\n";
            break;
        case 1:
            //cout << "GenKoopa: " << data1.ident << endl;
            sym.data.func_has_ret = (data1.func_type == "int") ? 1 : 0;
            current_node->table.insert(data1.ident, sym);
            SymbolTableNode *node = new SymbolTableNode;
            node->parent = current_node;
            node->table.id = tableId++;
            current_node = node;
            str += "fun @" + data1.ident + "(";
            bool first = true;
            for (auto it = data1.func_f_params->begin(); it != data1.func_f_params->end();it++) {
                if (!first)
                    str += ", ";
                (*it)->GenKoopa(str);
                first = 0;
            }
            //cout << "params done\n";
            str += ")";
            if (data1.func_type=="int") {
                str += ": i32";
                withinIntFunc = 1;
            }
            str += " {\n";
            str += "\%entry:\n";
            GenParamsAllocKoopa(str);
            data1.block->GenKoopa(str);
            if (!hasRet) {
                if (withinIntFunc)
                    str += "ret 0\n";
                else 
                    str += "ret\n";
            }
            hasRet = 0;
            withinIntFunc = 0;
            str += "}\n\n";
            current_node = node->parent;
            delete node;
            break;
        }
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
            str += ": i32";
    }
};

class FuncFParamAST : public BaseAST {
public:
    string b_type;
    string ident;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        //cout << ident << endl;
        Symbol sym(1, 0);
        current_node->table.insert(ident, sym);
        str += "@" + ident;
        if (b_type == "int")
            str += ": i32";
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
        if (hasRet)
            return;
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
    string b_type;
    unique_ptr<vector<unique_ptr<BaseAST>>> const_defs;

    void Dump() const override {
        cout << "ConstDeclAST { ";
        for (auto it = const_defs->begin(); it != const_defs->end();it++)
            (*it)->Dump();
        cout << " }";
    }

    void GenKoopa(string &str) const override {
        assert(b_type == "int");
        for (auto it = const_defs->begin(); it != const_defs->end();it++)
            (*it)->GenKoopa(str);
    }
};

class ConstDefAST : public BaseAST {
public:
    int tag;
    struct {
        string ident;
        unique_ptr<BaseAST> const_init_val;
    } data0;
    struct {
        string ident;
        unique_ptr<BaseAST> const_exp;
        unique_ptr<BaseAST> const_init_val;
    } data1;

    void Dump() const override {
        //cout << "ConstDef { " << ident << " }; ";
    }

    void GenKoopa(string &str) const override {
        bool global = (current_node->parent == nullptr);
        if (tag == 0) {
            CalcResult result = data0.const_init_val->Calc();
            assert(!result.err && !result.array);
            cout << data0.ident << " " << result.err << " " << result.result << endl;
            Symbol sym(0, result.result);
            current_node->table.insert(data0.ident, sym);
        }
        else if (tag == 1) {
            Symbol sym(3, 0);
            current_node->table.insert(data1.ident, sym);
            int dimension = data1.const_exp->Calc().result;
            assert(dimension > 0);
            arrayDimensions.clear();
            arrayDimensions.push_back(dimension);
            if (!global) {
                string variable = "@" + data1.ident + "_" + to_string(current_node->table.id);
                str += variable + " = alloc [i32, " + to_string(dimension) + "]\n";
                vector<int> *ptr = data1.const_init_val->Calc().ptr;
                int index = 0;
                for (auto it = ptr->begin(); it != ptr->end();it++, index++) {
                    str += "%" + to_string(id) + " = getelemptr " + variable + ", " + to_string(index) + "\n";
                    str += "store " + to_string(*it) + ", %" + to_string(id++) + "\n";
                }
                str += "\n";
                delete ptr;
            }
            else {
                str += "global @" + data1.ident + "_" + to_string(current_node->table.id) + " = alloc [i32, " + to_string(dimension) + "], {";
                vector<int> *ptr = data1.const_init_val->Calc().ptr;
                auto it = ptr->begin();
                str += to_string(*it);
                for (it++; it != ptr->end();it++) {
                    str += ", " + to_string(*it);
                }
                str += "}\n\n";
                delete ptr;
            }
        }
    }

    CalcResult Calc() override {
        switch (tag) {
        case 0:
            return data0.const_init_val->Calc();
        case 1:
            return CalcResult(true);
        }
    }
};

class ConstInitValAST : public BaseAST {
public:
    int tag;
    struct {
        unique_ptr<BaseAST> const_exp;
    } data0;
    struct {
        unique_ptr<vector<unique_ptr<BaseAST>>> const_exps;
    } data2;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        cout << "Const Init err\n";
    }

    CalcResult Calc() override {
        int num = 1;
        for (int i = 0; i < arrayDimensions.size();i++) {
            num *= arrayDimensions[i];
        }
        if (tag == 0) {
            return data0.const_exp->Calc();
        }
        else if (tag == 1) {
            CalcResult result(false, true, 0);
            result.ptr->resize(num, 0);
            return result;
        }
        else if (tag == 2) {
            CalcResult result(false, true, 0);
            result.ptr->resize(num, 0);
            for (int i = 0; i < data2.const_exps->size();i++) {
                (*result.ptr)[i] = (*data2.const_exps)[i]->Calc().result;
            }
            return result;
        }
    }
};

class InitValAST : public BaseAST {
public:
    int tag;
    struct {
        unique_ptr<BaseAST> exp;
    } data0;
    struct {
        unique_ptr<vector<unique_ptr<BaseAST>>> exps;
    } data2;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        switch (tag) {
        case 0:
            data0.exp->GenKoopa(str);
            break;
        case 1:
            break;
        case 2:
            break;
        }
    }

    CalcResult Calc() override
    {
        int num = 1;
        for (int i = 0; i < arrayDimensions.size();i++) {
            num *= arrayDimensions[i];
        }
        cout << "init " << num << endl;
        if (tag == 0) {
            return data0.exp->Calc();
        }
        else if (tag == 1) {
            CalcResult result(false, true, 0);
            result.ptr->resize(num, 0);
            return result;
        }
        else if (tag == 2) {
            CalcResult result(false, true, 0);
            result.ptr->resize(num, 0);
            for (int i = 0; i < data2.exps->size();i++) {
                (*result.ptr)[i] = (*data2.exps)[i]->Calc().result;
            }
            return result;
        }
    }
};

class VarDeclAST : public BaseAST {
public:
    string b_type;
    unique_ptr<vector<unique_ptr<BaseAST>>> var_defs;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        assert(b_type == "int");
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
    struct {
        string ident;
        unique_ptr<BaseAST> const_exp;
    } data2;   
    struct {
        string ident;
        unique_ptr<BaseAST> const_exp;
        unique_ptr<InitValAST> init_val;
    } data3;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        Symbol varSym(1, 0);
        Symbol arrSym(3, 0);
        bool global = (current_node->parent == nullptr);
        if (tag == 0) {
            current_node->table.insert(data0.ident, varSym);
            if (!global)
                str += "@" + data0.ident + "_" + to_string(current_node->table.id) + " = alloc i32\n";
            else 
                str += "global @" + data0.ident + "_" + to_string(current_node->table.id) + " = alloc i32, zeroinit\n\n";
        }
        else if (tag == 1) {
            current_node->table.insert(data1.ident, varSym);
            if (!global) {
                str += "@" + data1.ident + "_" + to_string(current_node->table.id) + " = alloc i32\n";
                data1.init_val->GenKoopa(str);
                str += "store %" + to_string(id - 1) + ", @" + data1.ident + "_" + to_string(current_node->table.id) + "\n";
            }
            else {
                str += "global @" + data1.ident + "_" + to_string(current_node->table.id) + " = alloc i32, ";
                CalcResult result = data1.init_val->Calc();
                assert(!result.err && !result.array);
                cout << "global " << data1.ident << " " << result.err << " " << result.result << endl;
                str += to_string(result.result) + "\n\n";
            }
        }
        else if (tag == 2) {
            current_node->table.insert(data2.ident, arrSym);
            int dimension = data2.const_exp->Calc().result;
            assert(dimension > 0);
            arrayDimensions.clear();
            arrayDimensions.push_back(dimension);
            string variable = "@" + data2.ident + "_" + to_string(current_node->table.id);
            if (!global) {
                str += variable + " = alloc [i32, " + to_string(dimension) + "]\n";
            }
            else {
                str += "global " + variable + " = alloc [i32, " + to_string(dimension) + "], zeroinit\n\n";
            }
        }
        else if (tag == 3) {
            current_node->table.insert(data3.ident, arrSym);
            int dimension = data3.const_exp->Calc().result;
            assert(dimension > 0);
            arrayDimensions.clear();
            arrayDimensions.push_back(dimension);
            string variable = "@" + data3.ident + "_" + to_string(current_node->table.id);
            if (!global) {
                str += variable + " = alloc [i32, " + to_string(dimension) + "]\n";
                assert(data3.init_val->tag != 0);
                if (data3.init_val->tag == 1) {
                    for (int i = 0; i < dimension;i++) {
                        str += "%" + to_string(id) + " = getelemptr " + variable + ", " + to_string(i) + "\n";
                        str += "store 0, %" + to_string(id) + "\n";
                        id++;
                    }
                    str += "\n";
                }
                else if (data3.init_val->tag == 2) {
                    int len = data3.init_val->data2.exps->size();
                    for (int i = 0; i < dimension;i++) {
                        str += "%" + to_string(id) + " = getelemptr " + variable + ", " + to_string(i) + "\n";
                        int destId = id++;
                        if (i < len) {
                            (*data3.init_val->data2.exps)[i]->GenKoopa(str);
                            str += "store %" + to_string(id - 1) + ", %" + to_string(destId) + "\n";
                        }
                        else {
                            str += "store 0, %" + to_string(destId) + "\n";
                        }
                    }
                    str += "\n";
                }
            }
            else {
                str += "global " + variable + " = alloc [i32, " + to_string(dimension) + "], {";
                vector<int> *ptr = data3.init_val->Calc().ptr;
                auto it = ptr->begin();
                str += to_string(*it);
                for (it++; it != ptr->end();it++) {
                    str += ", " + to_string(*it);
                }
                str += "}\n\n";
                delete ptr;
            }
        }
    }
};

class LValAST : public BaseAST {
public:
    int tag;
    struct {
        string ident;
    } data0;
    struct {
        string ident;
        unique_ptr<BaseAST> exp;
    } data1;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        if (tag == 0) {
            SymbolTable *table = current_node->findTable(data0.ident);
            Symbol sym = table->find(data0.ident);
            switch(sym.tag) {
            case 0:
                str += "%" + to_string(id++) + " = add 0, " + to_string(sym.data.const_val) + "\n";
                break;
            case 1:
                str += "%" + to_string(id++) + " = load @" + data0.ident + "_" + to_string(table->id) + "\n";
                break;
            }
        }
        else if (tag == 1) {
            SymbolTable *table = current_node->findTable(data1.ident);
            Symbol sym = table->find(data1.ident);
            assert(sym.tag == 3);
            data1.exp->GenKoopa(str);
            str += "%" + to_string(id) + " = getelemptr @" + data1.ident + "_" + to_string(table->id) + ", %" + to_string(id - 1) + "\n";
            id++;
            str += "%" + to_string(id) + " = load %" + to_string(id - 1) + "\n";
            id++;
        }
    }

    CalcResult Calc() override {
        if (tag == 0) {
            if (current_node == nullptr)
                return CalcResult(true);
            SymbolTable *table = current_node->findTable(data0.ident);
            if (!table || !table->check(data0.ident))
                return CalcResult(true);
            else {
                Symbol sym = table->find(data0.ident);
                assert(sym.tag == 0);
                return CalcResult(false, sym.data.const_val);
            }
        }
        else if (tag == 1) {
            return CalcResult(true);
        }
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
        unique_ptr<BaseAST> matched_stmt;
    } data0;
    struct {
        unique_ptr<BaseAST> open_stmt;
    } data1;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        if (hasRet)
            return;
        switch(tag) {
        case 0:
            data0.matched_stmt->GenKoopa(str);
            break;
        case 1:
            data1.open_stmt->GenKoopa(str);
            break;
        }
    }
};

class MatchedStmtAST : public BaseAST {
public:
    int tag;
    struct {
        unique_ptr<BaseAST> exp;
    } data0;
    struct {
        unique_ptr<LValAST> l_val;
        unique_ptr<BaseAST> exp;
    } data1;
    struct {
        unique_ptr<BaseAST> exp;
    } data2;
    struct {
        unique_ptr<BaseAST> block;
    } data3;
    struct {
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> matched_stmt1;
        unique_ptr<BaseAST> matched_stmt2;
    } data4;
    struct {
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> matched_stmt;
    } data5;

    void Dump() const override {
        switch (tag) {
        case 0:
            cout << "StmtAST { ";
            if (data0.exp!=nullptr)
                data0.exp->Dump();
            cout << " }";
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        }
    }

    void GenKoopa(string & str) const override {
        if (hasRet)
            return;
        SymbolTable *table;
        string flagThen, flagElse, flagEnd;
        string flagEntry, flagBody;
        string *lastWhileEntry, *lastWhileEnd;
        bool bothRet;
        switch (tag) {
        case 0:
            //cout << "Stmt, ret;" << endl;
            if (data0.exp!=nullptr) {
                if (withinIntFunc) {
                    data0.exp->GenKoopa(str);
                    str += "ret %" + to_string(id - 1) + "\n";
                }
                else {
                    str += "ret\n";
                }
            }
            else if (data0.exp==nullptr) {
                if (withinIntFunc) {
                    str += "ret 0\n";
                }
                else {
                    str += "ret\n";
                }
            }
            hasRet = 1;
            break;
        case 1:
            //cout << "Stmt, lval = exp;" << endl;
            if (data1.l_val->tag == 0) {
                data1.exp->GenKoopa(str);
                string ident = data1.l_val->data0.ident;
                table = current_node->findTable(ident);
                str += "store %" + to_string(id - 1) + ", @" + ident + "_" + to_string(table->id) + "\n\n";
            }
            else if (data1.l_val->tag == 1) {
                string ident = data1.l_val->data1.ident;
                table = current_node->findTable(ident);
                data1.l_val->data1.exp->GenKoopa(str);
                str += "%" + to_string(id) + " = getelemptr @" + ident + "_" + to_string(table->id) + ", %" + to_string(id - 1) + "\n";
                id++;
                string destId = "%" + to_string(id - 1);
                data1.exp->GenKoopa(str);
                str += "store %" + to_string(id - 1) + ", " + destId + "\n\n";
            }
            break;
        case 2:
            //cout << "Stmt, exp;" << endl;
            if (data2.exp!=nullptr) {
                data2.exp->GenKoopa(str);
            }
            break;
        case 3:
            //cout << "Stmt, block" << endl;
            data3.block->GenKoopa(str);
            break;
        case 4:
            withinIf = 1;
            bothRet = 1;
            data4.exp->GenKoopa(str);
            flagThen = "%then_" + to_string(blockId++);
            flagElse = "\%else_" + to_string(blockId++);
            flagEnd = "\%end_" + to_string(blockId++);
            str += "br %" + to_string(id - 1) + ", " + flagThen + ", " + flagElse + "\n";
            str += flagThen + ":\n";
            data4.matched_stmt1->GenKoopa(str);
            if (!hasRet) {
                str += "jump " + flagEnd + "\n";
                bothRet = 0;
            }
            hasRet = 0;
            str += flagElse + ":\n";
            data4.matched_stmt2->GenKoopa(str);
            if (!hasRet) {
                str += "jump " + flagEnd + "\n";
                bothRet = 0;
            }
            hasRet = bothRet;
            if (!hasRet)
                str += flagEnd + ":\n";
            withinIf = 0;
            break;
        case 5:
            flagEntry = "\%entry_" + to_string(blockId++);
            lastWhileEntry = curWhileEntry;
            curWhileEntry = &flagEntry;
            flagBody = "%body_" + to_string(blockId++);
            flagEnd = "\%end_" + to_string(blockId++);
            lastWhileEnd = curWhileEnd;
            curWhileEnd = &flagEnd;
            str += "jump " + flagEntry + "\n";
            str += flagEntry + ":\n";
            data5.exp->GenKoopa(str);
            str += "br %" + to_string(id - 1) + ", " + flagBody + ", " + flagEnd + "\n";
            str += flagBody + ":\n";
            data5.matched_stmt->GenKoopa(str);
            if (!hasRet) {
                str += "jump " + flagEntry + "\n";
            }
            hasRet = 0;
            str += flagEnd + ":\n";
            curWhileEntry = lastWhileEntry;
            curWhileEnd = lastWhileEnd;
            break;
        case 6:
            assert(curWhileEnd);
            str += "jump " + *curWhileEnd + "\n";
            str += "%body_" + to_string(blockId++) + ":\n";
            break;
        case 7:
            assert(curWhileEntry);
            str += "jump " + *curWhileEntry + "\n";
            str += "%body_" + to_string(blockId++) + ":\n";
            break;
        }
    }
};

class OpenStmtAST : public BaseAST {
public:
    int tag;
    struct {
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> stmt;
    } data0;
    struct {
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> matched_stmt;
        unique_ptr<BaseAST> open_stmt;
    } data1;
    struct {
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> open_stmt;
    } data2;

    void Dump() const override {

    }

    void GenKoopa(string &str) const override {
        if (hasRet)
            return;
        string flagThen, flagElse, flagEnd;
        string flagEntry, flagBody;
        string *lastWhileEntry, *lastWhileEnd;
        bool bothRet;
        switch(tag) {
        case 0:
            withinIf = 1;
            data0.exp->GenKoopa(str);
            flagThen = "%then_" + to_string(blockId++);
            flagEnd = "\%end_" + to_string(blockId++);
            str += "br %" + to_string(id - 1) + ", " + flagThen + ", " + flagEnd + "\n";
            str += flagThen + ":\n";
            data0.stmt->GenKoopa(str);
            if (!hasRet) {
                str += "jump " + flagEnd + "\n";
            }
            hasRet = 0;
            str += flagEnd + ":\n";
            withinIf = 0;
            break;
        case 1:
            withinIf = 1;
            bothRet = 1;
            data1.exp->GenKoopa(str);
            flagThen = "%then_" + to_string(blockId++);
            flagElse = "\%else_" + to_string(blockId++);
            flagEnd = "\%end_" + to_string(blockId++);
            str += "br %" + to_string(id - 1) + ", " + flagThen + ", " + flagElse + "\n";
            str += flagThen + ":\n";
            data1.matched_stmt->GenKoopa(str);
            if (!hasRet) {
                str += "jump " + flagEnd + "\n";
                bothRet = 0;
            }
            hasRet = 0;
            str += flagElse + ":\n";
            data1.open_stmt->GenKoopa(str);
            if (!hasRet) {
                str += "jump " + flagEnd + "\n";
                bothRet = 0;
            }
            hasRet = bothRet;
            if (!hasRet)
                str += flagEnd + ":\n";
            withinIf = 0;
            break;
        case 2:
            flagEntry = "\%entry_" + to_string(blockId++);
            lastWhileEntry = curWhileEntry;
            curWhileEntry = &flagEntry;
            flagBody = "%body_" + to_string(blockId++);
            flagEnd = "\%end_" + to_string(blockId++);
            lastWhileEnd = curWhileEnd;
            curWhileEnd = &flagEnd;
            str += "jump " + flagEntry + "\n";
            str += flagEntry + ":\n";
            data2.exp->GenKoopa(str);
            str += "br %" + to_string(id - 1) + ", " + flagBody + ", " + flagEnd + "\n";
            str += flagBody + ":\n";
            data2.open_stmt->GenKoopa(str);
            if (!hasRet) {
                str += "jump " + flagEntry + "\n";
            }
            hasRet = 0;
            str += flagEnd + ":\n";
            curWhileEntry = lastWhileEntry;
            curWhileEnd = lastWhileEnd;
            break;
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
            return CalcResult(false, data1.number);
        case 2:
            return data2.l_val->Calc();
        default:
            return CalcResult(true);
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
    struct {
        string ident;
    } data2;
    struct {
        string ident;
        unique_ptr<vector<unique_ptr<BaseAST>>> exps;
    } data3;

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
        SymbolTable *funcTable;
        Symbol funcSym;
        switch(tag) {
        case 0:
            data0.primary_exp->GenKoopa(str);
            break;
        case 1:
            data1.unary_exp->GenKoopa(str);
            data1.unary_op->GenKoopa(str);
            break;
        case 2:
            funcTable = current_node->findRootTable();
            funcSym = funcTable->find(data2.ident);
            assert(funcSym.tag == 2);
            if (funcSym.data.func_has_ret) {
                str += "%" + to_string(id++) + " = call @" + data2.ident + "()\n";
            }
            else {
                str += "call @" + data2.ident + "()\n";
            }
            break;
        case 3:
            funcTable = current_node->findRootTable();
            funcSym = funcTable->find(data3.ident);
            bool first = 1;
            string tmp = "";
            assert(funcSym.tag == 2);
            if (funcSym.data.func_has_ret) {
                tmp += " = call @" + data3.ident + "(";
                for (auto it = data3.exps->begin(); it != data3.exps->end();it++) {
                    if (!first)
                        tmp += ", ";
                    (*it)->GenKoopa(str);
                    tmp += "%" + to_string(id - 1);
                    first = false;
                }
                tmp += ")\n";
                str += "%" + to_string(id++) + tmp;
            }
            else {
                tmp += "call @" + data3.ident + "(";
                for (auto it = data3.exps->begin(); it != data3.exps->end();it++) {
                    if (!first)
                        tmp += ", ";
                    (*it)->GenKoopa(str);
                    tmp += "%" + to_string(id - 1);
                    first = false;
                }
                tmp += ")\n";
                str += tmp;
            }
            break;
        }
    }

    CalcResult Calc() override {
        switch(tag) {
        case 0:
            return data0.primary_exp->Calc();
        case 1:
            CalcResult t = data1.unary_exp->Calc();
            assert(!t.err && !t.array);
            int op = data1.unary_op->Calc().result;
            int result = 0;
            if (op==0)
                result = t.result;
            else if (op==1)
                result = -t.result;
            else if (op==2)
                result = !t.result;
            return CalcResult(t.err, result);
        }
        return CalcResult(true);
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
        return CalcResult(true);
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
            assert(!t1.err && !t1.array);
            assert(!t2.err && !t2.array);
            int result = 0;
            if (data1.op==DATA1::OP_MUL)
                result = t1.result * t2.result;
            else if (data1.op==DATA1::OP_DIV)
                result = t1.result / t2.result;
            else if (data1.op==DATA1::OP_MOD)
                result = t1.result % t2.result;
            return CalcResult(t1.err || t2.err, result);
        }
        return CalcResult(true);
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
            assert(!t1.err && !t1.array);
            assert(!t2.err && !t2.array);
            int result = -1;
            if (data1.op==DATA1::OP_ADD)
                result = t1.result + t2.result;
            else if (data1.op==DATA1::OP_SUB)
                result = t1.result - t2.result;
            return CalcResult(t1.err || t2.err, result);
        }
        return CalcResult(true);
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
            assert(!t1.err && !t1.array);
            assert(!t2.err && !t2.array);
            int result = 0;
            if (data1.comp==DATA1::COMP_LT)
                result = (t1.result < t2.result);
            else if (data1.comp==DATA1::COMP_GT)
                result = (t1.result > t2.result);
            else if (data1.comp==DATA1::COMP_LE)
                result = (t1.result <= t2.result);
            else if (data1.comp==DATA1::COMP_GE)
                result = (t1.result >= t2.result);
            return CalcResult(t1.err || t2.err, result);
        }
        return CalcResult(true);
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
            assert(!t1.err && !t1.array);
            assert(!t2.err && !t2.array);
            int result = 0;
            if (data1.comp==DATA1::COMP_EQ)
                result = (t1.result == t2.result);
            else if (data1.comp==DATA1::COMP_NE)
                result = (t1.result != t2.result);
            return CalcResult(t1.err || t2.err, result);
        }
        return CalcResult(true);
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
            string result = "%" + to_string(id++);
            str += result + "= alloc i32\n";
            str += "store 0, " + result + "\n";
            data1.l_and_exp->GenKoopa(str);
            str += "%" + to_string(id) + " = ne " + "%" + to_string(id - 1) + ", 0\n";
            string flagThen = "%then_" + to_string(blockId++);
            string flagEnd = "\%end_" + to_string(blockId++);
            str += "br %" + to_string(id) + ", " + flagThen + ", " + flagEnd + "\n";
            id++;
            str += flagThen + ":\n";
            data1.eq_exp->GenKoopa(str);
            str += "%" + to_string(id) + " = ne 0, %" + to_string(id - 1) + "\n";
            id++;
            str += "store %" + to_string(id - 1) + ", " + result + "\n";
            str += "jump " + flagEnd + "\n";
            str += flagEnd + ":\n";
            str += "%" + to_string(id) + " = load " + result + "\n";
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
            assert(!t1.err && !t1.array);
            assert(!t2.err && !t2.array);
            return CalcResult(t1.err || t2.err, t1.result && t2.result);
        }
        return CalcResult(true);
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
            string result = "%" + to_string(id++);
            str += result + "= alloc i32\n";
            str += "store 1, " + result + "\n";
            data1.l_or_exp->GenKoopa(str);
            str += "%" + to_string(id) + " = eq " + "%" + to_string(id - 1) + ", 0\n";
            string flagThen = "%then_" + to_string(blockId++);
            string flagEnd = "\%end_" + to_string(blockId++);
            str += "br %" + to_string(id) + ", " + flagThen + ", " + flagEnd + "\n";
            id++;
            str += flagThen + ":\n";
            data1.l_and_exp->GenKoopa(str);
            str += "%" + to_string(id) + " = ne 0, %" + to_string(id - 1) + "\n";
            id++;
            str += "store %" + to_string(id - 1) + ", " + result + "\n";
            str += "jump " + flagEnd + "\n";
            str += flagEnd + ":\n";
            str += "%" + to_string(id) + " = load " + result + "\n";
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
            assert(!t1.err && !t1.array);
            assert(!t2.err && !t2.array);
            return CalcResult(t1.err || t2.err, t1.result || t2.result);
        }
        return CalcResult(true);
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
        return exp->Calc();
    }
};