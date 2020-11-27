#include <mpi.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <queue>
#include <cmath>
#include <cstring>

using namespace std;

#define getcol(ID,s) ((ID)%(s))
#define getrow(ID,s) ((ID)/(s))

struct pData{
	char **map1;
	char **map2;
	int pID;
	int rowS;
	int rowE;
	int colS;
	int colE;
};

char **MakeMap(int rowLen, int colLen)
{
	char **array;
	char * Daddr;

	array = (char **)malloc(rowLen*sizeof(char*)+rowLen*colLen*sizeof(char));
	Daddr = (char *)(array + rowLen);

	memset(array, 0, sizeof(array));

	for(int i=0;i<rowLen;i++)
	{
		array[i] = Daddr + i*colLen;
	}

	return array;
}

pData **Init_p2d(int sideLen, int bc, int rc, int br, int rr)
{
	pData **array;
	pData * Daddr;
	int index = 0;
	int colLens[sideLen] = {};
	int rowLens[sideLen] = {};
	int cA=0, rA=0;
	int colAcc[sideLen] = {};
	int rowAcc[sideLen] = {};

	array = (pData **)malloc(sideLen*sizeof(pData*)+sideLen*sideLen*sizeof(pData));
	Daddr = (pData *)(array + sideLen);

	for(int i=0;i<sideLen;i++)
	{
		array[i] = Daddr + i*sideLen;
	}
	

	for(int i=0; i<sideLen; i++)
		for(int j=0; j<sideLen; j++)
			array[i][j].pID = index++;
	
	for(int i=0; i<sideLen; i++)
	{
		colLens[i] = bc;
		rowLens[i] = br;
	}
	
	if(rc > 0)
	{
		int idx = 0;
		int diff = sideLen-1;
		while(rc--)
		{
			colLens[idx]++;
			idx += diff;
			diff = (abs(diff)-1)*(-1);
		}
	}

	if(rr > 0)
	{
		int idx = 0;
		int diff = sideLen-1;
		while(rr--)
		{
			rowLens[idx]++;
			idx += diff;
			diff = (abs(diff)-1)*(-1);
		}
	}

	for(int i=0;i<sideLen-1;i++)
	{
		cA+=colLens[i];
		rA+=rowLens[i];
		colAcc[i+1] = cA;
		rowAcc[i+1] = rA;
	}

	for(int i=0;i<sideLen;i++)
	{
		for(int j=0;j<sideLen;j++){
			array[i][j].colS = colAcc[j];
			array[i][j].colE = colAcc[j]+colLens[j];
			array[i][j].rowS = rowAcc[i];
			array[i][j].rowE = rowAcc[i]+rowLens[i];
		}
	}
	for(int i=0;i<sideLen;i++){
		for(int j=0;j<sideLen;j++){
			array[i][j].map1 = MakeMap(array[i][j].rowE - array[i][j].rowS, array[i][j].colE-array[i][j].colS);
			array[i][j].map2 = MakeMap(array[i][j].rowE - array[i][j].rowS, array[i][j].colE-array[i][j].colS);

		}
	}

	return array;
}



void free_p2d(pData **p2d, int sideLen)
{
	for(int i=0;i<sideLen;i++){
		for(int j=0;j<sideLen;j++){
			free(p2d[i][j].map1);
			free(p2d[i][j].map2);
		}
	}
	free(p2d);
}

int DetermineP(int p[2], int sideLen, pData **p2d)
{
	for(int i=0;i<sideLen;i++){
		if(!(p[0] < p2d[i][0].rowE && p[0] >= p2d[i][0].rowS)) continue;
		for(int j=0;j<sideLen;j++){
			if(p[1] < p2d[i][j].colE && p[1] >= p2d[i][j].colS) return p2d[i][j].pID;
		}
	}
}

int main(int argc, char *argv[]){
	pData ** p2d;
	p2d = Init_p2d(4, 2, 2, 2, 2);
	string filename(argv[1]);
	ifstream readfile(filename);
	int sideLen = 4;
	if(readfile.is_open()){
		string line, _x, _y;

		while(getline(readfile, line)){
			int p[2];
			stringstream _s(line);

			getline(_s, _x, ' ');
			getline(_s, _y, ' ');

			p[1]=stoi(_x);
			p[0]=stoi(_y);
			
			int id = DetermineP(p, sideLen, p2d);

			int col = getcol(id, sideLen);
			int row = getrow(id, sideLen);

			char ** map = p2d[row][col].map1;
			int cIdx = p2d[row][col].colS;
			int rIdx = p2d[row][col].rowS;
			
			map[p[0]-rIdx][p[1]-cIdx]=1;
		}
	}else{
		fprintf(stderr, "error: readfile: no file\n");
		free_p2d(p2d, sideLen);
		exit(1);
	}

	for(int i=0;i<4;i++){
		for(int j=0;j<4;j++){
			char ** map = p2d[i][j].map1;
			int xlen = p2d[i][j].colE-p2d[i][j].colS;
			int ylen = p2d[i][j].rowE-p2d[i][j].rowS;
			for(int a=0;a<ylen;a++){
				for(int b=0;b<xlen;b++){
					if(map[a][b] == 0) printf(".");
					else printf("0");
				}
				printf("\n");
			}
			printf("\n");
		}
		printf("\n");
	}

//	for(int i=0;i<4; i++){
//		for(int j=0;j<4;j++){
//			printf("pID : %d, rowS : %d, rowE : %d, colS : %d, colE : %d\n", datas[i][j].pID, datas[i][j].rowS, datas[i][j].rowE, datas[i][j].colS, datas[i][j].colE);
//		}
//		printf("\n");
//	}
//	int a[2] = {17, 19};
//	int id = DetermineP(a, 4, datas);
//	printf("(y:%d, x:%d), id : %d\n", a[0],a[1],id);
	free_p2d(p2d, 4);
	return 0;
}
