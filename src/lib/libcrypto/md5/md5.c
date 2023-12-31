/* $OpenBSD: md5.c,v 1.6 2023/07/28 11:06:28 jsing Exp $ */
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

#include <stdio.h>
#include <openssl/opensslv.h>
#include <openssl/crypto.h>

#include <stdlib.h>
#include <string.h>

#include <openssl/opensslconf.h>

#include <openssl/md5.h>

#ifdef MD5_ASM
# if defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(__INTEL__) || \
     defined(__x86_64) || defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64)
#  define md5_block_data_order md5_block_asm_data_order
# elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
#  define md5_block_data_order md5_block_asm_data_order
# endif
#endif

__BEGIN_HIDDEN_DECLS

void md5_block_data_order (MD5_CTX *c, const void *p, size_t num);

__END_HIDDEN_DECLS

#define DATA_ORDER_IS_LITTLE_ENDIAN

#define HASH_LONG		MD5_LONG
#define HASH_CTX		MD5_CTX
#define HASH_CBLOCK		MD5_CBLOCK
#define HASH_UPDATE		MD5_Update
#define HASH_TRANSFORM		MD5_Transform
#define HASH_FINAL		MD5_Final
#define	HASH_MAKE_STRING(c,s)	do {	\
	unsigned long ll;		\
	ll=(c)->A; HOST_l2c(ll,(s));	\
	ll=(c)->B; HOST_l2c(ll,(s));	\
	ll=(c)->C; HOST_l2c(ll,(s));	\
	ll=(c)->D; HOST_l2c(ll,(s));	\
	} while (0)
#define	HASH_BLOCK_DATA_ORDER	md5_block_data_order

#include "md32_common.h"
LCRYPTO_ALIAS(MD5_Update);
LCRYPTO_ALIAS(MD5_Transform);
LCRYPTO_ALIAS(MD5_Final);

/*
#define	F(x,y,z)	(((x) & (y))  |  ((~(x)) & (z)))
#define	G(x,y,z)	(((x) & (z))  |  ((y) & (~(z))))
*/

/* As pointed out by Wei Dai <weidai@eskimo.com>, the above can be
 * simplified to the code below.  Wei attributes these optimizations
 * to Peter Gutmann's SHS code, and he attributes it to Rich Schroeppel.
 */
#define	F(b,c,d)	((((c) ^ (d)) & (b)) ^ (d))
#define	G(b,c,d)	((((b) ^ (c)) & (d)) ^ (c))
#define	H(b,c,d)	((b) ^ (c) ^ (d))
#define	I(b,c,d)	(((~(d)) | (b)) ^ (c))

#define R0(a,b,c,d,k,s,t) { \
	a+=((k)+(t)+F((b),(c),(d))); \
	a=ROTATE(a,s); \
	a+=b; };\

#define R1(a,b,c,d,k,s,t) { \
	a+=((k)+(t)+G((b),(c),(d))); \
	a=ROTATE(a,s); \
	a+=b; };

#define R2(a,b,c,d,k,s,t) { \
	a+=((k)+(t)+H((b),(c),(d))); \
	a=ROTATE(a,s); \
	a+=b; };

#define R3(a,b,c,d,k,s,t) { \
	a+=((k)+(t)+I((b),(c),(d))); \
	a=ROTATE(a,s); \
	a+=b; };

/* Implemented from RFC1321 The MD5 Message-Digest Algorithm
 */

#define INIT_DATA_A (unsigned long)0x67452301L
#define INIT_DATA_B (unsigned long)0xefcdab89L
#define INIT_DATA_C (unsigned long)0x98badcfeL
#define INIT_DATA_D (unsigned long)0x10325476L

int
MD5_Init(MD5_CTX *c)
{
	memset (c, 0, sizeof(*c));
	c->A = INIT_DATA_A;
	c->B = INIT_DATA_B;
	c->C = INIT_DATA_C;
	c->D = INIT_DATA_D;
	return 1;
}
LCRYPTO_ALIAS(MD5_Init);

#ifndef md5_block_data_order
#ifdef X
#undef X
#endif
void
md5_block_data_order(MD5_CTX *c, const void *data_, size_t num)
{
	const unsigned char *data = data_;
	unsigned MD32_REG_T A, B, C, D, l;
	unsigned MD32_REG_T X0, X1, X2, X3, X4, X5, X6, X7,
	    X8, X9, X10, X11, X12, X13, X14, X15;

	A = c->A;
	B = c->B;
	C = c->C;
	D = c->D;

	for (; num--; ) {
		HOST_c2l(data, l);
		X0 = l;
		HOST_c2l(data, l);
		X1 = l;
		/* Round 0 */
		R0(A, B, C, D, X0, 7, 0xd76aa478L);
		HOST_c2l(data, l);
		X2 = l;
		R0(D, A, B, C, X1, 12, 0xe8c7b756L);
		HOST_c2l(data, l);
		X3 = l;
		R0(C, D, A, B, X2, 17, 0x242070dbL);
		HOST_c2l(data, l);
		X4 = l;
		R0(B, C, D, A, X3, 22, 0xc1bdceeeL);
		HOST_c2l(data, l);
		X5 = l;
		R0(A, B, C, D, X4, 7, 0xf57c0fafL);
		HOST_c2l(data, l);
		X6 = l;
		R0(D, A, B, C, X5, 12, 0x4787c62aL);
		HOST_c2l(data, l);
		X7 = l;
		R0(C, D, A, B, X6, 17, 0xa8304613L);
		HOST_c2l(data, l);
		X8 = l;
		R0(B, C, D, A, X7, 22, 0xfd469501L);
		HOST_c2l(data, l);
		X9 = l;
		R0(A, B, C, D, X8, 7, 0x698098d8L);
		HOST_c2l(data, l);
		X10 = l;
		R0(D, A, B, C, X9, 12, 0x8b44f7afL);
		HOST_c2l(data, l);
		X11 = l;
		R0(C, D, A, B, X10, 17, 0xffff5bb1L);
		HOST_c2l(data, l);
		X12 = l;
		R0(B, C, D, A, X11, 22, 0x895cd7beL);
		HOST_c2l(data, l);
		X13 = l;
		R0(A, B, C, D, X12, 7, 0x6b901122L);
		HOST_c2l(data, l);
		X14 = l;
		R0(D, A, B, C, X13, 12, 0xfd987193L);
		HOST_c2l(data, l);
		X15 = l;
		R0(C, D, A, B, X14, 17, 0xa679438eL);
		R0(B, C, D, A, X15, 22, 0x49b40821L);
		/* Round 1 */
		R1(A, B, C, D, X1, 5, 0xf61e2562L);
		R1(D, A, B, C, X6, 9, 0xc040b340L);
		R1(C, D, A, B, X11, 14, 0x265e5a51L);
		R1(B, C, D, A, X0, 20, 0xe9b6c7aaL);
		R1(A, B, C, D, X5, 5, 0xd62f105dL);
		R1(D, A, B, C, X10, 9, 0x02441453L);
		R1(C, D, A, B, X15, 14, 0xd8a1e681L);
		R1(B, C, D, A, X4, 20, 0xe7d3fbc8L);
		R1(A, B, C, D, X9, 5, 0x21e1cde6L);
		R1(D, A, B, C, X14, 9, 0xc33707d6L);
		R1(C, D, A, B, X3, 14, 0xf4d50d87L);
		R1(B, C, D, A, X8, 20, 0x455a14edL);
		R1(A, B, C, D, X13, 5, 0xa9e3e905L);
		R1(D, A, B, C, X2, 9, 0xfcefa3f8L);
		R1(C, D, A, B, X7, 14, 0x676f02d9L);
		R1(B, C, D, A, X12, 20, 0x8d2a4c8aL);
		/* Round 2 */
		R2(A, B, C, D, X5, 4, 0xfffa3942L);
		R2(D, A, B, C, X8, 11, 0x8771f681L);
		R2(C, D, A, B, X11, 16, 0x6d9d6122L);
		R2(B, C, D, A, X14, 23, 0xfde5380cL);
		R2(A, B, C, D, X1, 4, 0xa4beea44L);
		R2(D, A, B, C, X4, 11, 0x4bdecfa9L);
		R2(C, D, A, B, X7, 16, 0xf6bb4b60L);
		R2(B, C, D, A, X10, 23, 0xbebfbc70L);
		R2(A, B, C, D, X13, 4, 0x289b7ec6L);
		R2(D, A, B, C, X0, 11, 0xeaa127faL);
		R2(C, D, A, B, X3, 16, 0xd4ef3085L);
		R2(B, C, D, A, X6, 23, 0x04881d05L);
		R2(A, B, C, D, X9, 4, 0xd9d4d039L);
		R2(D, A, B, C, X12, 11, 0xe6db99e5L);
		R2(C, D, A, B, X15, 16, 0x1fa27cf8L);
		R2(B, C, D, A, X2, 23, 0xc4ac5665L);
		/* Round 3 */
		R3(A, B, C, D, X0, 6, 0xf4292244L);
		R3(D, A, B, C, X7, 10, 0x432aff97L);
		R3(C, D, A, B, X14, 15, 0xab9423a7L);
		R3(B, C, D, A, X5, 21, 0xfc93a039L);
		R3(A, B, C, D, X12, 6, 0x655b59c3L);
		R3(D, A, B, C, X3, 10, 0x8f0ccc92L);
		R3(C, D, A, B, X10, 15, 0xffeff47dL);
		R3(B, C, D, A, X1, 21, 0x85845dd1L);
		R3(A, B, C, D, X8, 6, 0x6fa87e4fL);
		R3(D, A, B, C, X15, 10, 0xfe2ce6e0L);
		R3(C, D, A, B, X6, 15, 0xa3014314L);
		R3(B, C, D, A, X13, 21, 0x4e0811a1L);
		R3(A, B, C, D, X4, 6, 0xf7537e82L);
		R3(D, A, B, C, X11, 10, 0xbd3af235L);
		R3(C, D, A, B, X2, 15, 0x2ad7d2bbL);
		R3(B, C, D, A, X9, 21, 0xeb86d391L);

		A = c->A += A;
		B = c->B += B;
		C = c->C += C;
		D = c->D += D;
	}
}
#endif

unsigned char *
MD5(const unsigned char *d, size_t n, unsigned char *md)
{
	MD5_CTX c;
	static unsigned char m[MD5_DIGEST_LENGTH];

	if (md == NULL)
		md = m;
	if (!MD5_Init(&c))
		return NULL;
	MD5_Update(&c, d, n);
	MD5_Final(md, &c);
	explicit_bzero(&c, sizeof(c));
	return (md);
}
LCRYPTO_ALIAS(MD5);
