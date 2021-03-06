/*
 * Copyright (C) 2008-2014 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson, Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "utils.h"
#include "test-chains-issuer.h"

#define DEFAULT_THEN 1256803113
static time_t then = DEFAULT_THEN;

/* GnuTLS internally calls time() to find out the current time when
   verifying certificates.  To avoid a time bomb, we hard code the
   current time.  This should work fine on systems where the library
   call to time is resolved at run-time.  */
static time_t mytime(time_t * t)
{
	if (t)
		*t = then;

	return then;
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static int getissuer_callback(gnutls_x509_trust_list_t tlist,
			      const gnutls_x509_crt_t crt)
{
	gnutls_x509_crt_t issuer;
	gnutls_datum_t tmp;
	int ret;

	ret = gnutls_x509_crt_init(&issuer);
	if (ret < 0) {
		fprintf(stderr, "error: %s\n", gnutls_strerror(ret));
		return -1;
	}

	tmp.data = (unsigned char *)missing_cert_insert;
	tmp.size = strlen(missing_cert_insert);

	ret = gnutls_x509_crt_import(issuer, &tmp, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "error: %s\n", gnutls_strerror(ret));
		gnutls_x509_crt_deinit(issuer);
		return -1;
	}

	/* This transfers the ownership of `issuer` to `tlist`. */
	ret = gnutls_x509_trust_list_add_cas(tlist, &issuer, 1, 0);
	if (ret < 0) {
		fprintf(stderr, "error: %s\n", gnutls_strerror(ret));
		gnutls_x509_crt_deinit(issuer);
		return -1;
	}

	assert(gnutls_x509_crt_print(crt, GNUTLS_CRT_PRINT_ONELINE, &tmp) >= 0);

	if (debug)
		printf("\t Certificate missing issuer is: %.*s\n",
				tmp.size, tmp.data);
	gnutls_free(tmp.data);

	assert(gnutls_x509_crt_print(issuer, GNUTLS_CRT_PRINT_ONELINE, &tmp) >= 0);

	if (debug)
		printf("\t Appended issuer certificate is: %.*s\n",
				tmp.size, tmp.data);
	gnutls_free(tmp.data);
	return 0;

}

void doit(void)
{
	int exit_val = 0;
	int ret;
	gnutls_x509_trust_list_t tl;
	unsigned int verify_status;
	gnutls_x509_crt_t certs[MAX_CHAIN];
	gnutls_x509_crt_t ca;
	gnutls_datum_t tmp;
	size_t j;

	/* The overloading of time() seems to work in linux (ELF?)
	 * systems only. Disable it on windows.
	 */
#ifdef _WIN32
	exit(77);
#endif

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);

	if (debug)
		gnutls_global_set_log_level(4711);

	for (j = 0; j < MAX_CHAIN; j++) {
		if (debug > 2)
			printf("\tAdding certificate %d...", (int)j);

		ret = gnutls_x509_crt_init(&certs[j]);
		if (ret < 0) {
			fprintf(stderr,
					"gnutls_x509_crt_init[%d]: %s\n",
					(int)j, gnutls_strerror(ret));
			exit(1);
		}

		tmp.data = (unsigned char *)missing_issuer_chain[j];
		tmp.size = strlen(missing_issuer_chain[j]);

		ret =
			gnutls_x509_crt_import(certs[j], &tmp,
					GNUTLS_X509_FMT_PEM);
		if (debug > 2)
			printf("done\n");
		if (ret < 0) {
			fprintf(stderr,
					"gnutls_x509_crt_import[%d]: %s\n",
					(int)j,
					gnutls_strerror(ret));
			exit(1);
		}

		gnutls_x509_crt_print(certs[j],
				GNUTLS_CRT_PRINT_ONELINE, &tmp);
		if (debug)
			printf("\tCertificate %d: %.*s\n", (int)j,
					tmp.size, tmp.data);
		gnutls_free(tmp.data);
	}

	if (debug > 2)
		printf("\tAdding CA certificate...");

	ret = gnutls_x509_crt_init(&ca);
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_init: %s\n",
				gnutls_strerror(ret));
		exit(1);
	}

	tmp.data = (unsigned char *)missing_issuer_chain[MAX_CHAIN-1];
	tmp.size = strlen(missing_issuer_chain[MAX_CHAIN-1]);

	ret = gnutls_x509_crt_import(ca, &tmp, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_import: %s\n",
				gnutls_strerror(ret));
		exit(1);
	}

	if (debug > 2)
		printf("done\n");

	gnutls_x509_crt_print(ca, GNUTLS_CRT_PRINT_ONELINE, &tmp);
	if (debug)
		printf("\tCA Certificate: %.*s\n", tmp.size, tmp.data);
	gnutls_free(tmp.data);

	if (debug)
		printf("\tVerifying...");

	gnutls_x509_trust_list_init(&tl, 0);

	ret = gnutls_x509_trust_list_add_cas(tl, &ca, 1, 0);
	if (ret != 1) {
		fail("gnutls_x509_trust_list_add_trust_mem\n");
		exit(1);
	}

	gnutls_x509_trust_list_set_getissuer_function(tl, getissuer_callback);

	ret = gnutls_x509_trust_list_verify_crt(tl, certs, MAX_CHAIN,
			0,
			&verify_status,
			NULL);
	if (ret < 0) {
		fprintf(stderr,
				"gnutls_x509_crt_list_verify: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	if (debug)
		printf("\tCleanup...");

	gnutls_x509_trust_list_deinit(tl, 1);

	for (j = 0; j < MAX_CHAIN; j++)
		gnutls_x509_crt_deinit(certs[j]);

	if (debug)
		printf("done\n\n\n");

	gnutls_global_deinit();

	if (debug)
		printf("Exit status...%d\n", exit_val);

	exit(exit_val);
}
