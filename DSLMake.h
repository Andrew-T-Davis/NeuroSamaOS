#pragma once

#include "int.h"

typedef enum 
{
    AST_KEYWORD,
    AST_BLOCK,
    AST_IF,
    AST_FOR,
    AST_WHILE,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_BINOP,
    AST_UNARY,
    AST_FUNC_CALL,
    AST_RETURN,
    AST_NUMBER,
    AST_STRING,
    AST_IDENT,
    AST_CUSTOM,
    AST_JIT_BLOCK,
    AST_EMIT_CALL
} ASTNodeType;

typedef struct CaptureNode CaptureNode;
struct CaptureNode 
{
    char* Start;
    char* End;
    char* Name;
    CaptureNode* Next;
} __attribute__((packed));

typedef struct SyntaxRule SyntaxRule;
typedef struct ASTNode ASTNode;

typedef struct Keyword Keyword;
struct Keyword
{
    char* Name;
    u16 Type;
    void (*ToJITDo)(Keyword Keyword, ASTNode* ASTNode);
} __attribute__((packed));

struct ASTNode
{
    ASTNodeType type;
    struct ASTNode *children;
    struct ASTNode *next;
    struct ASTNode *parent;
    union
    {
        struct
        {
            char *keyword_name;
            void *keyword_ptr;
        };
        struct
        {
            struct ASTNode *cond;
            struct ASTNode *then_branch;
            struct ASTNode *else_branch;
        } if_node;
        struct
        {
            struct ASTNode *init;
            struct ASTNode *cond;
            struct ASTNode *inc;
            struct ASTNode *body;
        } for_node;
        struct
        {
            struct ASTNode *left;
            struct ASTNode *right;
            char op;
        } binop_node;
        struct
        {
            char *ident_name;
        };
        struct
        {
            u64 num_value;
        };
        struct
        {
            char *string_value;
        };
        struct
        {
            char *func_name;
            struct ASTNode *args;
        } func_call_node;
        struct
        {
            char *var_name;
            struct ASTNode *init_value;
        } var_decl_node;
        struct
        {
            struct ASTNode *target;
            struct ASTNode *value;
        } assign_node;
        struct
        {
            struct ASTNode *operand;
            char op;
        } unary_node;
        struct
        {
            struct ASTNode *expr;
        } return_node;
        struct
        {
            SyntaxRule *rule;
            CaptureNode *captures;
            void *custom_data;
        } custom_node;
        struct
        {
            ASTNode *jit_code;
            u64 emit_count;
        } jit_block_node;
    };
};

struct SyntaxRule
{
    char* Name;
    char* Pattern;
    CaptureNode* Captures;
    ASTNode* Body;
    void (*EmitCustom)(void* node, void* dsl, void* p);
} __attribute__((packed));

typedef struct
{
    u64 KeywordCount;
    Keyword** Keywords;
    u64 RuleCount;
    SyntaxRule** Rules;
    char PointerSymbol;
    char AddressSymbol;
    char DerefSymbol;
} __attribute__((packed)) FishDSL;

typedef struct
{
    char* code;
    char* pos;
    FishDSL* dsl;
    void* pool;
    void* root;
    void* current;
} ParserContext;

void AddKeyword(Keyword* KeyWord, FishDSL* DSL);
void AddSyntaxRule(SyntaxRule* Rule, FishDSL* DSL);
void AddChild(ASTNode* parent, ASTNode* child);

void RemoveSyntaxRule(char* Name, FishDSL* DSL);
void ClearRules(FishDSL* DSL);

ASTNode* CreateASTNode(ASTNodeType type, void* pool);
ASTNode* CodeToAST(char* code, FishDSL* dsl, void* pool);

void GrammarToJIT(ASTNode* node, FishDSL* dsl, u8* *p_ptr);
void SkipWhitespace(ParserContext* ctx);

char* ReadIdent(ParserContext* ctx);
u64 ReadNumber(ParserContext* ctx);

int IsIdentChar(char c);
int IsDigit(char c);
int IsAlpha(char c);
int IsWhitespace(char c);

ASTNode* ParseExpression(ParserContext* ctx);
ASTNode* ParseStatement(ParserContext* ctx);
ASTNode* ParseBlock(ParserContext* ctx);
ASTNode* ParseCustomStatement(ParserContext* ctx);
int MatchPattern(ParserContext* ctx, char* pattern, CaptureNode** out_captures);
char* CaptureToString(CaptureNode* captures, int index);
void FreeCaptures(CaptureNode* captures);