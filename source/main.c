// Elevator project

#include <types.h>
#include <stdio.h>

int main(int arg_count, char** args) {
    printf("Heii\n");

    for (u32 i = 0; i < arg_count; i++) {
        printf("Arg => %s\n", args[i]);
    }
    return 0;
}
