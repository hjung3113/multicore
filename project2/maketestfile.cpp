#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <omp.h>
#include <cstring>
#include <random>

using namespace std;

char buf[100];

int main(int argc, char* argv[])
{
    if(argc != 2){
        fprintf(stderr, "exe, <filelen>\n");
        return(EXIT_FAILURE);
    }
    int len = atoi(argv[1]);
    srand(time(NULL));
    FILE* fout = fopen("test.txt", "wt");
    for(int i=0; i<len; i++){
        int wordlen = rand() % 15 + 10;
        int j=0;
        for(j=0; j<wordlen; j++){
            int mod = rand() % 3;
            char c = 0;
            switch(mod)
            {
                case 0 :
                    c = rand()%('Z'-'A') + 'A';
                    break;
                case 1 :
                    c = rand()%('z'-'a') + 'a';
                    break;                
                case 2 :
                    c = rand()%10 + '0';
                    break;                
                default :
                    break;
            }
            buf[j] = c;
        }
        buf[j] = 0;
        if(i!=len-1)fprintf(fout, "%s\n", buf);
        else fprintf(fout, "%s", buf);
    }
    fclose(fout);
    return 0;
}