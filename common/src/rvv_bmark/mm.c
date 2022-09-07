#include <stdio.h>
#include <riscv_vector.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

const int N=16;

float* matrix_multiply_riscv_float(float* A, float* B) {
	vsetvl_e32m8(N);
	vfloat32m8_t vA, vB, vC;
	
	float* C = (float*) calloc(N * N, sizeof(float));
	float value;
	
	int n;

	if (N >= 8) {
		n = 8;
	} else {
		n = N;
	}

	for (int i = 0; i < N; i++) {
		for (int k = 0; k < N; k++) {
			value = A[i * N + k];
			for (int j = 0; j < N; j+=n) {
				// C[i * N + j] += A[i * N + k] * B[k * N + j];	

				vB = vle32_v_f32m8(&B[k * N + j], n);
				vC = vle32_v_f32m8(&C[i * N + j], n);

				// accumulator, constant, vector, number of elements
				vC = vfmacc_vf_f32m8(vC, value, vB, n);
				vse32_v_f32m8(&C[i * N + j], vC, n);
			}
		}
	}
	return C;
}


int* matrix_multiply_riscv_int(int* A, int* B) {
	vint32m8_t vA, vB, vC;
	
	int* C = (int*) calloc(N * N, sizeof(int));
	int value;
	
	int n;

	if (N >= 8) {
		n = 8;
	} else {
		n = N;
	}

	for (int i = 0; i < N; i++) {
		for (int k = 0; k < N; k++) {
			value = A[i * N + k];
			for (int j = 0; j < N; j+=n) {
				// C[i * N + j] += A[i * N + k] * B[k * N + j];	

				vB = vle32_v_i32m8(&B[k * N + j], n);
				vC = vle32_v_i32m8(&C[i * N + j], n);

				// accumulator, constant, vector, number of elements
				vC = vmacc_vx_i32m8(vC, value, vB, n);
				vse32_v_i32m8(&C[i * N + j], vC, n);
			}
		}
	}
	return C;
}


void print_matrix_float(float* A) {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			printf("%f ", A[i * N + j]);	
		}
		printf("\n");
	}
}

void print_matrix_int(int* A) {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			printf("%d ", A[i * N + j]);	
		}
		printf("\n");
	}
}

float* random_matrix_float() {
	float* A = calloc(N * N, sizeof(float));
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			A[i * N + j] = rand() % 5;	
		}
	}
	return A;
}

int* random_matrix_int() {
	int* A = calloc(N * N, sizeof(int));
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			A[i * N + j] = rand() % 5;	
		}
	}
	return A;
}

float* mm_scalar_float(float* A, float* B) {
	float* C = (float*)calloc(N * N, sizeof(float));
	float value;
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

int* mm_scalar_int(int* A, int* B) {
	int* C = (int*)calloc(N * N, sizeof(int));
	int value;
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

void check_mm_equal_float(float* rvv_mm, float* scalar_mm) {
	float tolerance = .001;
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			assert(fabs(rvv_mm[i * N + j] - scalar_mm[i * N + j]) < tolerance);		
		}
	}
	printf("Correct values from the matrix multiplication.\n");
}

void check_mm_equal_int(int* rvv_mm, int* scalar_mm) {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			assert(rvv_mm[i * N + j] - scalar_mm[i * N + j] == 0);		
		}
	}
	printf("Correct values from the matrix multiplication.\n");
}

// int main() {
// 	printf("performing integer matrix mulitplication... \n");
//     int* A = random_matrix_int();
// 	int* B = random_matrix_int();
// 	int* C = matrix_multiply_riscv_int(A, B);

// 	printf("Matrix A: \n");
// 	print_matrix_int(A);
// 	printf("Matrix B: \n");
// 	print_matrix_int(B);
// 	printf("Matrix C: \n");
// 	print_matrix_int(C);

// 	int* D = mm_scalar_int(A, B);
// 	printf("Matrix D: \n");
// 	print_matrix_int(D);

// 	check_mm_equal_int(C, D);
	
// 	free(A);
// 	free(B);
// 	free(C);
// 	free(D);
// 	return 0;

// }

int rvv_mm_test() {
	printf("performing integer matrix mulitplication... \n");
    int* A = random_matrix_int();
	int* B = random_matrix_int();
	int* C = matrix_multiply_riscv_int(A, B);

	printf("Matrix A: \n");
	print_matrix_int(A);
	printf("Matrix B: \n");
	print_matrix_int(B);
	printf("Matrix C: \n");
	print_matrix_int(C);

	int* D = mm_scalar_int(A, B);
	printf("Matrix D: \n");
	print_matrix_int(D);

	check_mm_equal_int(C, D);
	
	free(A);
	free(B);
	free(C);
	free(D);
	return 0;
}