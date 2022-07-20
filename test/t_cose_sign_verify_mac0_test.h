/*
 *  t_cose_sign_verify_mac0_test.h
 *
 * Copyright 2019, 2022, Laurence Lundblade
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * See BSD-3-Clause license in README.md
 */

#ifndef t_cose_sign_verify_mac0_test_h
#define t_cose_sign_verify_mac0_test_h

#include <stdint.h>


/**
 * \file t_cose_sign_verify_mac0_test.h
 *
 * \brief Tests that need symmetric key crypto to be implemented
 */


/**
 * \brief Self test using integrated  crypto.
 *
 * \return non-zero on failure.
 */
int_fast32_t sign_verify_mac0_basic_test(void);


/*
 * Sign some data, perturb the data and see that sig validation fails
 */
int_fast32_t sign_verify_mac0_sig_fail_test(void);


/*
 * Test the ability to calculate size of a COSE_Sign1
 */
int_fast32_t sign_verify_get_size_mac0_test(void);

#endif /* t_cose_sign_verify_mac0_test_h */
