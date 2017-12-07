/*
    Copyright (C) 2017 Daniel Schultz

    This file is part of FLINT.

    FLINT is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include "flint.h"
#include "mpoly.h"
#include "ulong_extras.h"

int
main(void)
{
    slong k, i, j, length, nfields, bits1, bits2;
    slong * bases;
    ulong * a, * b, * c, * d, * t;
    ulong max_length, max_fields;
    FLINT_TEST_INIT(state);

    flint_printf("pack_unpack_tight....");
    fflush(stdout);

    max_length = 100;
    max_fields = FLINT_BITS/8;  /* exponents should fit in one word */

    a = flint_malloc(max_length*max_fields*sizeof(ulong));
    b = flint_malloc(max_length*max_fields*sizeof(ulong));
    c = flint_malloc(max_length*max_fields*sizeof(ulong));
    d = flint_malloc(max_length*max_fields*sizeof(ulong));
    t = flint_malloc(max_length*sizeof(ulong));
    bases = flint_malloc(max_fields*sizeof(slong));

    for (k = 0; k < 1000 * flint_test_multiplier(); k++)
    {
        /* do FLINT_BITS => bits1 
                         => tight packing
                         => bits2 => FLINT_BITS and compare */
        for (bits1 = 8; bits1 <= FLINT_BITS; bits1 *= 2)
        for (bits2 = bits1; bits2 <= FLINT_BITS; bits2 *= 2)
        {
            mpoly_ctx_t mctx;

            length = n_randint(state, max_length) + 1;
            nfields = n_randint(state, FLINT_BITS/FLINT_MAX(bits1, bits2)) + 1;

            mpoly_ctx_init(mctx, nfields, ORD_LEX);

            for (j = 0; j < nfields; j++)
                bases[j] =  n_randint(state, 200) + 1;

            for (i = 0; i < nfields*length; i += nfields)
                for (j = 0; j < nfields; j++)
                    a[i + j] = n_randint(state, bases[nfields - j - 1]);

            /* FLINT_BITS => bits1 */
            for (i = 0; i < length; i++)
                mpoly_set_monomial_ui(b + i, a + i*nfields, bits1, mctx);

            /* bits1 => tight packing */
            mpoly_pack_monomials_tight(t, b, length, bases, nfields, 0, bits1);

            /* tight packing => bits2 */
            mpoly_unpack_monomials_tight(c, t, length, bases, nfields, 0, bits2);

            /* bits2 => FLINT_BITS */
            mpoly_repack_monomials(d, FLINT_BITS, c, bits2, length, mctx);

            for (i = 0; i < length*nfields; i++)
                if (a[i] != d[i])
                {
                    printf("FAIL\nunpack_monomials_tight\n");
                    flint_printf("bits1 = %wd, bits2 = %wd\n", bits1, bits2);
                    flint_abort();
                }

            mpoly_ctx_clear(mctx);
        }
    }

    flint_free(bases);
    flint_free(t);
    flint_free(d);
    flint_free(c);
    flint_free(b);
    flint_free(a);

    FLINT_TEST_CLEANUP(state);

    flint_printf("PASS\n");
    return 0;
}

