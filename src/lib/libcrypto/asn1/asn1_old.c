/* $OpenBSD: asn1_old.c,v 1.1 2021/12/25 12:00:22 jsing Exp $ */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <limits.h>
#include <stdio.h>

#include <openssl/asn1.h>
#include <openssl/buffer.h>
#include <openssl/err.h>

#include "asn1_locl.h"

#ifndef NO_OLD_ASN1

void *
ASN1_dup(i2d_of_void *i2d, d2i_of_void *d2i, void *x)
{
	unsigned char *b, *p;
	const unsigned char *p2;
	int i;
	char *ret;

	if (x == NULL)
		return (NULL);

	i = i2d(x, NULL);
	b = malloc(i + 10);
	if (b == NULL) {
		ASN1error(ERR_R_MALLOC_FAILURE);
		return (NULL);
	}
	p = b;
	i = i2d(x, &p);
	p2 = b;
	ret = d2i(NULL, &p2, i);
	free(b);
	return (ret);
}

void *
ASN1_d2i_fp(void *(*xnew)(void), d2i_of_void *d2i, FILE *in, void **x)
{
	BIO *b;
	void *ret;

	if ((b = BIO_new(BIO_s_file())) == NULL) {
		ASN1error(ERR_R_BUF_LIB);
		return (NULL);
	}
	BIO_set_fp(b, in, BIO_NOCLOSE);
	ret = ASN1_d2i_bio(xnew, d2i, b, x);
	BIO_free(b);
	return (ret);
}

void *
ASN1_d2i_bio(void *(*xnew)(void), d2i_of_void *d2i, BIO *in, void **x)
{
	BUF_MEM *b = NULL;
	const unsigned char *p;
	void *ret = NULL;
	int len;

	len = asn1_d2i_read_bio(in, &b);
	if (len < 0)
		goto err;

	p = (unsigned char *)b->data;
	ret = d2i(x, &p, len);

err:
	if (b != NULL)
		BUF_MEM_free(b);
	return (ret);
}

int
ASN1_i2d_fp(i2d_of_void *i2d, FILE *out, void *x)
{
	BIO *b;
	int ret;

	if ((b = BIO_new(BIO_s_file())) == NULL) {
		ASN1error(ERR_R_BUF_LIB);
		return (0);
	}
	BIO_set_fp(b, out, BIO_NOCLOSE);
	ret = ASN1_i2d_bio(i2d, b, x);
	BIO_free(b);
	return (ret);
}

int
ASN1_i2d_bio(i2d_of_void *i2d, BIO *out, unsigned char *x)
{
	char *b;
	unsigned char *p;
	int i, j = 0, n, ret = 1;

	n = i2d(x, NULL);
	b = malloc(n);
	if (b == NULL) {
		ASN1error(ERR_R_MALLOC_FAILURE);
		return (0);
	}

	p = (unsigned char *)b;
	i2d(x, &p);

	for (;;) {
		i = BIO_write(out, &(b[j]), n);
		if (i == n)
			break;
		if (i <= 0) {
			ret = 0;
			break;
		}
		j += i;
		n -= i;
	}
	free(b);
	return (ret);
}

#endif