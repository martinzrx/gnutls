/*
 * Copyright (C) 2005, 2007, 2008 Free Software Foundation
 *
 * This file is part of GNUTLS.
 *
 * GNUTLS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *               
 * GNUTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *                               
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

/* Gnulib portability files. */
#include <version-etc.h>

#ifndef ENABLE_PSK

#include <stdio.h>

int
main (int argc, char **argv)
{
  printf ("\nPSK not supported. This program is a dummy.\n\n");
  return 1;
};

#else

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/extra.h>
#include <psk-gaa.h>

#include "../lib/random.h"	/* for random */

#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
# include <pwd.h>
# include <unistd.h>
#else
# include <windows.h>
#endif

/* Gnulib portability files. */
#include <minmax.h>
#include "getpass.h"

static int write_key (const char *username, const char *key, int key_size,
		      char *passwd_file);

#define KPASSWD "/etc/passwd.psk"
#define MAX_KEY_SIZE 64
int
main (int argc, char **argv)
{
  gaainfo info;
  int ret;
  struct passwd *pwd;
  unsigned char key[MAX_KEY_SIZE];
  char hex_key[MAX_KEY_SIZE * 2 + 1];
  gnutls_datum_t dkey;
  size_t hex_key_size = sizeof (hex_key);

  if ((ret = gnutls_global_init ()) < 0)
    {
      fprintf (stderr, "global_init: %s\n", gnutls_strerror (ret));
      exit (1);
    }

  umask (066);

  if (gaa (argc, argv, &info) != -1)
    {
      fprintf (stderr, "Error in the arguments.\n");
      return -1;
    }

  if (info.passwd == NULL)
    info.passwd = (char*) KPASSWD;

  if (info.username == NULL)
    {
#ifndef _WIN32
      pwd = getpwuid (getuid ());

      if (pwd == NULL)
	{
	  fprintf (stderr, "No such user\n");
	  return -1;
	}

      info.username = pwd->pw_name;
#else
      fprintf (stderr, "Please specify a user\n");
      return -1;
#endif
    }

  if (info.key_size > MAX_KEY_SIZE)
    {
      fprintf (stderr, "Key size is too long\n");
      exit (1);
    }

  if (info.netconf_hint)
    {
      char *passwd;

      if (info.key_size != 0 && info.key_size != 20)
	{
	  fprintf (stderr, "For netconf, key size must always be 20.\n");
	  exit (1);
	}

      passwd = getpass ("Enter password: ");
      if (passwd == NULL)
	{
	  fprintf (stderr, "Please specify a password\n");
	  exit (1);
	}

      ret = gnutls_psk_netconf_derive_key (passwd,
					   info.username,
					   info.netconf_hint, &dkey);
    }
  else
    {
      if (info.key_size < 1)
	info.key_size = 16;

      printf ("Generating a random key for user '%s'\n", info.username);

      ret = _gnutls_rnd (GNUTLS_RND_RANDOM, (char *) key, info.key_size);
      if (ret < 0)
	{
	  fprintf (stderr, "Not enough randomness\n");
	  exit (1);
	}

      dkey.data = key;
      dkey.size = info.key_size;
    }

  ret = gnutls_hex_encode (&dkey, hex_key, &hex_key_size);
  if (info.netconf_hint)
    gnutls_free (dkey.data);
  if (ret < 0)
    {
      fprintf (stderr, "HEX encoding error\n");
      exit (1);
    }

  ret = write_key (info.username, hex_key, hex_key_size, info.passwd);
  if (ret == 0)
    printf ("Key stored to %s\n", info.passwd);

  return ret;
}

static int
filecopy (char *src, char *dst)
{
  FILE *fd, *fd2;
  char line[5 * 1024];
  char *p;

  fd = fopen (dst, "w");
  if (fd == NULL)
    {
      fprintf (stderr, "Cannot open '%s' for write\n", dst);
      return -1;
    }

  fd2 = fopen (src, "r");
  if (fd2 == NULL)
    {
      /* empty file */
      fclose (fd);
      return 0;
    }

  line[sizeof (line) - 1] = 0;
  do
    {
      p = fgets (line, sizeof (line) - 1, fd2);
      if (p == NULL)
	break;

      fputs (line, fd);
    }
  while (1);

  fclose (fd);
  fclose (fd2);

  return 0;
}

static int
write_key (const char *username, const char *key, int key_size,
	   char *passwd_file)
{
  FILE *fd;
  char line[5 * 1024];
  char *p, *pp;
  char tmpname[1024];


  /* delete previous entry */
  struct stat st;
  FILE *fd2;
  int put;

  if (strlen (passwd_file) > sizeof (tmpname) + 5)
    {
      fprintf (stderr, "file '%s' is tooooo long\n", passwd_file);
      return -1;
    }
  strcpy (tmpname, passwd_file);
  strcat (tmpname, ".tmp");

  if (stat (tmpname, &st) != -1)
    {
      fprintf (stderr, "file '%s' is locked\n", tmpname);
      return -1;
    }

  if (filecopy (passwd_file, tmpname) != 0)
    {
      fprintf (stderr, "Cannot copy '%s' to '%s'\n", passwd_file, tmpname);
      return -1;
    }

  fd = fopen (passwd_file, "w");
  if (fd == NULL)
    {
      fprintf (stderr, "Cannot open '%s' for write\n", passwd_file);
      remove (tmpname);
      return -1;
    }

  fd2 = fopen (tmpname, "r");
  if (fd2 == NULL)
    {
      fprintf (stderr, "Cannot open '%s' for read\n", tmpname);
      remove (tmpname);
      return -1;
    }

  put = 0;
  do
    {
      p = fgets (line, sizeof (line) - 1, fd2);
      if (p == NULL)
	break;

      pp = strchr (line, ':');
      if (pp == NULL)
	continue;

      if (strncmp (p, username,
		   MAX (strlen (username), (unsigned int) (pp - p))) == 0)
	{
	  put = 1;
	  fprintf (fd, "%s:%s\n", username, key);
	}
      else
	{
	  fputs (line, fd);
	}
    }
  while (1);

  if (put == 0)
    {
      fprintf (fd, "%s:%s\n", username, key);
    }

  fclose (fd);
  fclose (fd2);

  remove (tmpname);


  return 0;
}

#endif /* ENABLE_PSK */

void psktool_version (void);

void
psktool_version (void)
{
  const char *p = PACKAGE_NAME;
  if (strcmp (gnutls_check_version (NULL), PACKAGE_VERSION) != 0)
    p = PACKAGE_STRING;
  version_etc (stdout, "psktool", p, gnutls_check_version (NULL),
	       "Nikos Mavrogiannopoulos", (char *) NULL);
}
