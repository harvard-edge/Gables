#ifndef ERT_DRIVER_H
#define ERT_DRIVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <inttypes.h>
#include <syslog.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <assert.h>

#include <string>

#include <omp.h>

#define ERT_ALIGN           (32)    // the alignment (in bits) of the data being manipulated
#define ERT_WORKING_SET_MIN (1)     // min working set size in 8-byte, double FP precision chunks
#define ERT_TRIALS_MIN      (1)     // min times to repeatedly run the loop over the working set

#endif
