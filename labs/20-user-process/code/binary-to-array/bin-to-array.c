#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "libunix.h"

int main(int argc, char *argv[]) {
    if(argc != 2)
        die("invalid number of arguments: expected 2, have %d\n", argc);

    char *p = argv[1];
    unsigned nbytes;
    uint32_t *u = (void*)read_file(&nbytes, p);
    fprintf(stderr, "bin-to-array: nbytes=%d\n", nbytes);

    unsigned n = nbytes / 4 + 1;

    // replace non-alpha and non-digit with '_'
    printf("unsigned code_init[] = {\n\t");
    for(int i = 0; i < n; i++) {
        if(i && i % 6 == 0)
            printf("\n\t");
        printf("0x%.8x,", u[i]);
    }
    printf("\n};\n");
    return 0;
}
