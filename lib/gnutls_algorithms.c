/*
 *      Copyright (C) 2000 Nikos Mavroyanopoulos
 *
 * This file is part of GNUTLS.
 *
 * GNUTLS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GNUTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <defines.h>
#include "gnutls_int.h"
#include "gnutls_algorithms.h"
#include "gnutls_errors.h"

/* include all the kx handler's definitions */
#include "auth_anon.h"
/* #include "auth_dhe_dss.h" */
#include "auth_srp.h"

#define MAX_CIPHER 256
#define MAX_MAC 256
#define MAX_KX 256
#define MAX_CIPHERSUITE 256
#define MAX_COMPRESSION 256

/* TLS Versions */

typedef struct {
	char *name;
	GNUTLS_Version id;	/* gnutls internal version number */
	int major;		/* defined by the protocol */
	int minor;		/* defined by the protocol */
	int supported;		/* 0 not supported, > 0 is supported */
} gnutls_version_entry;

static const gnutls_version_entry sup_versions[] = {
	{"SSL3", GNUTLS_SSL3, 3, 0, 1},
	{"TLS1", GNUTLS_TLS1, 3, 1, 1},
	{0}
};

#define GNUTLS_VERSION_LOOP(b) \
        const gnutls_version_entry *p; \
                for(p = sup_versions; p->name != NULL; p++) { b ; }

#define GNUTLS_VERSION_ALG_LOOP(a) \
                        GNUTLS_VERSION_LOOP( if(p->id == version) { a; break; })


#define GNUTLS_CIPHER_ENTRY(name, blksize, keysize, block, iv) \
	{ #name, name, blksize, keysize, block, iv }

struct gnutls_cipher_entry {
	char *name;
	BulkCipherAlgorithm id;
	size_t blocksize;
	size_t keysize;
	size_t block;
	size_t iv;
};
typedef struct gnutls_cipher_entry gnutls_cipher_entry;

/* Note that all algorithms are in CBC or STREAM modes. 
 * Do not add any algorithms in other modes (like ECB).
 * View first: "The order of encryption and authentication for
 * protecting communications" by Hugo Krawczyk - CRYPTO 2001
 */
static const gnutls_cipher_entry algorithms[] = {
	GNUTLS_CIPHER_ENTRY(GNUTLS_3DES, 8, 24, 1, 8),
	GNUTLS_CIPHER_ENTRY(GNUTLS_RIJNDAEL, 16, 16, 1, 16),
	GNUTLS_CIPHER_ENTRY(GNUTLS_RIJNDAEL256, 16, 32, 1, 16),
	GNUTLS_CIPHER_ENTRY(GNUTLS_TWOFISH, 16, 16, 1, 16),
	GNUTLS_CIPHER_ENTRY(GNUTLS_ARCFOUR, 1, 16, 0, 0),
	GNUTLS_CIPHER_ENTRY(GNUTLS_NULL_CIPHER, 1, 0, 0, 0),
	{0}
};

#define GNUTLS_LOOP(b) \
        const gnutls_cipher_entry *p; \
                for(p = algorithms; p->name != NULL; p++) { b ; }

#define GNUTLS_ALG_LOOP(a) \
                        GNUTLS_LOOP( if(p->id == algorithm) { a; break; } )


#define GNUTLS_HASH_ENTRY(name, hashsize) \
	{ #name, name, hashsize }

struct gnutls_hash_entry {
	char *name;
	MACAlgorithm id;
	size_t digestsize;
};
typedef struct gnutls_hash_entry gnutls_hash_entry;

static const gnutls_hash_entry hash_algorithms[] = {
	GNUTLS_HASH_ENTRY(GNUTLS_MAC_SHA, 20),
	GNUTLS_HASH_ENTRY(GNUTLS_MAC_MD5, 16),
	GNUTLS_HASH_ENTRY(GNUTLS_NULL_MAC, 0),
	{0}
};

#define GNUTLS_HASH_LOOP(b) \
        const gnutls_hash_entry *p; \
                for(p = hash_algorithms; p->name != NULL; p++) { b ; }

#define GNUTLS_HASH_ALG_LOOP(a) \
                        GNUTLS_HASH_LOOP( if(p->id == algorithm) { a; break; } )


/* Compression Section */
#define GNUTLS_COMPRESSION_ENTRY(name, id) \
	{ #name, name, id }

struct gnutls_compression_entry {
	char *name;
	CompressionMethod id;
	int num; /* the number reserved in TLS for the specific compression method */
};

typedef struct gnutls_compression_entry gnutls_compression_entry;
static const gnutls_compression_entry compression_algorithms[] = {
	GNUTLS_COMPRESSION_ENTRY(GNUTLS_NULL_COMPRESSION, 0),
#ifdef HAVE_LIBZ
	GNUTLS_COMPRESSION_ENTRY(GNUTLS_ZLIB, 224),
#endif
	{0}
};

#define GNUTLS_COMPRESSION_LOOP(b) \
        const gnutls_compression_entry *p; \
                for(p = compression_algorithms; p->name != NULL; p++) { b ; }
#define GNUTLS_COMPRESSION_ALG_LOOP(a) \
                        GNUTLS_COMPRESSION_LOOP( if(p->id == algorithm) { a; break; } )
#define GNUTLS_COMPRESSION_ALG_LOOP_NUM(a) \
                        GNUTLS_COMPRESSION_LOOP( if(p->num == num) { a; break; } )


/* Key Exchange Section */
#define GNUTLS_KX_ALGO_ENTRY(name, server_cert, client_cert, RSA_premaster, DH_public_value, auth_struct) \
	{ #name, name, server_cert, client_cert, RSA_premaster, DH_public_value, auth_struct }

struct gnutls_kx_algo_entry {
	char *name;
	KXAlgorithm algorithm;
	int server_cert;
	int client_cert;
	int RSA_premaster;
	int DH_public_value;
	MOD_AUTH_STRUCT *auth_struct;
};
typedef struct gnutls_kx_algo_entry gnutls_kx_algo_entry;

static const gnutls_kx_algo_entry kx_algorithms[] = {
	GNUTLS_KX_ALGO_ENTRY(GNUTLS_KX_DH_ANON, 0, 0, 0, 1,
			     &anon_auth_struct),
	GNUTLS_KX_ALGO_ENTRY(GNUTLS_KX_RSA, 1, 1, 1, 0, NULL),
/*	GNUTLS_KX_ALGO_ENTRY(GNUTLS_KX_DHE_DSS, 1, 1, 0, 0,
			     &dhe_dss_auth_struct),*/
	GNUTLS_KX_ALGO_ENTRY(GNUTLS_KX_DHE_RSA, 1, 1, 0, 0, NULL),
	GNUTLS_KX_ALGO_ENTRY(GNUTLS_KX_DH_DSS, 1, 1, 0, 0, NULL),
	GNUTLS_KX_ALGO_ENTRY(GNUTLS_KX_DH_RSA, 1, 1, 0, 0, NULL),
	GNUTLS_KX_ALGO_ENTRY(GNUTLS_KX_SRP, 0, 0, 0, 0, &srp_auth_struct),
	{0}
};

#define GNUTLS_KX_LOOP(b) \
        const gnutls_kx_algo_entry *p; \
                for(p = kx_algorithms; p->name != NULL; p++) { b ; }

#define GNUTLS_KX_ALG_LOOP(a) \
                        GNUTLS_KX_LOOP( if(p->algorithm == algorithm) { a; break; } )



/* Cipher SUITES */
#define GNUTLS_CIPHER_SUITE_ENTRY( name, block_algorithm, kx_algorithm, mac_algorithm ) \
	{ #name, {name}, block_algorithm, kx_algorithm, mac_algorithm }

typedef struct {
	char *name;
	GNUTLS_CipherSuite id;
	BulkCipherAlgorithm block_algorithm;
	KXAlgorithm kx_algorithm;
	MACAlgorithm mac_algorithm;
} gnutls_cipher_suite_entry;

#define GNUTLS_DH_anon_3DES_EDE_CBC_SHA { 0x00, 0x1B }
#define GNUTLS_DH_anon_ARCFOUR_MD5 { 0x00, 0x18 }
#define GNUTLS_DH_anon_RIJNDAEL_128_CBC_SHA { 0x00, 0x34 }
#define GNUTLS_DH_anon_RIJNDAEL_256_CBC_SHA { 0x00, 0x3A }
/* Twofish is a gnutls extension */
#define GNUTLS_DH_anon_TWOFISH_128_CBC_SHA { 0xF6, 0x50 }

/* SRP is a gnutls extension - for now */
#define GNUTLS_SRP_3DES_EDE_CBC_SHA { 0xF6, 0x60 }
#define GNUTLS_SRP_ARCFOUR_MD5 { 0xF6, 0x61 }
#define GNUTLS_SRP_RIJNDAEL_128_CBC_SHA { 0xF6, 0x62 }
#define GNUTLS_SRP_RIJNDAEL_256_CBC_SHA { 0xF6, 0x63 }
#define GNUTLS_SRP_TWOFISH_128_CBC_SHA { 0xF6, 0x64 }

/* RSA */
#define GNUTLS_RSA_ARCFOUR_SHA { 0x00, 0x05 }
#define GNUTLS_RSA_ARCFOUR_MD5 { 0x00, 0x04 }
#define GNUTLS_RSA_3DES_EDE_CBC_SHA { 0x00, 0x0A }
#define GNUTLS_RSA_DES_CBC_SHA { 0x00, 0x09 }
#define GNUTLS_RSA_RIJNDAEL_128_CBC_SHA { 0x00, 0x2F }
#define GNUTLS_RSA_RIJNDAEL_256_CBC_SHA { 0x00, 0x35 }
#define GNUTLS_RSA_TWOFISH_128_CBC_SHA { 0xF6, 0x51 }

/* DH_DSS */
#define GNUTLS_DH_DSS_RIJNDAEL_128_CBC_SHA { 0x00, 0x30 }
#define GNUTLS_DH_DSS_TWOFISH_128_CBC_SHA { 0xF6, 0x52 }
#define GNUTLS_DH_DSS_DES_CBC_SHA { 0x00, 0x0C }
#define GNUTLS_DH_DSS_RIJNDAEL_256_CBC_SHA { 0x00, 0x36 }
#define GNUTLS_DH_DSS_3DES_EDE_CBC_SHA { 0x00, 0x0D }

/* DHE_DSS */
#define GNUTLS_DHE_DSS_RIJNDAEL_256_CBC_SHA { 0x00, 0x38 }
#define GNUTLS_DHE_DSS_RIJNDAEL_128_CBC_SHA { 0x00, 0x32 }
#define GNUTLS_DHE_DSS_DES_CBC_SHA { 0x00, 0x12 }
#define GNUTLS_DHE_DSS_TWOFISH_128_CBC_SHA { 0xF6, 0x54 }
#define GNUTLS_DHE_DSS_3DES_EDE_CBC_SHA { 0x00, 0x13 }

/* DHE_RSA */
#define GNUTLS_DHE_RSA_TWOFISH_128_CBC_SHA { 0xF6, 0x55 }
#define GNUTLS_DHE_RSA_3DES_EDE_CBC_SHA { 0x00, 0x16 }
#define GNUTLS_DHE_RSA_DES_CBC_SHA { 0x00, 0x15 }
#define GNUTLS_DHE_RSA_RIJNDAEL_128_CBC_SHA { 0x00, 0x33 }
#define GNUTLS_DHE_RSA_RIJNDAEL_256_CBC_SHA { 0x00, 0x39 }

/* DH_RSA */
#define GNUTLS_DH_RSA_TWOFISH_128_CBC_SHA { 0xF6, 0x53 }
#define GNUTLS_DH_RSA_DES_CBC_SHA { 0x00, 0x0F }
#define GNUTLS_DH_RSA_3DES_EDE_CBC_SHA { 0x00, 0x10 }
#define GNUTLS_DH_RSA_RIJNDAEL_256_CBC_SHA { 0x00, 0x37 }
#define GNUTLS_DH_RSA_RIJNDAEL_128_CBC_SHA { 0x00, 0x31 }


static const gnutls_cipher_suite_entry cs_algorithms[] = {
	/* DH_anon */
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_anon_ARCFOUR_MD5,
				  GNUTLS_ARCFOUR,
				  GNUTLS_KX_DH_ANON, GNUTLS_MAC_MD5),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_anon_3DES_EDE_CBC_SHA,
				  GNUTLS_3DES, GNUTLS_KX_DH_ANON,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_anon_RIJNDAEL_128_CBC_SHA,
				  GNUTLS_RIJNDAEL, GNUTLS_KX_DH_ANON,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_anon_RIJNDAEL_256_CBC_SHA,
				  GNUTLS_RIJNDAEL256, GNUTLS_KX_DH_ANON,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_anon_TWOFISH_128_CBC_SHA,
				  GNUTLS_TWOFISH, GNUTLS_KX_DH_ANON,
				  GNUTLS_MAC_SHA),

	/* SRP */
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_SRP_ARCFOUR_MD5,
				  GNUTLS_ARCFOUR,
				  GNUTLS_KX_SRP, GNUTLS_MAC_MD5),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_SRP_3DES_EDE_CBC_SHA,
				  GNUTLS_3DES, GNUTLS_KX_SRP,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_SRP_RIJNDAEL_128_CBC_SHA,
				  GNUTLS_RIJNDAEL, GNUTLS_KX_SRP,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_SRP_RIJNDAEL_256_CBC_SHA,
				  GNUTLS_RIJNDAEL256, GNUTLS_KX_SRP,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_SRP_TWOFISH_128_CBC_SHA,
				  GNUTLS_TWOFISH, GNUTLS_KX_SRP,
				  GNUTLS_MAC_SHA),

	/* DH_DSS */
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_DSS_3DES_EDE_CBC_SHA,
				  GNUTLS_3DES,
				  GNUTLS_KX_DH_DSS, GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_DSS_RIJNDAEL_128_CBC_SHA,
				  GNUTLS_RIJNDAEL, GNUTLS_KX_DH_DSS,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_DSS_RIJNDAEL_256_CBC_SHA,
				  GNUTLS_RIJNDAEL256, GNUTLS_KX_DH_DSS,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_DSS_TWOFISH_128_CBC_SHA,
				  GNUTLS_TWOFISH, GNUTLS_KX_DH_DSS,
				  GNUTLS_MAC_SHA),

	/* DH_RSA */ 
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_RSA_3DES_EDE_CBC_SHA,
				  GNUTLS_3DES,
				  GNUTLS_KX_DH_RSA, GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_RSA_RIJNDAEL_128_CBC_SHA,
				  GNUTLS_RIJNDAEL, GNUTLS_KX_DH_RSA,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_RSA_RIJNDAEL_256_CBC_SHA,
				  GNUTLS_RIJNDAEL256, GNUTLS_KX_DH_RSA,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DH_RSA_TWOFISH_128_CBC_SHA,
				  GNUTLS_TWOFISH, GNUTLS_KX_DH_RSA,
				  GNUTLS_MAC_SHA),

	/* DHE_DSS */
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DHE_DSS_TWOFISH_128_CBC_SHA,
				  GNUTLS_TWOFISH, GNUTLS_KX_DHE_DSS,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DHE_DSS_3DES_EDE_CBC_SHA,
				  GNUTLS_3DES, GNUTLS_KX_DHE_DSS,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DHE_DSS_RIJNDAEL_128_CBC_SHA,
				  GNUTLS_RIJNDAEL, GNUTLS_KX_DHE_DSS,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DHE_DSS_RIJNDAEL_256_CBC_SHA,
				  GNUTLS_RIJNDAEL256, GNUTLS_KX_DHE_DSS,
				  GNUTLS_MAC_SHA),

	/* DHE_RSA */
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DHE_RSA_TWOFISH_128_CBC_SHA,
				  GNUTLS_TWOFISH, GNUTLS_KX_DHE_RSA,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DHE_RSA_3DES_EDE_CBC_SHA,
				  GNUTLS_3DES, GNUTLS_KX_DHE_RSA,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DHE_RSA_RIJNDAEL_128_CBC_SHA,
				  GNUTLS_RIJNDAEL, GNUTLS_KX_DHE_RSA,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_DHE_RSA_RIJNDAEL_256_CBC_SHA,
				  GNUTLS_RIJNDAEL256, GNUTLS_KX_DHE_RSA,
				  GNUTLS_MAC_SHA),

	/* RSA */
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_RSA_ARCFOUR_SHA,
				  GNUTLS_ARCFOUR,
				  GNUTLS_KX_RSA, GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_RSA_ARCFOUR_MD5,
				  GNUTLS_ARCFOUR,
				  GNUTLS_KX_RSA, GNUTLS_MAC_MD5),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_RSA_3DES_EDE_CBC_SHA,
				  GNUTLS_3DES,
				  GNUTLS_KX_RSA, GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_RSA_RIJNDAEL_128_CBC_SHA,
				  GNUTLS_RIJNDAEL, GNUTLS_KX_RSA,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_RSA_RIJNDAEL_256_CBC_SHA,
				  GNUTLS_RIJNDAEL256, GNUTLS_KX_RSA,
				  GNUTLS_MAC_SHA),
	GNUTLS_CIPHER_SUITE_ENTRY(GNUTLS_RSA_TWOFISH_128_CBC_SHA,
				  GNUTLS_TWOFISH, GNUTLS_KX_RSA,
				  GNUTLS_MAC_SHA),

	{0}
};

#define GNUTLS_CIPHER_SUITE_LOOP(b) \
        const gnutls_cipher_suite_entry *p; \
                for(p = cs_algorithms; p->name != NULL; p++) { b ; }

#define GNUTLS_CIPHER_SUITE_ALG_LOOP(a) \
                        GNUTLS_CIPHER_SUITE_LOOP( if( (p->id.CipherSuite[0] == suite.CipherSuite[0]) && (p->id.CipherSuite[1] == suite.CipherSuite[1])) { a; break; } )



/* Generic Functions */

/* this function makes the whole string lowercase */
void _gnutls_tolow(char *str, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		str[i] = tolower(str[i]);
	}
}

/* HASHES */
int _gnutls_mac_get_digest_size(MACAlgorithm algorithm)
{
	size_t ret = 0;
	GNUTLS_HASH_ALG_LOOP(ret = p->digestsize);
	return ret;

}

inline int _gnutls_mac_priority(GNUTLS_STATE state, MACAlgorithm algorithm)
{				/* actually returns the priority */
	int i;
	for (i = 0;
	     i < state->gnutls_internals.MACAlgorithmPriority.algorithms;
	     i++) {
		if (state->gnutls_internals.
		    MACAlgorithmPriority.algorithm_priority[i] ==
		    algorithm)
			return i;
	}
	return -1;
}

/**
  * gnutls_mac_get_name - Returns a string with the name of the specified mac algorithm
  * @algorithm: is a MAC algorithm
  *
  * Returns an allocated (with malloc) string that contains the name 
  * of the specified MAC algorithm.
  **/
char *gnutls_mac_get_name(MACAlgorithm algorithm)
{
	char *ret = NULL;
	char *pointerTo_;

	/* avoid prefix */
	GNUTLS_HASH_ALG_LOOP(ret =
			     strdup(p->name + sizeof("GNUTLS_") - 1));


	if (ret != NULL) {
		_gnutls_tolow(ret, strlen(ret));
		pointerTo_ = strchr(ret, '_');

		while (pointerTo_ != NULL) {
			*pointerTo_ = '-';
			pointerTo_ = strchr(ret, '_');
		}
	}
	return ret;
}

int _gnutls_mac_count()
{
	uint8 i, counter = 0;
	for (i = 0; i < MAX_MAC; i++) {
		if (_gnutls_mac_is_ok(i) == 0)
			counter++;
	}
	return counter;
}

int _gnutls_mac_is_ok(MACAlgorithm algorithm)
{
	size_t ret = -1;
	GNUTLS_HASH_ALG_LOOP(ret = p->id);
	if (ret >= 0)
		ret = 0;
	else
		ret = 1;
	return ret;
}

/* Compression Functions */
inline
    int _gnutls_compression_priority(GNUTLS_STATE state,
				     CompressionMethod algorithm)
{				/* actually returns the priority */
	int i;
	for (i = 0;
	     i <
	     state->gnutls_internals.CompressionMethodPriority.algorithms;
	     i++) {
		if (state->gnutls_internals.
		    CompressionMethodPriority.algorithm_priority[i] ==
		    algorithm)
			return i;
	}
	return -1;
}

/**
  * gnutls_compression_get_name - Returns a string with the name of the specified compression algorithm
  * @algorithm: is a Compression algorithm
  *
  * Returns a localy allocated (with malloc) pointer to a string that contains the name 
  * of the specified compression algorithm.
  **/
char *gnutls_compression_get_name(CompressionMethod algorithm)
{
	char *ret = NULL;
	char *pointerTo_;

	/* avoid prefix */
	GNUTLS_COMPRESSION_ALG_LOOP(ret =
				    strdup(p->name + sizeof("GNUTLS_") -
					   1));


	if (ret != NULL) {
		_gnutls_tolow(ret, strlen(ret));
		pointerTo_ = strchr(ret, '_');

		while (pointerTo_ != NULL) {
			*pointerTo_ = '-';
			pointerTo_ = strchr(ret, '_');
		}
	}
	return ret;
}

/* return the tls number of the specified algorithm */
int _gnutls_compression_get_num(CompressionMethod algorithm)
{
	int ret = -1;

	/* avoid prefix */
	GNUTLS_COMPRESSION_ALG_LOOP(ret = p->num);

	return ret;
}

/* returns the gnutls internal ID of the TLS compression
 * method num
 */
CompressionMethod _gnutls_compression_get_id(int num)
{
	CompressionMethod ret = -1;

	/* avoid prefix */
	GNUTLS_COMPRESSION_ALG_LOOP_NUM(ret = p->id);

	return ret;
}

int _gnutls_compression_count()
{
	uint8 i, counter = 0;
	for (i = 0; i < MAX_COMPRESSION; i++) {
		if (_gnutls_compression_is_ok(i) == 0)
			counter++;
	}
	return counter;
}

int _gnutls_compression_is_ok(CompressionMethod algorithm)
{
	size_t ret = -1;
	GNUTLS_COMPRESSION_ALG_LOOP(ret = p->id);
	if (ret >= 0)
		ret = 0;
	else
		ret = 1;
	return ret;
}



/* CIPHER functions */
int _gnutls_cipher_get_block_size(BulkCipherAlgorithm algorithm)
{
	size_t ret = 0;
	GNUTLS_ALG_LOOP(ret = p->blocksize);
	return ret;

}

 /* returns the priority */
inline
    int
_gnutls_cipher_priority(GNUTLS_STATE state, BulkCipherAlgorithm algorithm)
{
	int i;
	for (i = 0;
	     i <
	     state->gnutls_internals.
	     BulkCipherAlgorithmPriority.algorithms; i++) {
		if (state->gnutls_internals.
		    BulkCipherAlgorithmPriority.algorithm_priority[i] ==
		    algorithm)
			return i;
	}
	return -1;
}


int _gnutls_cipher_is_block(BulkCipherAlgorithm algorithm)
{
	size_t ret = 0;

	GNUTLS_ALG_LOOP(ret = p->block);
	return ret;

}

int _gnutls_cipher_get_key_size(BulkCipherAlgorithm algorithm)
{				/* In bytes */
	size_t ret = 0;
	GNUTLS_ALG_LOOP(ret = p->keysize);
	return ret;

}

int _gnutls_cipher_get_iv_size(BulkCipherAlgorithm algorithm)
{				/* In bytes */
	size_t ret = 0;
	GNUTLS_ALG_LOOP(ret = p->iv);
	return ret;

}

/**
  * gnutls_cipher_get_name - Returns a string with the name of the specified cipher algorithm
  * @algorithm: is an encryption algorithm
  *
  * Returns a localy allocated (with malloc) pointer to a string that contains the name 
  * of the specified cipher.
  **/
char *gnutls_cipher_get_name(BulkCipherAlgorithm algorithm)
{
	char *ret = NULL;
	char *pointerTo_;

	/* avoid prefix */
	GNUTLS_ALG_LOOP(ret = strdup(p->name + sizeof("GNUTLS_") - 1));


	if (ret != NULL) {
		_gnutls_tolow(ret, strlen(ret));
		pointerTo_ = strchr(ret, '_');

		while (pointerTo_ != NULL) {
			*pointerTo_ = '-';
			pointerTo_ = strchr(ret, '_');
		}
	}
	return ret;
}

int _gnutls_cipher_count()
{
	uint8 i, counter = 0;
	for (i = 0; i < MAX_CIPHER; i++) {
		if (_gnutls_cipher_is_ok(i) == 0)
			counter++;
	}
	return counter;
}


int _gnutls_cipher_is_ok(BulkCipherAlgorithm algorithm)
{
	size_t ret = -1;
	GNUTLS_ALG_LOOP(ret = p->id);
	if (ret >= 0)
		ret = 0;
	else
		ret = 1;
	return ret;
}


/* Key EXCHANGE functions */
int _gnutls_kx_server_certificate(KXAlgorithm algorithm)
{
	size_t ret = 0;
	GNUTLS_KX_ALG_LOOP(ret = p->server_cert);
	return ret;

}

MOD_AUTH_STRUCT *_gnutls_kx_auth_struct(KXAlgorithm algorithm)
{
	MOD_AUTH_STRUCT *ret = NULL;
	GNUTLS_KX_ALG_LOOP(ret = p->auth_struct);
	return ret;

}

inline int _gnutls_kx_priority(GNUTLS_STATE state, KXAlgorithm algorithm)
{
	int i;
	for (i = 0;
	     i < state->gnutls_internals.KXAlgorithmPriority.algorithms;
	     i++) {
		if (state->gnutls_internals.
		    KXAlgorithmPriority.algorithm_priority[i] == algorithm)
			return i;
	}
	return -1;
}

int _gnutls_kx_server_key_exchange(KXAlgorithm algorithm)
{
	size_t ret = 0;
	void *ret2 = NULL;

	/* if the auth algorithm does not have a null value 
	 * for the kx2 generation then it supports it!
	 */
	GNUTLS_KX_ALG_LOOP(ret2 =
			   p->auth_struct->gnutls_generate_server_kx);
	if (ret2 != NULL)
		ret = 1;

	return ret;

}

int _gnutls_kx_server_key_exchange2(KXAlgorithm algorithm)
{
	size_t ret = 0;
	void *ret2 = NULL;

	/* if the auth algorithm does not have a null value 
	 * for the kx2 generation then it supports it!
	 */
	GNUTLS_KX_ALG_LOOP(ret2 =
			   p->auth_struct->gnutls_generate_server_kx2);
	if (ret2 != NULL)
		ret = 1;

	return ret;

}

int _gnutls_kx_client_key_exchange0(KXAlgorithm algorithm)
{
	size_t ret = 0;
	void *ret2 = NULL;

	/* if the auth algorithm does not have a null value 
	 * for the kx0 generation then it supports it!
	 */
	GNUTLS_KX_ALG_LOOP(ret2 =
			   p->auth_struct->gnutls_process_client_kx0);
	if (ret2 != NULL)
		ret = 1;

	return ret;

}

int _gnutls_kx_client_key_exchange(KXAlgorithm algorithm)
{
	size_t ret = 0;
	void *ret2 = NULL;

	/* if the auth algorithm does not have a null value 
	 * for the kx0 generation then it supports it!
	 */
	GNUTLS_KX_ALG_LOOP(ret2 =
			   p->auth_struct->gnutls_process_client_kx);
	if (ret2 != NULL)
		ret = 1;
	return ret;

}


int _gnutls_kx_client_cert_vrfy(KXAlgorithm algorithm)
{
	size_t ret = 0;
	void *ret2 = NULL;

	/* if the auth algorithm does not have a null value 
	 * for the cert_vrfy function then it supports it!
	 */
	GNUTLS_KX_ALG_LOOP(ret2 =
			   p->auth_struct->
			   gnutls_generate_client_cert_vrfy);
	if (ret2 != NULL)
		ret = 1;

	return ret;

}

int _gnutls_kx_server_cert_vrfy(KXAlgorithm algorithm)
{
	size_t ret = 0;
	void *ret2 = NULL;

	/* if the auth algorithm does not have a null value 
	 * for the cert_vrfy function then it supports it!
	 */
	GNUTLS_KX_ALG_LOOP(ret2 =
			   p->auth_struct->
			   gnutls_generate_server_cert_vrfy);
	if (ret2 != NULL)
		ret = 1;

	return ret;

}

int _gnutls_kx_client_certificate(KXAlgorithm algorithm)
{				/* In bytes */
	size_t ret = 0;
	GNUTLS_KX_ALG_LOOP(ret = p->client_cert);
	return ret;

}

int _gnutls_kx_RSA_premaster(KXAlgorithm algorithm)
{				/* In bytes */
	size_t ret = 0;
	GNUTLS_KX_ALG_LOOP(ret = p->RSA_premaster);
	return ret;

}

int _gnutls_kx_DH_public_value(KXAlgorithm algorithm)
{				/* In bytes */
	size_t ret = 0;
	GNUTLS_KX_ALG_LOOP(ret = p->DH_public_value);
	return ret;

}

/**
  * gnutls_kx_get_name - Returns a string with the name of the specified key exchange algorithm
  * @algorithm: is a key exchange algorithm
  *
  * Returns a localy allocated (with malloc) pointer to a string that contains the name 
  * of the specified key exchange algorithm.
  **/
char *gnutls_kx_get_name(KXAlgorithm algorithm)
{
	char *ret = NULL;
	char *pointerTo_;

	/* avoid prefix */
	GNUTLS_KX_ALG_LOOP(ret = strdup(p->name + sizeof("KX_") - 1));


	if (ret != NULL) {
		_gnutls_tolow(ret, strlen(ret));
		pointerTo_ = strchr(ret, '_');

		while (pointerTo_ != NULL) {
			*pointerTo_ = '-';
			pointerTo_ = strchr(ret, '_');
		}
	}
	return ret;
}

int _gnutls_kx_count()
{
	uint8 i, counter = 0;
	for (i = 0; i < MAX_KX; i++) {
		if (_gnutls_kx_is_ok(i) == 0)
			counter++;
	}
	return counter;
}


int _gnutls_kx_is_ok(KXAlgorithm algorithm)
{
	size_t ret = -1;
	GNUTLS_KX_ALG_LOOP(ret = p->algorithm);
	if (ret >= 0)
		ret = 0;
	else
		ret = 1;
	return ret;
}

int _gnutls_version_get_minor(GNUTLS_Version version)
{
	int ret = -1;

	GNUTLS_VERSION_ALG_LOOP(ret = p->minor);
	return ret;
}

GNUTLS_Version _gnutls_version_get(int major, int minor)
{
	int ret = -1;

	GNUTLS_VERSION_LOOP(if ((p->major == major) && (p->minor == minor))
			    ret = p->id);
	return ret;
}

int _gnutls_version_get_major(GNUTLS_Version version)
{
	int ret = -1;

	GNUTLS_VERSION_ALG_LOOP(ret = p->major);
	return ret;
}

/* Version Functions */
int _gnutls_version_cmp(GNUTLS_Version ver1, GNUTLS_Version ver2)
{
	if (ver1 != ver2)
		return 1;
	return 0;
}

int
_gnutls_version_is_supported(GNUTLS_STATE state,
			     const GNUTLS_Version version)
{
	size_t ret = 0;
	/* FIXME: make it to read it from the state */
	GNUTLS_VERSION_ALG_LOOP(ret = p->supported);
	return ret;
}


/* Cipher Suite's functions */
BulkCipherAlgorithm
_gnutls_cipher_suite_get_cipher_algo(const GNUTLS_CipherSuite suite)
{
	size_t ret = 0;
	GNUTLS_CIPHER_SUITE_ALG_LOOP(ret = p->block_algorithm);
	return ret;
}

KXAlgorithm _gnutls_cipher_suite_get_kx_algo(const GNUTLS_CipherSuite
					     suite)
{
	size_t ret = 0;

	GNUTLS_CIPHER_SUITE_ALG_LOOP(ret = p->kx_algorithm);
	return ret;

}

MACAlgorithm
_gnutls_cipher_suite_get_mac_algo(const GNUTLS_CipherSuite suite)
{				/* In bytes */
	size_t ret = 0;
	GNUTLS_CIPHER_SUITE_ALG_LOOP(ret = p->mac_algorithm);
	return ret;

}

char *_gnutls_cipher_suite_get_name(GNUTLS_CipherSuite suite)
{
	char *ret = NULL;
	char *pointerTo_;

	/* avoid prefix */
	GNUTLS_CIPHER_SUITE_ALG_LOOP(ret =
				     strdup(p->name + sizeof("GNUTLS_") -
					    1));


	if (ret != NULL) {
		_gnutls_tolow(ret, strlen(ret));
		pointerTo_ = strchr(ret, '_');

		while (pointerTo_ != NULL) {
			*pointerTo_ = '-';
			pointerTo_ = strchr(ret, '_');
		}
	}
	return ret;
}


int _gnutls_cipher_suite_is_ok(GNUTLS_CipherSuite suite)
{
	size_t ret;
	char *name = NULL;

	GNUTLS_CIPHER_SUITE_ALG_LOOP(name = p->name);
	if (name != NULL)
		ret = 0;
	else
		ret = 1;
	return ret;

}

/* quite expensive */
int _gnutls_cipher_suite_count()
{
	GNUTLS_CipherSuite suite;
	int i, counter = 0, j;

	for (j = 0; j < MAX_CIPHERSUITE; j++) {
		suite.CipherSuite[0] = j;
		if (j != 0x00 && j != 0xF6)
			continue;

		for (i = 0; i < MAX_CIPHERSUITE; i++) {
			suite.CipherSuite[1] = i;
			if (_gnutls_cipher_suite_is_ok(suite) == 0)
				counter++;
		}

	}
	return counter;
}

#define SWAP(x, y) memcpy(tmp,x,size); \
		   memcpy(x,y,size); \
		   memcpy(y,tmp,size);

#define MAX_ELEM_SIZE 4
inline
    static int _gnutls_partition(GNUTLS_STATE state, void *_base,
				 size_t nmemb, size_t size,
				 int (*compar) (GNUTLS_STATE, const void *,
						const void *))
{
	uint8 *base = _base;
	uint8 tmp[MAX_ELEM_SIZE];
	uint8 ptmp[MAX_ELEM_SIZE];
	int pivot;
	int i, j;
	int full;

	i = pivot = 0;
	j = full = (nmemb - 1) * size;

	memcpy(ptmp, &base[0], size);	/* set pivot item */

	while (i < j) {
		while ((compar(state, &base[i], ptmp) <= 0) && (i < full)) {
			i += size;
		}
		while ((compar(state, &base[j], ptmp) >= 0) && (j > 0))
			j -= size;

		if (i < j) {
			SWAP(&base[j], &base[i]);
		}
	}

	if (j > pivot) {
		SWAP(&base[pivot], &base[j]);
		pivot = j;
	} else if (i < pivot) {
		SWAP(&base[pivot], &base[i]);
		pivot = i;
	}
	return pivot / size;
}

static void
_gnutls_qsort(GNUTLS_STATE state, void *_base, size_t nmemb, size_t size,
	      int (*compar) (GNUTLS_STATE, const void *, const void *))
{
	int pivot;
	char *base = _base;
	int snmemb = nmemb;

#ifdef DEBUG
	if (size > MAX_ELEM_SIZE) {
		gnutls_assert();
		exit(1);
	}
#endif

	if (snmemb <= 1)
		return;
	pivot = _gnutls_partition(state, _base, nmemb, size, compar);

	_gnutls_qsort(state, base, pivot < nmemb ? pivot + 1 : pivot, size,
		      compar);
	_gnutls_qsort(state, &base[(pivot + 1) * size], nmemb - pivot - 1,
		      size, compar);
}


/* a compare function for KX algorithms (using priorities). For use with qsort */
static int
_gnutls_compare_algo(GNUTLS_STATE state, const void *i_A1,
		     const void *i_A2)
{
	KXAlgorithm kA1 =
	    _gnutls_cipher_suite_get_kx_algo(*(GNUTLS_CipherSuite *) i_A1);
	KXAlgorithm kA2 =
	    _gnutls_cipher_suite_get_kx_algo(*(GNUTLS_CipherSuite *) i_A2);
	BulkCipherAlgorithm cA1 =
	    _gnutls_cipher_suite_get_cipher_algo(*(GNUTLS_CipherSuite *)
						 i_A1);
	BulkCipherAlgorithm cA2 =
	    _gnutls_cipher_suite_get_cipher_algo(*(GNUTLS_CipherSuite *)
						 i_A2);
	MACAlgorithm mA1 =
	    _gnutls_cipher_suite_get_mac_algo(*(GNUTLS_CipherSuite *)
					      i_A1);
	MACAlgorithm mA2 =
	    _gnutls_cipher_suite_get_mac_algo(*(GNUTLS_CipherSuite *)
					      i_A2);

	int p1 = (_gnutls_kx_priority(state, kA1) + 1) * 100;
	int p2 = (_gnutls_kx_priority(state, kA2) + 1) * 100;
	p1 += (_gnutls_cipher_priority(state, cA1) + 1) * 10;
	p2 += (_gnutls_cipher_priority(state, cA2) + 1) * 10;
	p1 += _gnutls_mac_priority(state, mA1);
	p2 += _gnutls_mac_priority(state, mA2);

	if (p1 > p2) {
		return 1;
	} else {
		if (p1 == p2) {
			return 0;
		}
		return -1;
	}
}

#if 0
static void
_gnutls_bsort(GNUTLS_STATE state, void *_base, size_t nmemb,
	      size_t size, int (*compar) (GNUTLS_STATE, const void *,
					  const void *))
{
	int i, j;
	int full = nmemb * size;
	char *base = _base;
	char tmp[MAX_ELEM_SIZE];

	for (i = 0; i < full; i += size) {
		for (j = 0; j < full; j += size) {
			if (compar(state, &base[i], &base[j]) < 0) {
				SWAP(&base[j], &base[i]);
			}
		}
	}

}
#endif

int
_gnutls_supported_ciphersuites_sorted(GNUTLS_STATE state,
				      GNUTLS_CipherSuite ** ciphers)
{

	int i, ret_count, j = 0;
	int count = _gnutls_cipher_suite_count();
	GNUTLS_CipherSuite *tmp_ciphers;

	if (count == 0) {
		*ciphers = NULL;
		return 0;
	}

	tmp_ciphers = gnutls_malloc(count * sizeof(GNUTLS_CipherSuite));
	*ciphers = gnutls_malloc(count * sizeof(GNUTLS_CipherSuite));


	for (i = 0; i < count; i++) {
		tmp_ciphers[i].CipherSuite[0] =
		    cs_algorithms[i].id.CipherSuite[0];
		tmp_ciphers[i].CipherSuite[1] =
		    cs_algorithms[i].id.CipherSuite[1];
	}

#ifdef SORT_DEBUG
	fprintf(stderr, "Unsorted: \n");
	for (i = 0; i < count; i++)
		fprintf(stderr, "\t%d: %s\n", i,
			_gnutls_cipher_suite_get_name((tmp_ciphers)[i]));
#endif

	_gnutls_qsort(state, tmp_ciphers, count,
		      sizeof(GNUTLS_CipherSuite), _gnutls_compare_algo);

	for (i = 0; i < count; i++) {
		if (_gnutls_kx_priority
		    (state,
		     _gnutls_cipher_suite_get_kx_algo(tmp_ciphers[i])) < 0)
			continue;
		if (_gnutls_mac_priority
		    (state,
		     _gnutls_cipher_suite_get_mac_algo(tmp_ciphers[i])) <
		    0)
			continue;
		if (_gnutls_cipher_priority
		    (state,
		     _gnutls_cipher_suite_get_cipher_algo(tmp_ciphers[i]))
		    < 0)
			continue;

		(*ciphers)[j].CipherSuite[0] =
		    tmp_ciphers[i].CipherSuite[0];
		(*ciphers)[j].CipherSuite[1] =
		    tmp_ciphers[i].CipherSuite[1];
		j++;
	}

#ifdef SORT_DEBUG
	fprintf(stderr, "Sorted: \n");
	for (i = 0; i < j; i++)
		fprintf(stderr, "\t%d: %s\n", i,
			_gnutls_cipher_suite_get_name((*ciphers)[i]));
	exit(0);
#endif

	ret_count = j;

	if (ret_count > 0 && ret_count != count) {
		*ciphers =
		    gnutls_realloc(*ciphers,
				   ret_count * sizeof(GNUTLS_CipherSuite));
	} else {
		if (ret_count != count) {
			gnutls_free(*ciphers);
			*ciphers = NULL;
		}
	}

	gnutls_free(tmp_ciphers);
	return ret_count;
}

int
_gnutls_supported_ciphersuites(GNUTLS_STATE state,
			       GNUTLS_CipherSuite ** ciphers)
{

	int i, ret_count, j;
	int count = _gnutls_cipher_suite_count();
	GNUTLS_CipherSuite *tmp_ciphers;

	if (count == 0) {
		*ciphers = NULL;
		return 0;
	}

	tmp_ciphers = gnutls_malloc(count * sizeof(GNUTLS_CipherSuite));
	*ciphers = gnutls_malloc(count * sizeof(GNUTLS_CipherSuite));


	for (i = 0; i < count; i++) {
		tmp_ciphers[i].CipherSuite[0] =
		    cs_algorithms[i].id.CipherSuite[0];
		tmp_ciphers[i].CipherSuite[1] =
		    cs_algorithms[i].id.CipherSuite[1];
	}

	for (i = j = 0; i < count; i++) {
		if (_gnutls_kx_priority
		    (state,
		     _gnutls_cipher_suite_get_kx_algo(tmp_ciphers[i])) < 0)
			continue;
		if (_gnutls_mac_priority
		    (state,
		     _gnutls_cipher_suite_get_mac_algo(tmp_ciphers[i])) <
		    0)
			continue;
		if (_gnutls_cipher_priority
		    (state,
		     _gnutls_cipher_suite_get_cipher_algo(tmp_ciphers[i]))
		    < 0)
			continue;

		(*ciphers)[j].CipherSuite[0] =
		    tmp_ciphers[i].CipherSuite[0];
		(*ciphers)[j].CipherSuite[1] =
		    tmp_ciphers[i].CipherSuite[1];
		j++;
	}

	ret_count = j;

	if (ret_count > 0 && ret_count != count) {
		*ciphers =
		    gnutls_realloc(*ciphers,
				   ret_count * sizeof(GNUTLS_CipherSuite));
	} else {
		if (ret_count != count) {
			gnutls_free(*ciphers);
			*ciphers = NULL;
		}
	}

	gnutls_free(tmp_ciphers);
	return ret_count;
}


/* For compression  */

/* returns the TLS numbers of the compression methods we support */
#define SUPPORTED_COMPRESSION_METHODS state->gnutls_internals.CompressionMethodPriority.algorithms
int
_gnutls_supported_compression_methods(GNUTLS_STATE state, uint8 ** comp)
{
	int i, tmp;

	*comp = gnutls_malloc(SUPPORTED_COMPRESSION_METHODS);
	if (*comp == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	for (i = 0; i < SUPPORTED_COMPRESSION_METHODS; i++) {
		tmp = _gnutls_compression_get_num(state->gnutls_internals.
						  CompressionMethodPriority.
						  algorithm_priority[i]);
		if (tmp == -1) {
			gnutls_assert();
			/* we shouldn't get here */
			(*comp)[i] = 0;
			continue;
		}
		(*comp)[i] = (uint8) tmp;
	}

	return SUPPORTED_COMPRESSION_METHODS;
}
