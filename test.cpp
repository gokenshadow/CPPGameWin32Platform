#include <stdio.h>
#include <stdint.h>

typedef int16_t int16;
int main() {   
    int number = 131070;
    number = number  * 48000;
    // displays output
    printf("The number is: %d", sizeof(int16)*2);
    
    return 0;
}

