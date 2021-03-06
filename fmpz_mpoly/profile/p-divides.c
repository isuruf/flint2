/*
    Copyright 2019 Daniel Schultz

    This file is part of FLINT.

    FLINT is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include "profiler.h"
#include "fmpz_mpoly.h"
/*
    export LD_LIBRARY_PATH=/tmpbig/schultz/flint2
    likwid-setFrequencies -g performance
    nano fmpz_mpoly/profile/p-divides.c
    make profile MOD=fmpz_mpoly
    ./build/fmpz_mpoly/profile/p-divides
*/

slong max_threads;
int * cpu_affinities;

typedef struct _worker_arg_struct
{
    fmpz_mpoly_t Q;
    const fmpz_mpoly_struct * A, * B;
    const fmpz_mpoly_ctx_struct * ctx;
} worker_arg_struct;

typedef worker_arg_struct worker_arg_t[1];


static void worker_divides(void * varg)
{
    worker_arg_struct * W = (worker_arg_struct *) varg;
    fmpz_mpoly_divides_threaded(W->Q, W->A, W->B, W->ctx, 1);
}

void profile_divides(
    const fmpz_mpoly_t realQ,
    const fmpz_mpoly_t A,
    const fmpz_mpoly_t B,
    const fmpz_mpoly_ctx_t ctx,
    const char * name,
    slong m1, slong n1)
{
    fmpz_mpoly_t Q;
    timeit_t timer;
    slong num_threads;
    slong serial_time;

    flint_printf("\n******** starting %s (%wu, %wd) Abits: %wd:\n", name, m1, n1, _fmpz_vec_max_bits(A->coeffs, A->length));

    flint_set_num_threads(1);
    fmpz_mpoly_init(Q, ctx);
    timeit_start(timer);
    fmpz_mpoly_divides(Q, A, B, ctx);
    timeit_stop(timer);
    serial_time = FLINT_MAX(WORD(1), timer->wall);
    flint_printf("serial time: %wd\n", serial_time);
    if (!fmpz_mpoly_equal(Q, realQ, ctx))
    {
        printf("quotient wrong\n");
        flint_abort();
    }

    for (num_threads = 2; num_threads <= max_threads; num_threads++)
    {
        thread_pool_handle * handles;
        slong num_workers;
        worker_arg_struct * worker_args;
        slong parallel_time;
        slong i;
        double machine_efficiency, parallel_efficiency;

        flint_set_num_threads(num_threads);
        flint_set_thread_affinity(cpu_affinities, num_threads);

        /* find machine efficiency */

        handles = (thread_pool_handle *) flint_malloc((num_threads - 1)*sizeof(thread_pool_handle));
        num_workers = thread_pool_request(global_thread_pool, handles, num_threads - 1);
        worker_args = (worker_arg_struct *) flint_malloc((num_workers + 1)*sizeof(worker_arg_t));

        timeit_start(timer);
        for (i = 0; i <= num_workers; i++)
        {
            fmpz_mpoly_init((worker_args + i)->Q, ctx);
            (worker_args + i)->A = A;
            (worker_args + i)->B = B;
            (worker_args + i)->ctx = ctx;
            if (i < num_workers)
            {
                thread_pool_wake(global_thread_pool, handles[i], worker_divides, worker_args + i);
            }
            else
            {
                worker_divides(worker_args + i);
            }
        }
        for (i = 0; i < num_workers; i++)
        {
            thread_pool_wait(global_thread_pool, handles[i]);
        }
        timeit_stop(timer);
        parallel_time = FLINT_MAX(WORD(1), timer->wall);

        for (i = 0; i <= num_workers; i++)
        {
            if (!fmpz_mpoly_equal((worker_args + i)->Q, realQ, ctx))
            {
                printf("quotient wrong\n");
                flint_abort();
            }
            fmpz_mpoly_clear((worker_args + i)->Q, ctx);

            if (i < num_workers)
            {
                thread_pool_give_back(global_thread_pool, handles[i]);
            }
        }
        flint_free(worker_args);
        flint_free(handles);

        machine_efficiency = (double)(serial_time)/(double)(parallel_time);

        fmpz_mpoly_clear(Q, ctx);
        fmpz_mpoly_init(Q, ctx);
        timeit_start(timer);
        fmpz_mpoly_divides(Q, A, B, ctx);
        timeit_stop(timer);
        parallel_time = FLINT_MAX(WORD(1), timer->wall);
        if (!fmpz_mpoly_equal(Q, realQ, ctx))
        {
            printf("quotient wrong\n");
            flint_abort();
        }

        parallel_efficiency = (double)(serial_time)/(double)(parallel_time)/(double)(num_threads);

        flint_printf("parallel %wd time: %wd, efficiency %f (machine %f)\n", num_threads, parallel_time, parallel_efficiency, machine_efficiency);
    }

    fmpz_mpoly_clear(Q, ctx);
}


int main(int argc, char *argv[])
{
    slong i, m, n;

    max_threads = argc > 1 ? atoi(argv[1]) : 2;
    max_threads = FLINT_MIN(max_threads, WORD(32));
    max_threads = FLINT_MAX(max_threads, WORD(1));

    flint_printf("setting max_threads = %wd\n", max_threads);

    cpu_affinities = flint_malloc(max_threads*sizeof(int));
    for (i = 0; i < max_threads; i++)
        cpu_affinities[i] = i;

    for (m = 6 + max_threads/4; m <= 12 + max_threads/4; m += 3)
    for (n = 6 + max_threads/4; n <= 12 + max_threads/4; n += 3)
    {
        fmpz_mpoly_ctx_t ctx;
        fmpz_mpoly_t a, b, A, B, Q;
        const char * vars[] = {"x", "y", "z", "t", "u"};

        fmpz_mpoly_ctx_init(ctx, 5, ORD_LEX);
        fmpz_mpoly_init(a, ctx);
        fmpz_mpoly_init(b, ctx);
        fmpz_mpoly_init(A, ctx);
        fmpz_mpoly_init(B, ctx);
        fmpz_mpoly_init(Q, ctx);

        fmpz_mpoly_set_str_pretty(a, "1+x+1*y^2+1*z^3+1*t^4+1*u^5", vars, ctx);
        fmpz_mpoly_set_str_pretty(b, "1+u+1*t^2+1*z^3+1*y^4+1*x^5", vars, ctx);

        fmpz_mpoly_pow_ui(Q, a, m, ctx);
        fmpz_mpoly_pow_ui(B, b, n, ctx);
        fmpz_mpoly_mul(A, Q, B, ctx);

        profile_divides(Q, A, B, ctx, "sparse divides", m, n);

        fmpz_mpoly_clear(Q, ctx);
        fmpz_mpoly_clear(B, ctx);
        fmpz_mpoly_clear(A, ctx);
        fmpz_mpoly_clear(b, ctx);
        fmpz_mpoly_clear(a, ctx);
        fmpz_mpoly_ctx_clear(ctx);
    }

    flint_free(cpu_affinities);

    flint_cleanup();
    return 0;
}
