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

MPI gnutls_calc_dh_secret( MPI *ret_x );
MPI gnutls_get_dh_params(MPI *ret_p);
MPI gnutls_calc_dh_key( MPI f, MPI x );
MPI _gnutls_calc_dh_secret( MPI *ret_x, MPI g, MPI prime );
MPI _gnutls_calc_dh_key( MPI f, MPI x, MPI prime );
