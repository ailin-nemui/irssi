/*
 channel-events.c : irssi

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
#include "signals.h"
#include "signal-registry.h"
#include "misc.h"
#include "channel-events.h"
#include "channels-setup.h"
#include "settings.h"
#include "recode.h"

#include "irc-servers.h"
#include "irc-channels.h"

static void check_join_failure(IRC_SERVER_REC *server, const char *channel)
{
	CHANNEL_REC *chanrec;
	char *chan2;

	if (channel[0] == '!' && channel[1] == '!')
		channel++; /* server didn't understand !channels */

	chanrec = channel_find(SERVER(server), channel);
	if (chanrec == NULL && channel[0] == '!' && strlen(channel) > 6) {
		/* it probably replied with the full !channel name,
		   find the channel with the short name.. */
		chan2 = g_strdup_printf("!%s", channel+6);
		chanrec = channel_find(SERVER(server), chan2);
		g_free(chan2);
	}

	if (chanrec != NULL && !chanrec->joined) {
		chanrec->left = TRUE;
		channel_destroy(chanrec);
	}
}

static void irc_server_event(SERVER_REC *server, const char *line, const char *u0, const char *u1)
{
	IRC_SERVER_REC *irc_server;
	char *params, *numeric, *channel;

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	/* We'll be checking "4xx <your nick> <channel>" for channels
	   which we haven't joined yet. 4xx are error codes and should
	   indicate that the join failed. */
	params = event_get_params(line, 3, &numeric, NULL, &channel);

	if (numeric[0] == '4')
		check_join_failure(irc_server, channel);

	g_free(params);
}

static void event_no_such_channel(SERVER_REC *server, const char *data, const char *u0, const char *u1)
{
	IRC_SERVER_REC *irc_server;
	CHANNEL_REC *chanrec;
	CHANNEL_SETUP_REC *setup;
	char *params, *channel;

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	params = event_get_params(data, 2, NULL, &channel);
	chanrec = *channel == '!' && channel[1] != '\0' ?
		channel_find(SERVER(server), channel) : NULL;

	if (chanrec != NULL) {
                /* !channel didn't exist, so join failed */
		setup = channel_setup_find(chanrec->name,
					   chanrec->server->connrec->chatnet);
		if (setup != NULL && setup->autojoin) {
			/* it's autojoin channel though, so create it */
			irc_send_cmdv(irc_server, "JOIN !%s", chanrec->name);
			g_free(params);
                        return;
		}
	}

	check_join_failure(irc_server, channel);
	g_free(params);
}

static void event_duplicate_channel(SERVER_REC *server, const char *data,  const char *u0, const char *u1)
{
	IRC_SERVER_REC *irc_server;
	CHANNEL_REC *chanrec;
	char *params, *channel, *p;

	g_return_if_fail(data != NULL);

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	/* this new addition to ircd breaks completely with older
	   "standards", "nick Duplicate ::!!channel ...." */
	params = event_get_params(data, 3, NULL, NULL, &channel);
	p = strchr(channel, ' ');
	if (p != NULL) *p = '\0';

	if (channel[0] == '!') {
		chanrec = channel_find(SERVER(server),
				       channel+(channel[1] == '!'));
		if (chanrec != NULL && !chanrec->names_got) {
			chanrec->left = TRUE;
			channel_destroy(chanrec);
		}
	}

	g_free(params);
}

static void channel_change_topic(IRC_SERVER_REC *server, const char *channel,
				 const char *topic, const char *setby,
				 time_t settime)
{
	CHANNEL_REC *chanrec;
	char *recoded = NULL;

	chanrec = channel_find(SERVER(server), channel);
	if (chanrec == NULL) return;
	/* the topic may be send out encoded, so we need to
	   recode it back or /topic <tab> will not work properly */
	recoded = recode_in(SERVER(server), topic, channel);
	if (topic != NULL) {
		g_free_not_null(chanrec->topic);
		chanrec->topic = recoded == NULL ? NULL : g_strdup(recoded);
	}
	g_free(recoded);

	g_free_not_null(chanrec->topic_by);
	chanrec->topic_by = g_strdup(setby);

	if (chanrec->topic_by == NULL) {
		/* ensure invariant topic_time > 0 <=> topic_by != NULL.
		   this could be triggered by a topic command without sender */
		chanrec->topic_time = 0;
	} else {
		chanrec->topic_time = settime;
	}

	SIGNAL_EMIT(channel_topic_changed, chanrec);
}

static void event_topic_get(SERVER_REC *server, const char *data, const char *u0, const char *u1)
{
	IRC_SERVER_REC *irc_server;
	char *params, *channel, *topic;

	g_return_if_fail(data != NULL);

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	params = event_get_params(data, 3, NULL, &channel, &topic);
	channel_change_topic(irc_server, channel, topic, NULL, 0);
	g_free(params);
}

static void event_topic(SERVER_REC *server, const char *data,
			const char *nick, const char *addr)
{
	IRC_SERVER_REC *irc_server;
	char *params, *channel, *topic, *mask;

	g_return_if_fail(data != NULL);

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	params = event_get_params(data, 2, &channel, &topic);
	mask = addr == NULL ? g_strdup(nick) :
		g_strconcat(nick, "!", addr, NULL);
	channel_change_topic(irc_server, channel, topic, mask, time(NULL));
	g_free(mask);
	g_free(params);
}

static void event_topic_info(SERVER_REC *server, const char *data, const char *u0, const char *u1)
{
	IRC_SERVER_REC *irc_server;
	char *params, *channel, *topicby, *topictime;
	time_t t;

	g_return_if_fail(data != NULL);

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	params = event_get_params(data, 4, NULL, &channel,
				  &topicby, &topictime);

	t = (time_t) atol(topictime);
	channel_change_topic(irc_server, channel, NULL, topicby, t);
	g_free(params);
}

/* Find any unjoined channel that matches `channel'. Long channel names are
   also a bit problematic, so find a channel where start of the name matches. */
static IRC_CHANNEL_REC *channel_find_unjoined(IRC_SERVER_REC *server,
					      const char *channel)
{
	GSList *tmp;
	int len;

	len = strlen(channel);
	for (tmp = server->channels; tmp != NULL; tmp = tmp->next) {
		IRC_CHANNEL_REC *rec = tmp->data;

		if (!IS_IRC_CHANNEL(rec) || rec->joined)
			continue;

		if (g_ascii_strncasecmp(channel, rec->name, len) == 0 &&
		    (len > 20 || rec->name[len] == '\0'))
			return rec;
	}

	return NULL;
}

static void event_join(SERVER_REC *server, const char *data, const char *nick, const char *address)
{
	IRC_SERVER_REC *irc_server;
	char *params, *channel, *tmp, *shortchan;
	IRC_CHANNEL_REC *chanrec;

	g_return_if_fail(data != NULL);

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	if (g_ascii_strcasecmp(nick, irc_server->nick) != 0) {
		/* someone else joined channel, no need to do anything */
		return;
	}

	if (irc_server->userhost == NULL)
		irc_server->userhost = g_strdup(address);

	params = event_get_params(data, 1, &channel);
	tmp = strchr(channel, 7); /* ^G does something weird.. */
	if (tmp != NULL) *tmp = '\0';

	if (*channel != '!' || strlen(channel) < 7)
		shortchan = NULL;
	else {
		/* !channels have 5 chars long identification string before
		   it's name, it's not known when /join is called so rename
		   !channel here to !ABCDEchannel */
		shortchan = g_strdup_printf("!%s", channel+6);
		chanrec = channel_find_unjoined(irc_server, shortchan);
		if (chanrec != NULL) {
			channel_change_name(CHANNEL(chanrec), channel);
			g_free(chanrec->name);
			chanrec->name = g_strdup(channel);
		} else {
			/* well, did we join it with full name? if so, and if
			   this was the first short one, change it's name. */
			chanrec = channel_find_unjoined(irc_server, channel);
			if (chanrec != NULL &&
			    irc_channel_find(irc_server, shortchan) == NULL) {
				channel_change_visible_name(CHANNEL(chanrec),
							    shortchan);
			}
		}
	}

	chanrec = irc_channel_find(irc_server, channel);
	if (chanrec != NULL && chanrec->joined) {
		/* already joined this channel - probably a broken proxy that
		   forgot to send PART between */
		chanrec->left = TRUE;
		channel_destroy(CHANNEL(chanrec));
		chanrec = NULL;
	}

	if (chanrec == NULL) {
		/* look again, because of the channel name cut issues. */
		chanrec = channel_find_unjoined(irc_server, channel);
	}

	if (chanrec == NULL) {
		/* didn't get here with /join command.. */
		chanrec = irc_channel_create(irc_server, channel, shortchan, TRUE);
	}

	chanrec->joined = TRUE;
	if (g_strcmp0(chanrec->name, channel) != 0) {
                g_free(chanrec->name);
		chanrec->name = g_strdup(channel);
	}

	g_free(shortchan);
	g_free(params);
}

static void event_part(SERVER_REC *server, const char *data, const char *nick, const char *u0)
{
	IRC_SERVER_REC *irc_server;
	char *params, *channel, *reason;
	CHANNEL_REC *chanrec;

	g_return_if_fail(data != NULL);

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	if (g_ascii_strcasecmp(nick, irc_server->nick) != 0) {
		/* someone else part, no need to do anything here */
		return;
	}

	params = event_get_params(data, 2, &channel, &reason);

	chanrec = channel_find(SERVER(server), channel);
	if (chanrec != NULL && chanrec->joined) {
		chanrec->left = TRUE;
		channel_destroy(chanrec);
	}

	g_free(params);
}

static void event_kick(SERVER_REC *server, const char *data, const char *u0, const char *u1)
{
	IRC_SERVER_REC *irc_server;
	CHANNEL_REC *chanrec;
	char *params, *channel, *nick, *reason;

	g_return_if_fail(data != NULL);

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	params = event_get_params(data, 3, &channel, &nick, &reason);

	if (g_ascii_strcasecmp(nick, irc_server->nick) != 0) {
		/* someone else was kicked, no need to do anything */
		g_free(params);
		return;
	}

	chanrec = channel_find(SERVER(server), channel);
	if (chanrec != NULL) {
		irc_server_purge_output(irc_server, channel);
		chanrec->kicked = TRUE;
		channel_destroy(chanrec);
	}

	g_free(params);
}

static void event_invite(SERVER_REC *server, const char *data, const char *u0, const char *u1)
{
	IRC_SERVER_REC *irc_server;
	char *params, *channel, *shortchan;

	g_return_if_fail(data != NULL);

	if ((irc_server = IRC_SERVER(server)) == NULL)
		return;

	params = event_get_params(data, 2, NULL, &channel);

	if (irc_channel_find(irc_server, channel) == NULL) {
                /* check if we're supposed to autojoin this channel */
		CHANNEL_SETUP_REC *setup;

		setup = channel_setup_find(channel, irc_server->connrec->chatnet);
		if (setup == NULL && channel[0] == '!' &&
		    strlen(channel) > 6) {
			shortchan = g_strdup_printf("!%s", channel+6);
			setup = channel_setup_find(shortchan,
						   irc_server->connrec->chatnet);
			g_free(shortchan);
		}
		if (setup != NULL && setup->autojoin && settings_get_bool("join_auto_chans_on_invite"))
			irc_server->channels_join(SERVER(server), channel, TRUE);
	}

	g_free_not_null(irc_server->last_invite);
	irc_server->last_invite = g_strdup(channel);
	g_free(params);
}

void channel_events_init(void)
{
	settings_add_bool("misc", "join_auto_chans_on_invite", TRUE);

	signal_add_last__server_event(irc_server_event);
	signal_add_first__event_("403", event_no_such_channel); /* no such channel */
	signal_add_first__event_("407", event_duplicate_channel); /* duplicate channel */

	signal_add__event_("topic", event_topic);
	signal_add_first__event_("join", event_join);
	signal_add__event_("part", event_part);
	signal_add__event_("kick", event_kick);
	signal_add__event_("invite", event_invite);
	signal_add__event_("332", event_topic_get);
	signal_add__event_("333", event_topic_info);
}

void channel_events_deinit(void)
{
	signal_remove__server_event(irc_server_event);
	signal_remove__event_("403", event_no_such_channel); /* no such channel */
	signal_remove__event_("407", event_duplicate_channel); /* duplicate channel */

	signal_remove__event_("topic", event_topic);
	signal_remove__event_("join", event_join);
	signal_remove__event_("part", event_part);
	signal_remove__event_("kick", event_kick);
	signal_remove__event_("invite", event_invite);
	signal_remove__event_("332", event_topic_get);
	signal_remove__event_("333", event_topic_info);
}
