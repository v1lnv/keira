/**
 * Keira User Space: Keira C Compiler (kcc)
 *
 * A minimal single-pass compiler that compiles a subset of C into standalone
 * executable x86_64 ELF64 binaries directly on Keira.
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
    TOK_SEMICOLON
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

    return TOK_EOF;
}

void _start(void) {
    // We expect arguments to be passed but since we have a simple exec without argv,
    // we read from static inputs or configuration file.
    // For Keira self-hosting verification, we will compile `/apps/src/demo.c`
    // and write to `/apps/bin/demo.elf`.
    printf("Keira C Compiler (kcc) v0.11.1\n");
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

    TokenType tok;
    while ((tok = next_token()) != TOK_EOF) {
        if (tok == TOK_INT || tok == TOK_VOID) {
            tok = next_token();
            if (tok == TOK_MAIN) {
                // main() signature
                if (next_token() != TOK_LPAREN || next_token() != TOK_RPAREN || next_token() != TOK_LBRACE) {
                    printf("Error: Invalid main function syntax\n");
                    goto cleanup;
                }

                // Parse function body
                while ((tok = next_token()) != TOK_RBRACE && tok != TOK_EOF) {
                    if (tok == TOK_PRINTF) {
                        if (next_token() != TOK_LPAREN || next_token() != TOK_STRING) {
                            printf("Error: printf expects string argument\n");
                            goto cleanup;
                        }

                        // Store string literal into data segment
                        int str_len = (int)strlen(token_string) + 1; // include null terminator
                        if (data_idx + str_len >= MAX_DATA_SIZE) {
                            printf("Error: Data segment overflow\n");
                            goto cleanup;
                        }
                        int str_offset = data_idx;
                        memcpy(data_buf + data_idx, token_string, str_len);
                        data_idx += str_len;

                        if (next_token() != TOK_RPAREN || next_token() != TOK_SEMICOLON) {
                            printf("Error: Expected ); after printf\n");
                            goto cleanup;
                        }

                        // Emit print string sequence:
                        // The base address of user space executables is 0x40000000.
                        // ELF header (64 bytes) + Program Header (56 bytes) = 120 bytes.
                        // The text segment starts at 0x40000000 + 120 = 0x40000078.
                        // Let's defer string address calculation until we know code size,
                        // or we can calculate it relative to the end of the text segment.
                        // Since data segment is appended directly after the code segment:
                        // String VAddr = 0x40000000 + 120 + final_code_size + str_offset.
                        // We will write dummy address bytes and patch it later!

                        // mov rsi, imm64 (10 bytes) -> 48 be [8 bytes address]
                        code_buf[code_idx++] = 0x48;
                        code_buf[code_idx++] = 0xbe;
                        // Store the str_offset temporarily in the address slot for patching
                        unsigned long temp_offset = (unsigned long)str_offset;
                        memcpy(code_buf + code_idx, &temp_offset, 8);
                        code_idx += 8;

                        // movzx edi, byte ptr [rsi] (3 bytes) -> 0f b6 3e
                        code_buf[code_idx++] = 0x0f;
                        code_buf[code_idx++] = 0xb6;
                        code_buf[code_idx++] = 0x3e;

                        // test edi, edi (2 bytes) -> 85 ff
                        code_buf[code_idx++] = 0x85;
                        code_buf[code_idx++] = 0xff;

                        // jz +12 (2 bytes) -> 74 0c
                        code_buf[code_idx++] = 0x74;
                        code_buf[code_idx++] = 0x0c;

                        // mov eax, 1 (5 bytes) -> b8 01 00 00 00 (sys_print_char syscall number)
                        code_buf[code_idx++] = 0xb8;
                        code_buf[code_idx++] = 0x01;
                        code_buf[code_idx++] = 0x00;
                        code_buf[code_idx++] = 0x00;
                        code_buf[code_idx++] = 0x00;

                        // syscall (2 bytes) -> 0f 05
                        code_buf[code_idx++] = 0x0f;
                        code_buf[code_idx++] = 0x05;

                        // inc rsi (3 bytes) -> 48 ff c6
                        code_buf[code_idx++] = 0x48;
                        code_buf[code_idx++] = 0xff;
                        code_buf[code_idx++] = 0xc6;

                        // jmp -25 (2 bytes) -> eb ed (jump back to movzx)
                        code_buf[code_idx++] = 0xeb;
                        code_buf[code_idx++] = 0xed;
                    } else if (tok == TOK_RETURN) {
                        if (next_token() != TOK_NUM || next_token() != TOK_SEMICOLON) {
                            printf("Error: return expects simple number expression\n");
                            goto cleanup;
                        }
                        // mov eax, token_num (5 bytes) -> b8 [4 bytes num]
                        code_buf[code_idx++] = 0xb8;
                        unsigned int ret_val = (unsigned int)token_num;
                        memcpy(code_buf + code_idx, &ret_val, 4);
                        code_idx += 4;
                    }
                }
                break; // Handled main function
            }
        }
    }

    // Emitting standard epilogue:
    // pop rbp
    code_buf[code_idx++] = 0x5d;
    // sys_exit (5 bytes mov eax, 2 + 2 bytes syscall)
    // mov eax, 2
    code_buf[code_idx++] = 0xb8;
    code_buf[code_idx++] = 0x02;
    code_buf[code_idx++] = 0x00;
    code_buf[code_idx++] = 0x00;
    code_buf[code_idx++] = 0x00;
    // syscall
    code_buf[code_idx++] = 0x0f;
    code_buf[code_idx++] = 0x05;

    // Patch the string addresses in the emitted code now that code_idx is final
    unsigned long final_code_size = (unsigned long)code_idx;
    unsigned long base_vaddr = 0x40000000;
    unsigned long headers_size = 120;
    unsigned long text_start_vaddr = base_vaddr + headers_size;
    unsigned long data_start_vaddr = text_start_vaddr + final_code_size;

    int scan_idx = 0;
    while (scan_idx < code_idx) {
        if (code_buf[scan_idx] == 0x48 && code_buf[scan_idx + 1] == 0xbe) {
            // Patch target found! Read the temporary offset from the next 8 bytes
            unsigned long str_offset;
            memcpy(&str_offset, code_buf + scan_idx + 2, 8);
            unsigned long target_vaddr = data_start_vaddr + str_offset;
            // Write the correct absolute virtual address
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

    // Create the output file `/apps/bin/demo.elf`
    // Open in write-mode (creates if doesn't exist)
    int out_fd = sys_open("/apps/bin/demo.elf", 1);
    if (out_fd < 0) {
        printf("Error: Could not open output file /apps/bin/demo.elf\n");
        goto cleanup;
    }

    // Write Elf Header
    sys_write(out_fd, &ehdr, sizeof(ehdr));
    // Write Program Header
    sys_write(out_fd, &phdr, sizeof(phdr));
    // Write Code Segment
    sys_write(out_fd, code_buf, final_code_size);
    // Write Data Segment
    sys_write(out_fd, data_buf, data_idx);

    sys_close(out_fd);
    printf("Compilation Success! Created executable /apps/bin/demo.elf\n");

cleanup:
    free(code_buf);
    free(data_buf);
    free(src_buf);
    sys_exit();
}
