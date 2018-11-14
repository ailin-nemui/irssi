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
typedef void *int_in_ptr;
typedef void *uint_in_ptr;

#include "signal-registry.def"

#endif
