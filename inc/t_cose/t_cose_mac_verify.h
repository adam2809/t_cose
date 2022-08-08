/*
 * Copyright (c) 2019, Laurence Lundblade. All rights reserved.
 * Copyright (c) 2020 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __T_COSE_MAC_VERIFY_H_
#define __T_COSE_MAC_VERIFY_H_

#include <stdint.h>
#include "qcbor/qcbor.h"
#include "t_cose_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The maximum number of unprocessed tags that can be returned by
 * t_cose_sign1_get_nth_tag(). The CWT
 * tag is an example of the tags that might returned. The COSE tags
 * that are processed, don't count here.
 */
#define T_COSE_MAX_TAGS_TO_RETURN 4

/**
 * Context for tag verification.  It is about 24 bytes on a
 * 64-bit machine and 12 bytes on a 32-bit machine.
 */
struct t_cose_mac_verify_ctx {
    /* Private data structure */
    struct t_cose_key     verification_key;
    int32_t               option_flags;
    uint64_t              auTags[T_COSE_MAX_TAGS_TO_RETURN];
};


/**
 * \brief Initialize for \c COSE_Mac0 message verification.
 *
 * \param[in,out]  context       The context to initialize.
 * \param[in]      option_flags  Options controlling the verification.
 *
 * This must be called before using the verification context.
 */
static void
t_cose_mac_verify_init(struct t_cose_mac_verify_ctx *context,
                        int32_t                      option_flags);


/**
 * \brief Set key for \c COSE_Mac0 message verification.
 *
 * \param[in,out] context      The context of COSE_Mac0 verification
 * \param[in] verify_key       The verification key to use.
 *
 * Look up by kid parameter and fetch the key for MAC verification.
 * Setup the \ref verify_key structure and fill it in \ref context.
 */
static void
t_cose_mac_set_verify_key(struct t_cose_mac_verify_ctx *context,
                           struct t_cose_key            verify_key);

/**
 * \brief Verify a COSE_Mac0
 *
 * \param[in] context      The context of COSE_Mac0 verification
 * \param[in] cose_mac    Pointer and length of CBOR encoded \c COSE_Mac0
 *                         that is to be verified.
 * \param[out] payload     Pointer and length of the still CBOR encoded
 *                         payload
 *
 * \return This returns one of the error codes defined by \ref t_cose_err_t.
 *
 * Verification involves the following steps.
 *
 * The CBOR structure is parsed and verified. It makes sure \c COSE_Mac0
 * is valid CBOR and that it is tagged as a \c COSE_Mac0.
 *
 * The signing algorithm is pulled out of the protected headers.
 *
 * The kid (key ID) is parsed out of the unprotected headers if it exists.
 *
 * The payload is identified. It doesn't have to be parsed in detail
 * because it is wrapped in a bstr.
 *
 * Finally, the MAC verification is performed if \ref T_COSE_OPT_DECODE_ONLY
 * is not set in option flag. Otherwise, the verification will be skipped.
 * The MAC algorithm to use comes from the signing algorithm in the
 * protected headers.
 * If the algorithm is not known or not supported this will error out.
 *
 * If it is successful, the pointer of the CBOR-encoded payload is
 * returned.
 */
static enum t_cose_err_t t_cose_mac_verify(struct t_cose_mac_verify_ctx *context,
                                     struct q_useful_buf_c        cose_mac,
                                     struct q_useful_buf_c       *payload);

static enum t_cose_err_t t_cose_mac_verify_detached(struct t_cose_mac_verify_ctx *context,
                                     struct q_useful_buf_c        cose_mac,
                                     struct q_useful_buf_c       *detached_payload);

static enum t_cose_err_t t_cose_mac_verify_aad(struct t_cose_mac_verify_ctx *context,
                                     struct q_useful_buf_c        cose_mac,
                                     struct q_useful_buf_c         aad,
                                     struct q_useful_buf_c       *payload);

enum t_cose_err_t t_cose_mac_verify_private(struct t_cose_mac_verify_ctx *context,
                                            struct q_useful_buf_c         cose_mac,
                                            struct q_useful_buf_c         aad,
                                            bool                          payload_is_detached,
                                            struct q_useful_buf_c        *payload);

/* ------------------------------------------------------------------------
 * Inline implementations of public functions defined above.
 */
static inline void
t_cose_mac_verify_init(struct t_cose_mac_verify_ctx *context,
                        int32_t                      option_flags)
{
    context->option_flags = option_flags;
    context->verification_key = T_COSE_NULL_KEY;
}

static inline void
t_cose_mac_set_verify_key(struct t_cose_mac_verify_ctx *context,
                           struct t_cose_key            verify_key)
{
    context->verification_key = verify_key;
}

static inline enum t_cose_err_t
t_cose_mac_verify(struct t_cose_mac_verify_ctx *context,
                                     struct q_useful_buf_c        cose_mac,
                                     struct q_useful_buf_c       *payload){
    return t_cose_mac_verify_private(
        context,
        cose_mac,
        NULL_Q_USEFUL_BUF_C,
        false,
        payload 
    );
}

static inline enum t_cose_err_t
t_cose_mac_verify_detached(struct t_cose_mac_verify_ctx *context,
                           struct q_useful_buf_c        cose_mac,
                           struct q_useful_buf_c       *detached_payload){
    return t_cose_mac_verify_private(
        context,
        cose_mac,
        NULL_Q_USEFUL_BUF_C,
        true,
        detached_payload 
    );
}

static inline enum t_cose_err_t 
t_cose_mac_verify_aad(struct t_cose_mac_verify_ctx *context,
                      struct q_useful_buf_c         cose_mac,
                      struct q_useful_buf_c         aad,
                      struct q_useful_buf_c        *payload){
    return t_cose_mac_verify_private(
        context,
        cose_mac,
        aad,
        false,
        payload
    );
}
#ifdef __cplusplus
}
#endif

#endif /* __T_COSE_MAC_VERIFY_H_ */