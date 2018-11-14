#ifndef IRSSI_IRC_CORE_SIGNAL_REGISTRY_H
#define IRSSI_IRC_CORE_SIGNAL_REGISTRY_H

#include "modules.h"
#include "signal-registry-macros.h"

STYPE(IRC_SERVER_REC);
STYPE(BAN_REC);
STYPE(NETSPLIT_SERVER_REC);
STYPE(NETSPLIT_REC);
STYPE(DCC_REC);
STYPE(AUTOIGNORE_REC);
STYPE(NOTIFYLIST_REC);
STYPE(CLIENT_REC);
STYPE(REJOIN_REC);
typedef struct CHAT_DCC_REC CHAT_DCC_REC;
STYPE(GET_DCC_REC);
STYPE(SEND_DCC_REC);
typedef struct SERVER_DCC_REC SERVER_DCC_REC;
typedef void *int_in_ptr;
typedef void *uint_in_ptr;

#include "signal-registry.def"

#endif
