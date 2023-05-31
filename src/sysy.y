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
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN LE GE EQ NE LAND LOR
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block Stmt 
                Exp PrimaryExp UnaryExp UnaryOp MulExp AddExp 
                RelExp EqExp LAndExp LOrExp
%type <int_val> Number

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->type = string("int");
    $$ = ast;
  }
  ;

Block
  : '{' Stmt '}' {
    auto ast = new BlockAST();
    ast->stmt = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    //std::cout <<"Stmt\n";
    auto ast = new StmtAST();
    ast->exp = std::unique_ptr<BaseAST>($2);
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
  ;

UnaryOp
  : '+' {
    //std::cout <<"+\n";
    auto ast = new UnaryOpAST();
    ast->op = UnaryOpAST::OP_PLUS;
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

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
