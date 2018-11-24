/*
 lag.c : irssi

    Copyright (C) 1999 Timo Sirainen

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
#include "signals.h"
#include "signal-registry.h"
#include "misc.h"
#include "settings.h"

#include "irc-servers.h"
#include "servers-redirect.h"

static int timeout_tag;

static void lag_get(IRC_SERVER_REC *server)
{
	g_get_current_time(&server->lag_sent);
	server->lag_last_check = time(NULL);

	server_redirect_event(server, "ping", 1, NULL, FALSE,
			      "lag ping error",
                              "event pong", "lag pong", NULL);
	irc_send_cmdv(server, "PING %s", server->real_address);
}

/* we didn't receive PONG for some reason .. try again */
static void lag_ping_error(IRC_SERVER_REC *server)
{
	lag_get(server);
}

static void lag_event_pong(IRC_SERVER_REC *server, const char *data,
			   const char *nick, const char *addr)
{
	GTimeVal now;

	g_return_if_fail(data != NULL);

	if (server->lag_sent.tv_sec == 0) {
		/* not expecting lag reply.. */
		return;
	}

	g_get_current_time(&now);
	server->lag = (int) get_timeval_diff(&now, &server->lag_sent);
	memset(&server->lag_sent, 0, sizeof(server->lag_sent));

	SIGNAL_EMIT(server_lag, (SERVER_REC *)server);
}

static void sig_unknown_command(IRC_SERVER_REC *server, const char *data)
{
	char *params, *cmd;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &cmd);
	if (g_ascii_strcasecmp(cmd, "PING") == 0) {
		/* some servers have disabled PING command, don't bother
		   trying alternative methods to detect lag with these
		   servers. */
		server->disable_lag = TRUE;
		server->lag_sent.tv_sec = 0;
                server->lag = 0;
	}
	g_free(params);
}

static int sig_check_lag(void)
{
	GSList *tmp, *next;
	time_t now;
	int lag_check_time, max_lag;

	lag_check_time = settings_get_time("lag_check_time")/1000;
	max_lag = settings_get_time("lag_max_before_disconnect")/1000;

	if (lag_check_time <= 0)
		return 1;

	now = time(NULL);
	for (tmp = servers; tmp != NULL; tmp = next) {
		IRC_SERVER_REC *rec = tmp->data;

		next = tmp->next;
		if (!IS_IRC_SERVER(rec) || rec->disable_lag)
			continue;

		if (rec->lag_sent.tv_sec != 0) {
			/* waiting for lag reply */
			if (max_lag > 1 && now-rec->lag_sent.tv_sec > max_lag) {
				/* too much lag, disconnect */
				SIGNAL_EMIT(server_lag_disconnect, (SERVER_REC *)rec);
				rec->connection_lost = TRUE;
				server_disconnect((SERVER_REC *) rec);
			}
		} else if (rec->lag_last_check+lag_check_time < now &&
			 rec->cmdcount == 0 && rec->connected) {
			/* no commands in buffer - get the lag */
			lag_get(rec);
		}
	}

	return 1;
}

void lag_init(void)
{
	settings_add_time("misc", "lag_check_time", "1min");
	settings_add_time("misc", "lag_max_before_disconnect", "5min");

	timeout_tag = g_timeout_add(1000, (GSourceFunc) sig_check_lag, NULL);
	signal_add_first__lag_pong(lag_event_pong);
        signal_add__lag_ping_error(lag_ping_error);
        signal_add__event_("421", sig_unknown_command);
}

void lag_deinit(void)
{
	g_source_remove(timeout_tag);
	signal_remove__lag_pong(lag_event_pong);
        signal_remove__lag_ping_error(lag_ping_error);
        signal_remove__event_("421", sig_unknown_command);
}
