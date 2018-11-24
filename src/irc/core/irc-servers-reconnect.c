/*
 servers-reconnect.c : irssi

    Copyright (C) 1999-2000 Timo Sirainen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "module.h"
#include "commands.h"
#include "network.h"
#include "signals.h"
#include "core/signal-registry.h"
#include "signal-registry.h"

#include "modes.h"
#include "irc-servers.h"

#include "settings.h"

static void sig_server_connect_copy(SERVER_CONNECT_REC **dest,
				    SERVER_CONNECT_REC *src)
{
	IRC_SERVER_CONNECT_REC *rec, *irc_src;

	g_return_if_fail(dest != NULL);
	if ((irc_src = IRC_SERVER_CONNECT(src)) == NULL)
		return;

	rec = g_new0(IRC_SERVER_CONNECT_REC, 1);
	rec->chat_type = IRC_PROTOCOL;
	rec->max_cmds_at_once = irc_src->max_cmds_at_once;
	rec->cmd_queue_speed = irc_src->cmd_queue_speed;
        rec->max_query_chans = irc_src->max_query_chans;
	rec->max_kicks = irc_src->max_kicks;
	rec->max_modes = irc_src->max_modes;
	rec->max_msgs = irc_src->max_msgs;
	rec->max_whois = irc_src->max_whois;
	rec->usermode = g_strdup(irc_src->usermode);
	rec->alternate_nick = g_strdup(irc_src->alternate_nick);
	rec->sasl_mechanism = irc_src->sasl_mechanism;
	rec->sasl_username = irc_src->sasl_username;
	rec->sasl_password = irc_src->sasl_password;
	*dest = (SERVER_CONNECT_REC *) rec;
}

static void sig_server_reconnect_save_status(SERVER_CONNECT_REC *conn,
					     SERVER_REC *server)
{
	IRC_SERVER_CONNECT_REC *irc_conn;
	IRC_SERVER_REC *irc_server;
	if ((irc_conn = IRC_SERVER_CONNECT(conn)) == NULL || (irc_server = IRC_SERVER(server)) == NULL ||
	    !server->connected)
		return;

	g_free_not_null(irc_conn->channels);
	irc_conn->channels = irc_server_get_channels(irc_server);

	g_free_not_null(irc_conn->usermode);
	irc_conn->usermode = g_strdup(irc_server->wanted_usermode);
}

static void sig_connected(SERVER_REC *server, const char *u0, const char *u1, const char *u2)
{
	IRC_SERVER_REC *irc_server;
	if ((irc_server = IRC_SERVER(server)) == NULL || !irc_server->connrec->reconnection)
		return;

	if (irc_server->connrec->away_reason != NULL)
		irc_server_send_away(irc_server, irc_server->connrec->away_reason);
}

static void event_nick_collision(SERVER_REC *server, const char *data, const char *u0, const char *u1)
{
	IRC_SERVER_REC *irc_server;

	time_t new_connect;

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	/* after server kills us because of nick collision, we want to
	   connect back immediately. but no matter how hard they kill us,
	   don't connect to the server more than once in every 10 seconds. */

	new_connect = irc_server->connect_time+10 -
		settings_get_time("server_reconnect_time")/1000;
	if (irc_server->connect_time > new_connect)
		irc_server->connect_time = new_connect;

        irc_server->nick_collision = TRUE;
}

static void event_kill(SERVER_REC *server, const char *data,
		       const char *nick, const char *addr)
{
	IRC_SERVER_REC *irc_server;

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	if (addr != NULL && !irc_server->nick_collision) {
		/* don't reconnect if we were killed by an oper (not server) */
		irc_server->no_reconnect = TRUE;
	}
}

void irc_servers_reconnect_init(void)
{
	signal_add__server_connect_copy(sig_server_connect_copy);
	signal_add__server_reconnect_save_status(sig_server_reconnect_save_status);
	signal_add__event_("connected", sig_connected);
	signal_add__event_("436", event_nick_collision);
	signal_add__event_("kill", event_kill);
}

void irc_servers_reconnect_deinit(void)
{
	signal_remove__server_connect_copy(sig_server_connect_copy);
	signal_remove__server_reconnect_save_status(sig_server_reconnect_save_status);
	signal_remove__event_("connected", sig_connected);
	signal_remove__event_("436", event_nick_collision);
	signal_remove__event_("kill", event_kill);
}
