/*
 * Copyright (c) 2018-2019, Laurence Lundblade. All rights reserved.
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * See BSD-3-Clause license in README.md
 */

#include "qcbor/qcbor.h"
#include "t_cose_crypto.h"
#include "t_cose/t_cose_mac_sign.h"
#include "t_cose_util.h"
#include "t_cose/t_cose_parameters.h"

/**
 * \file t_cose_mac_sign.c
 *
 * \brief This creates t_cose Mac authentication structure without a recipient
 *        structure.
 *        Only HMAC is supported so far.
 */

#ifndef T_COSE_DISABLE_MAC0

#ifndef T_COSE_DISABLE_SHORT_CIRCUIT_SIGN
/**
 * \brief Create a short-circuit tag
 *
 * \param[in] cose_alg_id  Algorithm ID. This is used only to make
 *                         the short-circuit tag the same size as the
 *                         real tag would be for the particular algorithm.
 * \param[in] header       The Header of COSE_Mac0.
 * \param[in] payload      The payload of COSE_Mac0
 * \param[in] tag_buffer   Pointer and length of buffer into which
 *                         the resulting tag is put.
 * \param[out] tag         Pointer and length of the tag returned.
 *
 * \return This returns one of the error codes defined by \ref t_cose_err_t.
 *
 * This creates the short-circuit tag that is actually a hash of input bytes.
 * This is a test mode only has it has no security value. This is retained in
 * commercial production code as a useful test or demo that can run
 * even if key material is not set up or accessible.
 */
static inline enum t_cose_err_t
short_circuit_tag(int32_t                cose_alg_id,
                  struct q_useful_buf_c  header,
                  struct q_useful_buf_c  payload,
                  struct q_useful_buf    tag_buffer,
                  struct q_useful_buf_c *tag)
{
    /* approximate stack use on 32-bit machine: local use: 16 bytes */
    enum t_cose_err_t         return_value;
    struct t_cose_crypto_hash hash_ctx;
    size_t                    tag_size;
    int32_t                   hash_alg_id;

    /*
     * The length of Hash result equals that of HMAC result
     * with the same Hash algorithm.
     */
    tag_size = t_cose_tag_size(cose_alg_id);

    /* Check the tag length against buffer size */
    if(tag_size == INT32_MAX) {
        return_value = T_COSE_ERR_UNSUPPORTED_SIGNING_ALG;
        goto Done;
    }

    if(tag_size > tag_buffer.len) {
        /* Buffer too small for this tag */
        return_value = T_COSE_ERR_SIG_BUFFER_SIZE;
        goto Done;
    }

    hash_alg_id = t_cose_hmac_to_hash_alg_id(cose_alg_id);
    if(hash_alg_id == INT32_MAX) {
        return_value = T_COSE_ERR_UNSUPPORTED_SIGNING_ALG;
        goto Done;
    }

    return_value = t_cose_crypto_hash_start(&hash_ctx, hash_alg_id);
    if(return_value != T_COSE_SUCCESS) {
        goto Done;
    }

    /* Hash the Header */
    t_cose_crypto_hash_update(&hash_ctx, q_useful_buf_head(header, header.len));

    /* Hash the payload */
    t_cose_crypto_hash_update(&hash_ctx, payload);

    return_value = t_cose_crypto_hash_finish(&hash_ctx, tag_buffer, tag);

Done:
    return return_value;
}
#endif /* T_COSE_DISABLE_SHORT_CIRCUIT_SIGN */


enum t_cose_err_t
t_cose_sign_one_short(struct t_cose_mac_sign_ctx *context,
                      bool                         payload_is_detached,
                      struct q_useful_buf_c        aad,
                      struct q_useful_buf_c        payload,
                      struct q_useful_buf          out_buf,
                      struct q_useful_buf_c       *result)
{
    QCBOREncodeContext  encode_ctx;
    enum t_cose_err_t   return_value;

    /* -- Initialize CBOR encoder context with output buffer -- */
    QCBOREncode_Init(&encode_ctx, out_buf);

    /* -- Output the header parameters into the encoder context -- */
    return_value = t_cose_mac_encode_parameters(context, &encode_ctx);
    if(return_value != T_COSE_SUCCESS) {
        goto Done;
    }

    QCBOREncode_AddEncoded(&encode_ctx, payload);

    return_value = t_cose_mac_encode_tag(context,&encode_ctx);
    if(return_value) {
        goto Done;
    }

    /* -- Close off and get the resulting encoded CBOR -- */
    if(QCBOREncode_Finish(&encode_ctx, result)) {
        return_value = T_COSE_ERR_CBOR_NOT_WELL_FORMED;
        goto Done;
    }

Done:
    return return_value;
}

/*
 * Public function. See t_cose_mac.h
 */
enum t_cose_err_t
t_cose_mac_encode_parameters(struct t_cose_mac_sign_ctx *me,
                              QCBOREncodeContext        *cbor_encode_ctx)

{
    size_t                            tag_len;
    enum t_cose_err_t                 return_value;
    struct q_useful_buf               buffer_for_protected_parameters;
    struct q_useful_buf_c             kid;
    const struct t_cose_header_param *params_vector[3];
    struct t_cose_header_param        protected_params_arr[2];
#ifndef T_COSE_DISABLE_CONTENT_TYPE
    struct t_cose_header_param        unprotected_params_arr[3];
    struct t_cose_header_param        ct_param;
#else
    struct t_cose_header_param        unprotected_params_arr[3];
#endif

    struct t_cose_header_param alg_id_param;
    struct t_cose_header_param kid_param;
    struct t_cose_header_param end_param = T_COSE_END_PARAM;

    /*
     * Check the algorithm now by getting the algorithm as an early
     * error check even though it is not used until later.
     */
    tag_len = t_cose_tag_size(me->cose_algorithm_id);
    if(tag_len == INT32_MAX) {
        return T_COSE_ERR_UNSUPPORTED_SIGNING_ALG;
    }

    /* Add the CBOR tag indicating COSE_Mac0 */
    if(!(me->option_flags & T_COSE_OPT_OMIT_CBOR_TAG)) {
        QCBOREncode_AddTag(cbor_encode_ctx, CBOR_TAG_COSE_MAC0);
    }

    /* Get started with the tagged array that holds the parts of
     * a COSE_Mac0 message
     */
    QCBOREncode_OpenArray(cbor_encode_ctx);


    if(me->option_flags & T_COSE_OPT_SHORT_CIRCUIT_TAG) {
#ifndef T_COSE_DISABLE_SHORT_CIRCUIT_SIGN
        kid = NULL_Q_USEFUL_BUF_C;
#else
        return_value = T_COSE_ERR_SHORT_CIRCUIT_SIG_DISABLED;
        goto Done;
#endif
    } else {
        /* Get the kid because it goes into the parameters that are about
         * to be made.
         */
        kid = me->kid;
    }

    params_vector[0] = protected_params_arr;
    params_vector[1] = unprotected_params_arr;
    params_vector[2] = NULL;

    protected_params_arr[0] = T_COSE_MAKE_ALG_ID_PARAM(me->cose_algorithm_id);
    protected_params_arr[1] = T_COSE_END_PARAM;

    unprotected_params_arr[0] = T_COSE_KID_PARAM(me->kid);
    unprotected_params_arr[1] = T_COSE_END_PARAM;

#ifndef T_COSE_DISABLE_CONTENT_TYPE
    unprotected_params_arr[2] = T_COSE_END_PARAM;

    if(me->content_type_uint != T_COSE_EMPTY_UINT_CONTENT_TYPE &&
       !q_useful_buf_c_is_null(me->content_type_tstr)) {
        /* Both the string and int content types are not allowed */
        return T_COSE_ERR_DUPLICATE_PARAMETER;
    }

    if(me->content_type_uint != T_COSE_EMPTY_UINT_CONTENT_TYPE) {
        unprotected_params_arr[1] = T_COSE_CT_INT_PARAM(me->content_type_uint);
    }

    if(!q_useful_buf_c_is_null(me->content_type_tstr)) {
        unprotected_params_arr[1] = T_COSE_CT_TSTR_PARAM(me->content_type_tstr);
    }
#endif

    return_value = t_cose_encode_headers(
        cbor_encode_ctx,
        params_vector,
       &me->protected_parameters
    );

    /* --- Get started on the payload --- */
    QCBOREncode_BstrWrap(cbor_encode_ctx);

    /*
     * Any failures in CBOR encoding will be caught in finish
     * when the CBOR encoding is closed off. No need to track
     * here as the CBOR encoder tracks it internally.
     */

Done:
    return return_value;
}

/*
 * Public function. See t_cose_mac.h
 */
enum t_cose_err_t
t_cose_mac_encode_tag(struct t_cose_mac_sign_ctx *me,
                       QCBOREncodeContext        *cbor_encode_ctx)

{
    enum t_cose_err_t            return_value;
    QCBORError                   cbor_err;
    /* Pointer and length of the completed tag */
    struct q_useful_buf_c        tag;
    /* Buffer for the actual tag */
    Q_USEFUL_BUF_MAKE_STACK_UB(  tag_buf,
                                 T_COSE_CRYPTO_HMAC_TAG_MAX_SIZE);
    struct q_useful_buf_c        tbm_first_part;
    /* Buffer for the ToBeMaced */
    UsefulBuf_MAKE_STACK_UB(     tbm_first_part_buf,
                                 T_COSE_SIZE_OF_TBM);
    struct t_cose_crypto_hmac    hmac_ctx;
    struct q_useful_buf_c        maced_payload;

    QCBOREncode_CloseBstrWrap2(cbor_encode_ctx, false, &maced_payload);

    /* Check that there are no CBOR encoding errors before proceeding
     * with hashing and tagging. This is not actually necessary as the
     * errors will be caught correctly later, but it does make it a
     * bit easier for the caller to debug problems.
     */
    cbor_err = QCBOREncode_GetErrorState(cbor_encode_ctx);
    if(cbor_err == QCBOR_ERR_BUFFER_TOO_SMALL) {
        return_value = T_COSE_ERR_TOO_SMALL;
        goto Done;
    } else if(cbor_err != QCBOR_SUCCESS) {
        return_value = T_COSE_ERR_CBOR_FORMATTING;
        goto Done;
    }

    if(QCBOREncode_IsBufferNULL(cbor_encode_ctx)) {
        /* Just calculating sizes. All that is needed is the tag size. */
        tag.ptr = NULL;
        tag.len = t_cose_tag_size(me->cose_algorithm_id);

        return_value = T_COSE_SUCCESS;
        goto CloseArray;
    }

    /* Create the hash of the ToBeMaced bytes. Inputs to the
     * MAC are the protected parameters, the payload that is
     * getting MACed.
     */
    return_value = create_tbm(tbm_first_part_buf,
                              me->protected_parameters,
                              &tbm_first_part,
                              T_COSE_TBM_BARE_PAYLOAD,
                              maced_payload);
    if(return_value) {
        goto Done;
    }

    /*
     * Start the HMAC.
     * Calculate the tag of the first part of ToBeMaced and the wrapped
     * payload, to save a bigger buffer containing the entire ToBeMaced.
     *
     * Short-circuit tagging is invoked if requested. It does no HMAC operation
     * and requires no key. It is just a test mode that works without accessing
     * any device asset.
     */
    if(me->option_flags & T_COSE_OPT_SHORT_CIRCUIT_TAG) {
#ifndef T_COSE_DISABLE_SHORT_CIRCUIT_SIGN
        /* Short-circuit tag. Hash is used to generated tag instead of HMAC */
        return_value = short_circuit_tag(me->cose_algorithm_id,
                                         tbm_first_part,
                                         maced_payload,
                                         tag_buf,
                                         &tag);
        if(return_value) {
            goto Done;
        }

        goto CloseArray;
#else
        return_value = T_COSE_ERR_SHORT_CIRCUIT_SIG_DISABLED;
        goto Done;
#endif
    }

    return_value = t_cose_crypto_hmac_sign_setup(&hmac_ctx,
                                                 me->signing_key,
                                                 me->cose_algorithm_id);
    if(return_value) {
        goto Done;
    }

    /* Compute the tag of the first part. */
    return_value = t_cose_crypto_hmac_update(&hmac_ctx,
                                             q_useful_buf_head(tbm_first_part,
                                                           tbm_first_part.len));
    if(return_value) {
        goto Done;
    }

    /*
     * It is assumed that the context payload has been wrapped in a byte
     * string in CBOR format.
     */
    return_value = t_cose_crypto_hmac_update(&hmac_ctx, maced_payload);
    if(return_value) {
        goto Done;
    }

    return_value = t_cose_crypto_hmac_sign_finish(&hmac_ctx, tag_buf, &tag);
    if(return_value) {
        goto Done;
    }

CloseArray:
    /* Add tag to CBOR and close out the array */
    QCBOREncode_AddBytes(cbor_encode_ctx, tag);
    QCBOREncode_CloseArray(cbor_encode_ctx);

    /* CBOR encoding errors are tracked in the CBOR encoding context
     * and handled in the layer above this
     */

Done:
    return return_value;
}

#endif /* !T_COSE_DISABLE_MAC0 */