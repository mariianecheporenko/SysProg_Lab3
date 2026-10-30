%{
#include "ast.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>

int yylex(void);
void yyerror(const char* message);
extern FILE* yyin;
std::unique_ptr<BlockStmt> syntaxTree;
%}

%code requires {
#include "ast.h"
}

%union {
    int integer;
    char* text;
    Expr* expr;
    Stmt* stmt;
    BlockStmt* block;
}

%token INT IF ELSE WHILE
%token <integer> INTEGER
%token <text> IDENTIFIER
%token LE GE EQ NE AND OR
%type <expr> expression
%type <stmt> statement
%type <block> program statements

%destructor { free($$); } <text>
%destructor { delete $$; } <expr> <stmt> <block>

%left OR
%left AND
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%right UMINUS '!'
%precedence IF_WITHOUT_ELSE
%precedence ELSE

%%
program:
    statements {
        syntaxTree.reset($1);
        /* The AST is now owned by syntaxTree; prevent Bison's destructor
           for the start symbol from deleting the same pointer. */
        $$ = nullptr;
    }
;

statements:
    %empty { $$ = new BlockStmt(); }
  | statements statement { $1->add(std::unique_ptr<Stmt>($2)); $$ = $1; }
;

statement:
    INT IDENTIFIER ';' { $$ = new DeclStmt($2, nullptr); free($2); }
  | INT IDENTIFIER '=' expression ';' { $$ = new DeclStmt($2, std::unique_ptr<Expr>($4)); free($2); }
  | IDENTIFIER '=' expression ';' { $$ = new AssignStmt($1, std::unique_ptr<Expr>($3)); free($1); }
  | '{' statements '}' { $$ = $2; }
  | IF '(' expression ')' statement %prec IF_WITHOUT_ELSE { $$ = new IfStmt(std::unique_ptr<Expr>($3), std::unique_ptr<Stmt>($5)); }
  | IF '(' expression ')' statement ELSE statement { $$ = new IfStmt(std::unique_ptr<Expr>($3), std::unique_ptr<Stmt>($5), std::unique_ptr<Stmt>($7)); }
  | WHILE '(' expression ')' statement { $$ = new WhileStmt(std::unique_ptr<Expr>($3), std::unique_ptr<Stmt>($5)); }
;

expression:
    INTEGER { $$ = new IntegerExpr($1); }
  | IDENTIFIER { $$ = new VariableExpr($1); free($1); }
  | '(' expression ')' { $$ = $2; }
  | '-' expression %prec UMINUS { $$ = new UnaryExpr("-", std::unique_ptr<Expr>($2)); }
  | '+' expression %prec UMINUS { $$ = new UnaryExpr("+", std::unique_ptr<Expr>($2)); }
  | '!' expression { $$ = new UnaryExpr("!", std::unique_ptr<Expr>($2)); }
  | expression '+' expression { $$ = new BinaryExpr("+", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression '-' expression { $$ = new BinaryExpr("-", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression '*' expression { $$ = new BinaryExpr("*", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression '/' expression { $$ = new BinaryExpr("/", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression '%' expression { $$ = new BinaryExpr("%", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression '<' expression { $$ = new BinaryExpr("<", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression '>' expression { $$ = new BinaryExpr(">", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression LE expression { $$ = new BinaryExpr("<=", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression GE expression { $$ = new BinaryExpr(">=", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression EQ expression { $$ = new BinaryExpr("==", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression NE expression { $$ = new BinaryExpr("!=", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression AND expression { $$ = new BinaryExpr("&&", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
  | expression OR expression { $$ = new BinaryExpr("||", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); }
;

%%

void yyerror(const char* message) { std::fprintf(stderr, "Syntax error: %s\n", message); }

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::fprintf(stderr, "Usage: %s <input.c>\n", argv[0]);
        return 2;
    }

    yyin = std::fopen(argv[1], "r");
    if (!yyin) {
        std::perror(argv[1]);
        return 2;
    }

    const int parseStatus = yyparse();
    std::fclose(yyin);
    if (parseStatus != 0 || !syntaxTree) return 1;

    try {
        syntaxTree->saveToJson("ast.json");
        std::cerr << "AST saved to ast.json\n";
        Environment environment;
        syntaxTree->evaluate(environment);
        std::cout << "=== Results evaluated ===\n";
        for (const auto& [variable, value] : environment)
            std::cout << variable << " = " << value << '\n';
        std::cout << "\n=== Generated Python code ===\n";
        std::cout << static_cast<const ASTNode&>(*syntaxTree).generatePython();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Error: %s\n", error.what());
        return 1;
    }
    return 0;
}
