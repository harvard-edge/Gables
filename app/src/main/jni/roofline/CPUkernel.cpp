#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "rep.hpp"
#include "CPUkernel.hpp"

void initialize(uint64_t nsize,
                double *__restrict__ a,
                double value) {
    uint64_t i;
    for (i = 0; i < nsize; ++i) {
        a[i] = value;
    }
}

void kernel_1FLOPS(uint64_t nsize,
                   uint64_t ntrials,
                   double *__restrict__ A,
                   int *bytes_per_elem,
                   int *mem_accesses_per_elem) {
    *bytes_per_elem = sizeof(*A);
    *mem_accesses_per_elem = 2;

    double alpha = 0.5;
    uint64_t i, j;
    for (j = 0; j < ntrials; ++j) {
        for (i = 0; i < nsize; ++i) {
            double beta = 0.8;
            KERNEL1(beta, A[i], alpha);
            A[i] = beta;
        }
        alpha = alpha * (1 - 1e-8);
    }
}

void kernel_2FLOPS(uint64_t nsize,
                   uint64_t ntrials,
                   double *__restrict__ A,
                   int *bytes_per_elem,
                   int *mem_accesses_per_elem) {
    *bytes_per_elem = sizeof(*A);
    *mem_accesses_per_elem = 2;

    double alpha = 0.5;
    uint64_t i, j;
    for (j = 0; j < ntrials; ++j) {
        for (i = 0; i < nsize; ++i) {
            double beta = 0.8;
            KERNEL2(beta, A[i], alpha);
            A[i] = beta;
        }
        alpha = alpha * (1 - 1e-8);
    }
}

void kernel_4FLOPS(uint64_t nsize,
                   uint64_t ntrials,
                   double *__restrict__ A,
                   int *bytes_per_elem,
                   int *mem_accesses_per_elem) {
    *bytes_per_elem = sizeof(*A);
    *mem_accesses_per_elem = 2;

    double alpha = 0.5;
    uint64_t i, j;
    for (j = 0; j < ntrials; ++j) {
        for (i = 0; i < nsize; ++i) {
            double beta = 0.8;
            REP2(KERNEL2(beta, A[i], alpha));
            A[i] = beta;
        }
        alpha = alpha * (1 - 1e-8);
    }
}

void kernel_8FLOPS(uint64_t nsize,
                   uint64_t ntrials,
                   double *__restrict__ A,
                   int *bytes_per_elem,
                   int *mem_accesses_per_elem) {
    *bytes_per_elem = sizeof(*A);
    *mem_accesses_per_elem = 2;

    double alpha = 0.5;
    uint64_t i, j;
    for (j = 0; j < ntrials; ++j) {
        for (i = 0; i < nsize; ++i) {
            double beta = 0.8;
            REP4(KERNEL2(beta, A[i], alpha));
            A[i] = beta;
        }
        alpha = alpha * (1 - 1e-8);
    }
}

void kernel_16FLOPS(uint64_t nsize,
                    uint64_t ntrials,
                    double *__restrict__ A,
                    int *bytes_per_elem,
                    int *mem_accesses_per_elem) {
    *bytes_per_elem = sizeof(*A);
    *mem_accesses_per_elem = 2;

    double alpha = 0.5;
    uint64_t i, j;
    for (j = 0; j < ntrials; ++j) {
        for (i = 0; i < nsize; ++i) {
            double beta = 0.8;
            REP8(KERNEL2(beta, A[i], alpha));
            A[i] = beta;
        }
        alpha = alpha * (1 - 1e-8);
    }
}

void kernel_32FLOPS(uint64_t nsize,
                    uint64_t ntrials,
                    double *__restrict__ A,
                    int *bytes_per_elem,
                    int *mem_accesses_per_elem) {
    *bytes_per_elem = sizeof(*A);
    *mem_accesses_per_elem = 2;

    double alpha = 0.5;
    uint64_t i, j;
    for (j = 0; j < ntrials; ++j) {
        for (i = 0; i < nsize; ++i) {
            double beta = 0.8;
            REP16(KERNEL2(beta, A[i], alpha));
            A[i] = beta;
        }
        alpha = alpha * (1 - 1e-8);
    }
}

void kernel_64FLOPS(uint64_t nsize,
                    uint64_t ntrials,
                    double *__restrict__ A,
                    int *bytes_per_elem,
                    int *mem_accesses_per_elem) {
    *bytes_per_elem = sizeof(*A);
    *mem_accesses_per_elem = 2;

    double alpha = 0.5;
    uint64_t i, j;
    for (j = 0; j < ntrials; ++j) {
        for (i = 0; i < nsize; ++i) {
            double beta = 0.8;
            REP32(KERNEL2(beta, A[i], alpha));
            A[i] = beta;
        }
        alpha = alpha * (1 - 1e-8);
    }
}

void kernel_128FLOPS(uint64_t nsize,
                     uint64_t ntrials,
                     double *__restrict__ A,
                     int *bytes_per_elem,
                     int *mem_accesses_per_elem) {
    *bytes_per_elem = sizeof(*A);
    *mem_accesses_per_elem = 2;

    double alpha = 0.5;
    uint64_t i, j;
    for (j = 0; j < ntrials; ++j) {
        for (i = 0; i < nsize; ++i) {
            double beta = 0.8;
            REP64(KERNEL2(beta, A[i], alpha));
            A[i] = beta;
        }
        alpha = alpha * (1 - 1e-8);
    }
}

void kernel_256FLOPS(uint64_t nsize,
                     uint64_t ntrials,
                     double *__restrict__ A,
                     int *bytes_per_elem,
                     int *mem_accesses_per_elem) {
    *bytes_per_elem = sizeof(*A);
    *mem_accesses_per_elem = 2;

    double alpha = 0.5;
    uint64_t i, j;
    for (j = 0; j < ntrials; ++j) {
        for (i = 0; i < nsize; ++i) {
            double beta = 0.8;
            REP128(KERNEL2(beta, A[i], alpha));
            A[i] = beta;
        }
        alpha = alpha * (1 - 1e-8);
    }
}

void kernel_512FLOPS(uint64_t nsize,
                     uint64_t ntrials,
                     double *__restrict__ A,
                     int *bytes_per_elem,
                     int *mem_accesses_per_elem) {
    *bytes_per_elem = sizeof(*A);
    *mem_accesses_per_elem = 2;

    double alpha = 0.5;
    uint64_t i, j;
    for (j = 0; j < ntrials; ++j) {
        for (i = 0; i < nsize; ++i) {
            double beta = 0.8;
            REP256(KERNEL2(beta, A[i], alpha));
            A[i] = beta;
        }
        alpha = alpha * (1 - 1e-8);
    }
}

void kernel_1024FLOPS(uint64_t nsize,
                      uint64_t ntrials,
                      double *__restrict__ A,
                      int *bytes_per_elem,
                      int *mem_accesses_per_elem) {
    *bytes_per_elem = sizeof(*A);
    *mem_accesses_per_elem = 2;

    double alpha = 0.5;
    uint64_t i, j;
    for (j = 0; j < ntrials; ++j) {
        for (i = 0; i < nsize; ++i) {
            double beta = 0.8;
            REP512(KERNEL2(beta, A[i], alpha));
            A[i] = beta;
        }
        alpha = alpha * (1 - 1e-8);
    }
}
