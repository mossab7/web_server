#include <stdio.h>

int main() 
{
    setbuf(stdout, NULL);

    // Generate a large amount of output
    printf("Content-Type: text/plain\r\n\r\n");
    for (int i = 0; i < 500; ++i) {
        printf("Line %d: This is a test line to generate large output for CGI script testing.\n", i + 1);
    }
    return 0;
}