#include <stdio.h>
#include <riscv_vector.h>
#include <cstdlib>
#include <assert.h>

#include "base.h"
#include "menu.h"
#include "riscv.h"

#include "perf.h"

#include "mm.h"

namespace {

int32_t* matrix_multiply_riscv_int(int32_t* A, int32_t* B, int N) {
	vint32m4_t vA, vB, vC, vTmp;
	
	int32_t C [N * N];
	int32_t value;
	
	int stride = N;

	// if (N >= 8) {
	// 	stride = 8;
	// } else {
	// 	stride = N;
	// }

	vsetvl_e32m4(stride);

	for (int i = 0; i < N; i++) {
		for (int k = 0; k < N; k++) {
			value = A[i * N + k];
			for (int j = 0; j < N; j+=stride) {
				// C[i * N + j] += A[i * N + k] * B[k * N + j];	

				vB = vle32_v_i32m4(&B[k * N + j], stride);
				vC = vle32_v_i32m4(&C[i * N + j], stride);

				// accumulator, constant, vector, number of elements
				vTmp = vmul_vx_i32m4(vB, value, stride);
				vC = vadd_vv_i32m4(vC, vTmp, stride);

				vse32_v_i32m4(&C[i * N + j], vC, stride);
			}
		}
	}
	return C;
}

void print_matrix_int(int32_t* A, int N) {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			printf("%d ", A[i * N + j]);	
		}
		printf("\n");
	}
}

int32_t* random_matrix_int(int N) {
	int32_t A [N * N];
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			A[i * N + j] = rand() % 5;	
		}
	}
	return A;
}

int32_t* mm_scalar_int(int32_t* A, int32_t* B, int N) {
	int32_t C [N * N];
	int32_t value;
	for (int i = 0; i < N; i++) {
		for (int k = 0; k < N; k++) {
			value = A[i * N + k]; 
			for (int j = 0; j < N; j++) {
				C[i * N + j] += value * B[k * N + j];
			}
		}
	}
	return C;
}

void check_mm_equal_int(int32_t* rvv_mm, int32_t* scalar_mm, int N) {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			assert(rvv_mm[i * N + j] - scalar_mm[i * N + j] == 0);

			if (rvv_mm[i*N+j] != scalar_mm[i*N+j]) printf("MAYDAY MAYDAY\n");
		}
	}
	printf("Correct values from the matrix multiplication.\n");
}

void rvv_mm_test() {
	int N = 128;
	printf("performing integer matrix mulitplication... \n");
    int32_t* A = random_matrix_int(N);
	int32_t* B = random_matrix_int(N);

	volatile int start = perf_get_mcycle();
	int32_t* D = mm_scalar_int(A, B, N);

	volatile int mid = perf_get_mcycle();
	int32_t* C = matrix_multiply_riscv_int(A, B, N);
	volatile int end = perf_get_mcycle();
	// printf("Matrix A: \n");
	// print_matrix_int(A, N);
	// printf("Matrix B: \n");
	// print_matrix_int(B, N);
	// printf("Matrix C: \n");
	// print_matrix_int(C, N);
	
	// printf("Matrix D: \n");
	// print_matrix_int(D, N);

	check_mm_equal_int(C, D, N);

	printf("Timestamps: %d, %d, %d\n", start, mid, end);
	print_float("Speedup", (float)(mid-start)/(float)(end-mid)); // SCALAR OVER VECTOR DUMMY
	
// 	free(A);
// 	free(B);
// 	free(C);
// 	free(D);
}

struct Menu MENU = {
    "RVV Benchmarks",
    "benchmark",
    {
        MENU_ITEM('m', "Run MM test", rvv_mm_test),
        MENU_END,
    },
};

};

extern "C" void rvv_bmark_test() { menu_run(&MENU); }