//The program uses a claim graph consisting of processes, multi-unit resources, request edges,
//allocation edges, and claim edges to represent the state of allocated resources to processes.
//• The graph can be represented by a set of arrays:
//o Resource vector: an m-element vector, where m is the number of resources and each
//entry resource[j] records the total number of units of resource j
//o Available vector: an m-element vector, where m is the number of resources and each
//entry available[j] records the number of units of resource j that are available
//o Max claims array: an nxm-element array, where m is the number of resources and n
//is the number of processes, and each entry maxclaim[i][j] records the maximum
//number of units of resource j that process i may claim
//o Allocated array: an nxm-element array, where m is the number of resources and n is
//the number of processes, and each entry allocated[i][j] records the number of units of
//resource j that process i has been allocated
//o Need array: an nxm array, where m is the number of resources and n is the number of
//processes, and each entry need[i][j] records the number of units of resource j that
//process i may need in the future

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int n, m;
	int *R;
	int *Avail;
	int **Max;
	int **Alloc;
}
Sys;

static int** new_matrix(int r, int c){
	int **a = (int**)malloc(r*sizeof(*a));
	for (int i = 0; i < r; i++)
		a[i] = (int*)calloc(c,sizeof(**a));
	return a;
}
static void free_matrix(int **a,, int r){
	if (!a)
		return;
	for (int i = 0; i < r; i++)
		free(a[i]);
	free(a);
}

static void recompute_available(Sys *s){
	for (int j = 0; j < s->m; j++){
		int sum =0;
		for (int i = 0; i < s->n; i++)
			sum += s->Alloc[i][j];
		s->Avail[j] = s->R[j] - sum;
	}
}










