/*
 * Copyright 2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/ssl.h>
#include <openssl/evp.h>
#include "../ssl/ssl_locl.h"

#include "testutil.h"
#include "test_main.h"

#define IVLEN   12
#define KEYLEN  16

/* The following are self-generated test vectors. This gives us very little
 * confidence that we've got the implementation right, but at least tells us
 * if we accidentally  break something in the future. Until we can get some
 * other source of test vectors this is all we've got.
 * TODO(TLS1.3): As and when official vectors become available we should use
 * those, e.g. see
 * https://www.ietf.org/id/draft-thomson-tls-tls13-vectors-00.txt, however at
 * the time of writing these are not suitable because they are based on
 * draft -16, which works differently to the draft -19 vectors below.
 */

static unsigned char hs_start_hash[] = {
0xec, 0x14, 0x7a, 0x06, 0xde, 0xa3, 0xc8, 0x84, 0x6c, 0x02, 0xb2, 0x23, 0x8e,
0x41, 0xbd, 0xdc, 0x9d, 0x89, 0xf9, 0xae, 0xa1, 0x7b, 0x5e, 0xfd, 0x4d, 0x74,
0x82, 0xaf, 0x75, 0x88, 0x1c, 0x0a
};

static unsigned char hs_full_hash[] = {
0x75, 0x1a, 0x3d, 0x4a, 0x14, 0xdf, 0xab, 0xeb, 0x68, 0xe9, 0x2c, 0xa5, 0x91,
0x8e, 0x24, 0x08, 0xb9, 0xbc, 0xb0, 0x74, 0x89, 0x82, 0xec, 0x9c, 0x32, 0x30,
0xac, 0x30, 0xbb, 0xeb, 0x23, 0xe2,
};

static unsigned char early_secret[] = {
0x33, 0xad, 0x0a, 0x1c, 0x60, 0x7e, 0xc0, 0x3b, 0x09, 0xe6, 0xcd, 0x98, 0x93,
0x68, 0x0c, 0xe2, 0x10, 0xad, 0xf3, 0x00, 0xaa, 0x1f, 0x26, 0x60, 0xe1, 0xb2,
0x2e, 0x10, 0xf1, 0x70, 0xf9, 0x2a
};

static unsigned char ecdhe_secret[] = {
0xe7, 0xb8, 0xfe, 0xf8, 0x90, 0x3b, 0x52, 0x0c, 0xb9, 0xa1, 0x89, 0x71, 0xb6,
0x9d, 0xd4, 0x5d, 0xca, 0x53, 0xce, 0x2f, 0x12, 0xbf, 0x3b, 0xef, 0x93, 0x15,
0xe3, 0x12, 0x71, 0xdf, 0x4b, 0x40
};

static unsigned char handshake_secret[] = {
0xed, 0x80, 0x0a, 0x28, 0x28, 0xf6, 0x3b, 0x45, 0x6b, 0x26, 0xf6, 0x5c, 0x5e,
0x2e, 0x30, 0x57, 0xc8, 0x02, 0xe2, 0x03, 0x5c, 0xf6, 0xf2, 0xd7, 0x1b, 0x95,
0x45, 0x5f, 0xb0, 0x76, 0x23, 0x4e
};

static const char *client_hts_label = "client handshake traffic secret";

static unsigned char client_hts[] = {
0x39, 0x58, 0x9d, 0x3f, 0xaf, 0x5f, 0xc0, 0xc2, 0xa3, 0x56, 0x94, 0x4f, 0x0a,
0x08, 0x63, 0x84, 0xbc, 0xec, 0xcf, 0x2c, 0x25, 0x2f, 0xce, 0xfe, 0x0c, 0x57,
0xb3, 0xca, 0x21, 0xc4, 0x37, 0x73
};

static unsigned char client_hts_key[] = {
0x6d, 0x91, 0x24, 0xf2, 0xd8, 0xd7, 0x65, 0x90, 0x86, 0x4d, 0x04, 0xbc, 0x94,
0x26, 0xcb, 0x2f
};

static unsigned char client_hts_iv[] = {
0x16, 0x9b, 0x9f, 0x47, 0x16, 0x8a, 0xb5, 0x4d, 0xf5, 0x28, 0x1e, 0xe2
};

static const char *server_hts_label = "server handshake traffic secret";

static unsigned char server_hts[] = {
0x3a, 0xb5, 0x28, 0x21, 0x7d, 0xb3, 0xbc, 0x5a, 0xf4, 0x8b, 0xc2, 0x3a, 0x1b,
0x1e, 0x6f, 0x2b, 0x8e, 0xb0, 0xac, 0x26, 0xe9, 0x6d, 0xee, 0xa7, 0x3e, 0xfd,
0x0a, 0x9f, 0xc0, 0x28, 0x62, 0x70
};

static unsigned char server_hts_key[] = {
0x6d, 0x92, 0xe8, 0x71, 0x97, 0xf4, 0x12, 0xf5, 0x8f, 0x9c, 0xab, 0xf9, 0x55,
0xe9, 0x74, 0x7e
};

static unsigned char server_hts_iv[] = {
0x85, 0x45, 0xc8, 0x3e, 0x94, 0x68, 0x4f, 0xd9, 0xe4, 0xd8, 0x42, 0x64
};

static unsigned char master_secret[] = {
0x3d, 0x10, 0x81, 0xb3, 0x9d, 0x60, 0x3a, 0x9f, 0x3a, 0x1b, 0x7c, 0xec, 0x0d,
0xfc, 0x92, 0xe5, 0xca, 0xcc, 0x6c, 0xd6, 0xec, 0xd1, 0x58, 0xcd, 0xd9, 0x93,
0xf1, 0xfc, 0xe3, 0x10, 0x8e, 0x84
};

static const char *client_ats_label = "client application traffic secret";

static unsigned char client_ats[] = {
0xe6, 0xb3, 0xbd, 0x9b, 0x6b, 0xd5, 0xbf, 0x4c, 0xba, 0x8f, 0xbf, 0xc1, 0x15,
0xb2, 0x06, 0x34, 0x83, 0xfa, 0xad, 0x72, 0xb3, 0xb8, 0x08, 0xa7, 0xa8, 0xd1,
0x6e, 0xc5, 0x37, 0x1f, 0x4d, 0x9c
};

static unsigned char client_ats_key[] = {
0x87, 0xe6, 0xee, 0xdc, 0x4d, 0x9b, 0x0c, 0xa4, 0x65, 0xff, 0xe4, 0xb9, 0xeb,
0x2b, 0x26, 0xf9
};

static unsigned char client_ats_iv[] = {
0xf0, 0x15, 0xc6, 0xbc, 0x95, 0x89, 0xc8, 0x94, 0x03, 0x4d, 0x6c, 0x70
};

static const char *server_ats_label = "server application traffic secret";

static unsigned char server_ats[] = {
0x27, 0x72, 0x45, 0xe1, 0x1c, 0xcd, 0x40, 0x67, 0xdf, 0xa7, 0xf5, 0xd2, 0xa2,
0xda, 0xe8, 0x87, 0x61, 0x52, 0xc3, 0x0d, 0x7f, 0x62, 0x22, 0x03, 0xd4, 0x97,
0xfa, 0xaf, 0x31, 0xab, 0xaa, 0xba
};

static unsigned char server_ats_key[] = {
0xd2, 0xa3, 0x23, 0x54, 0x66, 0x92, 0x33, 0xa2, 0x49, 0x19, 0x74, 0xc2, 0x1a,
0x0e, 0x19, 0x01
};

static unsigned char server_ats_iv[] = {
0x70, 0x28, 0x1e, 0x0d, 0xf7, 0xd4, 0x16, 0x97, 0xd3, 0xc3, 0xc4, 0x51
};

/* Mocked out implementations of various functions */
int ssl3_digest_cached_records(SSL *s, int keep)
{
    return 1;
}

static int full_hash = 0;

/* Give a hash of the currently set handshake */
int ssl_handshake_hash(SSL *s, unsigned char *out, size_t outlen,
                       size_t *hashlen)
{
    if (sizeof(hs_start_hash) > outlen
            || sizeof(hs_full_hash) != sizeof(hs_start_hash))
        return 0;

    if (full_hash) {
        memcpy(out, hs_full_hash, sizeof(hs_full_hash));
        *hashlen = sizeof(hs_full_hash);
    } else {
        memcpy(out, hs_start_hash, sizeof(hs_start_hash));
        *hashlen = sizeof(hs_start_hash);
    }

    return 1;
}

const EVP_MD *ssl_handshake_md(SSL *s)
{
    return EVP_sha256();
}

void RECORD_LAYER_reset_read_sequence(RECORD_LAYER *rl)
{
}

void RECORD_LAYER_reset_write_sequence(RECORD_LAYER *rl)
{
}

int ssl_cipher_get_evp(const SSL_SESSION *s, const EVP_CIPHER **enc,
                       const EVP_MD **md, int *mac_pkey_type,
                       size_t *mac_secret_size, SSL_COMP **comp, int use_etm)

{
    return 0;
}

int tls1_alert_code(int code)
{
    return code;
}

int ssl_log_secret(SSL *ssl,
                   const char *label,
                   const uint8_t *secret,
                   size_t secret_len)
{
    return 1;
}

const EVP_MD *ssl_md(int idx)
{
    return EVP_sha256();
}

/* End of mocked out code */

static int test_secret(SSL *s, unsigned char *prk,
                       const unsigned char *label, size_t labellen,
                       const unsigned char *ref_secret,
                       const unsigned char *ref_key, const unsigned char *ref_iv)
{
    size_t hashsize;
    unsigned char gensecret[EVP_MAX_MD_SIZE];
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned char key[KEYLEN];
    unsigned char iv[IVLEN];
    const EVP_MD *md = ssl_handshake_md(s);

    if (!ssl_handshake_hash(s, hash, sizeof(hash), &hashsize)) {
        fprintf(stderr, "Failed to get hash\n");
        return 0;
    }

    if (!tls13_hkdf_expand(s, md, prk, label, labellen, hash, gensecret,
                           hashsize)) {
        fprintf(stderr, "Secret generation failed\n");
        return 0;
    }

    if (memcmp(gensecret, ref_secret, hashsize) != 0) {
        fprintf(stderr, "Generated secret does not match\n");
        return 0;
    }

    if (!tls13_derive_key(s, md, gensecret, key, KEYLEN)) {
        fprintf(stderr, "Key generation failed\n");
        return 0;
    }

    if (memcmp(key, ref_key, KEYLEN) != 0) {
        fprintf(stderr, "Generated key does not match\n");
        return 0;
    }

    if (!tls13_derive_iv(s, md, gensecret, iv, IVLEN)) {
        fprintf(stderr, "IV generation failed\n");
        return 0;
    }

    if (memcmp(iv, ref_iv, IVLEN) != 0) {
        fprintf(stderr, "Generated IV does not match\n");
        return 0;
    }

    return 1;
}

static int test_handshake_secrets(void)
{
    SSL_CTX *ctx = NULL;
    SSL *s = NULL;
    int ret = 0;
    size_t hashsize;
    unsigned char out_master_secret[EVP_MAX_MD_SIZE];
    size_t master_secret_length;

    ctx = SSL_CTX_new(TLS_method());
    if (ctx == NULL)
        goto err;

    s = SSL_new(ctx);
    if (s == NULL)
        goto err;

    s->session = SSL_SESSION_new();
    if (s->session == NULL)
        goto err;

    if (!tls13_generate_secret(s, ssl_handshake_md(s), NULL, NULL, 0,
                               (unsigned char *)&s->early_secret)) {
        fprintf(stderr, "Early secret generation failed\n");
        goto err;
    }

    if (memcmp(s->early_secret, early_secret, sizeof(early_secret)) != 0) {
        fprintf(stderr, "Early secret does not match\n");
        goto err;
    }

    if (!tls13_generate_handshake_secret(s, ecdhe_secret,
                                         sizeof(ecdhe_secret))) {
        fprintf(stderr, "Hanshake secret generation failed\n");
        goto err;
    }

    if (memcmp(s->handshake_secret, handshake_secret,
               sizeof(handshake_secret)) != 0) {
        fprintf(stderr, "Handshake secret does not match\n");
        goto err;
    }

    hashsize = EVP_MD_size(ssl_handshake_md(s));
    if (sizeof(client_hts) != hashsize || sizeof(client_hts_key) != KEYLEN
            || sizeof(client_hts_iv) != IVLEN) {
        fprintf(stderr, "Internal test error\n");
        goto err;
    }

    if (!test_secret(s, s->handshake_secret, (unsigned char *)client_hts_label,
                     strlen(client_hts_label), client_hts, client_hts_key,
                     client_hts_iv)) {
        fprintf(stderr, "Client handshake secret test failed\n");
        goto err;
    }

    if (sizeof(server_hts) != hashsize || sizeof(server_hts_key) != KEYLEN
            || sizeof(server_hts_iv) != IVLEN) {
        fprintf(stderr, "Internal test error\n");
        goto err;
    }

    if (!test_secret(s, s->handshake_secret, (unsigned char *)server_hts_label,
                     strlen(server_hts_label), server_hts, server_hts_key,
                     server_hts_iv)) {
        fprintf(stderr, "Server handshake secret test failed\n");
        goto err;
    }

    /*
     * Ensure the mocked out ssl_handshake_hash() returns the full handshake
     * hash.
     */
    full_hash = 1;

    if (!tls13_generate_master_secret(s, out_master_secret,
                                      s->handshake_secret, hashsize,
                                      &master_secret_length)) {
        fprintf(stderr, "Master secret generation failed\n");
        goto err;
    }

    if (master_secret_length != sizeof(master_secret) ||
            memcmp(out_master_secret, master_secret,
                   sizeof(master_secret)) != 0) {
        fprintf(stderr, "Master secret does not match\n");
        goto err;
    }

    if (sizeof(client_ats) != hashsize || sizeof(client_ats_key) != KEYLEN
            || sizeof(client_ats_iv) != IVLEN) {
        fprintf(stderr, "Internal test error\n");
        goto err;
    }

    if (!test_secret(s, out_master_secret, (unsigned char *)client_ats_label,
                     strlen(client_ats_label), client_ats, client_ats_key,
                     client_ats_iv)) {
        fprintf(stderr, "Client application data secret test failed\n");
        goto err;
    }

    if (sizeof(server_ats) != hashsize || sizeof(server_ats_key) != KEYLEN
            || sizeof(server_ats_iv) != IVLEN) {
        fprintf(stderr, "Internal test error\n");
        goto err;
    }

    if (!test_secret(s, out_master_secret, (unsigned char *)server_ats_label,
                     strlen(server_ats_label), server_ats, server_ats_key,
                     server_ats_iv)) {
        fprintf(stderr, "Server application data secret test failed\n");
        goto err;
    }

    ret = 1;
 err:
    SSL_free(s);
    SSL_CTX_free(ctx);
    return ret;
}

void register_tests()
{
    ADD_TEST(test_handshake_secrets);
}
