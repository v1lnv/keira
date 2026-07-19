#include "../../lib/include/stdio.h"

int main() {
    int x = 5;
    int y = 10;
    
    if (x < y) {
        printf("x is less than y\n");
    } else {
        printf("x is not less than y\n");
    }

    int i = 0;
    while (i < 3) {
        printf("Loop iteration in compiled program...\n");
        i = i + 1;
    }

    printf("Arithmetic check: 5 * 2 + 10 = 20\n");
    return 0;
}
