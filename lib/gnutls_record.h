/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2010 Free
 * Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA
 *
 */

#ifndef GNUTLS_RECORD_H
#define GNUTLS_RECORD_H

#include <gnutls/gnutls.h>
#include <gnutls_buffers.h>

ssize_t _gnutls_send_int (gnutls_session_t session, content_type_t type,
                          gnutls_handshake_description_t htype,
                          unsigned int epoch_rel, const void *data,
                          size_t sizeofdata, unsigned int mflags);
ssize_t _gnutls_recv_int (gnutls_session_t session, content_type_t type,
                          gnutls_handshake_description_t, opaque * data,
                          size_t sizeofdata, void* seq);

#endif
