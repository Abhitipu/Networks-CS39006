#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int mycat(char *buf, int bufsize, char *buf2)
{
    strcat(buf+bufsize, buf2);
    return bufsize + strlen(buf2) + 1;
}

int main() {
    char buf1[100] = "abcd";
    char buf2[] = "efgh";
    char buf3[] = "ijkl";
    char buf4[] = "\0";

    int cur = strlen(buf1) + 1;
    cur = mycat(buf1, cur, buf2);
    printf("%d ", cur);
    buf1[cur] = '\0';
    cur = mycat(buf1, cur, buf3);    
    printf("%d\n", cur);
    buf1[cur] = '\0';

    char *b = buf1;
    while(strlen(b) > 0)
    {
        printf("%s\n", b);
        b = b + strlen(b) + 1;
    }
    // buf1[cur] = '\0';
    return 0;
}