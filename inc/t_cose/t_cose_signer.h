//
//  t_cose_signer.h
//  t_cose
//
//  Created by Laurence Lundblade on 5/23/22.
//  Copyright © 2022 Laurence Lundblade. All rights reserved.
//

#ifndef t_cose_signer_h
#define t_cose_signer_h

#include "qcbor/qcbor_encode.h"

/* This is an "abstract base class" for all signers
 * of all types for all algorithms. This is the interface
 * and data structure that t_cose_sign knows about to be able
 * to invoke each signer regardles of its type or algorithm.
 *
 * Each concrete signer (e.g., ECDSA signer, RSA signer,... must implement this. Each signer
 * also implements a few methods of its own beyond this
 * that it needs to work.
 *
 * t_cose_signer_callback is the type of a function that every
 * signer must implement. It takes as input the context
 * for the particular signer, the hash to sign and
 * the encoder instance. The work it does is to produce
 * the signature and output the COSE_Signature to the
 * encoder instance.
 *
 * This design allows new signers for new algorithms to be
 * added without modifying or even recompiling t_cose.
 * It is a clean and simple design that allows outputting a COSE_Sign
 * that has multiple signings by multiple aglorithms, for example
 * an ECDSA signature and an HSS/LMS signature.
 *
 * What's really going on here is a bit of doing object orientation
 * implementned in C. This is an abstract base class, an object that
 * has no implementation of it's own. Each signer type, e.g.,
 * the ECDSA signer, inherits from this and provides an
 * implementation. The structure defined here holds
 * the vtable for the methods for the object. Only one method
 * happens to be needed. It's called a "callback" here, but it could
 * also be called the abstract sign method.
 *
 * Since C doesn't support object orientation there's a few
 * tricks to make this fly. The concrete instantiation
 * (e.g., an ECDSA signer) must make struct t_cose_signer the first
 * part of it's context and there will be casts back and
 * forth between this abstraction and the real instantion
 * of the signer. The other trick is that struct here
 * contains a pointer to a function and that makes up
 * the vtable, something that C++ would do for you.
 */

/* A declaration (not definition) of the generic structure for a signer.
 * See https://stackoverflow.com/questions/888386/resolve-circular-typedef-dependency
 */
struct t_cose_signer;

/* The main method / callback used to perform the signing and output
 * the COSE format signature.
 * If the qcbor_encoder buffer is NULL, then this should just
 * compute the size and add the size to qcbor_encoder because it
 * is being called in size calculation mode.
 */
typedef enum t_cose_err_t
(* t_cose_signer_callback)(struct t_cose_signer        *me,
                           const struct q_useful_buf_c  protected_body_headers,
                           const struct q_useful_buf_c  payload,
                           const struct q_useful_buf_c  aad,
                           QCBOREncodeContext          *qcbor_encoder);



/* The definition (not declaration) of the context that every
 * signer implemtnation has.
 */
struct t_cose_signer {
    t_cose_signer_callback   callback; /* some will call this a vtable with one entry */
    struct t_cose_signer    *next_in_list; /* Linked list of signers */
};


#endif /* t_cose_signer_h */
