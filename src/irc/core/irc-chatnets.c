/*
 irc-chatnets.c : irssi

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
#include "core/signal-registry.h"
#include "lib-config/iconfig.h"
#include "settings.h"

#include "irc-chatnets.h"

void ircnet_create(IRC_CHATNET_REC *rec)
{
	g_return_if_fail(rec != NULL);

	rec->chat_type = IRC_PROTOCOL;
        chatnet_create((CHATNET_REC *) rec);
}

static void sig_chatnet_read(CHATNET_REC *rec, CONFIG_NODE *node)
{
	IRC_CHATNET_REC *irc_rec;
	char *value;

	if ((irc_rec = IRC_CHATNET(rec)) == NULL)
		return;

	value = config_node_get_str(node, "usermode", NULL);
	irc_rec->usermode = (value != NULL && *value != '\0') ? g_strdup(value) : NULL;

	value = config_node_get_str(node, "alternate_nick", NULL);
	irc_rec->alternate_nick = (value != NULL && *value != '\0') ? g_strdup(value) : NULL;

	irc_rec->max_cmds_at_once = config_node_get_int(node, "cmdmax", 0);
	irc_rec->cmd_queue_speed = config_node_get_int(node, "cmdspeed", 0);
	irc_rec->max_query_chans = config_node_get_int(node, "max_query_chans", 0);

	irc_rec->max_kicks = config_node_get_int(node, "max_kicks", 0);
	irc_rec->max_msgs = config_node_get_int(node, "max_msgs", 0);
	irc_rec->max_modes = config_node_get_int(node, "max_modes", 0);
	irc_rec->max_whois = config_node_get_int(node, "max_whois", 0);

	irc_rec->sasl_mechanism = g_strdup(config_node_get_str(node, "sasl_mechanism", NULL));
	irc_rec->sasl_username = g_strdup(config_node_get_str(node, "sasl_username", NULL));
	irc_rec->sasl_password = g_strdup(config_node_get_str(node, "sasl_password", NULL));
}

static void sig_chatnet_saved(CHATNET_REC *rec, CONFIG_NODE *node)
{
	IRC_CHATNET_REC *irc_rec;
	if ((irc_rec = IRC_CHATNET(rec)) == NULL)
		return;

	if (irc_rec->usermode != NULL)
		iconfig_node_set_str(node, "usermode", irc_rec->usermode);

	if (irc_rec->alternate_nick != NULL)
		iconfig_node_set_str(node, "alternate_nick", irc_rec->alternate_nick);

	if (irc_rec->max_cmds_at_once > 0)
		iconfig_node_set_int(node, "cmdmax", irc_rec->max_cmds_at_once);
	if (irc_rec->cmd_queue_speed > 0)
		iconfig_node_set_int(node, "cmdspeed", irc_rec->cmd_queue_speed);
	if (irc_rec->max_query_chans > 0)
		iconfig_node_set_int(node, "max_query_chans", irc_rec->max_query_chans);

	if (irc_rec->max_kicks > 0)
		iconfig_node_set_int(node, "max_kicks", irc_rec->max_kicks);
	if (irc_rec->max_msgs > 0)
		iconfig_node_set_int(node, "max_msgs", irc_rec->max_msgs);
	if (irc_rec->max_modes > 0)
		iconfig_node_set_int(node, "max_modes", irc_rec->max_modes);
	if (irc_rec->max_whois > 0)
		iconfig_node_set_int(node, "max_whois", irc_rec->max_whois);

	if (irc_rec->sasl_mechanism != NULL)
		iconfig_node_set_str(node, "sasl_mechanism", irc_rec->sasl_mechanism);
	if (irc_rec->sasl_username != NULL)
		iconfig_node_set_str(node, "sasl_username", irc_rec->sasl_username);
	if (irc_rec->sasl_password != NULL)
		iconfig_node_set_str(node, "sasl_password", irc_rec->sasl_password);
}

static void sig_chatnet_destroyed(CHATNET_REC *rec)
{
	IRC_CHATNET_REC *irc_rec;
	if ((irc_rec = IRC_CHATNET(rec)) == NULL)
		return;

	g_free(irc_rec->usermode);
	g_free(irc_rec->alternate_nick);
	g_free(irc_rec->sasl_mechanism);
	g_free(irc_rec->sasl_username);
	g_free(irc_rec->sasl_password);
}

void irc_chatnets_init(void)
{
	signal_add__chatnet_read(sig_chatnet_read);
	signal_add__chatnet_saved(sig_chatnet_saved);
	signal_add__chatnet_destroyed(sig_chatnet_destroyed);
}

void irc_chatnets_deinit(void)
{
	GSList *tmp, *next;

	for (tmp = chatnets; tmp != NULL; tmp = next) {
		CHATNET_REC *rec = tmp->data;

		next = tmp->next;
		if (IS_IRC_CHATNET(rec))
                        chatnet_destroy(rec);
	}

	signal_remove__chatnet_read(sig_chatnet_read);
	signal_remove__chatnet_saved(sig_chatnet_saved);
	signal_remove__chatnet_destroyed(sig_chatnet_destroyed);
}
