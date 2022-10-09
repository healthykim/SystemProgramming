/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp, l, m, s, t1, t2, t3, t4, t5;

    if((N==64 && M==64) || (N==32 && M==32)){
        for (i = 0; i < N; i+=8) {
            for (j = 0; j < M; j+=8) {
                //mapping upper 4*8 of A[][] 
                //A's upper-front 4*4 will be mapped properly(to B's upper-front 4*4)
                //A's upper-back 4*4 will be temporarly saved(to B's upper-back 4*4)
                for(l=i; l<i+4; l++){
                    m = A[l][j];
                    s = A[l][j+1];
                    tmp = A[l][j+2];
                    t1 = A[l][j+3];
                    t2 = A[l][j+4];
                    t3 = A[l][j+5];
                    t4 = A[l][j+6];
                    t5 = A[l][j+7];

                    B[j][l] = m;
                    B[j][l+4] = t2;
                    B[j+1][l] = s;
                    B[j+1][l+4] = t3;
                    B[j+2][l] = tmp;
                    B[j+2][l+4] = t4;
                    B[j+3][l] = t1;
                    B[j+3][l+4] = t5;
                }
                //maping lower A's 4*8
                //move B's temporarly saved area to proper mapping area(B's upper-back 4*4 to B's lower-fron 4*4)
                //A's lower-front 4*4 will be properly mapped(to B's upper-back 4*4)
                //A's lower-back 4*4 will be properly saved(to B's lower-back 4*4)

                for(l=j; l<j+4; l++){
                    m = B[l][i+4];
                    s = B[l][i+5];
                    tmp = B[l][i+6];
                    t1 = B[l][i+7];
                    B[l][i+4] = A[i+4][l];
                    t2 = A[i+4][l+4];
                    B[l][i+5] = A[i+5][l];
                    t3 = A[i+5][l+4];
                    B[l][i+6] = A[i+6][l];
                    t4 = A[i+6][l+4];
                    B[l][i+7] = A[i+7][l];
                    t5 = A[i+7][l+4];

                    B[l+4][i] = m;
                    B[l+4][i+1] = s;
                    B[l+4][i+2] = tmp;
                    B[l+4][i+3] = t1;
                    B[l+4][i+4] = t2;
                    B[l+4][i+5] = t3;
                    B[l+4][i+6] = t4;
                    B[l+4][i+7] = t5;
                }
            }
        }
    }
    else{
        for (i = 0; i < N; i+=16) {
            for (j = 0; j < M; j+=16) {
                for(l=i; l<i+16 && l<N; l++){
                    for(m=j; m<j+16 && m<M;m++){
                        B[m][l] = A[l][m];
                    }
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

