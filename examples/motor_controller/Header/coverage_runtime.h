#ifndef COVERAGE_RUNTIME_H
#define COVERAGE_RUNTIME_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Embedded Coverage Runtime - Ultra lightweight probe tracking */
static inline void _cov_hit_line(int file_id, int line) {
    printf("[COV] L:%d:%d\n", file_id, line);
    fflush(stdout);
}

static inline void _cov_hit_func(int func_id) {
    printf("[COV] F:%d\n", func_id);
    fflush(stdout);
}

static inline int _cov_hit_branch(int branch_id, int cond) {
    if (cond) {
        printf("[COV] B:%d:T\n", branch_id);
    } else {
        printf("[COV] B:%d:F\n", branch_id);
    }
    fflush(stdout);
    return cond ? 1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* COVERAGE_RUNTIME_H */
