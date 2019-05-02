#ifndef KERNEL1_H
#define KERNEL1_H

#define KERNEL1(a, b, c)   ((a) = (b) + (c))
#define KERNEL2(a, b, c)   ((a) = (a)*(b) + (c))

#include "rep.hpp"

void initialize(uint64_t nsize,
                double *__restrict__ array,
                double value);

void kernel(uint64_t nsize,
            uint64_t ntrials,
            double *__restrict__ array,
            int *bytes_per_elem,
            int *mem_accesses_per_elem);

void kernel_1FLOPS(uint64_t nsize,
                   uint64_t ntrials,
                   double *__restrict__ array,
                   int *bytes_per_elem,
                   int *mem_accesses_per_elem);

void kernel_2FLOPS(uint64_t nsize,
                   uint64_t ntrials,
                   double *__restrict__ array,
                   int *bytes_per_elem,
                   int *mem_accesses_per_elem);

void kernel_4FLOPS(uint64_t nsize,
                   uint64_t ntrials,
                   double *__restrict__ array,
                   int *bytes_per_elem,
                   int *mem_accesses_per_elem);

void kernel_8FLOPS(uint64_t nsize,
                   uint64_t ntrials,
                   double *__restrict__ array,
                   int *bytes_per_elem,
                   int *mem_accesses_per_elem);

void kernel_16FLOPS(uint64_t nsize,
                    uint64_t ntrials,
                    double *__restrict__ array,
                    int *bytes_per_elem,
                    int *mem_accesses_per_elem);

void kernel_32FLOPS(uint64_t nsize,
                    uint64_t ntrials,
                    double *__restrict__ array,
                    int *bytes_per_elem,
                    int *mem_accesses_per_elem);

void kernel_64FLOPS(uint64_t nsize,
                    uint64_t ntrials,
                    double *__restrict__ array,
                    int *bytes_per_elem,
                    int *mem_accesses_per_elem);

void kernel_128FLOPS(uint64_t nsize,
                     uint64_t ntrials,
                     double *__restrict__ array,
                     int *bytes_per_elem,
                     int *mem_accesses_per_elem);

void kernel_256FLOPS(uint64_t nsize,
                     uint64_t ntrials,
                     double *__restrict__ array,
                     int *bytes_per_elem,
                     int *mem_accesses_per_elem);

void kernel_512FLOPS(uint64_t nsize,
                     uint64_t ntrials,
                     double *__restrict__ array,
                     int *bytes_per_elem,
                     int *mem_accesses_per_elem);

void kernel_1024FLOPS(uint64_t nsize,
                      uint64_t ntrials,
                      double *__restrict__ array,
                      int *bytes_per_elem,
                      int *mem_accesses_per_elem);

#endif
