#include <mpi.h>
#include <cstdlib>
#include <cstdio>
//#include <iostream>
#include <unistd.h>
//#include <fstream>
//#include <string>
//#include <sstream>
//#include <vector>
//#include <queue>
//#include <cmath>
#include <cstring>

using namespace std;

#define getcol(ID,s) ((ID)%(s))
#define getrow(ID,s) ((ID)/(s))
#define getpid(c,r,s) ((s)*(r)+(c))

struct pData{
	char **map[2];
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

	array = (char **)calloc(sizeof(char),rowLen*sizeof(char*)+rowLen*colLen*sizeof(char));
	Daddr = (char *)(array + rowLen);

//	memset(array, 0, sizeof(array)*(sizeof(char**)/sizeof(char)));

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
			array[i][j].map[0] = MakeMap(array[i][j].rowE - array[i][j].rowS, array[i][j].colE-array[i][j].colS);
			array[i][j].map[1] = MakeMap(array[i][j].rowE - array[i][j].rowS, array[i][j].colE-array[i][j].colS);
		}
	}

	return array;
}


void free_p2d(pData **p2d, int sideLen)
{
	for(int i=0;i<sideLen;i++){
		for(int j=0;j<sideLen;j++){
			free(p2d[i][j].map[0]);
			free(p2d[i][j].map[1]);
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

char judge(char **prev, int col, int row, int lives)
{
//	printf("in judge prev : %lx row : %d, col : %dn", prev, row, col);
	if(prev[row][col] == 0) {
		if(lives == 3) return 1;
		else return 0;
	}
	else
	{
		if(lives == 2 || lives == 3) return 1;
		else return 0;
	}

}

void update_cell_no_receive(char** prev, char** cur, int colLen, int rowLen)
{
//	printf("rowLen : %d, colLen : %d\n", rowLen, colLen);
//	printf("prev : %lx, cur : %lx\n", prev, cur);
	for(int i=1;i<rowLen-1;i++){
		for(int j=1; j<colLen-1;j++){
//			printf("-----------i:%d, j:%d---------------\n",i,j);
			int lives = 0;
			for(int a=i-1; a<=i+1;a++){
				for(int b=j-1; b<=j+1;b++){
					if(a==i && b==j) continue;
					if(prev[a][b] == 1) {
//						if(j == 1 && i==6) printf("live x : %d y : %d\n", b, a);
						lives++;
					}
				}
			}
//			printf("before judge i : %d, j : %d \n", i , j);
//			if(lives > 1) printf("x : %d, y :%d, lives : %d\n", j, i ,lives);
			cur[i][j] = judge(prev, j, i, lives);
//			printf("after judge\n");
		}
	}
}

int rowdist[8] = {-1, -1, -1, 0, 1, 1, 1, 0};
int coldist[8] = {-1, 0, 1, 1, 1, 0, -1, -1};

int cal_dest(int pcol, int prow, int tag, int sideLen)
{
	int destcol = pcol+coldist[tag];
	int destrow = prow+rowdist[tag];
	if(destrow < 0 || destcol < 0 || destrow >= sideLen || destcol >= sideLen) return -1;
	return getpid(destcol, destrow, sideLen);
}

bool send_bound(char** prev, int colLen, int rowLen, int count, MPI_Datatype type, int pcol, int prow, int tag, MPI_Comm comm, MPI_Request *request, int sideLen)
{
	int dest = cal_dest(pcol, prow, tag, sideLen);
	char buf[count];
	if(dest == -1){
		for(int i=0;i<count;i++) buf[i] = 0;
		return false;
	}
	switch(tag){
	case 0:
		buf[0] = prev[0][0];
		MPI_Isend(buf, count, type, dest, tag, comm, request);
		break;
	case 1:
		for(int i=0;i<count;i++)
			buf[i] = prev[0][i];
		MPI_Isend(buf, count, type, dest, tag, comm, request);
		break;
	case 2:
		buf[0] = prev[0][colLen-1];
		MPI_Isend(buf, count, type, dest, tag, comm, request);
		break;
	case 3:
		for(int i=0;i<count;i++)
			buf[i] = prev[i][colLen-1];
		MPI_Isend(buf, count, type, dest, tag, comm, request);
		break;
	case 4:
		buf[0] = prev[rowLen-1][colLen-1];
		MPI_Isend(buf, count, type, dest, tag, comm, request);
		break;
	case 5:
		for(int i=0;i<count;i++)
			buf[i] = prev[rowLen-1][i];
		MPI_Isend(buf, count, type, dest, tag, comm, request);
		break;
	case 6:
		buf[0] = prev[rowLen-1][0];
		MPI_Isend(buf, count, type, dest, tag, comm, request);
		break;
	case 7:
		for(int i=0;i<count;i++)
			buf[i] = prev[i][0];
		MPI_Isend(buf, count, type, dest, tag, comm, request);
		break;
	default : break;
	}
	return true;
}

bool request_bound(void *buf, int count, MPI_Datatype type, int pcol, int prow, int tag, MPI_Comm comm, MPI_Request *request, int sideLen)
{
	int dest = cal_dest(pcol, prow, (tag+4)%8, sideLen);

	if(dest == -1){
		return false;
	}
	switch(tag){
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		MPI_Irecv(buf, count, type, dest, tag, comm, request);
		break;
	default : break;
	}

	return true;
}

int main(int argc, char* argv[]){
	int rank, size;
	int sideLen;
	pData ** p2d;
	int *rowLens, *colLens;
	int baseRLen, remainRLen;
	int baseCLen, remainCLen;
	char* filename;
	FILE* infile;
	int Niter;
	int maxRow, maxCol;
	int pos=0;
	char **cur, **prev;
	int pcol, prow;
	int colLen, rowLen;
	pData *proc;
	int bound_lens[8];
	MPI_Request sendReq[8], receiveReq[8];
	MPI_Status receiveSt, sendSt;


	//printf("phase0\n");

	if(argc != 5)
	{
		fprintf(stderr, "mpiexec -np {nop} --machinefile hosts.txt ./exe inputfile.txt nog xlimit ylimit\n");
		return 1;
	}

	filename = argv[1];
	Niter = atoi(argv[2]);
	maxCol = atoi(argv[3]);
	maxRow = atoi(argv[4]);


	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
//	if(size == 1) {
//		serial(argc, argv);
//	}

	for(int i=1; i<100;i++){
		if(i*i == size){
			sideLen = i;
			break;
		}
	}
//	printf("sideLen : %d\n", sideLen);
	rowLens = (int *)malloc(sizeof(int)*sideLen);
	colLens = (int *)malloc(sizeof(int)*sideLen);

	baseCLen = maxCol / sideLen;
	remainCLen = maxCol % sideLen;
	baseRLen = maxRow / sideLen;
	remainRLen = maxRow % sideLen;

	p2d = Init_p2d(sideLen, baseCLen, remainCLen, baseRLen, remainRLen);
	prev = p2d[0][0].map[0];
/*	if(rank == 0){
		for(int i=0;i<12;i++){
			for(int j=0;j<12;j++){
				if(prev[i][j] == 0) printf(". ");
				else printf("0 ");
			}
			printf("\n");
		}
		printf("\n");
	}*/


	filename = argv[1];

//	ifstream readfile(filename);
//	printf("phase1\n");
	if(infile = fopen(filename, "r")){
		char coord_buf[100];
		char x_coord[50];
		char y_coord[50];
		while(fgets(coord_buf, 99, infile)!=NULL){
			int idx = 0, xidx = 0, yidx = 0;
			while(!(coord_buf[idx]<='9' && coord_buf[idx]>='0')) idx++;
			while(coord_buf[idx]<='9' && coord_buf[idx]>='0'){
				x_coord[xidx++] = coord_buf[idx++];
			}
			x_coord[xidx] = 0;
			while(!(coord_buf[idx]<='9' && coord_buf[idx]>='0')) idx++;
			while(coord_buf[idx]<='9' && coord_buf[idx]>='0'){
				y_coord[yidx++] = coord_buf[idx++];
			}
			y_coord[yidx] = 0;
			int p[2];
			p[0] = atoi(y_coord);
			p[1] = atoi(x_coord);

//			if(rank == 0) printf("p[0] : %d, p[1] : %d\n", p[0], p[1]);

			int id = DetermineP(p, sideLen, p2d);

			int col = getcol(id, sideLen);
			int row = getrow(id, sideLen);

			char ** map = p2d[row][col].map[0];
			int cIdx = p2d[row][col].colS;
			int rIdx = p2d[row][col].rowS;
			
			map[p[0]-rIdx][p[1]-cIdx]=1;

		}
		fclose(infile);
/*		while(getline(readfile, line)){
			int p[2];
			stringstream _s(line);

			getline(_s, _x, ' ');
			getline(_s, _y, ' ');

			p[1]=stoi(_x);
			p[0]=stoi(_y);
			
			int id = DetermineP(p, sideLen, p2d);

			int col = getcol(id, sideLen);
			int row = getrow(id, sideLen);

			char ** map = p2d[row][col].map[0];
			int cIdx = p2d[row][col].colS;
			int rIdx = p2d[row][col].rowS;
			
			map[p[0]-rIdx][p[1]-cIdx]=1;
		}*/
	}else{
		fprintf(stderr, "error: readfile: no file\n");
		free_p2d(p2d, sideLen);
		return 1;
	}

	//printf("phase2\n");
	
	pcol = getcol(rank, sideLen);
	prow = getrow(rank, sideLen);
	proc = &p2d[prow][pcol];
	cur = proc->map[1];
	prev = proc->map[0];
	colLen = proc->colE - proc->colS;
	rowLen = proc->rowE - proc->rowS;
//	printf("rank : %d, colS : %d, colE : %d, rowS : %d, rowE : %d\n", rank, proc->colS, proc->colE, proc->rowS, proc->rowE);
	//0 : upleft, 1:up, 2:upright, 3:right, 4:down right, 5:down, 6:downleft, 7:left
/*	if(rank == 0){
		for(int i=0;i<rowLen;i++){
			for(int j=0;j<colLen;j++){
				if(prev[i][j] == 0) printf(". ");
				else printf("0 ");
			}
			printf("\n");
		}
		printf("\n");
	}*/
	char *bound[8];
	for(int i=0; i<8; i++)
	{
		if(i%2 == 0) bound_lens[i] = 1;
		else if(i==1 || i == 4) bound_lens[i] = colLen;
		else bound_lens[i] = rowLen;
		bound[i] = (char *)malloc(sizeof(char)*bound_lens[i]);
	}
	bool sendSuccess[8];
	bool recSuccess[8];
	//printf("rank : %d, size :%d\n", rank, size); 
	for(int k=0;k<Niter;k++)
	{
	//	printf("phase3\n");
		pos = k%2;

		//0 : upleft, 1:up, 2:upright, 3:right, 4:down right, 5:down, 6:downleft, 7:left
		for(int i=0; i<8; i++)
		{
			sendSuccess[i] = send_bound(prev, colLen, rowLen, bound_lens[i], MPI_CHAR, pcol, prow, i, MPI_COMM_WORLD, &sendReq[i], sideLen);
		}
		for(int i=0; i<8; i++)
		{
			recSuccess[i] = request_bound(bound[i], bound_lens[i], MPI_CHAR, pcol, prow, (i+4)%8, MPI_COMM_WORLD, &receiveReq[i], sideLen);
		}


	//	printf("phase4\n");


		//no request
		update_cell_no_receive(prev, cur, colLen, rowLen);
	//	printf("phase5\n");

		//request
		for(int i=0; i<8;i++){
			if(!recSuccess[i]) continue;
			MPI_Wait(&receiveReq[i], &receiveSt);
		}

/*		if(rank == 0){
			for(int i=0; i<8; i++)
			{
				if(i==1) printf("recSuccess : %d\n", (recSuccess[i]==false)?1:0);
				if(recSuccess[i] == false){
					for(int j=0;j<bound_lens[i];j++)
						bound[i][j] = 0;
				}
			}
		}*/

		//MPI_Barrier(MPI_COMM_WORLD);
		
		for(int i=0; i<8; i++){
			int lives = 0;
			switch (i){
			case 0:
				if(bound[0][0] == 1) lives++;
				if(bound[1][0] == 1) lives++;
				if(bound[1][1] == 1) lives++;
				if(bound[7][0] == 1) lives++;
				if(bound[7][1] == 1) lives++;
				if(prev[0][1] == 1) lives++;
				if(prev[1][1] == 1) lives++;
				if(prev[1][0] == 1) lives++;
				cur[0][0] = judge(prev, 0, 0, lives);
				break;
			case 1:
				for(int j=1;j<colLen-1;j++){
					lives=0;
					for(int a=0;a<=1;a++){
						for(int b=j-1;b<=j+1;b++){
							if(a==0 && b==j) continue;
							if(prev[a][b]==1) lives++;
						}
					}
					for(int b=j-1;b<=j+1;b++){
						if(bound[1][b] == 1) lives++;
					}
					cur[0][j] = judge(prev, j, 0, lives);
				}
				break;
			case 2:
				if(bound[2][0] == 1) lives++;
				if(bound[1][colLen-1] == 1) lives++;
				if(bound[1][colLen-2] == 1) lives++;
				if(bound[3][0] == 1) lives++;
				if(bound[3][1] == 1) lives++;
				if(prev[0][colLen-2] == 1) lives++;
				if(prev[1][colLen-1] == 1) lives++;
				if(prev[1][colLen-2] == 1) lives++;
				cur[0][colLen-1] = judge(prev, colLen-1, 0, lives);
				break;
			case 3:
				for(int i=1;i<rowLen-1;i++){
					lives=0;
					for(int a=i-1;a<=i+1;a++){
						for(int b=colLen-2;b<=colLen-1;b++){
							if(a==i && b==colLen-1) continue;
							if(prev[a][b]==1) lives++;
						}
					}
//					if(rank==0) printf("lives?prev : %d\n", lives);
					for(int a=i-1;a<=i+1;a++){
						if(bound[3][a] == 1) lives++;
					}
//					if(rank==0) printf("lives?after : %d\n", lives);
					cur[i][colLen-1] = judge(prev, colLen-1, i, lives);
				}
				break;
			case 4:
				if(bound[4][0] == 1) lives++;
				if(bound[5][colLen-1] == 1) lives++;
				if(bound[5][colLen-2] == 1) lives++;
				if(bound[3][rowLen-1] == 1) lives++;
				if(bound[3][rowLen-2] == 1) lives++;
				if(prev[rowLen-1][colLen-2] == 1) lives++;
				if(prev[rowLen-2][colLen-1] == 1) lives++;
				if(prev[rowLen-2][colLen-2] == 1) lives++;
				cur[rowLen-1][colLen-1] = judge(prev, colLen-1, rowLen-1, lives);
				break;
			case 5:
				for(int j=1;j<colLen-1;j++){
					lives=0;
					for(int a=rowLen-2;a<=rowLen-1;a++){
						for(int b=j-1;b<=j+1;b++){
							if(a==rowLen-1 && b==j) continue;
							if(prev[a][b]==1) lives++;
						}
					}
					for(int b=j-1;b<=j+1;b++){
						if(bound[5][b] == 1) lives++;
					}
					cur[rowLen-1][j] = judge(prev, j, rowLen-1, lives);
				}
				break;
			case 6:
				if(bound[6][0] == 1) lives++;
				if(bound[5][0] == 1) lives++;
				if(bound[5][1] == 1) lives++;
				if(bound[7][rowLen-1] == 1) lives++;
				if(bound[7][rowLen-2] == 1) lives++;
				if(prev[rowLen-1][1] == 1) lives++;
				if(prev[rowLen-2][1] == 1) lives++;
				if(prev[rowLen-2][0] == 1) lives++;
				cur[rowLen-1][0] = judge(prev, 0, rowLen-1, lives);
				break;
			case 7:
				for(int i=1;i<rowLen-1;i++){
					lives=0;
					for(int a=i-1;a<=i+1;a++){
						for(int b=0;b<=1;b++){
							if(a==i && b==0) continue;
							if(prev[a][b]==1) lives++;
						}
					}
					for(int a=i-1;a<=i+1;a++){
						if(bound[7][a] == 1) lives++;
					}
					cur[i][0] = judge(prev, 0, i, lives);
				}
				break;
			default : break;
			}

		}

		
		
		for(int i=0; i<8; i++)
		{
			if(!sendSuccess[i]) continue;
			MPI_Wait(&sendReq[i], &sendSt);
		}

/*		if(rank == 0){
			for(int i=0;i<bound_lens[3];i++){
				printf("bound[3][%d] : %d\n", i, (int)bound[3][i]);
			}
			printf("\n");
			for(int i=0;i<bound_lens[1];i++){
				printf("bound[1][%d] : %d\n", i, (int)bound[0][i]);
			}
			printf("\n");
		}*/

		char **tmp = prev;
		prev = cur;
		cur = tmp;
		
	}

/*	if(rank == 0 || rank == 1){
		for(int i=0;i<12;i++){
			for(int j=0;j<12;j++){
				if(prev[i][j] == 0) printf(". ");
				else printf("0 ");
			}
			printf("\n");
		}
		printf("\n");
	}*/


	for(int i=0;i<rowLen;i++){
		for(int j=0;j<colLen;j++){
			if(prev[i][j] == 1) printf("%d %d\n", proc->colS+j, proc->rowS+i);
		}
	}

	for(int i=0;i<8;i++)
	{
		free(bound[i]);
	}
	free_p2d(p2d, sideLen);
	free(rowLens);
	free(colLens);

	MPI_Finalize();


}
