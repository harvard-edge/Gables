#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "CPUkernel.hpp"

#define KERNEL1(a, b, c)   ((a) = (b) + (c)) // special because it gets max RD bandwidth
#define KERNEL2(a, b, c)   ((a) = (a)*(b) + c)

#define KERNEL1_NEON(a, b, c)   ((a) = vaddq_f32(b, c))
#define KERNEL2_NEON(a, b, c)   ((a) = vmlaq_f32(c, a, b))

#define REP1(S)        S ;
#define REP2(S)        S ;        S
#define REP4(S)   REP2(S);   REP2(S)
#define REP8(S)   REP4(S);   REP4(S)
#define REP16(S)  REP8(S);   REP8(S)
#define REP32(S)  REP16(S);  REP16(S)
#define REP64(S)  REP32(S);  REP32(S)
#define REP128(S) REP64(S);  REP64(S)
#define REP256(S) REP128(S); REP128(S)
#define REP512(S) REP256(S); REP256(S)

#define KERNEL_SRC_GENERATOR(_name, _kernel, _reps) \
    RTYPE _name(uint64_t nsize, \
                            uint64_t ntrials, \
                            float *__restrict__ A, \
                            int *__restrict__ bytes_per_elem, \
                            int *__restrict__ flops_per_elem, \
                            int *__restrict__ mem_accesses_per_elem) { \
\
        *bytes_per_elem = sizeof(*A); \
        *flops_per_elem = 1; \
        *mem_accesses_per_elem = 2; \
\
        const float *endp = A + (nsize-1); \
\
        float alpha = 0.1; \
        for (uint64_t j = 0; j < ntrials; j++) { \
            for (float *p = A; p <= endp; p++) { \
                float beta = 0.2; \
                REP##_reps(KERNEL##_kernel(beta, *p, alpha)); \
                *p = beta; \
            } \
        } \
    } \


KERNEL_SRC_GENERATOR(CPUKernel_1FLOPS, 1, 1);

KERNEL_SRC_GENERATOR(CPUKernel_2FLOPS, 2, 1);

KERNEL_SRC_GENERATOR(CPUKernel_4FLOPS, 2, 2);

KERNEL_SRC_GENERATOR(CPUKernel_8FLOPS, 2, 4);

KERNEL_SRC_GENERATOR(CPUKernel_16FLOPS, 2, 8);

KERNEL_SRC_GENERATOR(CPUKernel_32FLOPS, 2, 16);

KERNEL_SRC_GENERATOR(CPUKernel_64FLOPS, 2, 32);

KERNEL_SRC_GENERATOR(CPUKernel_128FLOPS, 2, 64);

KERNEL_SRC_GENERATOR(CPUKernel_256FLOPS, 2, 128);

KERNEL_SRC_GENERATOR(CPUKernel_512FLOPS, 2, 256);

KERNEL_SRC_GENERATOR(CPUKernel_1024FLOPS, 2, 512);

#if 0
RTYPE CPUKernel_2FLOPS_NEON(uint64_t nsize,
                            uint64_t ntrials,
                            float *__restrict__ A,
                            int *__restrict__ bytes_per_elem,
                            int *__restrict__ flops_per_elem,
                            int *__restrict__ mem_accesses_per_elem) {
    *bytes_per_elem = 1;
    *flops_per_elem = 1;
    *mem_accesses_per_elem = 2;

    float alpha = 0.5;
    float32x4_t alpha_v = vld1q_dup_f32(&alpha);
    for (uint64_t j = 0; j < ntrials; ++j) {
        for (uint64_t i = 0; i < nsize; i += sizeof(float32x4_t) / sizeof(float)) {
            float beta = 0.8;
            float32x4_t beta_v = vld1q_dup_f32(&beta);
            float32x4_t arr_v = vld1q_f32(&A[i]);
            KERNEL2_NEON(beta_v, arr_v, alpha_v);
            vst1q_f32(&A[i], beta_v);
        }
        alpha = alpha * (float) 0.5;
        alpha_v = vld1q_dup_f32(&alpha);
    }
}
#endif