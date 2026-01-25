#ifndef IRSSI_COMPAT_H
#define IRSSI_COMPAT_H

#include <irssi/src/common.h>

/* this file is best efforts. patches welcome

   available options (before including)

   #define WANT_RAWLOG_COMPAT
   #define WANT_MAINWIN_COMPAT
   #define WANT_TIME_COMPAT

   available flags (after including)

   IRSSI_OLD_RAWLOG
   IRSSI_OLD_TIME
   IRSSI_OLD_SERVER_REDIRECT_REGISTER_LIST
   IRSSI_OLD_IRC_SERVER_GET_CHANNELS

*/

/****** IRSSI_ABI_VERSION    0    ******/

#ifndef IRSSI_ABI_VERSION
#define IRSSI_ABI_VERSION 0
#endif

/****** IRSSI_ABI_VERSION    6    ******/

#if IRSSI_ABI_VERSION < 6
#define use_tls use_ssl
#define tls_verify ssl_verify
#endif

/****** IRSSI_ABI_VERSION    18    ******/

#if IRSSI_ABI_VERSION < 18
#define IRSSI_OLD_RAWLOG
#endif

#ifdef WANT_RAWLOG_COMPAT
#ifdef IRSSI_OLD_RAWLOG
#define i_rawlog_get_size(rawlog) ((rawlog)->nlines)
#else
#define i_rawlog_get_size(rawlog) ((rawlog)->lines->length)
#endif
#endif

/****** IRSSI_ABI_VERSION    20    ******/

#ifdef WANT_MAINWIN_COMPAT
#if IRSSI_ABI_VERSION < 20
#define i_first_text_column(mainwin) 0
#else
#define i_first_text_column(mainwin) ((mainwin)->first_column + (mainwin)->statusbar_columns_left)
#endif
#endif

/****** IRSSI_ABI_VERSION    21    ******/

#if IRSSI_ABI_VERSION < 21
#define MODULE_ABICHECK(fn_modulename)                                                             \
	void fn_modulename##_abicheck(int *version)                                                \
	{                                                                                          \
		*version = IRSSI_ABI_VERSION;                                                      \
	}
#endif

/****** IRSSI_ABI_VERSION    25    ******/

#if IRSSI_ABI_VERSION < 25
#define IRSSI_OLD_TIME
#define ITimeType GTimeVal
#else
#define ITimeType gint64
#endif

#ifdef WANT_TIME_COMPAT
#ifdef IRSSI_OLD_TIME
#define i_set_current_time(var) g_get_current_time(&(var))
#define i_clear_time(var) memset(&(var), 0, sizeof((var)))
#define i_has_time(var) ((var).tv_sec != 0)
#define i_get_time_sec(var) ((var).tv_sec)
#define i_get_time_diff(to, from) (int) get_timeval_diff(&(to), &(from))
#else
#define i_set_current_time(var) (var) = g_get_real_time()
#define i_clear_time(var) (var) = 0
#define i_has_time(var) ((var) != 0)
#define i_get_time_sec(var) ((var) / G_TIME_SPAN_SECOND)
#define i_get_time_diff(to, from) (to) - (from)
#endif
#endif

/****** IRSSI_ABI_VERSION    32    ******/

#if IRSSI_ABI_VERSION < 32
#define I_INPUT_WRITE G_INPUT_WRITE
#define i_input_add g_input_add
#define i_slist_find_icase_string gslist_find_icase_string
#endif

/****** IRSSI_ABI_VERSION    33    ******/

#if IRSSI_ABI_VERSION < 33
#define i_list_find_string glist_find_string
#endif

/****** IRSSI_ABI_VERSION    35    ******/

#if IRSSI_ABI_VERSION < 35
/* server_redirect_register_list takes 6 arguments */
#define IRSSI_OLD_SERVER_REDIRECT_REGISTER_LIST
#else
/* server_redirect_register_list takes 7 arguments */
#endif

/****** IRSSI_ABI_VERSION    36    ******/

#if IRSSI_ABI_VERSION < 36
/* irc_server_get_channels takes 1 argument */
#define IRSSI_OLD_IRC_SERVER_GET_CHANNELS
#else
/* irc_server_get_channels takes 2 argument, rejoin_channels_on_reconnect setting exists */
#endif

#endif
