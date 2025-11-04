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
    int *R;        /* [m] */
    int *Avail;    /* [m] */
    int *Max;      /* [n*m] flattened row-major */
    int *Alloc;    /* [n*m] flattened row-major */
} Sys;

#define IDX(s,i,j) ((i)*(s)->m + (j))

static void free_sys(Sys *s){
    free(s->R);     
    s->R = NULL;
    free(s->Avail); 
    s->Avail = NULL;
    free(s->Max);   
    s->Max = NULL;
    free(s->Alloc); 
    s->Alloc = NULL;
    s->n = s->m = 0;
}

static void recompute_available(Sys *s){
    for (int j = 0; j < s->m; j++){
        int sum = 0;
        for (int i = 0; i < s->n; i++)
            sum += s->Alloc[IDX(s,i,j)];
        s->Avail[j] = s->R[j] - sum;
    }
}

static inline int NEED(const Sys *s, int i, int j){
    return s->Max[IDX(s,i,j)] - s->Alloc[IDX(s,i,j)];
}

/* --- Printing --- */
static void print_vector(const char *title, const int *v, int m){
    printf("\n%s\n\t", title);
    for (int j = 0; j < m; j++) 
    	printf("r%d\t", j);
    printf("\n\t");
    for (int j = 0; j < m; j++) 
    	printf("%d\t", v[j]);
    printf("\n");
}
static void print_matrix_named(const char *name, const Sys *s, int type /*0=Max,1=Alloc,2=Need*/){
    printf("\n%s\n\t", name);
    for (int j = 0; j < s->m; j++) 
    	printf("r%d\t", j);
    printf("\n");
    for (int i = 0; i < s->n; i++){
        printf("p%d\t", i);
        for (int j = 0; j < s->m; j++){
            int val = (type==0) ? s->Max[IDX(s,i,j)]
                    : (type==1) ? s->Alloc[IDX(s,i,j)]
                                : NEED(s,i,j);
            printf("%d\t", val);
        }
        printf("\n");
    }
}
static void print_all(const Sys *s){
    print_vector("Resources:", s->R, s->m);
    print_vector("Available:", s->Avail, s->m);
    print_matrix_named("Max Claim:", s, 0);
    print_matrix_named("Allocated:", s, 1);
    print_matrix_named("Need:", s, 2);
}

/* --- Banker's safety (no heap allocs) --- */
static int safety_check(const Sys *s, int print_steps){
    int Work[s->m];
    int Finish[s->n];
    int seq[s->n];
    int seq_len = 0;

    for (int j = 0; j < s->m; j++) 
    	Work[j] = s->Avail[j];
    for (int i = 0; i < s->n; i++) 
    	Finish[i] = 0;

    for (int pass = 0; pass < s->n; pass++){
        int progressed = 0;
        for (int i = 0; i < s->n; i++){
            if (Finish[i]) 
            	continue;

            int can = 1;
            for (int j = 0; j < s->m; j++){
                if (NEED(s,i,j) > Work[j]) { can = 0; 
                break; 
            }
        	}

            if (print_steps){
                printf("Comparing: < ");
                for (int j = 0; j < s->m; j++) 
                	printf("%d ", NEED(s,i,j));
                printf("> <= < ");
                for (int j = 0; j < s->m; j++) 
                	printf("%d ", Work[j]);
                printf("> : Process p%d %s be sequenced \n", i, can ? "can" : "cannot");
            }

            if (can){
                for (int j = 0; j < s->m; j++) 
                	Work[j] += s->Alloc[IDX(s,i,j)];
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
    	if (!Finish[i]) { safe = 0; 
    break; 
}

    if (print_steps && safe){
        printf("Safe sequence of processes: ");
        for (int k = 0; k < seq_len; k++)
            printf("p%d%s", seq[k], (k+1==seq_len) ? "\n" : "  ");
    }
    return safe;
}

/* --- Request/Release --- */
static int adjust(Sys *s, int pi, int rj, int dk){
    if (pi < 0 || pi >= s->n || rj < 0 || rj >= s->m) {
        printf("Invalid process/resource.\n");
        return 0;
    }
    if (dk == 0){ 
    	printf("No change.\n"); 
    	return 1; 
    }

    if (dk > 0){ /* request */
        if (dk > NEED(s,pi,rj)){ 
        	printf("Error: request exceeds need.\n"); 
        	return 0; 
        }
        if (dk > s->Avail[rj]){ 
        	printf("Error: request exceeds available units.\n"); 
        	return 0; 
        }
        /* tentative grant */
        s->Avail[rj] -= dk;
        s->Alloc[IDX(s,pi,rj)] += dk;

        if (!safety_check(s, 0)){
            /* rollback */
            s->Avail[rj] += dk;
            s->Alloc[IDX(s,pi,rj)] -= dk;
            printf("\nRequest would lead to UNSAFE state. Request denied.\n");
            return 0;
        }
    } else {     /* release */
        int k = -dk;
        int *cell = &s->Alloc[IDX(s,pi,rj)];
        if (k > *cell){ printf("Error: cannot release more than allocated.\n"); 
        return 0; 
    }
        *cell -= k;
        s->Avail[rj] += k;
    }

    print_vector("Available:", s->Avail, s->m);
    print_matrix_named("Allocated:", s, 1);
    print_matrix_named("Need:", s, 2);
    return 1;
}

/* --- UI flows --- */
static void enter_claim_graph(Sys *s){
    free_sys(s);

    printf("Enter number of resources: ");
    scanf("%d", &s->m);
    s->R     = (int*)calloc(s->m, sizeof(int));
    s->Avail = (int*)calloc(s->m, sizeof(int));

    printf("Enter number of units for resources (r0 to r%d): ", s->m - 1);
    for (int j = 0; j < s->m; j++) scanf("%d", &s->R[j]);

    printf("Enter number of processes: ");
    scanf("%d", &s->n);

    /* flat matrices */
    s->Max   = (int*)calloc((size_t)s->n * s->m, sizeof(int));
    s->Alloc = (int*)calloc((size_t)s->n * s->m, sizeof(int));

    for (int i = 0; i < s->n; i++){
        printf("Enter maximum number of units process p%d will claim from each resource (r0 to r%d): ", i, s->m - 1);
        for (int j = 0; j < s->m; j++) scanf("%d", &s->Max[IDX(s,i,j)]);
    }
    for (int i = 0; i < s->n; i++){
        printf("Enter number of units of each resource (r0 to r%d) currently allocated to process p%d: ", i, s->m - 1);
        for (int j = 0; j < s->m; j++) scanf("%d", &s->Alloc[IDX(s,i,j)]);
    }
    recompute_available(s);
    print_all(s);
}

static void request_flow(Sys *s){
    if (!s->R){ 
    	printf("Please enter a claim graph first.\n"); 
    return; 
}
    char pbuf[16], rbuf[16];
    int pi = -1, rj = -1, k = 0;

    printf("Enter requesting process: ");
    scanf("%15s", pbuf);
    if (pbuf[0]=='p'||pbuf[0]=='P') pi = atoi(pbuf+1);

    printf("Enter requested resource: ");
    scanf("%15s", rbuf);
    if (rbuf[0]=='r'||rbuf[0]=='R') rj = atoi(rbuf+1);

    printf("Enter number of units process p%d is requesting from resource r%d: ", pi, rj);
    scanf("%d", &k);

    adjust(s, pi, rj, +k);
}

static void release_flow(Sys *s){
    if (!s->R){ 
    	printf("Please enter a claim graph first.\n"); 
    	return; 
    }
    char pbuf[16], rbuf[16];
    int pi = -1, rj = -1, k = 0;

    printf("Enter releasing process: ");
    scanf("%15s", pbuf);
    if (pbuf[0]=='p'||pbuf[0]=='P') pi = atoi(pbuf+1);

    printf("Enter released resource: ");
    scanf("%15s", rbuf);
    if (rbuf[0]=='r'||rbuf[0]=='R') rj = atoi(rbuf+1);

    printf("Enter number of units process p%d is releasing from resource r%d: ", pi, rj);
    scanf("%d", &k);
    adjust(s, pi, rj, -k);
}

static void safe_sequence(Sys *s){
    if (!s->R){ 
    	printf("Please enter a claim graph first.\n"); 
    	return; 
    }
    (void)safety_check(s, 1);
}

static void menu(void){
    printf("\nBanker's Algorithm\n");
    printf("------------------\n");
    printf("1) Enter claim graph\n");
    printf("2) Request resource\n");
    printf("3) Release resource\n");
    printf("4) Determine safe sequence\n");
    printf("5) Quit program\n\n");
    printf("Enter selection: ");
}

int main(void){
    Sys s = {0};
    int choice;
    for (;;){
        menu();
        if (scanf("%d", &choice) != 1){
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }
        switch (choice){
            case 1: enter_claim_graph(&s); 
            	break;
            case 2: request_flow(&s);      
            	break;
            case 3: release_flow(&s);      
            	break;
            case 4: safe_sequence(&s);     
            	break;
            case 5:
                printf("Quitting program...\n");
                free_sys(&s);
                return 0;
            default:
                printf("Invalid selection.\n");
        }
    }
}



