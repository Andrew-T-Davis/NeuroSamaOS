#include "HolyNeuro.h"
#include "JIT.h"
#include "DSLMake.h"
#include "int.h"
#include "kmalloc.h"
#include "debug.h"

extern AllocPool KernelPool;

static FishDSL HolyNeuroDSL = {
    .AddressSymbol = '&',
    .PointerSymbol = '*',
    .DerefSymbol = '*',
    .KeywordCount = 0,
    .RuleCount = 0,
    .Keywords = 0,
    .Rules = 0
};

static u8* CurrentJITBuffer = 0;

__attribute__((naked)) void JITPrintFunc() {
    __asm__ volatile(
        "pushq %%rcx\n\t"
        "subq $32, %%rsp\n\t"
        "movq %%rax, %%rcx\n\t"
        "call DebugStr\n\t"
        "addq $32, %%rsp\n\t"
        "popq %%rcx\n\t"
        "ret\n\t"
        :
        :
        : "rcx", "memory"
    );
}

void EmitPrint(Keyword KeyWordSelf, ASTNode* ASTNodeSelf) {
    (void)KeyWordSelf;

    u8* p = CurrentJITBuffer;
    if (!p) {
        DebugStr("EmitPrint: No JIT buffer!\n");
        return;
    }
    
    ASTNode* expr = ASTNodeSelf->children;
    if (expr) {
        GrammarToJIT(expr, &HolyNeuroDSL, &p);
    }
    
    EmitCall(&p, (void*)JITPrintFunc, p);
    EmitRet(&p);
    
    CurrentJITBuffer = p;
}

void PrintJIT(void* node, void* dsl, void* p_ptr) {
    ASTNode* self = (ASTNode*)node;
    CaptureNode* captures = self->custom_node.captures;
    
    char* expr_str = CaptureToString(captures, 0);
    if (!expr_str) return;
    
    ParserContext ctx = {
        .code = expr_str,
        .pos = expr_str,
        .dsl = (FishDSL*)dsl,
        .pool = &KernelPool
    };
    
    ASTNode* expr_ast = ParseExpression(&ctx);
    if (!expr_ast) return;
    
    u8** p = (u8**)p_ptr;
    if (!p || !*p) return;
    
    GrammarToJIT(expr_ast, (FishDSL*)dsl, p);
    EmitCall(p, (void*)JITPrintFunc, *p);
}

static Keyword print_kw = {
    .Name = "Print",
    .Type = AST_KEYWORD,
    .ToJITDo = EmitPrint
};

static SyntaxRule print_rule = {
    .Name = "PrintRule",
    .Pattern = "Print %expr",
    .EmitCustom = PrintJIT,
    .Captures = 0,
    .Body = 0
};

void FreeHolyNeuroJIT(u64 entry) {
    if (entry) {
        Free(&KernelPool, (void*)entry);
    }
}

u64 HolyNeuroJIT(char* Code, u8* buffer, u64 buffer_size) {
    if (!buffer || buffer_size < 1024) {
        DebugStr("HolyNeuroJIT: Invalid buffer!\n");
        return 0;
    }
    
    if (HolyNeuroDSL.Keywords) {
        Free(&KernelPool, HolyNeuroDSL.Keywords);
        HolyNeuroDSL.Keywords = 0;
    }
    if (HolyNeuroDSL.Rules) {
        Free(&KernelPool, HolyNeuroDSL.Rules);
        HolyNeuroDSL.Rules = 0;
    }
    HolyNeuroDSL.KeywordCount = 0;
    HolyNeuroDSL.RuleCount = 0;
    
    AddKeyword(&print_kw, &HolyNeuroDSL);
    AddSyntaxRule(&print_rule, &HolyNeuroDSL);
    
    ASTNode* root = CodeToAST(Code, &HolyNeuroDSL, &KernelPool);
    if (!root) {
        DebugStr("HolyNeuroJIT: Parse failed!\n");
        return 0;
    }
    
    CurrentJITBuffer = buffer;
    u8* p = buffer;

    GrammarToJIT(root, &HolyNeuroDSL, &p);
    EmitRet(&p);
    
    return (u64)buffer;
}
