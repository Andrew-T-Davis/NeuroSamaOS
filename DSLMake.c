#include "DSLMake.h"
#include "int.h"
#include "kmalloc.h"
#include "str.h"
#include "JIT.h"
#include "debug.h"

extern AllocPool KernelPool;

void AddKeyword(Keyword* KeyWord, FishDSL* DSL) {
    Keyword** NewList = (Keyword**)AlignedAlloc(&KernelPool, sizeof(Keyword*) * (DSL->KeywordCount + 1), 64);
    for (u64 i = 0; i < DSL->KeywordCount; i++) {
        NewList[i] = DSL->Keywords[i];
    }
    if (DSL->Keywords) {
        Free(&KernelPool, DSL->Keywords);
    }
    NewList[DSL->KeywordCount] = KeyWord;
    DSL->KeywordCount++;
    DSL->Keywords = NewList;
}

void AddSyntaxRule(SyntaxRule* Rule, FishDSL* DSL) {
    SyntaxRule** NewList = (SyntaxRule**)AlignedAlloc(&KernelPool, sizeof(SyntaxRule*) * (DSL->RuleCount + 1), 64);
    for (u64 i = 0; i < DSL->RuleCount; i++) {
        NewList[i] = DSL->Rules[i];
    }
    if (DSL->Rules) {
        Free(&KernelPool, DSL->Rules);
    }
    NewList[DSL->RuleCount] = Rule;
    DSL->RuleCount++;
    DSL->Rules = NewList;
}

void RemoveSyntaxRule(char* Name, FishDSL* DSL) {
    for (u64 i = 0; i < DSL->RuleCount; i++) {
        if (StrIs(DSL->Rules[i]->Name, Name)) {
            for (u64 j = i; j < DSL->RuleCount - 1; j++) {
                DSL->Rules[j] = DSL->Rules[j+1];
            }
            DSL->RuleCount--;
            return;
        }
    }
}

void ClearRules(FishDSL* DSL) {
    if (DSL->Rules) {
        Free(&KernelPool, DSL->Rules);
        DSL->Rules = 0;
    }
    DSL->RuleCount = 0;
}

ASTNode* CreateASTNode(ASTNodeType type, void* pool) {
    ASTNode* node = (ASTNode*)Alloc((void*)pool, sizeof(ASTNode));
    node->type = type;
    node->children = 0;
    node->next = 0;
    node->parent = 0;
    return node;
}

void AddChild(ASTNode* parent, ASTNode* child) {
    if (!parent->children) {
        parent->children = child;
    } else {
        ASTNode* last = parent->children;
        while (last->next) last = last->next;
        last->next = child;
    }
    child->parent = parent;
}

void SkipWhitespace(ParserContext* ctx) {
    while (*ctx->pos == ' ' || *ctx->pos == '\t' || *ctx->pos == '\n' || *ctx->pos == '\r')
        ctx->pos++;
}

int IsDigit(char c) {
    return c >= '0' && c <= '9';
}

int IsAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int IsIdentChar(char c) {
    return IsAlpha(c) || IsDigit(c) || c == '*' || c == '^' || c == '@' || c == '_' || c == '?';
}

int IsWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

char* ReadIdent(ParserContext* ctx) {
    char* start = ctx->pos;
    while (*ctx->pos && IsIdentChar(*ctx->pos))
        ctx->pos++;
    Size64 len = (Size64)(ctx->pos - start);
    if (!len) return 0;
    char* result = (char*)Alloc(ctx->pool, len + 1);
    MemCopy(result, start, len);
    result[len] = 0;
    return result;
}

u64 ReadNumber(ParserContext* ctx) {
    u64 value = 0;
    while (*ctx->pos && IsDigit(*ctx->pos)) {
        value = value * 10 + (u64)(*ctx->pos - '0');
        ctx->pos++;
    }
    if (*ctx->pos == '_') {
        ctx->pos++;
        while (*ctx->pos && IsDigit(*ctx->pos)) {
            value = value * 10 + (u64)(*ctx->pos - '0');
            ctx->pos++;
        }
    }
    if (*ctx->pos == 'M' || *ctx->pos == 'm') {
        ctx->pos++;
        if (*ctx->pos == 'i' || *ctx->pos == 'I') {
            ctx->pos++;
            value = value << 20;
        } else {
            value = value * 1000000;
        }
    } else if (*ctx->pos == 'K' || *ctx->pos == 'k') {
        ctx->pos++;
        if (*ctx->pos == 'i' || *ctx->pos == 'I') {
            ctx->pos++;
            value = value << 10;
        } else {
            value = value * 1000;
        }
    }
    return value;
}

int MatchPattern(ParserContext* ctx, char* pattern, CaptureNode** out_captures) {
    char* p = pattern;
    char* pos = ctx->pos;
    char* save = ctx->pos;
    CaptureNode* head = 0;
    CaptureNode* tail = 0;
    int capture_count = 0;
    
    while (*p) {
        if (*p == '%') {
            p++;
            char* start = pos;
            char* name = p;
            char* name_end = p;
            while (*name_end && (IsAlpha(*name_end) || IsDigit(*name_end))) name_end++;
            p = name_end;
            
            if (StrIs("id", name)) {
                while (*pos && IsIdentChar(*pos)) pos++;
            } else if (StrIs("type", name)) {
                while (*pos && IsAlpha(*pos)) pos++;
            } else if (StrIs("expr", name)) {
                int depth = 0;
                while (*pos && !(depth == 0 && (*pos == ';' || *pos == ')' || *pos == ',' || *pos == '}'))) {
                    if (*pos == '(') depth++;
                    if (*pos == ')') depth--;
                    pos++;
                }
            } else if (StrIs("num", name)) {
                while (*pos && IsDigit(*pos)) pos++;
            } else if (StrIs("cond", name)) {
                int depth = 0;
                while (*pos && !(depth == 0 && (*pos == '=' || *pos == ')' || *pos == ';' || *pos == ','))) {
                    if (*pos == '(') depth++;
                    if (*pos == ')') depth--;
                    pos++;
                }
            } else if (StrIs("init", name)) {
                int depth = 0;
                while (*pos && !(depth == 0 && (*pos == '=' || *pos == ')' || *pos == ';'))) {
                    if (*pos == '(') depth++;
                    if (*pos == ')') depth--;
                    pos++;
                }
            } else if (StrIs("inc", name)) {
                while (*pos && *pos != ')' && *pos != ';') pos++;
            } else if (StrIs("str", name)) {
                if (*pos == '"') {
                    pos++;
                    while (*pos && *pos != '"') pos++;
                    if (*pos == '"') pos++;
                }
            } else if (StrIs("body", name)) {
                int depth = 0;
                while (*pos) {
                    if (*pos == '{') depth++;
                    if (*pos == '}') {
                        if (depth == 0) break;
                        depth--;
                    }
                    pos++;
                }
                if (*pos == '}') pos++;
            } else if (StrIs("kw", name)) {
                while (*pos && IsIdentChar(*pos)) pos++;
            } else {
                p = name_end;
                continue;
            }
            
            if (out_captures) {
                CaptureNode* cap = (CaptureNode*)Alloc(ctx->pool, sizeof(CaptureNode));
                cap->Start = start;
                cap->End = pos;
                cap->Name = name;
                cap->Next = 0;
                if (!head) {
                    head = cap;
                } else {
                    tail->Next = cap;
                }
                tail = cap;
                capture_count++;
            }
            continue;
        }
        if (*p == ' ') {
            while (*p == ' ') p++;
            while (*pos == ' ') pos++;
            continue;
        }
        if (*p != *pos) {
            ctx->pos = save;
            return 0;
        }
        p++;
        pos++;
    }
    ctx->pos = pos;
    if (out_captures) {
        *out_captures = head;
    }
    return capture_count;
}

char* CaptureToString(CaptureNode* captures, int index) {
    int i = 0;
    CaptureNode* cur = captures;
    while (cur) {
        if (i == index) {
            u64 len = (u64)(cur->End - cur->Start);
            char* result = (char*)Alloc(&KernelPool, len + 1);
            MemCopy(result, cur->Start, len);
            result[len] = 0;
            return result;
        }
        i++;
        cur = cur->Next;
    }
    return 0;
}

void FreeCaptures(CaptureNode* captures) {
    CaptureNode* cur = captures;
    while (cur) {
        CaptureNode* next = cur->Next;
        cur = next;
    }
}

ASTNode* ParseIdent(ParserContext* ctx) {
    ASTNode* node = CreateASTNode(AST_IDENT, ctx->pool);
    node->ident_name = ReadIdent(ctx);
    return node;
}

ASTNode* ParseNumber(ParserContext* ctx) {
    ASTNode* node = CreateASTNode(AST_NUMBER, ctx->pool);
    node->num_value = ReadNumber(ctx);
    return node;
}

ASTNode* ParseString(ParserContext* ctx) {
    if (*ctx->pos != '"') return 0;
    ctx->pos++;
    char* start = ctx->pos;
    while (*ctx->pos && *ctx->pos != '"')
        ctx->pos++;
    Size64 len = (u64)(ctx->pos - start);
    ASTNode* node = CreateASTNode(AST_STRING, ctx->pool);
    char* str = (char*)Alloc(ctx->pool, len + 1);
    MemCopy(str, start, len);
    str[len] = 0;
    node->string_value = str;
    if (*ctx->pos == '"') ctx->pos++;
    return node;
}

ASTNode* ParsePrimary(ParserContext* ctx) {
    SkipWhitespace(ctx);
    if (*ctx->pos == '(') {
        ctx->pos++;
        ASTNode* node = ParseExpression(ctx);
        SkipWhitespace(ctx);
        if (*ctx->pos == ')') ctx->pos++;
        return node;
    }
    if (*ctx->pos == '"')
        return ParseString(ctx);
    if (IsDigit(*ctx->pos))
        return ParseNumber(ctx);
    if (IsAlpha(*ctx->pos) || *ctx->pos == '*' || *ctx->pos == '^' || *ctx->pos == '@')
        return ParseIdent(ctx);
    return 0;
}

ASTNode* ParseUnary(ParserContext* ctx) {
    SkipWhitespace(ctx);
    char op = 0;
    FishDSL* dsl = (FishDSL*)ctx->dsl;
    char ptr_sym = dsl ? dsl->PointerSymbol : '*';
    char addr_sym = dsl ? dsl->AddressSymbol : '&';
    char deref_sym = dsl ? dsl->DerefSymbol : '*';
    
    if (*ctx->pos == '-' || *ctx->pos == '!' || *ctx->pos == '~') {
        op = *ctx->pos;
        ctx->pos++;
        ASTNode* operand = ParseUnary(ctx);
        ASTNode* node = CreateASTNode(AST_UNARY, ctx->pool);
        node->unary_node.op = op;
        node->unary_node.operand = operand;
        return node;
    }
    if (*ctx->pos == ptr_sym || *ctx->pos == deref_sym) {
        ctx->pos++;
        ASTNode* operand = ParseUnary(ctx);
        ASTNode* node = CreateASTNode(AST_UNARY, ctx->pool);
        node->unary_node.op = '*';
        node->unary_node.operand = operand;
        return node;
    }
    if (*ctx->pos == addr_sym) {
        ctx->pos++;
        ASTNode* operand = ParseUnary(ctx);
        ASTNode* node = CreateASTNode(AST_UNARY, ctx->pool);
        node->unary_node.op = '&';
        node->unary_node.operand = operand;
        return node;
    }
    return ParsePrimary(ctx);
}

ASTNode* ParseMul(ParserContext* ctx) {
    ASTNode* left = ParseUnary(ctx);
    SkipWhitespace(ctx);
    while (*ctx->pos == '*' || *ctx->pos == '/' || *ctx->pos == '%') {
        char op = *ctx->pos;
        ctx->pos++;
        ASTNode* right = ParseUnary(ctx);
        ASTNode* node = CreateASTNode(AST_BINOP, ctx->pool);
        node->binop_node.left = left;
        node->binop_node.right = right;
        node->binop_node.op = op;
        left = node;
        SkipWhitespace(ctx);
    }
    if (left && left->type == AST_NUMBER && *ctx->pos && IsAlpha(*ctx->pos)) {
        ASTNode* right = ParseUnary(ctx);
        if (right) {
            ASTNode* node = CreateASTNode(AST_BINOP, ctx->pool);
            node->binop_node.left = left;
            node->binop_node.right = right;
            node->binop_node.op = '*';
            left = node;
        }
    }
    return left;
}

ASTNode* ParseAdd(ParserContext* ctx) {
    ASTNode* left = ParseMul(ctx);
    SkipWhitespace(ctx);
    while (*ctx->pos == '+' || *ctx->pos == '-') {
        char op = *ctx->pos;
        ctx->pos++;
        ASTNode* right = ParseMul(ctx);
        ASTNode* node = CreateASTNode(AST_BINOP, ctx->pool);
        node->binop_node.left = left;
        node->binop_node.right = right;
        node->binop_node.op = op;
        left = node;
        SkipWhitespace(ctx);
    }
    return left;
}

ASTNode* ParseCompare(ParserContext* ctx) {
    ASTNode* left = ParseAdd(ctx);
    SkipWhitespace(ctx);
    if (*ctx->pos == '<' || *ctx->pos == '>' || *ctx->pos == '=') {
        char op = *ctx->pos;
        ctx->pos++;
        if (*ctx->pos == '=') {
            ctx->pos++;
            op = (op == '=') ? 'E' : (op == '<') ? 'L' : 'G';
        }
        ASTNode* right = ParseAdd(ctx);
        ASTNode* node = CreateASTNode(AST_BINOP, ctx->pool);
        node->binop_node.left = left;
        node->binop_node.right = right;
        node->binop_node.op = op;
        left = node;
        SkipWhitespace(ctx);
    }
    return left;
}

ASTNode* ParseExpression(ParserContext* ctx) {
    return ParseCompare(ctx);
}

ASTNode* ParseBlock(ParserContext* ctx) {
    SkipWhitespace(ctx);
    if (*ctx->pos != '{') return 0;
    ctx->pos++;
    ASTNode* node = CreateASTNode(AST_BLOCK, ctx->pool);
    while (*ctx->pos && *ctx->pos != '}') {
        SkipWhitespace(ctx);
        if (*ctx->pos == '}') break;
        ASTNode* stmt = ParseStatement(ctx);
        if (stmt) AddChild(node, stmt);
        SkipWhitespace(ctx);
        if (*ctx->pos == ';') ctx->pos++;
    }
    if (*ctx->pos == '}') ctx->pos++;
    return node;
}

ASTNode* ParseCustomStatement(ParserContext* ctx) {
    FishDSL* dsl = (FishDSL*)ctx->dsl;
    if (!dsl || !dsl->Rules) return 0;
    
    for (u64 i = 0; i < dsl->RuleCount; i++) {
        SyntaxRule* rule = dsl->Rules[i];
        char* save = ctx->pos;
        CaptureNode* captures = 0;
        
        int matched = MatchPattern(ctx, rule->Pattern, &captures);
        if (matched > 0) {
            ASTNode* node = CreateASTNode(AST_CUSTOM, ctx->pool);
            node->custom_node.rule = rule;
            node->custom_node.captures = captures;
            if (rule->EmitCustom) {
                rule->EmitCustom((void*)node, (void*)dsl, 0);
            }
            return node;
        }
        ctx->pos = save;
    }
    return 0;
}

ASTNode* ParseKeyword(ParserContext* ctx) {
    FishDSL* dsl = (FishDSL*)ctx->dsl;
    if (!dsl) return 0;
    char* save = ctx->pos;
    char* name = ReadIdent(ctx);
    if (!name) {
        ctx->pos = save;
        return 0;
    }
    for (u64 i = 0; i < dsl->KeywordCount; i++) {
        if (StrIs(name, dsl->Keywords[i]->Name)) {
            ASTNode* node = CreateASTNode(AST_KEYWORD, ctx->pool);
            node->keyword_name = name;
            node->keyword_ptr = dsl->Keywords[i];
            return node;
        }
    }
    ctx->pos = save;
    return 0;
}

ASTNode* ParseVarDecl(ParserContext* ctx) {
    char* save = ctx->pos;
    char* type_name = ReadIdent(ctx);
    if (!type_name) return 0;
    
    SkipWhitespace(ctx);
    char* var_name = ReadIdent(ctx);
    if (!var_name) {
        ctx->pos = save;
        return 0;
    }
    
    ASTNode* node = CreateASTNode(AST_VAR_DECL, ctx->pool);
    node->var_decl_node.var_name = var_name;
    SkipWhitespace(ctx);
    if (*ctx->pos == '=') {
        ctx->pos++;
        node->var_decl_node.init_value = ParseExpression(ctx);
    }
    return node;
}

ASTNode* ParseAssign(ParserContext* ctx) {
    ASTNode* target = ParseIdent(ctx);
    if (!target) return 0;
    SkipWhitespace(ctx);
    if (*ctx->pos != '=') return target;
    ctx->pos++;
    ASTNode* value = ParseExpression(ctx);
    ASTNode* node = CreateASTNode(AST_ASSIGN, ctx->pool);
    node->assign_node.target = target;
    node->assign_node.value = value;
    return node;
}

ASTNode* ParseFuncCall(ParserContext* ctx) {
    char* name = ReadIdent(ctx);
    if (!name) return 0;
    SkipWhitespace(ctx);
    if (*ctx->pos != '(') {
        ctx->pos -= StrLen(name);
        return 0;
    }
    ctx->pos++;
    ASTNode* node = CreateASTNode(AST_FUNC_CALL, ctx->pool);
    node->func_call_node.func_name = name;
    while (*ctx->pos && *ctx->pos != ')') {
        ASTNode* arg = ParseExpression(ctx);
        if (arg) AddChild(node, arg);
        SkipWhitespace(ctx);
        if (*ctx->pos == ',') ctx->pos++;
    }
    if (*ctx->pos == ')') ctx->pos++;
    return node;
}

ASTNode* ParseStatement(ParserContext* ctx) {
    SkipWhitespace(ctx);
    if (!*ctx->pos) return 0;
    if (*ctx->pos == '{') return ParseBlock(ctx);
    
    char* save = ctx->pos;
    
    ASTNode* custom = ParseCustomStatement(ctx);
    if (custom) {
        SkipWhitespace(ctx);
        if (*ctx->pos == ';') {
            ctx->pos++;
            return custom;
        } else {
            DebugStr("Parse Error: Expected ';' after custom statement\n");
            ctx->pos = save;
            return 0;
        }
    }
    
    ASTNode* kw = ParseKeyword(ctx);
    if (kw) {
        SkipWhitespace(ctx);
        if (*ctx->pos == ';') {
            ctx->pos++;
            return kw;
        } else {
            DebugStr("Parse Error: Expected ';' after keyword\n");
            ctx->pos = save;
            return 0;
        }
    }

    ctx->pos = save;
    ASTNode* assign = ParseAssign(ctx);
    if (assign) {
        SkipWhitespace(ctx);
        if (*ctx->pos == ';') {
            ctx->pos++;
            return assign;
        } else {
            DebugStr("Parse Error: Expected ';' after assignment\n");
            ctx->pos = save;
            return 0;
        }
    }
    
    ctx->pos = save;
    ASTNode* expr = ParseExpression(ctx);
    if (expr) {
        SkipWhitespace(ctx);
        if (*ctx->pos == ';') {
            ctx->pos++;
            return expr;
        } else {
            DebugStr("Parse Error: Expected ';' after expression\n");
            ctx->pos = save;
            return 0;
        }
    }
    
    ctx->pos = save;
    return 0;
}

ASTNode* CodeToAST(char* code, FishDSL* dsl, void* pool) {
    ParserContext ctx;
    ctx.code = code;
    ctx.pos = code;
    ctx.dsl = dsl;
    ctx.pool = pool;
    
    ASTNode* root = CreateASTNode(AST_BLOCK, pool);
    u8 has_error = 0;
    
    while (*ctx.pos) {
        SkipWhitespace(&ctx);
        if (!*ctx.pos) break;
        
        char* stmt_start = ctx.pos;
        ASTNode* stmt = ParseStatement(&ctx);
        
        if (!stmt) {
            if (!has_error) {
                DebugStr("Parse Error: ");
                for (int i = 0; i < 20 && stmt_start[i] && stmt_start[i] != '\n'; i++) {
                    DebugChar(stmt_start[i]);
                }
                DebugStr(" ... (missing ';'?)\n");
                has_error = 1;
            }
            while (*ctx.pos && *ctx.pos != '\n') ctx.pos++;
            if (*ctx.pos == '\n') ctx.pos++;
            continue;
        }
        
        AddChild(root, stmt);
        SkipWhitespace(&ctx);
        if (*ctx.pos == ';') ctx.pos++;
    }
    
    return has_error ? 0 : root;
}

static void EmitBinOp(char op, u8** p) {
    if (op=='+') {EmitBinaryOp(p, OP_ADD, REG_RBX, REG_RAX);}
    else if (op=='-') {EmitBinaryOp(p, OP_SUB, REG_RBX, REG_RAX);}
    else if (op=='*') {EmitUnaryMuldv(p, 4, REG_RBX);}
    else if (op=='/') {EmitUnaryMuldv(p, 6, REG_RBX);}
    else if (op=='%') {EmitUnaryMuldv(p, 6, REG_RBX); EmitMovRegReg(p, REG_RAX, REG_RDX);}
    else if (op=='<') {EmitBinaryOp(p, OP_CMP, REG_RBX, REG_RAX); EmitSetcc(p, COND_L, REG_RAX);}
    else if (op=='>') {EmitBinaryOp(p, OP_CMP, REG_RBX, REG_RAX); EmitSetcc(p, COND_G, REG_RAX);}
    else if (op=='E') {EmitBinaryOp(p, OP_CMP, REG_RBX, REG_RAX); EmitSetcc(p, COND_E, REG_RAX);}
    else if (op=='L') {EmitBinaryOp(p, OP_CMP, REG_RBX, REG_RAX); EmitSetcc(p, COND_LE, REG_RAX);}
    else if (op=='G') {EmitBinaryOp(p, OP_CMP, REG_RBX, REG_RAX); EmitSetcc(p, COND_GE, REG_RAX);}
}

void GrammarToJIT(ASTNode* node, FishDSL* dsl, u8** p_ptr) {
    if (!node || !p_ptr || !*p_ptr) return;
    
    u8* p = *p_ptr;

    if (node->type==AST_BLOCK) {
        ASTNode* child = node->children;
        while (child) {
            GrammarToJIT(child, dsl, &p);
            child = child->next;
        }
    }
    if (node->type==AST_KEYWORD) {
        if (node->keyword_ptr) {
            ((Keyword*)node->keyword_ptr)->ToJITDo(*((Keyword*)node->keyword_ptr), node);
        }
    }
    if (node->type==AST_CUSTOM) {
        if (node->custom_node.rule && node->custom_node.rule->EmitCustom) {
            node->custom_node.rule->EmitCustom((void*)node, (void*)dsl, (void*)&p);
        }
    }
    if (node->type==AST_IF) {
        GrammarToJIT(node->if_node.cond, dsl, &p);
        EmitJcc(&p, COND_E, 0, 0);
        u8* jmp_else = p - 6;
        GrammarToJIT(node->if_node.then_branch, dsl, &p);
        if (node->if_node.else_branch) {
            EmitJmp(&p, 0, 0);
            u8* jmp_end = p - 5;
            u8* else_start = p;
            *(s32*)(jmp_else + 2) = (s32)((u64)else_start - (u64)(jmp_else + 6));
            GrammarToJIT(node->if_node.else_branch, dsl, &p);
            u8* end_else = p;
            *(s32*)(jmp_end + 1) = (s32)((u64)end_else - (u64)(jmp_end + 5));
        } else {
            u8* end_then = p;
            *(s32*)(jmp_else + 2) = (s32)((u64)end_then - (u64)(jmp_else + 6));
        }
    }
    if (node->type==AST_FOR) {
        GrammarToJIT(node->for_node.init, dsl, &p);
        u8* loop_start = p;
        GrammarToJIT(node->for_node.cond, dsl, &p);
        EmitJcc(&p, COND_E, 0, 0);
        u8* jmp_end = p - 6;
        GrammarToJIT(node->for_node.body, dsl, &p);
        GrammarToJIT(node->for_node.inc, dsl, &p);
        EmitJmp(&p, loop_start, p);
        u8* end_loop = p;
        *(s32*)(jmp_end + 2) = (s32)((u64)end_loop - (u64)(jmp_end + 6));
    }
    if (node->type==AST_WHILE) {
        u8* loop_start = p;
        GrammarToJIT(node->for_node.cond, dsl, &p);
        EmitJcc(&p, COND_E, 0, 0);
        u8* jmp_end = p - 6;
        GrammarToJIT(node->for_node.body, dsl, &p);
        EmitJmp(&p, loop_start, p);
        u8* end_loop = p;
        *(s32*)(jmp_end + 2) = (s32)((u64)end_loop - (u64)(jmp_end + 6));
    }
    if (node->type==AST_BINOP) {
        GrammarToJIT(node->binop_node.left, dsl, &p);
        EmitPush(&p, REG_RAX);
        GrammarToJIT(node->binop_node.right, dsl, &p);
        EmitMovRegReg(&p, REG_RBX, REG_RAX);
        EmitPop(&p, REG_RAX);
        EmitBinOp(node->binop_node.op, &p);
    }
    if (node->type==AST_UNARY) {
        GrammarToJIT(node->unary_node.operand, dsl, &p);
        if (node->unary_node.op == '-') EmitUnaryMuldv(&p, 3, REG_RAX);
        if (node->unary_node.op == '!') EmitUnaryMuldv(&p, 2, REG_RAX);
        if (node->unary_node.op == '~') EmitUnaryMuldv(&p, 2, REG_RAX); 
        if (node->unary_node.op == '*') {
            EmitMovRegPtrReg(&p, REG_RAX, REG_RAX);
        }
    }
    if (node->type==AST_FUNC_CALL) {
        ASTNode* arg = node->children;
        while (arg) {
            GrammarToJIT(arg, dsl, &p);
            EmitPush(&p, REG_RAX);
            arg = arg->next;
        }
        char* name = node->func_call_node.func_name;
        u64 serial = 0;
        u64 func_count = GetFuncCount();
        for (u64 i = 0; i < func_count; i++) {
            if (StrIs(name, (char*)GetFuncPoint(i))) {
                serial = i;
            }
        }
        if (serial) {
            void* func = GetFuncPoint(serial);
            EmitCall(&p, func, p);
        }
    }
    if (node->type==AST_RETURN) {
        GrammarToJIT(node->return_node.expr, dsl, &p);
        EmitRet(&p);
    }
    if (node->type==AST_NUMBER) {
        EmitMovRegImm64(&p, REG_RAX, node->num_value);
    }
    if (node->type==AST_STRING) {
        EmitMovRegImm64(&p, REG_RAX, (u64)node->string_value);
    }
    if (node->type==AST_VAR_DECL) {
        u64 serial = VarAdd((JITVar){0, TypeUnsigned|TypeLong, 0, 0});
        if (serial != (u64)-1) {
            if (node->var_decl_node.init_value) {
                GrammarToJIT(node->var_decl_node.init_value, dsl, &p);
                void* data = GetVarDataPoint(serial);
                if (data) {
                    EmitMovRegImm64(&p, REG_R11, (u64)data);
                    *p++ = 0x4C; *p++ = 0x89; *p++ = 0x03;
                }
            }
        }
    }
    if (node->type==AST_ASSIGN) {
        GrammarToJIT(node->assign_node.value, dsl, &p);
        if (node->assign_node.target && node->assign_node.target->type == AST_IDENT) {
            u64 serial = 0;
            for (u64 i = 0; i < VarEnd; i++) {
                if (Vars[i].Type && Vars[i].DataPoint) {
                    if (StrIs((char*)Vars[i].DataPoint, node->assign_node.target->ident_name)) {
                        serial = i;
                    }
                }
            }
            if (serial) {
                void* data = GetVarDataPoint(serial);
                if (data) {
                    EmitMovRegImm64(&p, REG_R11, (u64)data);
                    *p++ = 0x4C; *p++ = 0x89; *p++ = 0x03;
                }
            }
        }
    }
    if (node->type==AST_IDENT) {
        u64 serial = 0;
        for (u64 i = 0; i < VarEnd; i++) {
            if (Vars[i].Type && Vars[i].DataPoint) {
                if (StrIs((char*)Vars[i].DataPoint, node->ident_name)) {
                    serial = i;
                }
            }
        }
        if (serial) {
            void* data = GetVarDataPoint(serial);
            if (data) {
                EmitMovRegImm64(&p, REG_RAX, (u64)data);
            }
        }
    }

    *p_ptr = p;
}
