#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <string>
#include <sstream>

#define BUFSIZE 100

using namespace std;
bool **board;
int lx = 10;
int ly = 10;
int main(){    
    board = (bool **)calloc(sizeof(bool*), lx+2);
    for(int i=0;i<lx+2;i++){
        board[i] = (bool*)calloc(sizeof(bool), ly+2);
    }

    board[5][2] = true;
    board[1][3] = true;
    board[4][2] = true;
    board[6][2] = true;

    for(int i=0;i<ly+2;i++){
        for(int j=0; j<lx+2; j++){
            if(board[j][i] == false) printf(". ");
            else printf("0 ");
        }
        printf("\n");
    }
    for(int i=0;i<lx+2;i++) free(board[i]);
    free(board);
}