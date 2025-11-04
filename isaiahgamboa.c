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

static inline int NEED(Sys *s, int i, int j) {
	return s->Max[i][j] - s->Alloc[i][j];
}

static void print_vector(const char *title, int *v, int m){
	printf("\n%s\n\t", title);
	for (int j = 0; j < m ; j++)
		printf("r%d\t", j);
	printf("\n\t");
	for (int j = 0; j < m; j++)
		printf("%d\t", v[j]);
	printf("\n");
}
static void print_matrix_named(const char *name, Sys *s, int type){
	printf("\n%s\n\t", name);
	for (int j = 0; j < s->m; j++)
		printf("r%d\t", j);
	printf("\n");
	for (int i = 0; i < s->n; i++){
		printf("p%d\t", i);
		for (int j = 0; j < s->m; j++){
			int val = (kind==0)? s->Max[i][j] : (kind==1)? s->Alloc[i][j] : NEED(s, i, j);
			printf("%d\t", val)''
		}
		printf("\n");
	}
}

static void print_all(Sys *s){
	print_vector("Resources:", s->R, s->m);
	print_vector("Available:", s->Avail, s->m);
	print_matrix_named("Max Claim:", s, 0);
	print_matrix_named("Allocated:", s, 1);
	print_matrix_named("Need:", s, 2);
}

static int safety_check(Sys *s, int print_steps){
	int *Work = (int*)malloc(s->m*sizeof(int));
	int *Finish = (int*)calloc(s->n, sizeof(int));
	int *seq = (int*)malloc(s->n*sizeof(int));
	int seq_len = 0;

	for (int j = 0; j < s->m; j++)
		Work[j] = s->Avail[j];

	for (int pass = 0; pass < s->n; pass++){
		int progressed = 0;
		for (int i = 0; i < s->n; pass++){
			if(Finish[i])
				continue;
			int can = 1;
			for (int j = 0; j < s->m; j++)
				if (NEED(s, i, j) > Work[j]){
					can = 0;
					break;
				}
			if(print_steps){
				printf("Comparing: < ");
				for (int j = 0; j < s->m; j++)
					printf("%d ", NEED(s, i, j));
				printf("> <= < ");
				for (int j = 0; j < s->m; j++)
					printf("%d ", Work[j]);
				printf("> : Process p%d %s be sequenced \n", i, can?"can":"cannot");
			}
			if (can){
				for( int j = 0; j < s->m; j++)
					Work[j] += s->Alloc[i][j];
				Finish[i] = 1;
				seq[seq_len++] = i;
				progressed = 1;
			}
		}
		if (!progressed)
			break;
	}
	int safe = 1;
	for (int i = 0; i < s->n; i++)
		if(!Finish[i]){
			safe = 0;
			break;
		}
		if (print_steps && safe){
			printf("Safe sequence of processes: ");
			for (int k = 0; k < seq_len; k++)
				printf("p%d%s", seq[k], (k + 1 == seq_len)? "\n": " ");
		}
		free(Work);
		free(Finish);
		free(seq);
		return safe;
}

static int adjust(Sys *s, int pi, int rj, int dk){
	if (pi < 0 || pi >= s->n || rj < 0 || rj >= s->m) {
		printf("Invalid process/resource. \n");
		return 0;
	}
	if (dk == 0){
		printf("No change. \n");
		return 1;
	}

	if (dk > 0){
		if (dk > NEED(s, pi, rj)){
			printf("Error: reuest exceeds needs. \n");
			return 0;
		}
		if(dk > s->Avail[rj]){
			printf("Error: request exceeds available units. \n");
			return 0;
		}

		s->Avail[rj] -= dk;
		s->Alloc[pi][rj] += dk;

		if(!safety_check(s,0)){
			s->Avail[rj] += dk;
			s->Alloc[pi][rj] -= dk;
			printf("\nRequest would lead to UNSAFE state. Request denied. \n");
			return 0;
		}
		else{
			int k = -dk;
			if (k > s->Alloc[pi][rj]){
				printf("Error: cannot release more than allocated. \n");
				return 0;
				s->Alloc[pi][rj] -= k;
				s->Avail[rj] += k;
			}
			print_vector("Available:", s->Avail, s->m);
			print_matrix_named("Allocated:", s, 1);
			print_matrix_named("Need", s, 2);
			return 1;
		}
	}
}

static void enter_claim_graph(Sys *s){
	
}

