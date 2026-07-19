/**
 * Keira User Space: Keira C Compiler (kcc)
 *
 * A minimal single-pass compiler that compiles a subset of C into standalone
 * executable x86_64 ELF64 binaries directly on Keira.
 * Supports variables, basic arithmetic, comparisons, while loops, if-else, and printf.
 */

#include "../../lib/include/stdio.h"
#include "../../lib/include/string.h"
#include "../../lib/include/syscall.h"
#include "../../lib/include/malloc.h"

#define MAX_CODE_SIZE 8192
#define MAX_DATA_SIZE 4096
#define MAX_SOURCE_SIZE 16384

typedef struct {
    unsigned char e_ident[16];
    unsigned short e_type;
    unsigned short e_machine;
    unsigned int e_version;
    unsigned long e_entry;
    unsigned long e_phoff;
    unsigned long e_shoff;
    unsigned int e_flags;
    unsigned short e_ehsize;
    unsigned short e_phentsize;
    unsigned short e_phnum;
    unsigned short e_shentsize;
    unsigned short e_shnum;
    unsigned short e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    unsigned int p_type;
    unsigned int p_flags;
    unsigned long p_offset;
    unsigned long p_vaddr;
    unsigned long p_paddr;
    unsigned long p_filesz;
    unsigned long p_memsz;
    unsigned long p_align;
} Elf64_Phdr;

// Lexer tokens
typedef enum {
    TOK_EOF,
    TOK_INT,
    TOK_VOID,
    TOK_MAIN,
    TOK_PRINTF,
    TOK_RETURN,
    TOK_IDENT,
    TOK_NUM,
    TOK_STRING,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMICOLON,
    // New tokens
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_ASSIGN,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_LT,
    TOK_GT,
    TOK_EQ,
    TOK_NEQ,
    TOK_COMMA
} TokenType;

static const char *src_ptr = NULL;
static char token_string[256];
static int token_num = 0;

static void skip_whitespace(void) {
    while (*src_ptr == ' ' || *src_ptr == '\t' || *src_ptr == '\r' || *src_ptr == '\n' || *src_ptr == '#') {
        if (*src_ptr == '#') {
            // Skip preprocessor lines
            while (*src_ptr && *src_ptr != '\n') {
                src_ptr++;
            }
        } else {
            src_ptr++;
        }
    }
}

static TokenType next_token(void) {
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
                token_string[len++] = *src_ptr;
            }
            src_ptr++;
        }
        token_string[len] = '\0';

        if (strcmp(token_string, "int") == 0) return TOK_INT;
        if (strcmp(token_string, "void") == 0) return TOK_VOID;
        if (strcmp(token_string, "main") == 0) return TOK_MAIN;
        if (strcmp(token_string, "printf") == 0) return TOK_PRINTF;
        if (strcmp(token_string, "return") == 0) return TOK_RETURN;
        if (strcmp(token_string, "if") == 0) return TOK_IF;
        if (strcmp(token_string, "else") == 0) return TOK_ELSE;
        if (strcmp(token_string, "while") == 0) return TOK_WHILE;
        return TOK_IDENT;
    }

    // Numbers
    if (*src_ptr >= '0' && *src_ptr <= '9') {
        token_num = 0;
        while (*src_ptr >= '0' && *src_ptr <= '9') {
            token_num = token_num * 10 + (*src_ptr - '0');
            src_ptr++;
        }
        return TOK_NUM;
    }

    // Strings
    if (*src_ptr == '"') {
        src_ptr++; // skip '"'
        int len = 0;
        while (*src_ptr && *src_ptr != '"') {
            if (*src_ptr == '\\' && *(src_ptr + 1) == 'n') {
                token_string[len++] = '\n';
                src_ptr += 2;
            } else {
                token_string[len++] = *src_ptr++;
            }
        }
        if (*src_ptr == '"') {
            src_ptr++;
        }
        token_string[len] = '\0';
        return TOK_STRING;
    }

    char c = *src_ptr++;
    if (c == '(') return TOK_LPAREN;
    if (c == ')') return TOK_RPAREN;
    if (c == '{') return TOK_LBRACE;
    if (c == '}') return TOK_RBRACE;
    if (c == ';') return TOK_SEMICOLON;
    if (c == ',') return TOK_COMMA;
    if (c == '+') return TOK_PLUS;
    if (c == '-') return TOK_MINUS;
    if (c == '*') return TOK_STAR;
    if (c == '/') return TOK_SLASH;
    if (c == '<') return TOK_LT;
    if (c == '>') return TOK_GT;
    if (c == '=') {
        if (*src_ptr == '=') {
            src_ptr++;
            return TOK_EQ;
        }
        return TOK_ASSIGN;
    }
    if (c == '!') {
        if (*src_ptr == '=') {
            src_ptr++;
            return TOK_NEQ;
        }
    }

    return TOK_EOF;
}

// Compiler state & Symbol table
typedef struct {
    char name[32];
    int offset;
} Symbol;

static Symbol symbol_table[32];
static int symbol_count = 0;

static int lookup_symbol(const char *name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return symbol_table[i].offset;
        }
    }
    return 0;
}

static int add_symbol(const char *name) {
    int offset = lookup_symbol(name);
    if (offset != 0) return offset;
    
    symbol_count++;
    Symbol *sym = &symbol_table[symbol_count - 1];
    strcpy(sym->name, name);
    sym->offset = -4 * symbol_count;
    return sym->offset;
}

static TokenType tok;

static void match(TokenType expected) {
    if (tok == expected) {
        tok = next_token();
    } else {
        printf("Error: Expected token %d, got %d\n", expected, tok);
        sys_exit();
    }
}

// Expression parser & code generator
static void expression(unsigned char *code_buf, int *code_idx);

static void primary_expr(unsigned char *code_buf, int *code_idx) {
    if (tok == TOK_NUM) {
        // mov eax, token_num -> b8 [4 bytes]
        code_buf[(*code_idx)++] = 0xb8;
        unsigned int val = (unsigned int)token_num;
        memcpy(code_buf + *code_idx, &val, 4);
        *code_idx += 4;
        match(TOK_NUM);
    } else if (tok == TOK_IDENT) {
        int offset = lookup_symbol(token_string);
        if (offset == 0) {
            printf("Error: Undefined variable %s\n", token_string);
            sys_exit();
        }
        // mov eax, [rbp + offset] -> 8b 45 [1 byte offset]
        code_buf[(*code_idx)++] = 0x8b;
        code_buf[(*code_idx)++] = 0x45;
        code_buf[(*code_idx)++] = (unsigned char)offset;
        match(TOK_IDENT);
    } else if (tok == TOK_LPAREN) {
        match(TOK_LPAREN);
        expression(code_buf, code_idx);
        match(TOK_RPAREN);
    } else {
        printf("Error: Invalid primary expression, got token %d\n", tok);
        sys_exit();
    }
}

static void mul_expr(unsigned char *code_buf, int *code_idx) {
    primary_expr(code_buf, code_idx);
    while (tok == TOK_STAR || tok == TOK_SLASH) {
        TokenType op = tok;
        match(op);
        // push rax -> 50
        code_buf[(*code_idx)++] = 0x50;
        primary_expr(code_buf, code_idx);
        // pop rcx -> 59
        code_buf[(*code_idx)++] = 0x59;
        
        if (op == TOK_STAR) {
            // imul eax, ecx -> 0f af c1
            code_buf[(*code_idx)++] = 0x0f;
            code_buf[(*code_idx)++] = 0xaf;
            code_buf[(*code_idx)++] = 0xc1;
        } else {
            // xchg eax, ecx -> 91
            code_buf[(*code_idx)++] = 0x91;
            // cdq -> 99
            code_buf[(*code_idx)++] = 0x99;
            // idiv ecx -> f7 f9
            code_buf[(*code_idx)++] = 0xf7;
            code_buf[(*code_idx)++] = 0xf9;
        }
    }
}

static void add_expr(unsigned char *code_buf, int *code_idx) {
    mul_expr(code_buf, code_idx);
    while (tok == TOK_PLUS || tok == TOK_MINUS) {
        TokenType op = tok;
        match(op);
        // push rax -> 50
        code_buf[(*code_idx)++] = 0x50;
        mul_expr(code_buf, code_idx);
        // pop rcx -> 59
        code_buf[(*code_idx)++] = 0x59;
        
        if (op == TOK_PLUS) {
            // add eax, ecx -> 03 c1
            code_buf[(*code_idx)++] = 0x03;
            code_buf[(*code_idx)++] = 0xc1;
        } else {
            // xchg eax, ecx -> 91
            code_buf[(*code_idx)++] = 0x91;
            // sub eax, ecx -> 2b c1
            code_buf[(*code_idx)++] = 0x2b;
            code_buf[(*code_idx)++] = 0xc1;
        }
    }
}

static void expression(unsigned char *code_buf, int *code_idx) {
    add_expr(code_buf, code_idx);
    while (tok == TOK_LT || tok == TOK_GT || tok == TOK_EQ || tok == TOK_NEQ) {
        TokenType op = tok;
        match(op);
        // push rax -> 50
        code_buf[(*code_idx)++] = 0x50;
        add_expr(code_buf, code_idx);
        // pop rcx -> 59
        code_buf[(*code_idx)++] = 0x59;
        
        // cmp ecx, eax -> 39 c1
        code_buf[(*code_idx)++] = 0x39;
        code_buf[(*code_idx)++] = 0xc1;
        
        if (op == TOK_LT) {
            // setl al -> 0f 9c c0
            code_buf[(*code_idx)++] = 0x0f;
            code_buf[(*code_idx)++] = 0x9c;
            code_buf[(*code_idx)++] = 0xc0;
        } else if (op == TOK_GT) {
            // setg al -> 0f 9f c0
            code_buf[(*code_idx)++] = 0x0f;
            code_buf[(*code_idx)++] = 0x9f;
            code_buf[(*code_idx)++] = 0xc0;
        } else if (op == TOK_EQ) {
            // sete al -> 0f 94 c0
            code_buf[(*code_idx)++] = 0x0f;
            code_buf[(*code_idx)++] = 0x94;
            code_buf[(*code_idx)++] = 0xc0;
        } else {
            // setne al -> 0f 95 c0
            code_buf[(*code_idx)++] = 0x0f;
            code_buf[(*code_idx)++] = 0x95;
            code_buf[(*code_idx)++] = 0xc0;
        }
        // movzx eax, al -> 0f b6 c0
        code_buf[(*code_idx)++] = 0x0f;
        code_buf[(*code_idx)++] = 0xb6;
        code_buf[(*code_idx)++] = 0xc0;
    }
}

// Statement and block parser & code generator
static void block(unsigned char *code_buf, int *code_idx, unsigned char *data_buf, int *data_idx);

static void statement(unsigned char *code_buf, int *code_idx, unsigned char *data_buf, int *data_idx) {
    if (tok == TOK_LBRACE) {
        match(TOK_LBRACE);
        block(code_buf, code_idx, data_buf, data_idx);
        match(TOK_RBRACE);
    } else if (tok == TOK_INT) {
        match(TOK_INT);
        char var_name[256];
        strcpy(var_name, token_string);
        match(TOK_IDENT);
        
        int offset = add_symbol(var_name);
        if (tok == TOK_ASSIGN) {
            match(TOK_ASSIGN);
            expression(code_buf, code_idx);
        } else {
            // Default initialize to 0: mov eax, 0
            code_buf[(*code_idx)++] = 0xb8;
            unsigned int zero = 0;
            memcpy(code_buf + *code_idx, &zero, 4);
            *code_idx += 4;
        }
        // mov [rbp + offset], eax -> 89 45 [offset]
        code_buf[(*code_idx)++] = 0x89;
        code_buf[(*code_idx)++] = 0x45;
        code_buf[(*code_idx)++] = (unsigned char)offset;
        match(TOK_SEMICOLON);
    } else if (tok == TOK_IDENT) {
        char var_name[256];
        strcpy(var_name, token_string);
        match(TOK_IDENT);
        
        int offset = lookup_symbol(var_name);
        if (offset == 0) {
            printf("Error: Undefined variable %s\n", var_name);
            sys_exit();
        }
        match(TOK_ASSIGN);
        expression(code_buf, code_idx);
        // mov [rbp + offset], eax -> 89 45 [offset]
        code_buf[(*code_idx)++] = 0x89;
        code_buf[(*code_idx)++] = 0x45;
        code_buf[(*code_idx)++] = (unsigned char)offset;
        match(TOK_SEMICOLON);
    } else if (tok == TOK_IF) {
        match(TOK_IF);
        match(TOK_LPAREN);
        expression(code_buf, code_idx);
        match(TOK_RPAREN);
        
        // test eax, eax -> 85 c0
        code_buf[(*code_idx)++] = 0x85;
        code_buf[(*code_idx)++] = 0xc0;
        
        // jz else_label -> 0f 84 [4 bytes relative offset]
        code_buf[(*code_idx)++] = 0x0f;
        code_buf[(*code_idx)++] = 0x84;
        int jz_offset_idx = *code_idx;
        *code_idx += 4; // reserve space
        
        statement(code_buf, code_idx, data_buf, data_idx);
        
        if (tok == TOK_ELSE) {
            match(TOK_ELSE);
            // jmp end_label -> e9 [4 bytes relative offset]
            code_buf[(*code_idx)++] = 0xe9;
            int jmp_offset_idx = *code_idx;
            *code_idx += 4; // reserve space
            
            // Patch else_label to point here
            int else_offset = *code_idx - (jz_offset_idx + 4);
            memcpy(code_buf + jz_offset_idx, &else_offset, 4);
            
            statement(code_buf, code_idx, data_buf, data_idx);
            
            // Patch end_label to point here
            int end_offset = *code_idx - (jmp_offset_idx + 4);
            memcpy(code_buf + jmp_offset_idx, &end_offset, 4);
        } else {
            // Patch else_label (which is end_label) to point here
            int else_offset = *code_idx - (jz_offset_idx + 4);
            memcpy(code_buf + jz_offset_idx, &else_offset, 4);
        }
    } else if (tok == TOK_WHILE) {
        int start_addr = *code_idx;
        match(TOK_WHILE);
        match(TOK_LPAREN);
        expression(code_buf, code_idx);
        match(TOK_RPAREN);
        
        // test eax, eax -> 85 c0
        code_buf[(*code_idx)++] = 0x85;
        code_buf[(*code_idx)++] = 0xc0;
        
        // jz end_label -> 0f 84 [4 bytes relative offset]
        code_buf[(*code_idx)++] = 0x0f;
        code_buf[(*code_idx)++] = 0x84;
        int jz_offset_idx = *code_idx;
        *code_idx += 4; // reserve space
        
        statement(code_buf, code_idx, data_buf, data_idx);
        
        // jmp start_addr -> e9 [4 bytes relative offset]
        code_buf[(*code_idx)++] = 0xe9;
        int jump_back = start_addr - (*code_idx + 4);
        memcpy(code_buf + *code_idx, &jump_back, 4);
        *code_idx += 4;
        
        // Patch end_label to point here
        int end_offset = *code_idx - (jz_offset_idx + 4);
        memcpy(code_buf + jz_offset_idx, &end_offset, 4);
    } else if (tok == TOK_PRINTF) {
        match(TOK_PRINTF);
        match(TOK_LPAREN);
        
        // Store string literal into data segment
        int str_len = (int)strlen(token_string) + 1; // include null terminator
        if (*data_idx + str_len >= MAX_DATA_SIZE) {
            printf("Error: Data segment overflow\n");
            sys_exit();
        }
        int str_offset = *data_idx;
        memcpy(data_buf + *data_idx, token_string, str_len);
        *data_idx += str_len;
        match(TOK_STRING);
        match(TOK_RPAREN);
        match(TOK_SEMICOLON);

        // Emit print string sequence:
        // mov rsi, imm64 (10 bytes) -> 48 be [8 bytes address]
        code_buf[(*code_idx)++] = 0x48;
        code_buf[(*code_idx)++] = 0xbe;
        unsigned long temp_offset = (unsigned long)str_offset;
        memcpy(code_buf + *code_idx, &temp_offset, 8);
        *code_idx += 8;

        // movzx edi, byte ptr [rsi] (3 bytes) -> 0f b6 3e
        code_buf[(*code_idx)++] = 0x0f;
        code_buf[(*code_idx)++] = 0xb6;
        code_buf[(*code_idx)++] = 0x3e;

        // test edi, edi (2 bytes) -> 85 ff
        code_buf[(*code_idx)++] = 0x85;
        code_buf[(*code_idx)++] = 0xff;

        // jz +12 (2 bytes) -> 74 0c
        code_buf[(*code_idx)++] = 0x74;
        code_buf[(*code_idx)++] = 0x0c;

        // mov eax, 1 (5 bytes) -> b8 01 00 00 00 (sys_print_char syscall number)
        code_buf[(*code_idx)++] = 0xb8;
        code_buf[(*code_idx)++] = 0x01;
        code_buf[(*code_idx)++] = 0x00;
        code_buf[(*code_idx)++] = 0x00;
        code_buf[(*code_idx)++] = 0x00;

        // syscall (2 bytes) -> 0f 05
        code_buf[(*code_idx)++] = 0x0f;
        code_buf[(*code_idx)++] = 0x05;

        // inc rsi (3 bytes) -> 48 ff c6
        code_buf[(*code_idx)++] = 0x48;
        code_buf[(*code_idx)++] = 0xff;
        code_buf[(*code_idx)++] = 0xc6;

        // jmp -25 (2 bytes) -> eb ed
        code_buf[(*code_idx)++] = 0xeb;
        code_buf[(*code_idx)++] = 0xed;
    } else if (tok == TOK_RETURN) {
        match(TOK_RETURN);
        expression(code_buf, code_idx);
        match(TOK_SEMICOLON);
    } else {
        printf("Error: Unexpected token in statement: %d\n", tok);
        sys_exit();
    }
}

static void block(unsigned char *code_buf, int *code_idx, unsigned char *data_buf, int *data_idx) {
    while (tok != TOK_RBRACE && tok != TOK_EOF) {
        statement(code_buf, code_idx, data_buf, data_idx);
    }
}

void _start(void) {
    printf("Keira C Compiler (kcc) v0.13.0\n");
    printf("Compiling source: /apps/src/demo.c -> /apps/bin/demo.elf\n");

    // Open source file
    int in_fd = sys_open("/apps/src/demo.c", 0);
    if (in_fd < 0) {
        printf("Error: Could not open /apps/src/demo.c\n");
        sys_exit();
    }

    char *src_buf = (char *)malloc(MAX_SOURCE_SIZE);
    if (src_buf == NULL) {
        printf("Error: Memory allocation failed for source buffer\n");
        sys_close(in_fd);
        sys_exit();
    }
    memset(src_buf, 0, MAX_SOURCE_SIZE);

    int read_len = sys_read(in_fd, src_buf, MAX_SOURCE_SIZE - 1);
    sys_close(in_fd);
    if (read_len <= 0) {
        printf("Error: Read empty or failed for /apps/src/demo.c\n");
        free(src_buf);
        sys_exit();
    }

    src_ptr = src_buf;

    // Output buffers
    unsigned char *code_buf = (unsigned char *)malloc(MAX_CODE_SIZE);
    unsigned char *data_buf = (unsigned char *)malloc(MAX_DATA_SIZE);
    if (code_buf == NULL || data_buf == NULL) {
        printf("Error: Output buffers allocation failed\n");
        if (code_buf) free(code_buf);
        if (data_buf) free(data_buf);
        free(src_buf);
        sys_exit();
    }
    memset(code_buf, 0, MAX_CODE_SIZE);
    memset(data_buf, 0, MAX_DATA_SIZE);

    int code_idx = 0;
    int data_idx = 0;

    // Emitting standard prologue:
    // push rbp
    code_buf[code_idx++] = 0x55;
    // mov rbp, rsp
    code_buf[code_idx++] = 0x48;
    code_buf[code_idx++] = 0x89;
    code_buf[code_idx++] = 0xe5;
    // sub rsp, 128 (to support up to 32 local variables)
    code_buf[code_idx++] = 0x48;
    code_buf[code_idx++] = 0x81;
    code_buf[code_idx++] = 0xec;
    code_buf[code_idx++] = 0x80;
    code_buf[code_idx++] = 0x00;
    code_buf[code_idx++] = 0x00;
    code_buf[code_idx++] = 0x00;

    tok = next_token();
    while (tok != TOK_EOF) {
        if (tok == TOK_INT || tok == TOK_VOID) {
            tok = next_token();
            if (tok == TOK_MAIN) {
                match(TOK_MAIN);
                match(TOK_LPAREN);
                match(TOK_RPAREN);
                match(TOK_LBRACE);
                block(code_buf, &code_idx, data_buf, &data_idx);
                match(TOK_RBRACE);
                break; // Compiled main function successfully
            }
        } else {
            tok = next_token();
        }
    }

    // Emitting standard epilogue:
    // mov rsp, rbp
    code_buf[code_idx++] = 0x48;
    code_buf[code_idx++] = 0x89;
    code_buf[code_idx++] = 0xec;
    // pop rbp
    code_buf[code_idx++] = 0x5d;
    // sys_exit:
    // mov eax, 2
    code_buf[code_idx++] = 0xb8;
    code_buf[code_idx++] = 0x02;
    code_buf[code_idx++] = 0x00;
    code_buf[code_idx++] = 0x00;
    code_buf[code_idx++] = 0x00;
    // syscall
    code_buf[code_idx++] = 0x0f;
    code_buf[code_idx++] = 0x05;

    // Patch string absolute virtual addresses
    unsigned long final_code_size = (unsigned long)code_idx;
    unsigned long base_vaddr = 0x40000000;
    unsigned long headers_size = 120;
    unsigned long text_start_vaddr = base_vaddr + headers_size;
    unsigned long data_start_vaddr = text_start_vaddr + final_code_size;

    int scan_idx = 0;
    while (scan_idx < code_idx) {
        if (code_buf[scan_idx] == 0x48 && code_buf[scan_idx + 1] == 0xbe) {
            unsigned long str_offset;
            memcpy(&str_offset, code_buf + scan_idx + 2, 8);
            unsigned long target_vaddr = data_start_vaddr + str_offset;
            memcpy(code_buf + scan_idx + 2, &target_vaddr, 8);
            scan_idx += 10;
        } else {
            scan_idx++;
        }
    }

    // Package into ELF format
    Elf64_Ehdr ehdr;
    memset(&ehdr, 0, sizeof(ehdr));
    ehdr.e_ident[0] = 0x7F;
    ehdr.e_ident[1] = 'E';
    ehdr.e_ident[2] = 'L';
    ehdr.e_ident[3] = 'F';
    ehdr.e_ident[4] = 2; // ELFCLASS64
    ehdr.e_ident[5] = 1; // ELFDATA2LSB
    ehdr.e_ident[6] = 1; // EV_CURRENT
    ehdr.e_type = 2;     // ET_EXEC
    ehdr.e_machine = 0x3E; // EM_X86_64
    ehdr.e_version = 1;
    ehdr.e_entry = text_start_vaddr;
    ehdr.e_phoff = sizeof(Elf64_Ehdr);
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_phentsize = sizeof(Elf64_Phdr);
    ehdr.e_phnum = 1;

    Elf64_Phdr phdr;
    memset(&phdr, 0, sizeof(phdr));
    phdr.p_type = 1; // PT_LOAD
    phdr.p_flags = 7; // PF_R | PF_W | PF_X
    phdr.p_offset = 0;
    phdr.p_vaddr = base_vaddr;
    phdr.p_paddr = base_vaddr;
    phdr.p_filesz = headers_size + final_code_size + data_idx;
    phdr.p_memsz = phdr.p_filesz;
    phdr.p_align = 4096;

    // Create output executable file
    int out_fd = sys_open("/apps/bin/demo.elf", 1);
    if (out_fd < 0) {
        printf("Error: Could not open output file /apps/bin/demo.elf\n");
        goto cleanup;
    }

    sys_write(out_fd, &ehdr, sizeof(ehdr));
    sys_write(out_fd, &phdr, sizeof(phdr));
    sys_write(out_fd, code_buf, final_code_size);
    sys_write(out_fd, data_buf, data_idx);
    sys_close(out_fd);

    printf("Compilation Success! Created executable /apps/bin/demo.elf\n");

cleanup:
    free(code_buf);
    free(data_buf);
    free(src_buf);
    sys_exit();
}
