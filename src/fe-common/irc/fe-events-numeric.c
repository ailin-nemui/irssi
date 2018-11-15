/*
 fe-events-numeric.c : irssi

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
#include "module-formats.h"
#include "signals.h"
#include "misc.h"
#include "settings.h"
#include "levels.h"
#include "recode.h"

#include "irc-servers.h"
#include "irc-channels.h"
#include "nicklist.h"
#include "mode-lists.h"

#include "../core/module-formats.h"
#include "printtext.h"
#include "fe-channels.h"
#include "fe-irc-server.h"

static void print_event_received(IRC_SERVER_REC *server, const char *data,
				 const char *nick, int target_param);

static char *last_away_nick = NULL;
static char *last_away_msg = NULL;

static void event_user_mode(IRC_SERVER_REC *server, const char *data)
{
	char *params, *mode;

	g_return_if_fail(data != NULL);
	g_return_if_fail(server != NULL);

	params = event_get_params(data, 2, NULL, &mode);
        printformat(server, NULL, MSGLEVEL_CRAP, IRCTXT_USER_MODE,
                    g_strchomp(mode));
	g_free(params);
}

static void event_ison(IRC_SERVER_REC *server, const char *data)
{
	char *params, *online;

	g_return_if_fail(data != NULL);
	g_return_if_fail(server != NULL);

	params = event_get_params(data, 2, NULL, &online);
	printformat(server, NULL, MSGLEVEL_CRAP, IRCTXT_ONLINE, online);
	g_free(params);
}

static void event_names_list(IRC_SERVER_REC *server, const char *data)
{
	IRC_CHANNEL_REC *chanrec;
	char *params, *channel, *names;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 4, NULL, NULL, &channel, &names);

	chanrec = irc_channel_find(server, channel);
	if (chanrec == NULL || chanrec->names_got) {
		printformat_module("fe-common/core", server, channel,
				   MSGLEVEL_CRAP, TXT_NAMES,
				   channel, 0, 0, 0, 0, 0);
                printtext(server, channel, MSGLEVEL_CRAP, "%s", names);

	}
	g_free(params);
}

static void event_end_of_names(IRC_SERVER_REC *server, const char *data,
			       const char *nick)
{
	IRC_CHANNEL_REC *chanrec;
	char *params, *channel;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &channel);

	chanrec = irc_channel_find(server, channel);
	if (chanrec == NULL || chanrec->names_got)
		print_event_received(server, data, nick, FALSE);
	g_free(params);
}

static void event_who(IRC_SERVER_REC *server, const char *data)
{
	char *params, *nick, *channel, *user, *host, *stat, *realname, *hops;
	char *serv, *recoded;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 8, NULL, &channel, &user,
				  &host, &serv, &nick, &stat, &realname);

	/* split hops/realname */
	hops = realname;
	while (*realname != '\0' && *realname != ' ') realname++;
	if (*realname == ' ')
		*realname++ = '\0';

	recoded = recode_in(SERVER(server), realname, nick);
	printformat(server, NULL, MSGLEVEL_CRAP, IRCTXT_WHO,
		    channel, nick, stat, hops, user, host, recoded, serv);

	g_free(params);
	g_free(recoded);
}

static void event_end_of_who(IRC_SERVER_REC *server, const char *data)
{
	char *params, *channel;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &channel);
	printformat(server, NULL, MSGLEVEL_CRAP, IRCTXT_END_OF_WHO, channel);
	g_free(params);
}

static void event_ban_list(IRC_SERVER_REC *server, const char *data)
{
	IRC_CHANNEL_REC *chanrec;
	BAN_REC *banrec;
	const char *channel;
	char *params, *ban, *setby, *tims;
	long secs;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 5, NULL, &channel,
				  &ban, &setby, &tims);
	secs = *tims == '\0' ? 0 :
		(long) (time(NULL) - atol(tims));

	chanrec = irc_channel_find(server, channel);
	banrec = chanrec == NULL ? NULL : banlist_find(chanrec->banlist, ban);

	channel = get_visible_target(server, channel);
	printformat(server, channel, MSGLEVEL_CRAP,
		    *setby == '\0' ? IRCTXT_BANLIST : IRCTXT_BANLIST_LONG,
		    banrec == NULL ? 0 : g_slist_index(chanrec->banlist, banrec)+1,
		    channel, ban, setby, secs);

	g_free(params);
}

static void event_eban_list(IRC_SERVER_REC *server, const char *data)
{
	const char *channel;
	char *params, *ban, *setby, *tims;
	long secs;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 5, NULL, &channel,
				  &ban, &setby, &tims);
	secs = *tims == '\0' ? 0 :
		(long) (time(NULL) - atol(tims));

	channel = get_visible_target(server, channel);
	printformat(server, channel, MSGLEVEL_CRAP,
		    *setby == '\0' ? IRCTXT_EBANLIST : IRCTXT_EBANLIST_LONG,
		    channel, ban, setby, secs);

	g_free(params);
}

static void event_silence_list(IRC_SERVER_REC *server, const char *data)
{
	char *params, *nick, *mask;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 3, NULL, &nick, &mask);
	printformat(server, NULL, MSGLEVEL_CRAP,
		    IRCTXT_SILENCE_LINE, nick, mask);
	g_free(params);
}

static void event_accept_list(IRC_SERVER_REC *server, const char *data)
{
	char *params, *accepted;

	g_return_if_fail(data != NULL);
	g_return_if_fail(server != NULL);

	params = event_get_params(data, 2 | PARAM_FLAG_GETREST,
			NULL, &accepted);
	printformat(server, NULL, MSGLEVEL_CRAP, IRCTXT_ACCEPT_LIST, accepted);
	g_free(params);
}

static void event_invite_list(IRC_SERVER_REC *server, const char *data)
{
	const char *channel;
	char *params, *invite, *setby, *tims;
	long secs;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 5, NULL, &channel, &invite,
			&setby, &tims);
	secs = *tims == '\0' ? 0 :
		(long) (time(NULL) - atol(tims));

	channel = get_visible_target(server, channel);
	printformat(server, channel, MSGLEVEL_CRAP,
		    *setby == '\0' ? IRCTXT_INVITELIST : IRCTXT_INVITELIST_LONG,
		    channel, invite, setby, secs);
	g_free(params);
}

static void event_nick_in_use(IRC_SERVER_REC *server, const char *data)
{
	char *params, *nick;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &nick);
	if (server->connected) {
		printformat(server, NULL, MSGLEVEL_CRAP,
			    IRCTXT_NICK_IN_USE, nick);
	}

	g_free(params);
}

static void event_topic_get(IRC_SERVER_REC *server, const char *data)
{
	const char *channel;
	char *params, *topic, *recoded;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 3, NULL, &channel, &topic);
	recoded = recode_in(SERVER(server), topic, channel);
	channel = get_visible_target(server, channel);
	printformat(server, channel, MSGLEVEL_CRAP,
		    IRCTXT_TOPIC, channel, recoded);
	g_free(params);
	g_free(recoded);
}

static void event_topic_info(IRC_SERVER_REC *server, const char *data)
{
	const char *channel;
	char *params, *timestr, *bynick, *byhost, *topictime;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 4, NULL, &channel,
				  &bynick, &topictime);

        timestr = my_asctime((time_t) atol(topictime));

	byhost = strchr(bynick, '!');
	if (byhost != NULL)
		*byhost++ = '\0';

	channel = get_visible_target(server, channel);
	printformat(server, channel, MSGLEVEL_CRAP, IRCTXT_TOPIC_INFO,
		    bynick, timestr, byhost == NULL ? "" : byhost);
	g_free(timestr);
	g_free(params);
}

static void event_channel_mode(IRC_SERVER_REC *server, const char *data)
{
	const char *channel;
	char *params, *mode;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 3 | PARAM_FLAG_GETREST,
				  NULL, &channel, &mode);
	channel = get_visible_target(server, channel);
	printformat(server, channel, MSGLEVEL_CRAP,
		    IRCTXT_CHANNEL_MODE, channel, g_strchomp(mode));
	g_free(params);
}

static void event_channel_created(IRC_SERVER_REC *server, const char *data)
{
	const char *channel;
	char *params, *createtime, *timestr;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 3, NULL, &channel, &createtime);

        timestr = my_asctime((time_t) atol(createtime));
	channel = get_visible_target(server, channel);
	printformat(server, channel, MSGLEVEL_CRAP,
		    IRCTXT_CHANNEL_CREATED, channel, timestr);
	g_free(timestr);
	g_free(params);
}

static void event_nowaway(IRC_SERVER_REC *server, const char *data)
{
	printformat(server, NULL, MSGLEVEL_CRAP, IRCTXT_AWAY);
}

static void event_unaway(IRC_SERVER_REC *server, const char *data)
{
	printformat(server, NULL, MSGLEVEL_CRAP, IRCTXT_UNAWAY);
}

static void event_away(IRC_SERVER_REC *server, const char *data)
{
	char *params, *nick, *awaymsg, *recoded;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 3, NULL, &nick, &awaymsg);
	recoded = recode_in(SERVER(server), awaymsg, nick);
	if (!settings_get_bool("show_away_once") ||
	    last_away_nick == NULL ||
	    g_ascii_strcasecmp(last_away_nick, nick) != 0 ||
	    last_away_msg == NULL ||
	    g_ascii_strcasecmp(last_away_msg, awaymsg) != 0) {
		/* don't show the same away message
		   from the same nick all the time */
		g_free_not_null(last_away_nick);
		g_free_not_null(last_away_msg);
		last_away_nick = g_strdup(nick);
		last_away_msg = g_strdup(awaymsg);

		printformat(server, nick, MSGLEVEL_CRAP,
			    IRCTXT_NICK_AWAY, nick, recoded);
	}
	g_free(params);
	g_free(recoded);
}

static void event_userhost(IRC_SERVER_REC *server, const char *data)
{
	char *params, *hosts;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &hosts);
	printtext(server, NULL, MSGLEVEL_CRAP, "%s", hosts);
	g_free(params);
}

static void event_sent_invite(IRC_SERVER_REC *server, const char *data)
{
        char *params, *nick, *channel;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 3, NULL, &nick, &channel);
	printformat(server, nick, MSGLEVEL_CRAP,
		    IRCTXT_INVITING, nick, channel);
	g_free(params);
}

static void event_chanserv_url(IRC_SERVER_REC *server, const char *data)
{
	const char *channel;
	char *params, *url;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 3, NULL, &channel, &url);
	channel = get_visible_target(server, channel);
	printformat(server, channel, MSGLEVEL_CRAP,
		    IRCTXT_CHANNEL_URL, channel, url);
	g_free(params);
}

static void event_target_unavailable(IRC_SERVER_REC *server, const char *data,
				     const char *nick, const char *addr)
{
	IRC_CHANNEL_REC *chanrec;
	char *params, *target;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &target);
	if (!server_ischannel(SERVER(server), target)) {
		/* nick unavailable */
		printformat(server, NULL, MSGLEVEL_CRAP,
			    IRCTXT_NICK_UNAVAILABLE, target);
	} else {
		chanrec = irc_channel_find(server, target);
		if (chanrec != NULL && chanrec->joined) {
			/* dalnet - can't change nick while being banned */
			print_event_received(server, data, nick, FALSE);
		} else {
			/* channel is unavailable. */
			printformat(server, NULL, MSGLEVEL_CRAP,
				    IRCTXT_JOINERROR_UNAVAIL, target);
		}
	}

	g_free(params);
}

static void event_no_such_nick(IRC_SERVER_REC *server, const char *data,
				     const char *nick, const char *addr)
{
	char *params, *unick;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &unick);
	if (!g_strcmp0(unick, "*"))
		/* more information will be in the description,
		 * e.g. * :Target left IRC. Failed to deliver: [hi] */
		print_event_received(server, data, nick, FALSE);
	else
		printformat(server, unick, MSGLEVEL_CRAP, IRCTXT_NO_SUCH_NICK, unick);
	g_free(params);
}

static void event_no_such_channel(IRC_SERVER_REC *server, const char *data)
{
	char *params, *channel;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &channel);
	printformat(server, channel, MSGLEVEL_CRAP,
		    IRCTXT_NO_SUCH_CHANNEL, channel);
	g_free(params);
}

static void cannot_join(IRC_SERVER_REC *server, const char *data, int format)
{
	char *params, *channel;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &channel);
	printformat(server, NULL, MSGLEVEL_CRAP, format, channel);
	g_free(params);
}

static void event_too_many_channels(IRC_SERVER_REC *server, const char *data)
{
	cannot_join(server, data, IRCTXT_JOINERROR_TOOMANY);
}

static void event_duplicate_channel(IRC_SERVER_REC *server, const char *data,
		const char *nick)
{
	char *params, *channel, *p;

	g_return_if_fail(data != NULL);

	/* this new addition to ircd breaks completely with older
	   "standards", "nick Duplicate ::!!channel ...." */
	params = event_get_params(data, 3, NULL, NULL, &channel);
	p = strchr(channel, ' ');
	if (p != NULL) *p = '\0';

	if (channel[0] == '!' && channel[1] == '!') {
		printformat(server, NULL, MSGLEVEL_CRAP,
			    IRCTXT_JOINERROR_DUPLICATE, channel+1);
	} else
		print_event_received(server, data, nick, FALSE);

	g_free(params);
}

static void event_channel_is_full(IRC_SERVER_REC *server, const char *data)
{
	cannot_join(server, data, IRCTXT_JOINERROR_FULL);
}

static void event_invite_only(IRC_SERVER_REC *server, const char *data)
{
	cannot_join(server, data, IRCTXT_JOINERROR_INVITE);
}

static void event_banned(IRC_SERVER_REC *server, const char *data)
{
	cannot_join(server, data, IRCTXT_JOINERROR_BANNED);
}

static void event_bad_channel_key(IRC_SERVER_REC *server, const char *data)
{
	cannot_join(server, data, IRCTXT_JOINERROR_BAD_KEY);
}

static void event_bad_channel_mask(IRC_SERVER_REC *server, const char *data)
{
	cannot_join(server, data, IRCTXT_JOINERROR_BAD_MASK);
}

static void event_477(IRC_SERVER_REC *server, const char *data,
		      const char *nick)
{
	/* Numeric 477 can mean many things:
	 * modeless channel, cannot join/send to channel (+r/+R/+M).
	 * If we tried to join this channel, display the error in the
	 * status window. Otherwise display it in the channel window.
	 */
	IRC_CHANNEL_REC *chanrec;
	char *params, *channel;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &channel);

	chanrec = irc_channel_find(server, channel);
	print_event_received(server, data, nick, chanrec == NULL || chanrec->joined);
	g_free(params);
}

static void event_target_too_fast(IRC_SERVER_REC *server, const char *data,
		      const char *nick)
{
	/* Target change too fast, could be nick or channel.
	 * If we tried to join this channel, display the error in the
	 * status window. Otherwise display it in the channel window.
	 */
	IRC_CHANNEL_REC *chanrec;
	char *params, *channel;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &channel);

	chanrec = irc_channel_find(server, channel);
	print_event_received(server, data, nick, chanrec == NULL || chanrec->joined);
	g_free(params);
}

static void event_unknown_mode(IRC_SERVER_REC *server, const char *data)
{
	char *params, *mode;

	g_return_if_fail(data != NULL);

	params = event_get_params(data, 2, NULL, &mode);
	printformat(server, NULL, MSGLEVEL_CRAP, IRCTXT_UNKNOWN_MODE, mode);
	g_free(params);
}

static void event_numeric(IRC_SERVER_REC *server, const char *data,
			  const char *nick)
{
	data = strchr(data, ' ');
	if (data != NULL)
                print_event_received(server, data+1, nick, FALSE);
}

static void print_event_received(IRC_SERVER_REC *server, const char *data,
				 const char *nick, int target_param)
{
	char *target, *args, *ptr, *ptr2, *recoded;
	int format;

	g_return_if_fail(data != NULL);

        /* first param is our nick, "*" or a channel */
	ptr = strchr(data, ' ');
	if (ptr == NULL)
		return;
	ptr++;

	if (server_ischannel(SERVER(server), data)) /* directed at channel */
		target = g_strndup(data, (int)(ptr - data - 1));
	else if (!target_param || *ptr == ':' || (ptr2 = strchr(ptr, ' ')) == NULL)
		target = NULL;
	else {
                /* target parameter expected and present */
                target = g_strndup(ptr, (int) (ptr2-ptr));
	}

	/* param1 param2 ... :last parameter */
	if (*ptr == ':') {
                /* only one parameter */
		args = g_strdup(ptr+1);
	} else {
		args = g_strdup(ptr);
		ptr = strstr(args, " :");
		if (ptr != NULL)
			g_memmove(ptr+1, ptr+2, strlen(ptr+1));
	}

	recoded = recode_in(SERVER(server), args, NULL);
	format = nick == NULL || server->real_address == NULL ||
		g_strcmp0(nick, server->real_address) == 0 ?
		IRCTXT_DEFAULT_EVENT : IRCTXT_DEFAULT_EVENT_SERVER;
	printformat(server, target, MSGLEVEL_CRAP, format,
		    nick, recoded, current_server_event);

	g_free(recoded);
	g_free(args);
	g_free(target);
}

static void event_received(IRC_SERVER_REC *server, const char *data,
			   const char *nick)
{
        print_event_received(server, data, nick, FALSE);
}

static void event_target_received(IRC_SERVER_REC *server, const char *data,
				  const char *nick)
{
        print_event_received(server, data, nick, TRUE);
}

static void event_motd(IRC_SERVER_REC *server, const char *data,
		       const char *nick, const char *addr)
{
	/* don't ignore motd anymore after 3 seconds of connection time -
	   we might have called /MOTD */
	if (settings_get_bool("skip_motd") && !server->motd_got)
		return;

        print_event_received(server, data, nick, FALSE);
}

static void sig_empty(void)
{
}

void fe_events_numeric_init(void)
{
	last_away_nick = NULL;
	last_away_msg = NULL;

	signal_add__event_("221", event_user_mode);
	signal_add__event_("303", event_ison);
	signal_add__event_("353", event_names_list);
	signal_add_first__event_366(event_end_of_names);
	signal_add__event_("352", event_who);
	signal_add__event_("315", event_end_of_who);
	signal_add__event_("271", event_silence_list);
	signal_add__event_("272", sig_empty);
	signal_add__event_("281", event_accept_list);
	signal_add__event_("367", event_ban_list);
	signal_add__event_("348", event_eban_list);
	signal_add__event_("346", event_invite_list);
	signal_add__event_("433", event_nick_in_use);
	signal_add__event_("332", event_topic_get);
	signal_add__event_("333", event_topic_info);
	signal_add__event_("324", event_channel_mode);
	signal_add__event_("329", event_channel_created);
	signal_add__event_("306", event_nowaway);
	signal_add__event_("305", event_unaway);
	signal_add__event_("301", event_away);
	signal_add__event_("328", event_chanserv_url);
	signal_add__event_("302", event_userhost);
	signal_add__event_("341", event_sent_invite);

	signal_add__event_("437", event_target_unavailable);
	signal_add__event_("401", event_no_such_nick);
	signal_add__event_("403", event_no_such_channel);
	signal_add__event_("405", event_too_many_channels);
	signal_add__event_("407", event_duplicate_channel);
	signal_add__event_("471", event_channel_is_full);
	signal_add__event_("472", event_unknown_mode);
	signal_add__event_("473", event_invite_only);
	signal_add__event_("474", event_banned);
	signal_add__event_("475", event_bad_channel_key);
	signal_add__event_("476", event_bad_channel_mask);
	signal_add__event_("477", event_477);
	signal_add__event_("375", event_motd);
	signal_add__event_("376", event_motd);
	signal_add__event_("372", event_motd);
	signal_add__event_("422", event_motd);
	signal_add__event_("439", event_target_too_fast);
	signal_add__event_("707", event_target_too_fast);

        signal_add__default_event_numeric(event_numeric);
	/* Because default event numeric only fires if there is no specific
	 * event, add all numerics with a handler elsewhere in irssi that
	 * should not be printed specially here.
	 */
	signal_add__event_("001", event_received);
	signal_add__event_("004", event_received);
	signal_add__event_("005", event_received);
	signal_add__event_("254", event_received);
	signal_add__event_("364", event_received);
	signal_add__event_("365", event_received);
	signal_add__event_("381", event_received);
	signal_add__event_("396", event_received);
	signal_add__event_("421", event_received);
	signal_add__event_("432", event_received);
	signal_add__event_("436", event_received);
	signal_add__event_("438", event_received);
	signal_add__event_("465", event_received);
	signal_add__event_("470", event_received);
	signal_add__event_("479", event_received);

	signal_add__event_("344", event_target_received); /* reop list */
	signal_add__event_("345", event_target_received); /* end of reop list */
	signal_add__event_("347", event_target_received); /* end of invite exception list */
	signal_add__event_("349", event_target_received); /* end of ban exception list */
	signal_add__event_("368", event_target_received); /* end of ban list */
	signal_add__event_("386", event_target_received); /* owner list; old rsa challenge (harmless) */
	signal_add__event_("387", event_target_received); /* end of owner list */
	signal_add__event_("388", event_target_received); /* protect list */
	signal_add__event_("389", event_target_received); /* end of protect list */
	signal_add__event_("404", event_target_received); /* cannot send to channel */
	signal_add__event_("408", event_target_received); /* cannot send (+c) */
	signal_add__event_("442", event_target_received); /* you're not on that channel */
	signal_add__event_("478", event_target_received); /* ban list is full */
	signal_add__event_("482", event_target_received); /* not chanop */
	signal_add__event_("486", event_target_received); /* cannot /msg (+R) */
	signal_add__event_("489", event_target_received); /* not chanop or voice */
	signal_add__event_("494", event_target_received); /* cannot /msg (own +R) */
	signal_add__event_("506", event_target_received); /* cannot send (+R) */
	signal_add__event_("716", event_target_received); /* cannot /msg (+g) */
	signal_add__event_("717", event_target_received); /* +g notified */
	signal_add__event_("728", event_target_received); /* quiet (or other) list */
	signal_add__event_("729", event_target_received); /* end of quiet (or other) list */
}

void fe_events_numeric_deinit(void)
{
	g_free_not_null(last_away_nick);
	g_free_not_null(last_away_msg);

	signal_remove__event_("221", event_user_mode);
	signal_remove__event_("303", event_ison);
	signal_remove__event_("353", event_names_list);
	signal_remove__event_("366", event_end_of_names);
	signal_remove__event_("352", event_who);
	signal_remove__event_("315", event_end_of_who);
	signal_remove__event_("271", event_silence_list);
	signal_remove__event_("272", sig_empty);
	signal_remove__event_("281", event_accept_list);
	signal_remove__event_("367", event_ban_list);
	signal_remove__event_("348", event_eban_list);
	signal_remove__event_("346", event_invite_list);
	signal_remove__event_("433", event_nick_in_use);
	signal_remove__event_("332", event_topic_get);
	signal_remove__event_("333", event_topic_info);
	signal_remove__event_("324", event_channel_mode);
	signal_remove__event_("329", event_channel_created);
	signal_remove__event_("306", event_nowaway);
	signal_remove__event_("305", event_unaway);
	signal_remove__event_("301", event_away);
	signal_remove__event_("328", event_chanserv_url);
	signal_remove__event_("302", event_userhost);
	signal_remove__event_("341", event_sent_invite);

	signal_remove__event_("437", event_target_unavailable);
	signal_remove__event_("401", event_no_such_nick);
	signal_remove__event_("403", event_no_such_channel);
	signal_remove__event_("405", event_too_many_channels);
	signal_remove__event_("407", event_duplicate_channel);
	signal_remove__event_("471", event_channel_is_full);
	signal_remove__event_("472", event_unknown_mode);
	signal_remove__event_("473", event_invite_only);
	signal_remove__event_("474", event_banned);
	signal_remove__event_("475", event_bad_channel_key);
	signal_remove__event_("476", event_bad_channel_mask);
	signal_remove__event_("477", event_477);
	signal_remove__event_("375", event_motd);
	signal_remove__event_("376", event_motd);
	signal_remove__event_("372", event_motd);
	signal_remove__event_("422", event_motd);
	signal_remove__event_("439", event_target_too_fast);
	signal_remove__event_("707", event_target_too_fast);

        signal_remove__default_event_numeric(event_numeric);
	signal_remove__event_("001", event_received);
	signal_remove__event_("004", event_received);
	signal_remove__event_("005", event_received);
	signal_remove__event_("254", event_received);
	signal_remove__event_("364", event_received);
	signal_remove__event_("365", event_received);
	signal_remove__event_("381", event_received);
	signal_remove__event_("396", event_received);
	signal_remove__event_("421", event_received);
	signal_remove__event_("432", event_received);
	signal_remove__event_("436", event_received);
	signal_remove__event_("438", event_received);
	signal_remove__event_("465", event_received);
	signal_remove__event_("470", event_received);
	signal_remove__event_("479", event_received);

	signal_remove__event_("344", event_target_received);
	signal_remove__event_("345", event_target_received);
	signal_remove__event_("347", event_target_received);
	signal_remove__event_("349", event_target_received);
	signal_remove__event_("368", event_target_received);
	signal_remove__event_("386", event_target_received);
	signal_remove__event_("387", event_target_received);
	signal_remove__event_("388", event_target_received);
	signal_remove__event_("389", event_target_received);
	signal_remove__event_("404", event_target_received);
	signal_remove__event_("408", event_target_received);
	signal_remove__event_("442", event_target_received);
	signal_remove__event_("478", event_target_received);
	signal_remove__event_("482", event_target_received);
	signal_remove__event_("486", event_target_received);
	signal_remove__event_("489", event_target_received);
	signal_remove__event_("494", event_target_received);
	signal_remove__event_("506", event_target_received);
	signal_remove__event_("716", event_target_received);
	signal_remove__event_("717", event_target_received);
	signal_remove__event_("728", event_target_received);
	signal_remove__event_("729", event_target_received);
}
