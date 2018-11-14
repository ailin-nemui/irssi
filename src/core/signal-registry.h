#ifndef IRSSI_CORE_SIGNAL_REGISTRY_H
#define IRSSI_CORE_SIGNAL_REGISTRY_H

#include "modules.h"
#include "signal-registry-macros.h"

STYPE(COMMAND_REC);
STYPE(IGNORE_REC);
STYPE(RAWLOG_REC);
STYPE(LOG_REC);
STYPE(RECONNECT_REC);
STYPE(TLS_REC);
STYPE(CONFIG_REC);
STYPE(SESSION_REC);
STYPE(CONFIG_NODE);
STYPE(CHAT_PROTOCOL_REC);
STYPE(CHANNEL_REC);
STYPE(QUERY_REC);
STYPE(CHANNEL_SETUP_REC);
STYPE(CHATNET_REC);
STYPE(WI_ITEM_REC);
STYPE(SERVER_REC);
STYPE(SERVER_SETUP_REC);
STYPE(SERVER_CONNECT_REC);
STYPE(NICK_REC);
typedef void *int_in_ptr;
typedef void *uint_in_ptr;

#include "signal-registry.def"

#endif
