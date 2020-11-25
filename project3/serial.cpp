#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#define BUFSIZE 100

using namespace std;

bool **board;
int NumGenerate;
int lx, ly;
char buf[100];
vector <pair<int, int>> lives;

void printboard(){
    for(int i=1;i<=ly;i++){
        for(int j=1; j<=lx; j++){
            if(board[j][i] == false) printf(". ");
            else printf("0 ");
        }
        printf("\n");
    }
    printf("\n");
}

void updateboard(){
    bool backup[lx+2][ly+2] = {};
    bool visit[lx+2][ly+2] = {};
    for(int i=1;i<ly+1;i++){
        for(int j=1; j<lx+1; j++){
            if(board[j][i] == true) backup[j][i] = true;
        }
    }

    // for(int i=1;i<ly+1;i++){
    //     for(int j=1; j<lx+1; j++){

    //     }
    // }

    int nlives = lives.size();
    while(nlives--){
        pair<int, int> cur = lives.back();
        lives.pop_back();

        int x,y;
        x=cur.first;
        y=cur.second;

        for(int a=y-1; a<=y+1; a++){
            if(a<1 || a>ly) continue;
            for(int b=x-1; b<=x+1; b++){
                if(b<1 || b>lx) continue;
                if(visit[b][a] == true) continue;
                visit[b][a] = true;
                int alive = 0;
                for(int c=-1; c<=1; c++){
                    for(int d=-1; d<=1; d++){
                        if(c==0 && d==0) continue;
                        if(backup[b+d][a+c] == true) alive++;
                    }
                }

                if(board[b][a] == true){
                    if(alive < 2) board[b][a] = false;
                    if(alive > 3) board[b][a] = false;
                    else{
                        lives.push_back(make_pair(b,a));
                    }
                }else{
                    if(alive == 3) {
                        board[b][a] = true;
                        lives.push_back(make_pair(b,a));
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if(argc != 5){
        fprintf(stderr, "usage : exe file ng limit_x limity\n");
        exit(1);
    }
    string filename(argv[1]);
    NumGenerate = atoi(argv[2]);
    lx = atoi(argv[3]);
    ly = atoi(argv[4]);
    // bool board[lx+2][ly+2] = {};
    board = (bool **)calloc(sizeof(bool*), lx+2);
    for(int i=0;i<lx+2;i++){
        board[i] = (bool*)calloc(sizeof(bool), ly+2);
    }

    printf("%d %d\n", lx, ly);
    ifstream readfile(filename);
    if(readfile.is_open()){
        string line, _x, _y;

        while(getline(readfile, line)){
            int x,y;
            stringstream _s(line);

            getline(_s, _x, ' ');
            getline(_s, _y, ' ');

            x = stoi(_x);
            y = stoi(_y);

            board[x+1][y+1] = true;
            lives.push_back(make_pair(x+1,y+1));
        }
    }else {
        fprintf(stderr, "err : nofile\n");
        exit(1);
    }

    while(true){
        printboard();
        updateboard();
        usleep(200000);
    }

    for(int i=0;i<lx+2;i++) free(board[i]);
    free(board);

    return 0;
}
