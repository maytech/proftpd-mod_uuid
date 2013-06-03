/*
 * ProFTPD: mod_uuid -- a module for generating a UUID for each FTP session.
 *
 * Copyright (c) 2013 Mike Futerko
 * Copyright (c) 2006-2011 TJ Saunders
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 * As a special exemption, TJ Saunders and other respective copyright holders
 * give permission to link this program with OpenSSL, and distribute the
 * resulting executable, without including the source code for OpenSSL in the
 * source distribution.
 *
 * This is mod_uuid, contrib software for proftpd 1.3.x and above.
 * For more information contact Mike Futerko <mike@maytech.net>.
 *
 * $Id$
 */

#include <uuid/uuid.h>
#include "conf.h"

/* Most of this module code is modification of the mod_unique_id
 * module for ProFTPd by TJ Saunders <tj@castaglia.org>.
 */

#define MOD_UUID_VERSION		"mod_uuid/0.1"

/* Make sure the version of proftpd is as necessary. */
#if PROFTPD_VERSION_NUMBER < 0x0001030402
# error "ProFTPD 1.3.4rc2 or later required"
#endif

module uuid_module;


#ifndef UUID_PRINTABLE_STRING_LENGTH
#define UUID_PRINTABLE_STRING_LENGTH    37
#endif


/* Configuration handlers
 */

/* usage: UUIDEngine on|off */
MODRET set_uuidengine(cmd_rec *cmd) {
  int bool = -1;
  config_rec *c = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  bool = get_boolean(cmd, 1);
  if (bool == -1)
    CONF_ERROR(cmd, "expected Boolean parameter");

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[0]) = bool;

  return PR_HANDLED(cmd);
}

/* Event handlers
 */

#if defined(PR_SHARED_MODULE)
static void uuid_mod_unload_ev(const void *event_data, void *user_data) {
  if (strcmp("mod_uuid.c", (const char *) event_data) == 0) {
    /* Unregister ourselves from all events. */
    pr_event_unregister(&uuid_module, NULL, NULL);
  }
}
#endif /* PR_SHARED_MODULE */


/* Initialization functions
 */

static int uuid_init(void) {

#if defined(PR_SHARED_MODULE)
  pr_event_register(&uuid_module, "core.module-unload",
    uuid_mod_unload_ev, NULL);
#endif /* PR_SHARED_MODULE */

  return 0;
}

static int uuid_sess_init(void) {
  config_rec *c;
  int uniqid_engine = TRUE;
  char *key = "UUID";
  char id[UUID_PRINTABLE_STRING_LENGTH];
  uuid_t uu;


  c = find_config(main_server->conf, CONF_PARAM, "UUIDEngine", FALSE);
  if (c) {
    uniqid_engine = *((int *) c->argv[0]);
  }

  if (!uniqid_engine)
    return 0;

  pr_log_debug(DEBUG8, MOD_UUID_VERSION ": generating session UUID");


  uuid_generate_random(uu);
  uuid_unparse(uu, id);
  
  if (pr_env_set(session.pool, key, id) < 0) {
    pr_log_debug(DEBUG0, MOD_UUID_VERSION
      ": error setting UUID environment variable: %s", strerror(errno));

  } else {
    pr_log_debug(DEBUG8, MOD_UUID_VERSION
      ": session UUID is '%s'", id);
  }

  if (pr_table_add_dup(session.notes, pstrdup(session.pool, key), id, 0) < 0) {
    pr_log_debug(DEBUG0, MOD_UUID_VERSION
      ": error adding %s session note: %s", key, strerror(errno));
  }

  return 0;
}

/* Module API tables
 */

static conftable uuid_conftab[] = {
  { "UUIDEngine",	set_uuidengine,	NULL },
  { NULL }
};

module uuid_module = {
  NULL, NULL,

  /* Module API version 2.0 */
  0x20,

  /* Module name */
  "uuid",

  /* Module configuration handler table */
  uuid_conftab,

  /* Module command handler table */
  NULL,

  /* Module authentication handler table */
  NULL,

  /* Module initialization function */
  uuid_init,

  /* Session initialization function */
  uuid_sess_init,

  /* Module version */
  MOD_UUID_VERSION
};

