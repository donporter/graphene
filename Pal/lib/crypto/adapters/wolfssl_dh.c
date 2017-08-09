/* Copyright (C) 2017 Fortanix, Inc.

   This file is part of Graphene Library OS.

   Graphene Library OS is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Graphene Library OS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <string.h>
#include "pal.h"
#include "pal_crypto.h"
#include "../cmac.h"
#include "../rsa.h"

static struct {
    uint8_t p[DH_SIZE], q[20], g[DH_SIZE];
} dh_param = {
    {
        0xfd, 0x7f, 0x53, 0x81, 0x1d, 0x75, 0x12, 0x29,
        0x52, 0xdf, 0x4a, 0x9c, 0x2e, 0xec, 0xe4, 0xe7,
        0xf6, 0x11, 0xb7, 0x52, 0x3c, 0xef, 0x44, 0x00,
        0xc3, 0x1e, 0x3f, 0x80, 0xb6, 0x51, 0x26, 0x69,
        0x45, 0x5d, 0x40, 0x22, 0x51, 0xfb, 0x59, 0x3d,
        0x8d, 0x58, 0xfa, 0xbf, 0xc5, 0xf5, 0xba, 0x30,
        0xf6, 0xcb, 0x9b, 0x55, 0x6c, 0xd7, 0x81, 0x3b,
        0x80, 0x1d, 0x34, 0x6f, 0xf2, 0x66, 0x60, 0xb7,
        0x6b, 0x99, 0x50, 0xa5, 0xa4, 0x9f, 0x9f, 0xe8,
        0x04, 0x7b, 0x10, 0x22, 0xc2, 0x4f, 0xbb, 0xa9,
        0xd7, 0xfe, 0xb7, 0xc6, 0x1b, 0xf8, 0x3b, 0x57,
        0xe7, 0xc6, 0xa8, 0xa6, 0x15, 0x0f, 0x04, 0xfb,
        0x83, 0xf6, 0xd3, 0xc5, 0x1e, 0xc3, 0x02, 0x35,
        0x54, 0x13, 0x5a, 0x16, 0x91, 0x32, 0xf6, 0x75,
        0xf3, 0xae, 0x2b, 0x61, 0xd7, 0x2a, 0xef, 0xf2,
        0x22, 0x03, 0x19, 0x9d, 0xd1, 0x48, 0x01, 0xc7,
    },

    {
        0x97, 0x60, 0x50, 0x8f, 0x15, 0x23, 0x0b, 0xcc,
        0xb2, 0x92, 0xb9, 0x82, 0xa2, 0xeb, 0x84, 0x0b,
        0xf0, 0x58, 0x1c, 0xf5,
    },

    {
        0xf7, 0xe1, 0xa0, 0x85, 0xd6, 0x9b, 0x3d, 0xde,
        0xcb, 0xbc, 0xab, 0x5c, 0x36, 0xb8, 0x57, 0xb9,
        0x79, 0x94, 0xaf, 0xbb, 0xfa, 0x3a, 0xea, 0x82,
        0xf9, 0x57, 0x4c, 0x0b, 0x3d, 0x07, 0x82, 0x67,
        0x51, 0x59, 0x57, 0x8e, 0xba, 0xd4, 0x59, 0x4f,
        0xe6, 0x71, 0x07, 0x10, 0x81, 0x80, 0xb4, 0x49,
        0x16, 0x71, 0x23, 0xe8, 0x4c, 0x28, 0x16, 0x13,
        0xb7, 0xcf, 0x09, 0x32, 0x8c, 0xc8, 0xa6, 0xe1,
        0x3c, 0x16, 0x7a, 0x8b, 0x54, 0x7c, 0x8d, 0x28,
        0xe0, 0xa3, 0xae, 0x1e, 0x2b, 0xb3, 0xa6, 0x75,
        0x91, 0x6e, 0xa3, 0x7f, 0x0b, 0xfa, 0x21, 0x35,
        0x62, 0xf1, 0xfb, 0x62, 0x7a, 0x01, 0x24, 0x3b,
        0xcc, 0xa4, 0xf1, 0xbe, 0xa8, 0x51, 0x90, 0x89,
        0xa8, 0x83, 0xdf, 0xe1, 0x5a, 0xe5, 0x9f, 0x06,
        0x92, 0x8b, 0x66, 0x5e, 0x80, 0x7b, 0x55, 0x25,
        0x64, 0x01, 0x4c, 0x3b, 0xfe, 0xcf, 0x49, 0x2a,
    },
};

int DkDhInit(PAL_DH_CONTEXT *context)
{
    memset(context, 0, sizeof *context);
    InitDhKey(&context->key);
    return DhSetKey(&context->key, dh_param.p, sizeof dh_param.p,
                    dh_param.g, sizeof dh_param.g);
}

int DkDhCreatePublic(PAL_DH_CONTEXT *context, uint8_t *public,
                     PAL_NUM *_public_size)
{
    uint32_t public_size;
    int ret;
    
    if (*_public_size != DH_SIZE)
        return -EINVAL;

    public_size = (uint32_t) *_public_size;
    ret = DhGenerateKeyPair(&context->key, context->priv, &context->priv_size,
                            public, &public_size);
    *_public_size = public_size;
    return ret;
}

int DkDhCalcSecret(PAL_DH_CONTEXT *context, uint8_t *peer, PAL_NUM peer_size,
                 uint8_t *secret, PAL_NUM *_secret_size)
{
    int ret;
    uint32_t secret_size;

    if (peer_size > DH_SIZE)
        return -EINVAL;
        
    if (*_secret_size != DH_SIZE)
        return -EINVAL;

    secret_size = (uint32_t) *_secret_size;
    
    ret = DhAgree(&context->key, secret, secret_size, context->priv,
                  context->priv_size, peer, (uint32_t) peer_size);
    *_secret_size = secret_size;
    return ret;
}

void DkDhFinal(PAL_DH_CONTEXT *context)
{
    /* Frees memory associated with the bignums. */
    FreeDhKey(&context->key);

    /* Clear the buffer to avoid any potential information leaks. */
    memset(context, 0, sizeof *context);
}

int DkAESCMAC(const uint8_t *key, PAL_NUM key_len, const uint8_t *input,
              PAL_NUM input_len, uint8_t *mac, PAL_NUM mac_len)
{
    /* The old code only supports 128-bit AES CMAC, and length is a 32-bit
     * value. */
    if (key_len != 16 || input_len > INT32_MAX || mac_len < 16) {
        return -EINVAL;
    }
    AES_CMAC((unsigned char *) key, (unsigned char *) input, length, mac);
    return 0;
}

int DkRSAInitKey(PAL_RSA_KEY *key)
{
    InitRSAKey(key);
    return 0;
}

int DkRSAGenerateKey(PAL_RSA_KEY *key, PAL_NUM length_in_bits, PAL_NUM exponent)
{
    if (length_in_bits > INT_MAX || (int) length_in_bits <= 0) {
        /* PAL_NUM is unsigned long, but MakeRSAKey takes an int. */
        return -EINVAL;
    }
    if ((int64_t) exponent <= 0) {
        /* Similarly, PAL_NUM is unsigned long, but exponent is signed long. */
        return -EINVAL;
    }
    return MakeRSAKey(key, (int) length_in_bits, exponent);
}

int DkRSAExportPublicKey(PAL_RSA_KEY *key, uint8_t *e, PAL_NUM *e_size,
                         uint8_t *n, PAL_NUM *n_size)
{
    int ret;
    word32 e_size_out;
    word32 n_size_out;

    /* PAL_NUM is a 64-bit number, but the wolfSSL parameters are 32-bit
     * values. */

    if (*e_size > UINT_MAX || *n_size > UINT_MAX) {
        return -EINVAL;
    }
    e_size_out = (word32) *e_size;
    n_size_out = (word32) *n_size;
    
    ret = RSAFlattenPublicKey(key, e, &e_size_out, n, &n_size_out);

    *e_size = e_size_out;
    *n_size = n_size_out;
    return ret;
}

int DkRSAImportPublicKey(PAL_RSA_KEY *key, const uint8_t *e, PAL_NUM e_size,
                         const uint8_t *n, PAL_NUM n_size)
{
    if (e_size > UINT_MAX || n_size > UINT_MAX) {
        return -EINVAL;
    }
    return RSAPublicKeyDecodeRaw(n, n_size, e, e_size, key);
}


int DkRSAVerifySHA256(PAL_RSA_KEY *key, const uint8_t *signature,
                      PAL_NUM signature_len, uint8_t *signed_data_out,
                      PAL_NUM signed_data_out_len) {
    
    if (signature_len > UINT_MAX) {
        return -EINVAL;
    }
    if (signed_data_out_len > UINT_MAX) {
        return -EINVAL;
    }

    int actual_signed_data_out_len =
        RSASSL_Verify(signature, (word32) signature_len, signed_data_out,
                      (word32) signed_data_out_len, key);
    if (actual_signed_data_out_len < 0) {
        return actual_signed_data_out_len;
    }
    if (actual_signed_data_out_len != SHA256_DIGEST_LEN) {
        return -EINVAL;
    }
    return 0;
}



int DkRSAFreeKey(PAL_RSA_KEY *key)
{
    return FreeRSAKey(key);
}