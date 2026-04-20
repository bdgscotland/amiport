#include <stdio.h>

int square(int x)
{
    int result = x * x;
    return result;
}

int main(int argc, char **argv)
{
    int n = 5;
    int s = square(n);
    printf("square(%d) = %d\n", n, s);
    return 0;
}
