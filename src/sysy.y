%code requires {
  #include <memory>
  #include <string>
  #include "IRAST.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "IRAST.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  string *str_val;
  int int_val;
  BaseAST *ast_val;
  vector<unique_ptr<BaseAST>> *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN LE GE EQ NE LAND LOR CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncFParam 
                Decl ConstDecl ConstDef ConstInitVal VarDecl VarDef InitVal
                Block BlockItem Stmt MatchedStmt OpenStmt 
                ExpOrNone Exp LVal PrimaryExp UnaryExp UnaryOp MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp
%type <int_val> Number
%type <vec_val> CompUnitSet ConstDefSet ArrDimSet ConstInitValSet VarDefSet InitValSet FuncFParams BlockItemSet ArrUseSet FuncRParams
%type <str_val> Type

%%

CompUnit
  : CompUnitSet {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->comp_units = unique_ptr<vector<unique_ptr<BaseAST>>>($1);
    ast = move(comp_unit);
  }
  ;
CompUnitSet
  : FuncDef {
    auto vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | Decl {
    auto vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | CompUnitSet FuncDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  | CompUnitSet Decl {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->tag = 0;
    ast->data0.const_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->tag = 1;
    ast->data1.var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;
ConstDecl
  : CONST Type ConstDefSet ';' {
    auto ast = new ConstDeclAST();
    ast->b_type = *($2);
    ast->const_defs = unique_ptr<vector<unique_ptr<BaseAST>>>($3);
    $$ = ast;
  }
  ;
ConstDefSet
  : ConstDef {
    auto vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstDefSet ',' ConstDef{
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;
ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->tag = 0;
    ast->data0.ident = *($1);
    ast->data0.const_init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT ArrDimSet '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->tag = 1;
    ast->data1.ident = *($1);
    ast->data1.const_exps = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    ast->data1.const_init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;
ArrDimSet
  : '[' ConstExp ']' {
    auto vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  | ArrDimSet '[' ConstExp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;
ConstInitVal 
  : ConstExp{
    auto ast = new ConstInitValAST();
    ast->tag = 0;
    ast->data0.const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new ConstInitValAST();
    ast->tag = 1;
    $$ = ast;
  }
  | '{' ConstInitValSet '}' {
    auto ast = new ConstInitValAST();
    ast->tag = 2;
    ast->data2.const_init_vals = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  ;
ConstInitValSet 
  : ConstInitVal {
    auto vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstInitValSet ',' ConstInitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;
VarDecl
  : Type VarDefSet ';' {
    auto ast = new VarDeclAST();
    ast->b_type = *($1);
    ast->var_defs = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  } 
  ;
VarDefSet
  : VarDef {
    auto vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | VarDefSet ',' VarDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;
VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->tag = 0;
    ast->data0.ident = *($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    //cout << "start" << endl;
    auto ast = new VarDefAST();
    ast->tag = 1;
    ast->data1.ident = *($1);
    ast->data1.init_val = unique_ptr<BaseAST>($3);
    //cout << "done1" << endl;
    $$ = ast;
  }
  | IDENT ArrDimSet {
    auto ast = new VarDefAST();
    ast->tag = 2;
    ast->data2.ident = *($1);
    ast->data2.const_exps = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  | IDENT ArrDimSet '=' InitVal {
    auto ast = new VarDefAST();
    ast->tag = 3;
    ast->data3.ident = *($1);
    ast->data3.const_exps = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    ast->data3.init_val = unique_ptr<InitValAST>(dynamic_cast<InitValAST*>($4));
    $$ = ast;
  }
  ;
InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->tag = 0;
    ast->data0.exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitValAST();
    ast->tag = 1;
    $$ = ast;
  }
  | '{' InitValSet '}' {
    auto ast = new InitValAST();
    ast->tag = 2;
    ast->data2.init_vals = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  ;
InitValSet
  : InitVal {
    auto vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | InitValSet ',' InitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

FuncDef
  : Type IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->tag = 0;
    ast->data0.func_type = *($1);
    ast->data0.ident = *($2);
    ast->data0.block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | Type IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->tag = 1;
    ast->data1.func_type = *($1);
    ast->data1.ident = *($2);
    ast->data1.func_f_params = unique_ptr<vector<unique_ptr<BaseAST>>>($4);
    ast->data1.block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;
Type
  : INT {
    string *str = new string("int");
    $$ = str;
  }
  | VOID {
    string *str = new string("void");
    $$ = str;
  }
  ;
FuncFParams
  : FuncFParam {
    auto vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncFParams ',' FuncFParam {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;
FuncFParam 
  : Type IDENT {
    auto ast = new FuncFParamAST();
    ast->tag = 0;
    ast->data0.b_type = *($1);
    ast->data0.ident = *($2);
    $$ = ast;
  }
  | Type IDENT '[' ']' {
    auto ast = new FuncFParamAST();
    ast->tag = 1;
    ast->data1.b_type = *($1);
    ast->data1.ident = *($2);
    $$ = ast;
  }
  | Type IDENT '[' ']' ArrDimSet {
    auto ast = new FuncFParamAST();
    ast->tag = 2;
    ast->data2.b_type = *($1);
    ast->data2.ident = *($2);
    ast->data2.const_exps = unique_ptr<vector<unique_ptr<BaseAST>>>($5);
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemSet '}' {
    auto ast = new BlockAST();
    ast->block_items = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  ;
BlockItemSet
  : {
    auto vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | BlockItemSet BlockItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;
BlockItem
  : Decl{
    auto ast = new BlockItemAST();
    ast->tag = 0;
    ast->data0.decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->tag = 1;
    ast->data1.stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;
Stmt
  : MatchedStmt {
    auto ast = new StmtAST();
    ast->tag = 0;
    ast->data0.matched_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | OpenStmt {
    auto ast = new StmtAST();
    ast->tag = 1;
    ast->data1.open_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;
MatchedStmt
  : RETURN ExpOrNone ';' {
    //cout <<"Stmt return\n";
    auto ast = new MatchedStmtAST();
    ast->tag = 0;
    ast->data0.exp = unique_ptr<BaseAST>($2);
    //cout << "done" << endl;
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new MatchedStmtAST(); 
    ast->tag = 1;
    ast->data1.l_val = unique_ptr<LValAST>(dynamic_cast<LValAST*>($1));
    ast->data1.exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | ExpOrNone ';' {
    auto ast = new MatchedStmtAST();
    ast->tag = 2;
    ast->data2.exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Block {
    auto ast = new MatchedStmtAST();
    ast->tag = 3;
    ast->data3.block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
    auto ast = new MatchedStmtAST();
    ast->tag = 4;
    ast->data4.exp = unique_ptr<BaseAST>($3);
    ast->data4.matched_stmt1 = unique_ptr<BaseAST>($5);
    ast->data4.matched_stmt2 = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' MatchedStmt {
    auto ast = new MatchedStmtAST();
    ast->tag = 5;
    ast->data5.exp = unique_ptr<BaseAST>($3);
    ast->data5.matched_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';'{
    auto ast = new MatchedStmtAST();
    ast->tag = 6;
    $$ = ast;
  } 
  | CONTINUE ';' {
    auto ast = new MatchedStmtAST();
    ast->tag = 7;
    $$ = ast;
  }
  ;
OpenStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new OpenStmtAST();
    ast->tag = 0;
    ast->data0.exp = unique_ptr<BaseAST>($3);
    ast->data0.stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE OpenStmt {
    auto ast = new OpenStmtAST();
    ast->tag = 1;
    ast->data1.exp = unique_ptr<BaseAST>($3);
    ast->data1.matched_stmt = unique_ptr<BaseAST>($5);
    ast->data1.open_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' OpenStmt {
    auto ast = new OpenStmtAST();
    ast->tag = 2;
    ast->data2.exp = unique_ptr<BaseAST>($3);
    ast->data2.open_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;


ExpOrNone
  : Exp {
    auto ast = $1;
    $$ = ast;
  }
  | {
    BaseAST* ast = nullptr;
    $$ = ast;
  }
  ;
Exp
  : LOrExp {
    //std::cout <<"Exp\n";
    auto ast = new ExpAST();
    ast->l_or_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;
LVal
  : IDENT {
    auto ast = new LValAST();
    ast->tag = 0;
    ast->data0.ident = *($1);
    $$ = ast;
  }
  | IDENT ArrUseSet {
    auto ast = new LValAST();
    ast->tag = 1;
    ast->data1.ident = *($1);
    ast->data1.exps = unique_ptr<vector<unique_ptr<BaseAST>>>($2);
    $$ = ast;
  }
  ;
ArrUseSet
  : '[' Exp ']' {
    auto vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  | ArrUseSet '[' Exp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;
PrimaryExp
  : '(' Exp ')' {
    //std::cout <<"PrimaryExp_(exp)\n";
    auto ast = new PrimaryExpAST();
    ast->tag = 0;
    ast->data0.exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    //std::cout <<"PrimaryExp_number\n";
    auto ast = new PrimaryExpAST();
    ast->tag = 1;
    ast->data1.number = $1;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->tag = 2;
    ast->data2.l_val = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;
Number
  : INT_CONST {
    $$ = $1;
  }
  ;
UnaryExp
  : PrimaryExp {
    //std::cout <<"UnaryExp_PrimaryExp\n";
    auto ast = new UnaryExpAST();
    ast->tag = 0;
    ast->data0.primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    //std::cout <<"UnaryExp_OpExp\n";
    auto ast = new UnaryExpAST();
    ast->tag = 1;
    ast->data1.unary_op = unique_ptr<BaseAST>($1);
    ast->data1.unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  } 
  | IDENT '(' ')' {
    auto ast = new UnaryExpAST();
    ast->tag = 2;
    ast->data2.ident = *($1);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->tag = 3;
    ast->data3.ident = *($1);
    ast->data3.exps = unique_ptr<vector<unique_ptr<BaseAST>>>($3);
    $$ = ast;
  }
  ;
FuncRParams 
  : Exp {
    auto vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncRParams ',' Exp {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;
UnaryOp
  : '+' {
    //std::cout <<"+\n";
    auto ast = new UnaryOpAST();
    ast->op = UnaryOpAST::OP_ADD;
    $$ = ast;
  }
  | '-' {
    //std::cout <<"-\n";
    auto ast = new UnaryOpAST();
    ast->op = UnaryOpAST::OP_SUB;
    $$ = ast;
  }
  | '!' {
    //std::cout <<"!\n";
    auto ast = new UnaryOpAST();
    ast->op = UnaryOpAST::OP_NOT;
    $$ = ast;
  }
  ;
MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->tag = 0;
    ast->data0.unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpAST();
    ast->tag = 1;
    ast->data1.mul_exp = unique_ptr<BaseAST>($1);
    ast->data1.op = MulExpAST::DATA1::OP_MUL;
    ast->data1.unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpAST();
    ast->tag = 1;
    ast->data1.mul_exp = unique_ptr<BaseAST>($1);
    ast->data1.op = MulExpAST::DATA1::OP_DIV;
    ast->data1.unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpAST();
    ast->tag = 1;
    ast->data1.mul_exp = unique_ptr<BaseAST>($1);
    ast->data1.op = MulExpAST::DATA1::OP_MOD;
    ast->data1.unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;
AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->tag = 0;
    ast->data0.mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpAST();
    ast->tag = 1;
    ast->data1.add_exp = unique_ptr<BaseAST>($1);
    ast->data1.op = AddExpAST::DATA1::OP_ADD;
    ast->data1.mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpAST();
    ast->tag = 1;
    ast->data1.add_exp = unique_ptr<BaseAST>($1);
    ast->data1.op = AddExpAST::DATA1::OP_SUB;
    ast->data1.mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;
RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->tag = 0;
    ast->data0.add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp '<' AddExp {
    auto ast = new RelExpAST();
    ast->tag = 1;
    ast->data1.rel_exp = unique_ptr<BaseAST>($1);
    ast->data1.comp = RelExpAST::DATA1::COMP_LT;
    ast->data1.add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp '>' AddExp {
    auto ast = new RelExpAST();
    ast->tag = 1;
    ast->data1.rel_exp = unique_ptr<BaseAST>($1);
    ast->data1.comp = RelExpAST::DATA1::COMP_GT;
    ast->data1.add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp LE AddExp {
    auto ast = new RelExpAST();
    ast->tag = 1;
    ast->data1.rel_exp = unique_ptr<BaseAST>($1);
    ast->data1.comp = RelExpAST::DATA1::COMP_LE;
    ast->data1.add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp GE AddExp {
    auto ast = new RelExpAST();
    ast->tag = 1;
    ast->data1.rel_exp = unique_ptr<BaseAST>($1);
    ast->data1.comp = RelExpAST::DATA1::COMP_GE;
    ast->data1.add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;
EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->tag = 0;
    ast->data0.rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQ RelExp {
    auto ast = new EqExpAST();
    ast->tag = 1;
    ast->data1.eq_exp = unique_ptr<BaseAST>($1);
    ast->data1.comp = EqExpAST::DATA1::COMP_EQ;
    ast->data1.rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | EqExp NE RelExp {
    auto ast = new EqExpAST();
    ast->tag = 1;
    ast->data1.eq_exp = unique_ptr<BaseAST>($1);
    ast->data1.comp = EqExpAST::DATA1::COMP_NE;
    ast->data1.rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;
LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->tag = 0;
    ast->data0.eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LAND EqExp {
    auto ast = new LAndExpAST();
    ast->tag = 1;
    ast->data1.l_and_exp = unique_ptr<BaseAST>($1);
    ast->data1.eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  };
LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->tag = 0;
    ast->data0.l_and_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  } 
  | LOrExp LOR LAndExp {
    auto ast = new LOrExpAST();
    ast->tag = 1;
    ast->data1.l_or_exp = unique_ptr<BaseAST>($1);
    ast->data1.l_and_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  } 
  ;
ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
