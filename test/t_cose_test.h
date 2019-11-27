/*
 *  t_cose_test.h
 *
 * Copyright 2019, Laurence Lundblade
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * See BSD-3-Clause license in README.md
 */

#ifndef t_cose_test_h
#define t_cose_test_h

#include <stdint.h>

/**
 * \file t_cose_test.h
 *
 * \brief Entry points for the basic t_cose_tests.
 */


/**
 * \brief Minimal message creation test using a short-circuit signature.
 *
 * \return non-zero on failure.
 *
 * This test makes a simple COSE_Sign1 and verify it.  It uses
 * short-circuit signatures so no keys or even integration with public
 * key crypto is necessary.
 */
int_fast32_t short_circuit_self_test(void);


/**
 * \brief Test where payload bytes are corrupted and sig fails.
 *
 * \return non-zero on failure.
 *
 * This test makes a simple COSE_Sign1 modify the payload and see that
 * verification fails.  It uses short-circuit signatures so no keys or
 * even integration with public key crypto is necessary.
 */
int_fast32_t short_circuit_verify_fail_test(void);


/**
 * \brief Tests error condidtions for creating COSE_Sign1.
 *
 * \return non-zero on failure.
 *
 * It uses short-circuit signatures so no keys or even integration
 * with public key crypto is necessary.
 */
int_fast32_t short_circuit_signing_error_conditions_test(void);


/* Make a CWT and see that it compares to the sample in the CWT RFC
 */
int_fast32_t short_circuit_make_cwt_test(void);


/*
 * Test the decode only mode.
 */
int_fast32_t short_circuit_decode_only_test(void);


/*
- protected header parameters not well formed CBOR
- unprotected header parameters not well formed CBOR
- unknown algorithm ID
- No algorithm ID parameter

 */
int_fast32_t bad_parameters_test(void);


/* Test that makes a CWT (CBOR Web Token)
 */
int_fast32_t cose_example_test(void);


/*
 Various tests involving the crit parameter.
 */
int_fast32_t crit_parameters_test(void);


/*
 Check that all types of headers are correctly returned.
 */
int_fast32_t all_header_parameters_test(void);

/*
 * Check that setting the content type works
 */
int_fast32_t content_type_test(void);


/*
 * Check that setting the content type works
 */
int_fast32_t sign1_structure_decode_test(void);


/*
 * Use the short-circuit signing to test hash algorithm failures
 */
int_fast32_t short_circuit_hash_fail_test(void);


#endif /* t_cose_test_h */
