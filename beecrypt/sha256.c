/*
 * Copyright (c) 2000, 2001 Virtual Unlimited B.V.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*!\file sha256.c
 * \brief SHA-256 hash function, as specified by NIST DFIPS 180-2.
 * \author Bob Deblier <bob.deblier@pandora.be>
 * \ingroup HASH_m HASH_sha256_m
 */
 
#include "system.h"
#include "sha256.h"
#include "mp.h"
#include "endianness.h"
#include "debug.h"

/*!\addtogroup HASH_sha256_m
 * \{
 */

/**
 */
/*@observer@*/ /*@unchecked@*/
static const uint32_t k[64] = {
	0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
	0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
	0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
	0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
	0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
	0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
	0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
	0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

/**
 */
/*@observer@*/ /*@unchecked@*/
static const uint32_t hinit[8] = {
	0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU, 0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U
};

/*@-sizeoftype@*/
const hashFunction sha256 = { "SHA-256", sizeof(sha256Param), 64U, 8U * sizeof(uint32_t), (hashFunctionReset) sha256Reset, (hashFunctionUpdate) sha256Update, (hashFunctionDigest) sha256Digest };
/*@=sizeoftype@*/

int sha256Reset(sha256Param* sp)
{
	memcpy(sp->h, hinit, sizeof(sp->h));
	memset(sp->data, 0, sizeof(sp->data));
	memset(&sp->length, 0, sizeof(sp->length));
	sp->offset = 0;
	return 0;
}

#define R(x,s)  ((x) >> (s))
#define S(x,s) ROTR32(x, s)

#define CH(x,y,z) ((x&(y^z))^z)
#define MAJ(x,y,z) (((x|y)&z)|(x&y))
#define SIG0(x)	(S(x,2) ^ S(x,13) ^ S(x,22))
#define SIG1(x)	(S(x,6) ^ S(x,11) ^ S(x,25))
#define sig0(x) (S(x,7) ^ S(x,18) ^ R(x,3))
#define sig1(x) (S(x,17) ^ S(x,19) ^ R(x,10))

#define ROUND(a,b,c,d,e,f,g,h,w,k)	\
	temp = h + SIG1(e) + CH(e,f,g) + k + w;	\
	h = temp + SIG0(a) + MAJ(a,b,c);	\
	d += temp

#ifndef ASM_SHA256PROCESS
void sha256Process(sha256Param* sp)
{
	register uint32_t a, b, c, d, e, f, g, h, temp;
	register uint32_t *w;
	register byte t;
	
	#if WORDS_BIGENDIAN
	w = sp->data + 16;
	#else
	w = sp->data;
	t = 16;
	while (t--)
	{
		temp = swapu32(*w);
		*(w++) = temp;
	}
	#endif

	t = 48;
	while (t--)
	{
		temp = sig1(w[-2]) + w[-7] + sig0(w[-15]) + w[-16];
		*(w++) = temp;
	}

	w = sp->data;

	a = sp->h[0]; b = sp->h[1]; c = sp->h[2]; d = sp->h[3];
	e = sp->h[4]; f = sp->h[5]; g = sp->h[6]; h = sp->h[7];

	ROUND(a,b,c,d,e,f,g,h,w[ 0],k[ 0]);
	ROUND(h,a,b,c,d,e,f,g,w[ 1],k[ 1]);
	ROUND(g,h,a,b,c,d,e,f,w[ 2],k[ 2]);
	ROUND(f,g,h,a,b,c,d,e,w[ 3],k[ 3]);
	ROUND(e,f,g,h,a,b,c,d,w[ 4],k[ 4]);
	ROUND(d,e,f,g,h,a,b,c,w[ 5],k[ 5]);
	ROUND(c,d,e,f,g,h,a,b,w[ 6],k[ 6]);
	ROUND(b,c,d,e,f,g,h,a,w[ 7],k[ 7]);
	ROUND(a,b,c,d,e,f,g,h,w[ 8],k[ 8]);
	ROUND(h,a,b,c,d,e,f,g,w[ 9],k[ 9]);
	ROUND(g,h,a,b,c,d,e,f,w[10],k[10]);
	ROUND(f,g,h,a,b,c,d,e,w[11],k[11]);
	ROUND(e,f,g,h,a,b,c,d,w[12],k[12]);
	ROUND(d,e,f,g,h,a,b,c,w[13],k[13]);
	ROUND(c,d,e,f,g,h,a,b,w[14],k[14]);
	ROUND(b,c,d,e,f,g,h,a,w[15],k[15]);
	ROUND(a,b,c,d,e,f,g,h,w[16],k[16]);
	ROUND(h,a,b,c,d,e,f,g,w[17],k[17]);
	ROUND(g,h,a,b,c,d,e,f,w[18],k[18]);
	ROUND(f,g,h,a,b,c,d,e,w[19],k[19]);
	ROUND(e,f,g,h,a,b,c,d,w[20],k[20]);
	ROUND(d,e,f,g,h,a,b,c,w[21],k[21]);
	ROUND(c,d,e,f,g,h,a,b,w[22],k[22]);
	ROUND(b,c,d,e,f,g,h,a,w[23],k[23]);
	ROUND(a,b,c,d,e,f,g,h,w[24],k[24]);
	ROUND(h,a,b,c,d,e,f,g,w[25],k[25]);
	ROUND(g,h,a,b,c,d,e,f,w[26],k[26]);
	ROUND(f,g,h,a,b,c,d,e,w[27],k[27]);
	ROUND(e,f,g,h,a,b,c,d,w[28],k[28]);
	ROUND(d,e,f,g,h,a,b,c,w[29],k[29]);
	ROUND(c,d,e,f,g,h,a,b,w[30],k[30]);
	ROUND(b,c,d,e,f,g,h,a,w[31],k[31]);
	ROUND(a,b,c,d,e,f,g,h,w[32],k[32]);
	ROUND(h,a,b,c,d,e,f,g,w[33],k[33]);
	ROUND(g,h,a,b,c,d,e,f,w[34],k[34]);
	ROUND(f,g,h,a,b,c,d,e,w[35],k[35]);
	ROUND(e,f,g,h,a,b,c,d,w[36],k[36]);
	ROUND(d,e,f,g,h,a,b,c,w[37],k[37]);
	ROUND(c,d,e,f,g,h,a,b,w[38],k[38]);
	ROUND(b,c,d,e,f,g,h,a,w[39],k[39]);
	ROUND(a,b,c,d,e,f,g,h,w[40],k[40]);
	ROUND(h,a,b,c,d,e,f,g,w[41],k[41]);
	ROUND(g,h,a,b,c,d,e,f,w[42],k[42]);
	ROUND(f,g,h,a,b,c,d,e,w[43],k[43]);
	ROUND(e,f,g,h,a,b,c,d,w[44],k[44]);
	ROUND(d,e,f,g,h,a,b,c,w[45],k[45]);
	ROUND(c,d,e,f,g,h,a,b,w[46],k[46]);
	ROUND(b,c,d,e,f,g,h,a,w[47],k[47]);
	ROUND(a,b,c,d,e,f,g,h,w[48],k[48]);
	ROUND(h,a,b,c,d,e,f,g,w[49],k[49]);
	ROUND(g,h,a,b,c,d,e,f,w[50],k[50]);
	ROUND(f,g,h,a,b,c,d,e,w[51],k[51]);
	ROUND(e,f,g,h,a,b,c,d,w[52],k[52]);
	ROUND(d,e,f,g,h,a,b,c,w[53],k[53]);
	ROUND(c,d,e,f,g,h,a,b,w[54],k[54]);
	ROUND(b,c,d,e,f,g,h,a,w[55],k[55]);
	ROUND(a,b,c,d,e,f,g,h,w[56],k[56]);
	ROUND(h,a,b,c,d,e,f,g,w[57],k[57]);
	ROUND(g,h,a,b,c,d,e,f,w[58],k[58]);
	ROUND(f,g,h,a,b,c,d,e,w[59],k[59]);
	ROUND(e,f,g,h,a,b,c,d,w[60],k[60]);
	ROUND(d,e,f,g,h,a,b,c,w[61],k[61]);
	ROUND(c,d,e,f,g,h,a,b,w[62],k[62]);
	ROUND(b,c,d,e,f,g,h,a,w[63],k[63]);

	sp->h[0] += a;
	sp->h[1] += b;
	sp->h[2] += c;
	sp->h[3] += d;
	sp->h[4] += e;
	sp->h[5] += f;
	sp->h[6] += g;
	sp->h[7] += h;
}
#endif

int sha256Update(sha256Param* sp, const byte* data, size_t size)
{
	register unsigned short proclength;

	#if (MP_WBITS == 64)
	mpw add[1];
	mpsetw(1, add, size);
	mplshift(1, add, 3);
	(void) mpadd(1, sp->length, add);
	#elif (MP_WBITS == 32)
	mpw add[2];
	mpsetw(2, add, size);
	mplshift(2, add, 3);
	(void) mpadd(2, sp->length, add);
	#else
	# error
	#endif

/*@-type@*/
	while (size > 0)
	{
		proclength = ((sp->offset + size) > 64U) ? (64U - sp->offset) : size;
		memmove(((byte *) sp->data) + sp->offset, data, proclength);
		size -= proclength;
		data += proclength;
		sp->offset += proclength;

		if (sp->offset == 64U)
		{
			sha256Process(sp);
			sp->offset = 0;
		}
	}
/*@=type@*/
	return 0;
}

/**
 */
static void sha256Finish(sha256Param* sp)
	/*@globals internalState @*/
	/*@modifies sp, internalState @*/
{
	register byte *ptr = ((byte *) sp->data) + sp->offset++;

	*(ptr++) = 0x80;

/*@-type@*/
	if (sp->offset > 56)
	{
		while (sp->offset++ < 64)
			*(ptr++) = 0;

		sha256Process(sp);
		sp->offset = 0;
	}

	ptr = ((byte *) sp->data) + sp->offset;
	while (sp->offset++ < 56)
		*(ptr++) = 0;
/*@=type@*/

	#if (MP_WBITS == 64)
	ptr[0] = (byte)(sp->length[0] >> 56);
	ptr[1] = (byte)(sp->length[0] >> 48);
	ptr[2] = (byte)(sp->length[0] >> 40);
	ptr[3] = (byte)(sp->length[0] >> 32);
	ptr[4] = (byte)(sp->length[0] >> 24);
	ptr[5] = (byte)(sp->length[0] >> 16);
	ptr[6] = (byte)(sp->length[0] >>  8);
	ptr[7] = (byte)(sp->length[0]      );
	#elif (MP_WBITS == 32)
	ptr[0] = (byte)(sp->length[0] >> 24);
	ptr[1] = (byte)(sp->length[0] >> 16);
	ptr[2] = (byte)(sp->length[0] >>  8);
	ptr[3] = (byte)(sp->length[0]      );
	ptr[4] = (byte)(sp->length[1] >> 24);
	ptr[5] = (byte)(sp->length[1] >> 16);
	ptr[6] = (byte)(sp->length[1] >>  8);
	ptr[7] = (byte)(sp->length[1]      );
	#else
	# error
	#endif

	sha256Process(sp);
	sp->offset = 0;
}

int sha256Digest(sha256Param* sp, byte* digest)
{
	sha256Finish(sp);

	/* encode 8 integers big-endian style */
	digest[ 0] = (byte)(sp->h[0] >> 24);
	digest[ 1] = (byte)(sp->h[0] >> 16);
	digest[ 2] = (byte)(sp->h[0] >>  8);
	digest[ 3] = (byte)(sp->h[0] >>  0);
	digest[ 4] = (byte)(sp->h[1] >> 24);
	digest[ 5] = (byte)(sp->h[1] >> 16);
	digest[ 6] = (byte)(sp->h[1] >>  8);
	digest[ 7] = (byte)(sp->h[1] >>  0);
	digest[ 8] = (byte)(sp->h[2] >> 24);
	digest[ 9] = (byte)(sp->h[2] >> 16);
	digest[10] = (byte)(sp->h[2] >>  8);
	digest[11] = (byte)(sp->h[2] >>  0);
	digest[12] = (byte)(sp->h[3] >> 24);
	digest[13] = (byte)(sp->h[3] >> 16);
	digest[14] = (byte)(sp->h[3] >>  8);
	digest[15] = (byte)(sp->h[3] >>  0);
	digest[16] = (byte)(sp->h[4] >> 24);
	digest[17] = (byte)(sp->h[4] >> 16);
	digest[18] = (byte)(sp->h[4] >>  8);
	digest[19] = (byte)(sp->h[4] >>  0);
	digest[20] = (byte)(sp->h[5] >> 24);
	digest[21] = (byte)(sp->h[5] >> 16);
	digest[22] = (byte)(sp->h[5] >>  8);
	digest[23] = (byte)(sp->h[5] >>  0);
	digest[24] = (byte)(sp->h[6] >> 24);
	digest[25] = (byte)(sp->h[6] >> 16);
	digest[26] = (byte)(sp->h[6] >>  8);
	digest[27] = (byte)(sp->h[6] >>  0);
	digest[28] = (byte)(sp->h[7] >> 24);
	digest[29] = (byte)(sp->h[7] >> 16);
	digest[30] = (byte)(sp->h[7] >>  8);
	digest[31] = (byte)(sp->h[7] >>  0);

	(void) sha256Reset(sp);
	return 0;
}

/*!\}
 */
