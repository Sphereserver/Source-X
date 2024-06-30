/* Copyright (C) 2013 by MontyProgram AB

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02111-1301, USA */

/* defines for the libmariadb library */

#ifndef _ma_common_h
#define _ma_common_h

#include <mysql.h>
#include <mysql/client_plugin.h>
#include <ma_hashtbl.h>

enum enum_multi_status {
  COM_MULTI_OFF= 0,
  COM_MULTI_CANCEL,
  COM_MULTI_ENABLED,
  COM_MULTI_DISABLED,
  COM_MULTI_END
};


typedef enum {
  ALWAYS_ACCEPT,       /* heuristics is disabled, use CLIENT_LOCAL_FILES */
  WAIT_FOR_QUERY,      /* heuristics is enabled, not sending files */
  ACCEPT_FILE_REQUEST  /* heuristics is enabled, ready to send a file */
} auto_local_infile_state;

typedef struct st_mariadb_db_driver
{
  struct st_mariadb_client_plugin_DB *plugin;
  char *name;
  void *buffer;
} MARIADB_DB_DRIVER;

struct mysql_async_context;

struct st_mysql_options_extension {
  char *plugin_dir;
  char *default_auth;
  char *ssl_crl;
  char *ssl_crlpath;
  char *server_public_key_path;
  struct mysql_async_context *async_context;
  MA_HASHTBL connect_attrs;
  size_t connect_attrs_len;
  void (*report_progress)(const MYSQL *mysql,
                          unsigned int stage,
                          unsigned int max_stage,
                          double progress,
                          const char *proc_info,
                          unsigned int proc_info_length);
  MARIADB_DB_DRIVER *db_driver;
  char *tls_fp; /* finger print of server certificate */
  char *tls_fp_list; /* white list of finger prints */
  char *tls_pw; /* password for encrypted certificates */
  my_bool multi_command; /* indicates if client wants to send multiple
                            commands in one packet */
  char *url; /* for connection handler we need to save URL for reconnect */
  unsigned int tls_cipher_strength;
  char *tls_version;
  my_bool read_only;
  char *connection_handler;
  my_bool (*set_option)(MYSQL *mysql, const char *config_option, const char *config_value);
  MA_HASHTBL userdata;
  char *server_public_key;
  char *proxy_header;
  size_t proxy_header_len;
  int (*io_wait)(my_socket handle, my_bool is_read, int timeout);
  my_bool skip_read_response;
  char *restricted_auth;
  char *rpl_host;
  unsigned short rpl_port;
  void (*status_callback)(void *ptr, enum enum_mariadb_status_info type, ...);
  void *status_data;
};

typedef struct st_connection_handler
{
  struct st_ma_connection_plugin *plugin;
  void *data;
  my_bool active;
  my_bool free_data;
} MA_CONNECTION_HANDLER;

struct st_mariadb_net_extension {
  enum enum_multi_status multi_status;
  int extended_errno;
  ma_compress_ctx *compression_ctx;
  MARIADB_COMPRESSION_PLUGIN *compression_plugin;
};

struct st_mariadb_session_state
{
  LIST *list,
       *current;
};

struct st_mariadb_extension {
  MA_CONNECTION_HANDLER *conn_hdlr;
  struct st_mariadb_session_state session_state[SESSION_TRACK_TYPES];
  unsigned long mariadb_client_flag; /* MariaDB specific client flags */
  unsigned long mariadb_server_capabilities; /* MariaDB specific server capabilities */
  my_bool auto_local_infile;
};

#define OPT_EXT_VAL(a,key) \
  (((a)->options.extension && (a)->options.extension->key) ?\
    (a)->options.extension->key : 0)

#endif


typedef struct st_mariadb_field_extension
{
  MARIADB_CONST_STRING metadata[MARIADB_FIELD_ATTR_LAST+1]; /* 10.5 */
} MA_FIELD_EXTENSION;
