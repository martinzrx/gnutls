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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "../lib/gnutls.h"
#include <signal.h>
#include "port.h"
#include <sys/time.h>

#define SA struct sockaddr
#define ERR(err,s) if (err==-1) {perror(s);return(1);}
#define MAX_BUF 4096

#define RESUME

#define MAX(X,Y) (X >= Y ? X : Y);

static int print_info( GNUTLS_STATE state) {
char *tmp;
const DH_ANON_AUTH_INFO *dh_info;

	tmp = gnutls_kx_get_name(gnutls_get_current_kx( state));
	printf("- Key Exchange: %s\n", tmp); free(tmp);
	if (gnutls_get_current_kx(state) == GNUTLS_KX_DH_ANON) {
		dh_info = gnutls_get_auth_info(state);
		if (dh_info != NULL)
			printf("- Anonymous DH using prime of %d bits\n",
			       dh_info->bits);
	}

	tmp = gnutls_compression_get_name(gnutls_get_current_compression_method( state));
	printf("- Compression: %s\n", tmp); free(tmp);

	tmp = gnutls_cipher_get_name(gnutls_get_current_cipher( state));
	printf("- Cipher: %s\n", tmp); free(tmp);

	tmp = gnutls_mac_get_name(gnutls_get_current_mac_algorithm( state));
	printf("- MAC: %s\n", tmp); free(tmp);

	return 0;
}

int main()
{
	int err, ret;
	int sd, ii;
	struct sockaddr_in sa;
	GNUTLS_STATE state;
	char buffer[MAX_BUF+1];
	char *session;
	char* session_id;
	int session_size;
	int session_id_size;
	char* tmp_session_id;
	int tmp_session_id_size;
	fd_set rset;
	int maxfd;
	struct timeval tv;
	int user_term = 0;
	SRP_CLIENT_CREDENTIALS cred;
	
	cred.username = "test";
	cred.password = "test";
	
//	signal(SIGPIPE, SIG_IGN);

	sd = socket(AF_INET, SOCK_STREAM, 0);
	ERR(sd, "socket");

	memset(&sa, '\0', sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(SERVER);
	sa.sin_port = htons(PORT);

	err = connect(sd, (SA *) & sa, sizeof(sa));
	ERR(err, "connect");

#ifdef RESUME
	gnutls_init(&state, GNUTLS_CLIENT);
	gnutls_set_current_version( state, GNUTLS_TLS1);

	gnutls_set_cipher_priority( state, GNUTLS_3DES, GNUTLS_ARCFOUR, GNUTLS_RIJNDAEL, 0);
	gnutls_set_compression_priority( state, GNUTLS_ZLIB, GNUTLS_NULL_COMPRESSION, 0);
	gnutls_set_kx_priority( state, GNUTLS_KX_SRP, GNUTLS_KX_DH_ANON, 0);
	gnutls_set_kx_cred( state, GNUTLS_KX_DH_ANON, NULL);
	gnutls_set_kx_cred( state, GNUTLS_KX_SRP, &cred);

	gnutls_set_mac_priority( state, GNUTLS_MAC_SHA, GNUTLS_MAC_MD5, 0);
	ret = gnutls_handshake(sd, state);

	if (ret < 0) {
		fprintf(stderr, "*** Handshake has failed\n");
		gnutls_perror(ret);
		gnutls_deinit(state);
		return 1;
	} else {
		printf("- Handshake was completed\n");
	}
	gnutls_get_current_session( state, NULL, &session_size);
	session = malloc(session_size);
	gnutls_get_current_session( state, session, &session_size);

	gnutls_get_current_session_id( state, NULL, &session_id_size);
	session_id = malloc(session_id_size);
	gnutls_get_current_session_id( state, session_id, &session_id_size);

/* print some information */
	print_info( state);

	printf("- Disconnecting\n");
	gnutls_close(sd, state);
	shutdown( sd, SHUT_WR);
	close(sd);	
	gnutls_deinit( state);	
	
	printf("\n\n- Connecting again- trying to resume previous session\n");
	sd = socket(AF_INET, SOCK_STREAM, 0);
	ERR(sd, "socket");

	err = connect(sd, (SA *) & sa, sizeof(sa));
	ERR(err, "connect");

#endif

	/* Begin handshake again */
	gnutls_init(&state, GNUTLS_CLIENT);
	
	gnutls_set_current_version( state, GNUTLS_TLS1);

	gnutls_set_cipher_priority( state, GNUTLS_3DES, GNUTLS_TWOFISH , GNUTLS_RIJNDAEL, GNUTLS_ARCFOUR, 0);
	gnutls_set_compression_priority( state, GNUTLS_NULL_COMPRESSION, 0);
	gnutls_set_kx_priority( state, GNUTLS_KX_SRP, GNUTLS_KX_DH_ANON, 0);
	gnutls_set_kx_cred( state, GNUTLS_KX_DH_ANON, NULL);
	gnutls_set_kx_cred( state, GNUTLS_KX_SRP, &cred);

	gnutls_set_mac_priority( state, GNUTLS_MAC_SHA, GNUTLS_MAC_MD5, 0);

#ifdef RESUME
	gnutls_set_current_session( state, session, session_size);
	free(session);
#endif

	ret = gnutls_handshake(sd, state);

	if (ret < 0) {
		fprintf(stderr, "*** Handshake failed\n");
		gnutls_perror(ret);
		gnutls_deinit(state);
		return 1;
	} else {
		printf("- Handshake was completed\n");
	}

	/* check if we actually resumed the previous session */
	gnutls_get_current_session_id( state, NULL, &tmp_session_id_size);
	tmp_session_id = malloc(tmp_session_id_size);
	gnutls_get_current_session_id( state, tmp_session_id, &tmp_session_id_size);
	if (memcmp( tmp_session_id, session_id, session_id_size)==0) {
		printf("- Previous session was resumed\n");
	} else {
		fprintf(stderr, "*** Previous session was NOT resumed\n");	
	}
	free(tmp_session_id);
	free(session_id);

/* print some information */
	print_info( state);
	
	printf("\n- Simple Client Mode:\n\n");

	FD_ZERO(&rset);
	for(;;) {
		FD_SET(fileno(stdin), &rset);
		FD_SET(sd, &rset);
		
		maxfd = MAX(fileno(stdin), sd);
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		select(maxfd+1, &rset, NULL, NULL, &tv);

		if (FD_ISSET(sd, &rset)) {
			bzero(buffer, MAX_BUF+1);

			ret = gnutls_read(sd, state, buffer, MAX_BUF);
			/* remove new line */

			if (gnutls_is_fatal_error(ret) == 1) {
				if (ret == GNUTLS_E_CLOSURE_ALERT_RECEIVED || ret == GNUTLS_E_INVALID_SESSION) {
					printf("- Peer has closed the GNUTLS connection\n");
					break;
				} else {
					fprintf(stderr, "*** Received corrupted data(%d) - server has terminated the connection abnormally\n",
						ret);
					break;
				}
			} else {
				if (ret==GNUTLS_E_WARNING_ALERT_RECEIVED || ret==GNUTLS_E_FATAL_ALERT_RECEIVED)
					printf("* Received alert [%d]\n", gnutls_get_last_alert(state));
				if (ret==GNUTLS_E_GOT_HELLO_REQUEST)
					printf("* Received HelloRequest message\n");

				if (ret > 0) {
					printf("- Received[%d]: ", ret);
					for (ii=0;ii<ret;ii++) {
						fputc(buffer[ii], stdout);
					}
					fputs("\n", stdout);
				}
			}
			if (user_term!=0) break;
		}
		if (FD_ISSET(fileno(stdin), &rset)) {
	
			if( fgets(buffer, MAX_BUF, stdin) == NULL) {
				gnutls_close(sd, state);
				user_term = 1;
				continue;
			}
			gnutls_write( sd, state, buffer, strlen(buffer));
			printf("- Sent: %d bytes\n", strlen(buffer));
		}
	}
	if (user_term!=0) gnutls_close(sd, state);
	
	shutdown( sd, SHUT_RDWR); /* no more receptions */
	close(sd);
	
	gnutls_deinit( state);
	return 0;
}
