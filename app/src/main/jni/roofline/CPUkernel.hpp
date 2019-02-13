#ifndef KERNEL1_H
#define KERNEL1_H

#include <stdint.h>

/* ======================================================================= */
/*  Common data types for functions                                        */
/* ======================================================================= */

#define RTYPE void
#define ITYPE float

/* ======================================================================= */
/*  Function pointer data type                                             */
/* ======================================================================= */

typedef RTYPE (*CPUKernelPtr)(uint64_t,
                              uint64_t,
                              ITYPE *__restrict__,
                              int *__restrict__,
                              int *__restrict__,
                              int *__restrict__);

CPUKernelPtr CPU_get_kernel(int nFlops);

/* ======================================================================= */
/*  Plain FP instruction kernels                                           */
/* ======================================================================= */

#define KERNEL_HDR_GENERATOR(_name, _kernel, _reps) \
    RTYPE _name(uint64_t nsize, \
                           uint64_t ntrials, \
                           ITYPE *__restrict__ array, \
                           int *__restrict__ bytes_per_elem, \
                           int *__restrict__ flops_per_elem, \
                           int *__restrict__ mem_accesses_per_elem) \


KERNEL_HDR_GENERATOR(CPUKernel_1FLOPS, 1, 1);

KERNEL_HDR_GENERATOR(CPUKernel_2FLOPS, 2, 1);

KERNEL_HDR_GENERATOR(CPUKernel_4FLOPS, 2, 2);

KERNEL_HDR_GENERATOR(CPUKernel_8FLOPS, 2, 4);

KERNEL_HDR_GENERATOR(CPUKernel_16FLOPS, 2, 8);

KERNEL_HDR_GENERATOR(CPUKernel_32FLOPS, 2, 16);

KERNEL_HDR_GENERATOR(CPUKernel_64FLOPS, 2, 32);

KERNEL_HDR_GENERATOR(CPUKernel_128FLOPS, 2, 64);

KERNEL_HDR_GENERATOR(CPUKernel_256FLOPS, 2, 128);

KERNEL_HDR_GENERATOR(CPUKernel_512FLOPS, 2, 256);

KERNEL_HDR_GENERATOR(CPUKernel_1024FLOPS, 2, 512);

/* ======================================================================= */
/*  NEON instruction kernels                                               */
/* ======================================================================= */

#endif