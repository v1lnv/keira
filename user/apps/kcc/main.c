/**
 * Keira User Space: Keira C Compiler (kcc)
 *
 * A minimal single-pass compiler that compiles a subset of C into standalone
 * executable x86_64 ELF64 binaries directly on Keira.
 *
 * Supports variables, global arrays, pointers, arithmetic, comparisons, loops,
 * functions with parameters, and direct system calls.
 */

#include "../../lib/include/stdio.h"
#include "../../lib/include/string.h"
#include "../../lib/include/syscall.h"
#include "../../lib/include/malloc.h"

// Token types (plain integer constants to avoid enum/typedef)
int TOK_EOF = 0;
int TOK_INT = 1;
int TOK_VOID = 2;
int TOK_MAIN = 3;
int TOK_PRINTF = 4;
int TOK_RETURN = 5;
int TOK_IDENT = 6;
int TOK_NUM = 7;
int TOK_STRING = 8;
int TOK_LPAREN = 9;
int TOK_RPAREN = 10;
int TOK_LBRACE = 11;
int TOK_RBRACE = 12;
int TOK_SEMICOLON = 13;
int TOK_IF = 14;
int TOK_ELSE = 15;
int TOK_WHILE = 16;
int TOK_ASSIGN = 17;
int TOK_PLUS = 18;
int TOK_MINUS = 19;
int TOK_STAR = 20;
int TOK_SLASH = 21;
int TOK_LT = 22;
int TOK_GT = 23;
int TOK_EQ = 24;
int TOK_NEQ = 25;
int TOK_COMMA = 26;
int TOK_LBRACKET = 27;
int TOK_RBRACKET = 28;
int TOK_CHAR = 29;

// Global constants
int MAX_CODE_SIZE = 16384;
int MAX_DATA_SIZE = 8192;
int MAX_SOURCE_SIZE = 32768;

// Lexer variables
char *src_ptr;
char token_string[256];
int token_num;
int tok;

// Compiler global buffers
char src_buf[32768];
unsigned char code_buf[16384];
unsigned char data_buf[8192];
int code_idx;
int data_idx;

// Global symbols table (parallel arrays)
char global_names[64 * 32];
int global_offsets[64];
int global_count;

// Local symbols table (parallel arrays)
char local_names[32 * 32];
int local_offsets[32];
int local_count;

// Functions table (parallel arrays)
char function_names[32 * 32];
int function_addresses[32];
int function_count;

// Function call patches (parallel arrays)
char patch_names[128 * 32];
int patch_addresses[128];
int patch_count;

// Global patches (parallel arrays)
int val_patch_addresses[512];
int val_patch_offsets[512];
int val_patch_count;

// Custom Helper Functions to avoid standard library conflicts
int k_strcmp(char *s1, char *s2) {
    while (*s1 == *s2) {
        if (*s1 == 0) return 0;
        s1 = s1 + 1;
        s2 = s2 + 1;
    }
    return *s1 - *s2;
}

int k_strlen(char *s) {
    int len = 0;
    while (*s != 0) {
        len = len + 1;
        s = s + 1;
    }
    return len;
}

void k_strcpy(char *dest, char *src) {
    while (*src != 0) {
        *dest = *src;
        dest = dest + 1;
        src = src + 1;
    }
    *dest = 0;
}

void k_memcpy(char *dest, char *src, int n) {
    int i = 0;
    while (i < n) {
        *dest = *src;
        dest = dest + 1;
        src = src + 1;
        i = i + 1;
    }
}

void k_memset(char *dest, int val, int n) {
    int i = 0;
    while (i < n) {
        *dest = val;
        dest = dest + 1;
        i = i + 1;
    }
}

// Inline printer functions to replace printf
void print_str(char *s) {
    int len = k_strlen(s);
    sys_write(1, s, len);
}

void print_num(int val) {
    char buf[16];
    int idx = 15;
    buf[15] = 0;
    if (val == 0) {
        print_str("0");
        return;
    }
    int is_neg = 0;
    if (val < 0) {
        is_neg = 1;
        val = 0 - val;
    }
    while (val > 0) {
        idx = idx - 1;
        buf[idx] = 48 + (val % 10);
        val = val / 10;
    }
    if (is_neg) {
        idx = idx - 1;
        buf[idx] = '-';
    }
    print_str(buf + idx);
}

// ELF header builder helper functions
void write_u8(char *buf, int offset, int val) {
    buf[offset] = val;
}
void write_u16(char *buf, int offset, int val) {
    buf[offset] = val & 255;
    buf[offset + 1] = (val >> 8) & 255;
}
void write_u32(char *buf, int offset, int val) {
    buf[offset] = val & 255;
    buf[offset + 1] = (val >> 8) & 255;
    buf[offset + 2] = (val >> 16) & 255;
    buf[offset + 3] = (val >> 24) & 255;
}
void write_u64(char *buf, int offset, unsigned long val) {
    buf[offset] = val & 255;
    buf[offset + 1] = (val >> 8) & 255;
    buf[offset + 2] = (val >> 16) & 255;
    buf[offset + 3] = (val >> 24) & 255;
    buf[offset + 4] = (val >> 32) & 255;
    buf[offset + 5] = (val >> 40) & 255;
    buf[offset + 6] = (val >> 48) & 255;
    buf[offset + 7] = (val >> 56) & 255;
}

// Symbol and function table lookups
int lookup_global(char *name) {
    int i = 0;
    while (i < global_count) {
        if (k_strcmp(global_names + i * 32, name) == 0) {
            return global_offsets[i];
        }
        i = i + 1;
    }
    return 0 - 1;
}

int add_global(char *name, int size) {
    int offset = lookup_global(name);
    if (offset != 0 - 1) return offset;
    
    int current_offset = data_idx;
    data_idx = data_idx + size;
    
    k_strcpy(global_names + global_count * 32, name);
    global_offsets[global_count] = current_offset;
    global_count = global_count + 1;
    return current_offset;
}

int lookup_local(char *name) {
    int i = 0;
    while (i < local_count) {
        if (k_strcmp(local_names + i * 32, name) == 0) {
            return local_offsets[i];
        }
        i = i + 1;
    }
    return 0;
}

int add_local(char *name) {
    int offset = lookup_local(name);
    if (offset != 0) return offset;
    
    local_count = local_count + 1;
    k_strcpy(local_names + (local_count - 1) * 32, name);
    local_offsets[local_count - 1] = 0 - (8 * local_count);
    return 0 - (8 * local_count);
}

int lookup_function(char *name) {
    int i = 0;
    while (i < function_count) {
        if (k_strcmp(function_names + i * 32, name) == 0) {
            return function_addresses[i];
        }
        i = i + 1;
    }
    return 0 - 1;
}

void add_function(char *name, int address) {
    k_strcpy(function_names + function_count * 32, name);
    function_addresses[function_count] = address;
    function_count = function_count + 1;
}

// Lexer implementation
void skip_whitespace(void) {
    while (*src_ptr == ' ' || *src_ptr == '\t' || *src_ptr == '\r' || *src_ptr == '\n' || *src_ptr == '#') {
        if (*src_ptr == '#') {
            while (*src_ptr && *src_ptr != '\n') {
                src_ptr = src_ptr + 1;
            }
        } else {
            src_ptr = src_ptr + 1;
        }
    }
}

int next_token(void) {
    skip_whitespace();
    if (!*src_ptr) {
        return TOK_EOF;
    }

    // Alphabetical identifier
    if ((*src_ptr >= 'a' && *src_ptr <= 'z') || (*src_ptr >= 'A' && *src_ptr <= 'Z') || *src_ptr == '_') {
        int len = 0;
        while ((*src_ptr >= 'a' && *src_ptr <= 'z') || (*src_ptr >= 'A' && *src_ptr <= 'Z') ||
               (*src_ptr >= '0' && *src_ptr <= '9') || *src_ptr == '_') {
            if (len < 255) {
                token_string[len] = *src_ptr;
                len = len + 1;
            }
            src_ptr = src_ptr + 1;
        }
        token_string[len] = 0;

        if (k_strcmp(token_string, "int") == 0) return TOK_INT;
        if (k_strcmp(token_string, "char") == 0) return TOK_CHAR;
        if (k_strcmp(token_string, "void") == 0) return TOK_VOID;
        if (k_strcmp(token_string, "main") == 0) return TOK_MAIN;
        if (k_strcmp(token_string, "printf") == 0) return TOK_PRINTF;
        if (k_strcmp(token_string, "return") == 0) return TOK_RETURN;
        if (k_strcmp(token_string, "if") == 0) return TOK_IF;
        if (k_strcmp(token_string, "else") == 0) return TOK_ELSE;
        if (k_strcmp(token_string, "while") == 0) return TOK_WHILE;
        return TOK_IDENT;
    }

    // Numbers
    if (*src_ptr >= '0' && *src_ptr <= '9') {
        token_num = 0;
        while (*src_ptr >= '0' && *src_ptr <= '9') {
            token_num = token_num * 10 + (*src_ptr - '0');
            src_ptr = src_ptr + 1;
        }
        return TOK_NUM;
    }

    // Character literals
    if (*src_ptr == '\'') {
        src_ptr = src_ptr + 1;
        int val = 0;
        if (*src_ptr == '\\') {
            src_ptr = src_ptr + 1;
            if (*src_ptr == 'n') val = 10;
            else if (*src_ptr == 't') val = 9;
            else if (*src_ptr == 'r') val = 13;
            else if (*src_ptr == '0') val = 0;
            else val = *src_ptr;
            src_ptr = src_ptr + 1;
        } else {
            val = *src_ptr;
            src_ptr = src_ptr + 1;
        }
        if (*src_ptr == '\'') {
            src_ptr = src_ptr + 1;
        }
        token_num = val;
        return TOK_NUM;
    }

    // Strings
    if (*src_ptr == '"') {
        src_ptr = src_ptr + 1;
        int len = 0;
        while (*src_ptr && *src_ptr != '"') {
            if (*src_ptr == '\\' && *(src_ptr + 1) == 'n') {
                token_string[len] = '\n';
                len = len + 1;
                src_ptr = src_ptr + 2;
            } else {
                token_string[len] = *src_ptr;
                len = len + 1;
                src_ptr = src_ptr + 1;
            }
        }
        if (*src_ptr == '"') {
            src_ptr = src_ptr + 1;
        }
        token_string[len] = 0;
        return TOK_STRING;
    }

    char c = *src_ptr;
    src_ptr = src_ptr + 1;
    if (c == '(') return TOK_LPAREN;
    if (c == ')') return TOK_RPAREN;
    if (c == '{') return TOK_LBRACE;
    if (c == '}') return TOK_RBRACE;
    if (c == ';') return TOK_SEMICOLON;
    if (c == ',') return TOK_COMMA;
    if (c == '[') return TOK_LBRACKET;
    if (c == ']') return TOK_RBRACKET;
    if (c == '+') return TOK_PLUS;
    if (c == '-') return TOK_MINUS;
    if (c == '*') return TOK_STAR;
    if (c == '/') return TOK_SLASH;
    if (c == '<') return TOK_LT;
    if (c == '>') return TOK_GT;
    if (c == '=') {
        if (*src_ptr == '=') {
            src_ptr = src_ptr + 1;
            return TOK_EQ;
        }
        return TOK_ASSIGN;
    }
    if (c == '!') {
        if (*src_ptr == '=') {
            src_ptr = src_ptr + 1;
            return TOK_NEQ;
        }
    }

    return TOK_EOF;
}

void match(int expected) {
    if (tok == expected) {
        tok = next_token();
    } else {
        print_str("Error: Expected token ");
        print_num(expected);
        print_str(", got ");
        print_num(tok);
        print_str("\n");
        sys_exit();
    }
}

// Expression recursive descent parsing & code generation
void expression(void);

void primary_expr(void) {
    if (tok == TOK_NUM) {
        // mov eax, token_num -> b8 [4 bytes]
        code_buf[code_idx] = 0xb8;
        unsigned int val = (unsigned int)token_num;
        k_memcpy((char *)(code_buf + code_idx + 1), (char *)&val, 4);
        code_idx = code_idx + 5;
        match(TOK_NUM);
    } else if (tok == TOK_STAR) {
        match(TOK_STAR);
        primary_expr(); // evaluates address into EAX
        // mov rsi, rax -> 48 89 c6
        code_buf[code_idx] = 0x48;
        code_buf[code_idx + 1] = 0x89;
        code_buf[code_idx + 2] = 0xc6;
        // movzx eax, byte ptr [rsi] -> 0f b6 06
        code_buf[code_idx + 3] = 0x0f;
        code_buf[code_idx + 4] = 0xb6;
        code_buf[code_idx + 5] = 0x06;
        code_idx = code_idx + 6;
    } else if (tok == TOK_IDENT) {
        char name[256];
        k_strcpy(name, token_string);
        match(TOK_IDENT);
        
        if (tok == TOK_LPAREN) {
            match(TOK_LPAREN);
            
            // Builtin generic system call interface: syscall(num, arg1, arg2, arg3)
            if (k_strcmp(name, "syscall") == 0) {
                expression();
                code_buf[code_idx] = 0x50; // push rax
                code_idx = code_idx + 1;
                
                match(TOK_COMMA);
                expression();
                code_buf[code_idx] = 0x50; // push rax
                code_idx = code_idx + 1;
                
                match(TOK_COMMA);
                expression();
                code_buf[code_idx] = 0x50; // push rax
                code_idx = code_idx + 1;
                
                match(TOK_COMMA);
                expression();
                code_buf[code_idx] = 0x50; // push rax
                code_idx = code_idx + 1;
                
                match(TOK_RPAREN);
                
                // Pop args into ABI registers
                code_buf[code_idx] = 0x5a;     // pop rdx (arg3)
                code_buf[code_idx + 1] = 0x5e; // pop rsi (arg2)
                code_buf[code_idx + 2] = 0x5f; // pop rdi (arg1)
                code_buf[code_idx + 3] = 0x58; // pop rax (syscall num)
                
                // Emit syscall -> 0f 05
                code_buf[code_idx + 4] = 0x0f;
                code_buf[code_idx + 5] = 0x05;
                code_idx = code_idx + 6;
                return;
            }

            // Normal function call
            int arg_count = 0;
            if (tok != TOK_RPAREN) {
                expression();
                code_buf[code_idx] = 0x50; // push rax
                code_idx = code_idx + 1;
                arg_count = arg_count + 1;
                while (tok == TOK_COMMA) {
                    match(TOK_COMMA);
                    expression();
                    code_buf[code_idx] = 0x50; // push rax
                    code_idx = code_idx + 1;
                    arg_count = arg_count + 1;
                }
            }
            match(TOK_RPAREN);

            // Pop args into RDI, RSI, RDX, RCX
            if (arg_count == 3) {
                code_buf[code_idx] = 0x5a;     // pop rdx
                code_buf[code_idx + 1] = 0x5e; // pop rsi
                code_buf[code_idx + 2] = 0x5f; // pop rdi
                code_idx = code_idx + 3;
            } else if (arg_count == 2) {
                code_buf[code_idx] = 0x5e;     // pop rsi
                code_buf[code_idx + 1] = 0x5f; // pop rdi
                code_idx = code_idx + 2;
            } else if (arg_count == 1) {
                code_buf[code_idx] = 0x5f;     // pop rdi
                code_idx = code_idx + 1;
            }

            // call relative address -> e8 [4 bytes dummy]
            code_buf[code_idx] = 0xe8;
            int patch_pos = code_idx + 1;
            int zero = 0;
            k_memcpy((char *)(code_buf + code_idx + 1), (char *)&zero, 4);
            code_idx = code_idx + 5;
            
            // Record patch
            k_strcpy(patch_names + patch_count * 32, name);
            patch_addresses[patch_count] = patch_pos;
            patch_count = patch_count + 1;
        } else {
            // Local variable lookup
            int local_offset = lookup_local(name);
            if (local_offset != 0) {
                // mov rax, [rbp + local_offset] -> 48 8b 45 [offset]
                code_buf[code_idx] = 0x48;
                code_buf[code_idx + 1] = 0x8b;
                code_buf[code_idx + 2] = 0x45;
                code_buf[code_idx + 3] = (unsigned char)local_offset;
                code_idx = code_idx + 4;
                
                if (tok == TOK_LBRACKET) {
                    match(TOK_LBRACKET);
                    // Push pointer address onto stack
                    code_buf[code_idx] = 0x50; // push rax
                    code_idx = code_idx + 1;
                    
                    expression(); // index into EAX
                    
                    code_buf[code_idx] = 0x5e; // pop rsi (base)
                    // add rsi, rax -> 48 01 c6
                    code_buf[code_idx + 1] = 0x48;
                    code_buf[code_idx + 2] = 0x01;
                    code_buf[code_idx + 3] = 0xc6;
                    // movzx eax, byte ptr [rsi] -> 0f b6 06
                    code_buf[code_idx + 4] = 0x0f;
                    code_buf[code_idx + 5] = 0xb6;
                    code_buf[code_idx + 6] = 0x06;
                    code_idx = code_idx + 7;
                    match(TOK_RBRACKET);
                }
            } else {
                // Global variable lookup
                int global_offset = lookup_global(name);
                if (global_offset == 0 - 1) {
                    print_str("Error: Undefined variable ");
                    print_str(name);
                    print_str("\n");
                    sys_exit();
                }
                
                // mov rsi, absolute_address -> 48 be [8 bytes dummy]
                code_buf[code_idx] = 0x48;
                code_buf[code_idx + 1] = 0xbe;
                int patch_pos = code_idx + 2;
                unsigned long dummy_offset = (unsigned long)global_offset;
                k_memcpy((char *)(code_buf + code_idx + 2), (char *)&dummy_offset, 8);
                code_idx = code_idx + 10;
                
                // Record global patch
                val_patch_addresses[val_patch_count] = patch_pos;
                val_patch_offsets[val_patch_count] = global_offset;
                val_patch_count = val_patch_count + 1;
                
                if (tok == TOK_LBRACKET) {
                    match(TOK_LBRACKET);
                    // Push array address onto stack
                    code_buf[code_idx] = 0x56; // push rsi
                    code_idx = code_idx + 1;
                    
                    expression(); // index into EAX
                    
                    code_buf[code_idx] = 0x5e; // pop rsi
                    // add rsi, rax -> 48 01 c6
                    code_buf[code_idx + 1] = 0x48;
                    code_buf[code_idx + 2] = 0x01;
                    code_buf[code_idx + 3] = 0xc6;
                    // movzx eax, byte ptr [rsi] -> 0f b6 06
                    code_buf[code_idx + 4] = 0x0f;
                    code_buf[code_idx + 5] = 0xb6;
                    code_buf[code_idx + 6] = 0x06;
                    code_idx = code_idx + 7;
                    match(TOK_RBRACKET);
                } else {
                    // Simple global variable: mov rax, [rsi] -> 48 8b 06
                    code_buf[code_idx] = 0x48;
                    code_buf[code_idx + 1] = 0x8b;
                    code_buf[code_idx + 2] = 0x06;
                    code_idx = code_idx + 3;
                }
            }
        }
    } else if (tok == TOK_LPAREN) {
        match(TOK_LPAREN);
        expression();
        match(TOK_RPAREN);
    } else {
        print_str("Error: Invalid primary expression, got token ");
        print_num(tok);
        print_str("\n");
        sys_exit();
    }
}

void mul_expr(void) {
    primary_expr();
    while (tok == TOK_STAR || tok == TOK_SLASH) {
        int op = tok;
        match(op);
        code_buf[code_idx] = 0x50; // push rax
        code_idx = code_idx + 1;
        primary_expr();
        code_buf[code_idx] = 0x59; // pop rcx
        code_idx = code_idx + 1;
        
        if (op == TOK_STAR) {
            // imul eax, ecx -> 0f af c1
            code_buf[code_idx] = 0x0f;
            code_buf[code_idx + 1] = 0xaf;
            code_buf[code_idx + 2] = 0xc1;
            code_idx = code_idx + 3;
        } else {
            // xchg eax, ecx -> 91
            // cdq -> 99
            // idiv ecx -> f7 f9
            code_buf[code_idx] = 0x91;
            code_buf[code_idx + 1] = 0x99;
            code_buf[code_idx + 2] = 0xf7;
            code_buf[code_idx + 3] = 0xf9;
            code_idx = code_idx + 4;
        }
    }
}

void add_expr(void) {
    mul_expr();
    while (tok == TOK_PLUS || tok == TOK_MINUS) {
        int op = tok;
        match(op);
        code_buf[code_idx] = 0x50; // push rax
        code_idx = code_idx + 1;
        mul_expr();
        code_buf[code_idx] = 0x59; // pop rcx
        code_idx = code_idx + 1;
        
        if (op == TOK_PLUS) {
            // add eax, ecx -> 03 c1
            code_buf[code_idx] = 0x03;
            code_buf[code_idx + 1] = 0xc1;
            code_idx = code_idx + 2;
        } else {
            // xchg eax, ecx -> 91
            // sub eax, ecx -> 2b c1
            code_buf[code_idx] = 0x91;
            code_buf[code_idx + 1] = 0x2b;
            code_buf[code_idx + 2] = 0xc1;
            code_idx = code_idx + 3;
        }
    }
}

void expression(void) {
    add_expr();
    while (tok == TOK_LT || tok == TOK_GT || tok == TOK_EQ || tok == TOK_NEQ) {
        int op = tok;
        match(op);
        code_buf[code_idx] = 0x50; // push rax
        code_idx = code_idx + 1;
        add_expr();
        code_buf[code_idx] = 0x59; // pop rcx
        code_idx = code_idx + 1;
        
        // cmp ecx, eax -> 39 c1
        code_buf[code_idx] = 0x39;
        code_buf[code_idx + 1] = 0xc1;
        code_idx = code_idx + 2;
        
        if (op == TOK_LT) {
            // setl al -> 0f 9c c0
            code_buf[code_idx] = 0x0f;
            code_buf[code_idx + 1] = 0x9c;
            code_buf[code_idx + 2] = 0xc0;
        } else if (op == TOK_GT) {
            // setg al -> 0f 9f c0
            code_buf[code_idx] = 0x0f;
            code_buf[code_idx + 1] = 0x9f;
            code_buf[code_idx + 2] = 0xc0;
        } else if (op == TOK_EQ) {
            // sete al -> 0f 94 c0
            code_buf[code_idx] = 0x0f;
            code_buf[code_idx + 1] = 0x94;
            code_buf[code_idx + 2] = 0xc0;
        } else {
            // setne al -> 0f 95 c0
            code_buf[code_idx] = 0x0f;
            code_buf[code_idx + 1] = 0x95;
            code_buf[code_idx + 2] = 0xc0;
        }
        // movzx eax, al -> 0f b6 c0
        code_buf[code_idx + 3] = 0x0f;
        code_buf[code_idx + 4] = 0xb6;
        code_buf[code_idx + 5] = 0xc0;
        code_idx = code_idx + 6;
    }
}

// Statement and block recursive parsing & code generation
void statement(void);

void block(void) {
    while (tok != TOK_RBRACE && tok != TOK_EOF) {
        statement();
    }
}

void statement(void) {
    if (tok == TOK_LBRACE) {
        match(TOK_LBRACE);
        block();
        match(TOK_RBRACE);
    } else if (tok == TOK_INT || tok == TOK_CHAR) {
        int type = tok;
        match(type);
        
        int is_ptr = 0;
        if (tok == TOK_STAR) {
            match(TOK_STAR);
            is_ptr = 1;
        }
        
        char var_name[256];
        k_strcpy(var_name, token_string);
        match(TOK_IDENT);
        
        int offset = add_local(var_name);
        if (tok == TOK_ASSIGN) {
            match(TOK_ASSIGN);
            expression();
        } else {
            // mov eax, 0
            code_buf[code_idx] = 0xb8;
            int zero = 0;
            k_memcpy((char *)(code_buf + code_idx + 1), (char *)&zero, 4);
            code_idx = code_idx + 5;
        }
        // mov [rbp + offset], rax -> 48 89 45 [offset]
        code_buf[code_idx] = 0x48;
        code_buf[code_idx + 1] = 0x89;
        code_buf[code_idx + 2] = 0x45;
        code_buf[code_idx + 3] = (unsigned char)offset;
        code_idx = code_idx + 4;
        match(TOK_SEMICOLON);
    } else if (tok == TOK_STAR) {
        match(TOK_STAR);
        char var_name[256];
        k_strcpy(var_name, token_string);
        match(TOK_IDENT);
        
        int offset = lookup_local(var_name);
        if (offset != 0) {
            // mov rsi, [rbp + offset] -> 48 8b 75 [offset]
            code_buf[code_idx] = 0x48;
            code_buf[code_idx + 1] = 0x8b;
            code_buf[code_idx + 2] = 0x75;
            code_buf[code_idx + 3] = (unsigned char)offset;
            code_idx = code_idx + 4;
        } else {
            int global_offset = lookup_global(var_name);
            if (global_offset == 0 - 1) {
                print_str("Error: Undefined variable ");
                print_str(var_name);
                print_str("\n");
                sys_exit();
            }
            // mov rsi, absolute_address -> 48 be [8 bytes]
            code_buf[code_idx] = 0x48;
            code_buf[code_idx + 1] = 0xbe;
            int patch_pos = code_idx + 2;
            unsigned long dummy_offset = (unsigned long)global_offset;
            k_memcpy((char *)(code_buf + code_idx + 2), (char *)&dummy_offset, 8);
            code_idx = code_idx + 10;
            
            val_patch_addresses[val_patch_count] = patch_pos;
            val_patch_offsets[val_patch_count] = global_offset;
            val_patch_count = val_patch_count + 1;
            
            // mov rsi, [rsi] -> 48 8b 36
            code_buf[code_idx] = 0x48;
            code_buf[code_idx + 1] = 0x8b;
            code_buf[code_idx + 2] = 0x36;
            code_idx = code_idx + 3;
        }
        
        code_buf[code_idx] = 0x56; // push rsi
        code_idx = code_idx + 1;
        
        match(TOK_ASSIGN);
        expression();
        match(TOK_SEMICOLON);
        
        code_buf[code_idx] = 0x5e; // pop rsi
        // mov [rsi], al -> 88 06 (write char byte)
        code_buf[code_idx + 1] = 0x88;
        code_buf[code_idx + 2] = 0x06;
        code_idx = code_idx + 3;
    } else if (tok == TOK_IDENT) {
        char name[256];
        k_strcpy(name, token_string);
        match(TOK_IDENT);
        
        if (tok == TOK_LBRACKET) {
            match(TOK_LBRACKET);
            
            int local_offset = lookup_local(name);
            if (local_offset != 0) {
                // mov rsi, [rbp + local_offset] -> 48 8b 75 [offset]
                code_buf[code_idx] = 0x48;
                code_buf[code_idx + 1] = 0x8b;
                code_buf[code_idx + 2] = 0x75;
                code_buf[code_idx + 3] = (unsigned char)local_offset;
                code_idx = code_idx + 4;
            } else {
                int global_offset = lookup_global(name);
                if (global_offset == 0 - 1) {
                    print_str("Error: Undefined variable ");
                    print_str(name);
                    print_str("\n");
                    sys_exit();
                }
                // mov rsi, absolute_address -> 48 be [8 bytes]
                code_buf[code_idx] = 0x48;
                code_buf[code_idx + 1] = 0xbe;
                int patch_pos = code_idx + 2;
                unsigned long dummy_offset = (unsigned long)global_offset;
                k_memcpy((char *)(code_buf + code_idx + 2), (char *)&dummy_offset, 8);
                code_idx = code_idx + 10;
                
                val_patch_addresses[val_patch_count] = patch_pos;
                val_patch_offsets[val_patch_count] = global_offset;
                val_patch_count = val_patch_count + 1;
            }
            
            code_buf[code_idx] = 0x56; // push rsi
            code_idx = code_idx + 1;
            
            expression(); // index into EAX
            match(TOK_RBRACKET);
            
            code_buf[code_idx] = 0x5e; // pop rsi
            // add rsi, rax -> 48 01 c6
            code_buf[code_idx + 1] = 0x48;
            code_buf[code_idx + 2] = 0x01;
            code_buf[code_idx + 3] = 0xc6;
            // push rsi
            code_buf[code_idx + 4] = 0x56;
            code_idx = code_idx + 5;
            
            match(TOK_ASSIGN);
            expression();
            match(TOK_SEMICOLON);
            
            code_buf[code_idx] = 0x5e; // pop rsi
            // mov [rsi], al -> 88 06 (write char byte to array index)
            code_buf[code_idx + 1] = 0x88;
            code_buf[code_idx + 2] = 0x06;
            code_idx = code_idx + 3;
        } else {
            int local_offset = lookup_local(name);
            if (local_offset != 0) {
                match(TOK_ASSIGN);
                expression();
                // mov [rbp + local_offset], rax -> 48 89 45 [offset]
                code_buf[code_idx] = 0x48;
                code_buf[code_idx + 1] = 0x89;
                code_buf[code_idx + 2] = 0x45;
                code_buf[code_idx + 3] = (unsigned char)local_offset;
                code_idx = code_idx + 4;
                match(TOK_SEMICOLON);
            } else {
                int global_offset = lookup_global(name);
                if (global_offset == 0 - 1) {
                    print_str("Error: Undefined variable ");
                    print_str(name);
                    print_str("\n");
                    sys_exit();
                }
                
                // mov rsi, absolute_address -> 48 be [8 bytes]
                code_buf[code_idx] = 0x48;
                code_buf[code_idx + 1] = 0xbe;
                int patch_pos = code_idx + 2;
                unsigned long dummy_offset = (unsigned long)global_offset;
                k_memcpy((char *)(code_buf + code_idx + 2), (char *)&dummy_offset, 8);
                code_idx = code_idx + 10;
                
                val_patch_addresses[val_patch_count] = patch_pos;
                val_patch_offsets[val_patch_count] = global_offset;
                val_patch_count = val_patch_count + 1;
                
                code_buf[code_idx] = 0x56; // push rsi
                code_idx = code_idx + 1;
                
                match(TOK_ASSIGN);
                expression();
                match(TOK_SEMICOLON);
                
                code_buf[code_idx] = 0x5e; // pop rsi
                // mov [rsi], rax -> 48 89 06
                code_buf[code_idx + 1] = 0x48;
                code_buf[code_idx + 2] = 0x89;
                code_buf[code_idx + 3] = 0x06;
                code_idx = code_idx + 4;
            }
        }
    } else if (tok == TOK_IF) {
        match(TOK_IF);
        match(TOK_LPAREN);
        expression();
        match(TOK_RPAREN);
        
        // test eax, eax -> 85 c0
        // jz else_label -> 0f 84 [4 bytes relative offset]
        code_buf[code_idx] = 0x85;
        code_buf[code_idx + 1] = 0xc0;
        code_buf[code_idx + 2] = 0x0f;
        code_buf[code_idx + 3] = 0x84;
        int jz_offset_idx = code_idx + 4;
        code_idx = code_idx + 8;
        
        statement();
        
        if (tok == TOK_ELSE) {
            match(TOK_ELSE);
            // jmp end_label -> e9 [4 bytes]
            code_buf[code_idx] = 0xe9;
            int jmp_offset_idx = code_idx + 1;
            code_idx = code_idx + 5;
            
            int else_offset = code_idx - (jz_offset_idx + 4);
            k_memcpy((char *)(code_buf + jz_offset_idx), (char *)&else_offset, 4);
            
            statement();
            
            int end_offset = code_idx - (jmp_offset_idx + 4);
            k_memcpy((char *)(code_buf + jmp_offset_idx), (char *)&end_offset, 4);
        } else {
            int else_offset = code_idx - (jz_offset_idx + 4);
            k_memcpy((char *)(code_buf + jz_offset_idx), (char *)&else_offset, 4);
        }
    } else if (tok == TOK_WHILE) {
        int start_addr = code_idx;
        match(TOK_WHILE);
        match(TOK_LPAREN);
        expression();
        match(TOK_RPAREN);
        
        // test eax, eax -> 85 c0
        // jz end_label -> 0f 84 [4 bytes]
        code_buf[code_idx] = 0x85;
        code_buf[code_idx + 1] = 0xc0;
        code_buf[code_idx + 2] = 0x0f;
        code_buf[code_idx + 3] = 0x84;
        int jz_offset_idx = code_idx + 4;
        code_idx = code_idx + 8;
        
        statement();
        
        // jmp start_addr -> e9 [4 bytes]
        code_buf[code_idx] = 0xe9;
        int jump_back = start_addr - (code_idx + 5);
        k_memcpy((char *)(code_buf + code_idx + 1), (char *)&jump_back, 4);
        code_idx = code_idx + 5;
        
        int end_offset = code_idx - (jz_offset_idx + 4);
        k_memcpy((char *)(code_buf + jz_offset_idx), (char *)&end_offset, 4);
    } else if (tok == TOK_PRINTF) {
        match(TOK_PRINTF);
        match(TOK_LPAREN);
        
        int str_len = k_strlen(token_string) + 1;
        if (data_idx + str_len >= MAX_DATA_SIZE) {
            print_str("Error: Data segment overflow\n");
            sys_exit();
        }
        int str_offset = data_idx;
        k_memcpy((char *)(data_buf + data_idx), token_string, str_len);
        data_idx = data_idx + str_len;
        
        match(TOK_STRING);
        match(TOK_RPAREN);
        match(TOK_SEMICOLON);

        // mov rsi, imm64 -> 48 be [8 bytes str_offset]
        code_buf[code_idx] = 0x48;
        code_buf[code_idx + 1] = 0xbe;
        unsigned long temp_offset = (unsigned long)str_offset;
        k_memcpy((char *)(code_buf + code_idx + 2), (char *)&temp_offset, 8);
        
        // Record global patch for the string
        val_patch_addresses[val_patch_count] = code_idx + 2;
        val_patch_offsets[val_patch_count] = str_offset;
        val_patch_count = val_patch_count + 1;
        
        code_idx = code_idx + 10;

        // movzx edi, byte ptr [rsi] -> 0f b6 3e
        // test edi, edi -> 85 ff
        // jz +12 -> 74 0c
        // mov eax, 1 -> b8 01 00 00 00
        // syscall -> 0f 05
        // inc rsi -> 48 ff c6
        // jmp -25 -> eb ed
        code_buf[code_idx] = 0x0f;
        code_buf[code_idx + 1] = 0xb6;
        code_buf[code_idx + 2] = 0x3e;
        code_buf[code_idx + 3] = 0x85;
        code_buf[code_idx + 4] = 0xff;
        code_buf[code_idx + 5] = 0x74;
        code_buf[code_idx + 6] = 0x0c;
        code_buf[code_idx + 7] = 0xb8;
        code_buf[code_idx + 8] = 0x01;
        code_buf[code_idx + 9] = 0x00;
        code_buf[code_idx + 10] = 0x00;
        code_buf[code_idx + 11] = 0x00;
        code_buf[code_idx + 12] = 0x0f;
        code_buf[code_idx + 13] = 0x05;
        code_buf[code_idx + 14] = 0x48;
        code_buf[code_idx + 15] = 0xff;
        code_buf[code_idx + 16] = 0xc6;
        code_buf[code_idx + 17] = 0xeb;
        code_buf[code_idx + 18] = 0xed;
        code_idx = code_idx + 19;
    } else if (tok == TOK_RETURN) {
        match(TOK_RETURN);
        expression();
        match(TOK_SEMICOLON);
    } else {
        print_str("Error: Unexpected token in statement: ");
        print_num(tok);
        print_str("\n");
        sys_exit();
    }
}

void compile_global_declarations(void) {
    tok = next_token();
    while (tok != TOK_EOF) {
        if (tok == TOK_INT || tok == TOK_VOID || tok == TOK_CHAR) {
            int type = tok;
            match(type);
            
            int is_ptr = 0;
            if (tok == TOK_STAR) {
                match(TOK_STAR);
                is_ptr = 1;
            }
            
            char name[256];
            k_strcpy(name, token_string);
            match(TOK_IDENT);
            
            if (tok == TOK_LPAREN) {
                // Function definition
                match(TOK_LPAREN);
                add_function(name, code_idx);
                local_count = 0;
                
                int param_count = 0;
                if (tok != TOK_RPAREN) {
                    int p_type = tok;
                    match(p_type);
                    int p_is_ptr = 0;
                    if (tok == TOK_STAR) {
                        match(TOK_STAR);
                        p_is_ptr = 1;
                    }
                    char p_name[256];
                    k_strcpy(p_name, token_string);
                    match(TOK_IDENT);
                    
                    add_local(p_name);
                    param_count = param_count + 1;
                    
                    while (tok == TOK_COMMA) {
                        match(TOK_COMMA);
                        int next_p_type = tok;
                        match(next_p_type);
                        int next_p_is_ptr = 0;
                        if (tok == TOK_STAR) {
                            match(TOK_STAR);
                            next_p_is_ptr = 1;
                        }
                        char next_p_name[256];
                        k_strcpy(next_p_name, token_string);
                        match(TOK_IDENT);
                        
                        add_local(next_p_name);
                        param_count = param_count + 1;
                    }
                }
                match(TOK_RPAREN);
                
                // Emit prologue
                code_buf[code_idx] = 0x55;     // push rbp
                code_buf[code_idx + 1] = 0x48; // mov rbp, rsp
                code_buf[code_idx + 2] = 0x89;
                code_buf[code_idx + 3] = 0xe5;
                code_buf[code_idx + 4] = 0x48; // sub rsp, 128
                code_buf[code_idx + 5] = 0x81;
                code_buf[code_idx + 6] = 0xec;
                code_buf[code_idx + 7] = 0x80;
                code_buf[code_idx + 8] = 0x00;
                code_buf[code_idx + 9] = 0x00;
                code_buf[code_idx + 10] = 0x00;
                code_idx = code_idx + 11;
                
                // Save parameters from ABI registers into stack slots
                if (param_count >= 1) {
                    code_buf[code_idx] = 0x48;     // mov [rbp - 8], rdi
                    code_buf[code_idx + 1] = 0x89;
                    code_buf[code_idx + 2] = 0x7d;
                    code_buf[code_idx + 3] = 0xf8;
                    code_idx = code_idx + 4;
                }
                if (param_count >= 2) {
                    code_buf[code_idx] = 0x48;     // mov [rbp - 16], rsi
                    code_buf[code_idx + 1] = 0x89;
                    code_buf[code_idx + 2] = 0x75;
                    code_buf[code_idx + 3] = 0xf0;
                    code_idx = code_idx + 4;
                }
                if (param_count >= 3) {
                    code_buf[code_idx] = 0x48;     // mov [rbp - 24], rdx
                    code_buf[code_idx + 1] = 0x89;
                    code_buf[code_idx + 2] = 0x55;
                    code_buf[code_idx + 3] = 0xe8;
                    code_idx = code_idx + 4;
                }
                if (param_count >= 4) {
                    code_buf[code_idx] = 0x48;     // mov [rbp - 32], rcx
                    code_buf[code_idx + 1] = 0x89;
                    code_buf[code_idx + 2] = 0x4d;
                    code_buf[code_idx + 3] = 0xe0;
                    code_idx = code_idx + 4;
                }
                
                match(TOK_LBRACE);
                block();
                match(TOK_RBRACE);
                
                // Emit epilogue
                code_buf[code_idx] = 0x48;     // mov rsp, rbp
                code_buf[code_idx + 1] = 0x89;
                code_buf[code_idx + 2] = 0xec;
                code_buf[code_idx + 3] = 0x5d; // pop rbp
                code_idx = code_idx + 4;
                
                if (k_strcmp(name, "main") == 0) {
                    // sys_exit (Syscall 2)
                    code_buf[code_idx] = 0xb8;
                    code_buf[code_idx + 1] = 0x02;
                    code_buf[code_idx + 2] = 0x00;
                    code_buf[code_idx + 3] = 0x00;
                    code_buf[code_idx + 4] = 0x00;
                    code_buf[code_idx + 5] = 0x0f;
                    code_buf[code_idx + 6] = 0x05;
                    code_idx = code_idx + 7;
                } else {
                    code_buf[code_idx] = 0xc3;     // ret
                    code_idx = code_idx + 1;
                }
            } else {
                // Global variable or global array
                if (tok == TOK_LBRACKET) {
                    match(TOK_LBRACKET);
                    int size = token_num;
                    match(TOK_NUM);
                    match(TOK_RBRACKET);
                    add_global(name, size);
                } else {
                    add_global(name, 8);
                }
                match(TOK_SEMICOLON);
            }
        } else {
            tok = next_token();
        }
    }
}

void _start(void) {
    print_str("Keira C Compiler (kcc) v0.14.0\n");
    print_str("Compiling source: /apps/src/demo.c -> /apps/bin/demo.elf\n");

    // Initialize compiler state
    code_idx = 0;
    data_idx = 0;
    global_count = 0;
    local_count = 0;
    function_count = 0;
    patch_count = 0;
    val_patch_count = 0;

    // Open and read source file
    int in_fd = sys_open("/apps/src/demo.c", 0);
    if (in_fd < 0) {
        print_str("Error: Could not open /apps/src/demo.c\n");
        sys_exit();
    }

    k_memset(src_buf, 0, 32768);
    int read_len = sys_read(in_fd, src_buf, 32768 - 1);
    sys_close(in_fd);
    if (read_len <= 0) {
        print_str("Error: Read empty or failed for /apps/src/demo.c\n");
        sys_exit();
    }

    src_ptr = src_buf;
    compile_global_declarations();

    // Patch function calls relative addresses
    int i = 0;
    while (i < patch_count) {
        int patch_address = patch_addresses[i];
        int address = lookup_function(patch_names + i * 32);
        if (address == 0 - 1) {
            print_str("Error: Undefined function reference: ");
            print_str(patch_names + i * 32);
            print_str("\n");
            sys_exit();
        }
        int rel_offset = address - (patch_address + 4);
        k_memcpy((char *)(code_buf + patch_address), (char *)&rel_offset, 4);
        i = i + 1;
    }

    // Resolve main function entry point offset
    int main_offset = lookup_function("main");
    if (main_offset == 0 - 1) {
        print_str("Error: main function not defined\n");
        sys_exit();
    }

    // Set segment layouts
    unsigned long final_code_size = (unsigned long)code_idx;
    unsigned long base_vaddr = 0x40000000;
    unsigned long headers_size = 120;
    unsigned long text_start_vaddr = base_vaddr + headers_size;
    unsigned long data_start_vaddr = text_start_vaddr + final_code_size;

    // Patch global variables absolute virtual addresses in text segment
    i = 0;
    while (i < val_patch_count) {
        int patch_address = val_patch_addresses[i];
        int offset = val_patch_offsets[i];
        unsigned long target_vaddr = data_start_vaddr + offset;
        k_memcpy((char *)(code_buf + patch_address), (char *)&target_vaddr, 8);
        i = i + 1;
    }

    // Construct ELF binary header buffer
    char header_buf[120];
    k_memset(header_buf, 0, 120);

    // e_ident
    write_u8(header_buf, 0, 127);
    write_u8(header_buf, 1, 69); // 'E'
    write_u8(header_buf, 2, 76); // 'L'
    write_u8(header_buf, 3, 70); // 'F'
    write_u8(header_buf, 4, 2);  // ELFCLASS64
    write_u8(header_buf, 5, 1);  // ELFDATA2LSB
    write_u8(header_buf, 6, 1);  // EV_CURRENT

    // e_type, e_machine, e_version
    write_u16(header_buf, 16, 2);      // ET_EXEC
    write_u16(header_buf, 18, 0x3E);   // EM_X86_64
    write_u32(header_buf, 20, 1);      // EV_CURRENT

    // e_entry
    write_u64(header_buf, 24, text_start_vaddr + main_offset);
    // e_phoff
    write_u64(header_buf, 32, 64);

    // sizes
    write_u16(header_buf, 52, 64); // e_ehsize
    write_u16(header_buf, 54, 56); // e_phentsize
    write_u16(header_buf, 56, 1);  // e_phnum

    // Phdr segment: p_type, p_flags
    write_u32(header_buf, 64, 1);  // PT_LOAD
    write_u32(header_buf, 68, 7);  // PF_R | PF_W | PF_X

    // Phdr segment: offsets & addresses
    write_u64(header_buf, 72, 0);          // p_offset
    write_u64(header_buf, 80, base_vaddr); // p_vaddr
    write_u64(header_buf, 88, base_vaddr); // p_paddr

    // Phdr segment: sizes
    unsigned long total_filesz = 120 + final_code_size + data_idx;
    write_u64(header_buf, 96, total_filesz);  // p_filesz
    write_u64(header_buf, 104, total_filesz); // p_memsz
    write_u64(header_buf, 112, 4096);         // p_align

    // Open and write to output file
    int out_fd = sys_open("/apps/bin/demo.elf", 1);
    if (out_fd < 0) {
        print_str("Error: Could not open output file /apps/bin/demo.elf\n");
        sys_exit();
    }

    sys_write(out_fd, header_buf, 120);
    sys_write(out_fd, (char *)code_buf, final_code_size);
    sys_write(out_fd, (char *)data_buf, data_idx);
    sys_close(out_fd);

    print_str("Compilation Success! Created executable /apps/bin/demo.elf\n");
    sys_exit();
}
