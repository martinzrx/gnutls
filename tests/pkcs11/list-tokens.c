/*
 * Copyright (C) 2016-2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include <getopt.h>
#include <assert.h>

/* lists the registered PKCS#11 modules by p11-kit.
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

int
_gnutls_pkcs11_token_get_url(unsigned int seq,
                            gnutls_pkcs11_url_type_t detailed, char **url,
                            unsigned flags);

int main(int argc, char **argv)
{
	int ret;
	unsigned i;
	int opt;
	char *url;
	gnutls_certificate_credentials_t cred;
	unsigned flag = 1;

	ret = gnutls_global_init();
	if (ret != 0) {
		fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_global_set_log_function(tls_log_func);
	//gnutls_global_set_log_level(4711);

	while((opt = getopt(argc, argv, "mvatd")) != -1) {
		switch(opt) {
			case 'm':
				ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_MANUAL, NULL);
				if (ret != 0) {
					fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
					exit(1);
				}
				break;
			case 'd':
				flag = 0;
				break;
			case 'a':
				ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_AUTO, NULL);
				if (ret != 0) {
					fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
					exit(1);
				}
				break;
			case 't':
				ret = gnutls_pkcs11_init(GNUTLS_PKCS11_FLAG_AUTO_TRUSTED, NULL);
				if (ret != 0) {
					fprintf(stderr, "error at %d: %s\n", __LINE__, gnutls_strerror(ret));
					exit(1);
				}
				break;
			case 'v':
				assert(gnutls_certificate_allocate_credentials(&cred) >= 0);
				assert(gnutls_certificate_set_x509_system_trust(cred) >= 0);
				gnutls_certificate_free_credentials(cred);
				break;
			default:
				fprintf(stderr, "Unknown option %c\n", (char)opt);
				exit(1);
		}
	}


	for (i=0;;i++) {
		ret = _gnutls_pkcs11_token_get_url(i, 0, &url, flag);
		if (ret < 0)
			break;
		printf("%s\n", url);
		free(url);
	}

	gnutls_global_deinit();
}
