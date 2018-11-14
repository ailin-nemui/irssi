#ifndef IRSSI_FE_TEXT_SIGNAL_REGISTRY_H
#define IRSSI_FE_TEXT_SIGNAL_REGISTRY_H

#include "modules.h"
#include "signal-registry-macros.h"

STYPE(LINE_REC);
STYPE(TEXT_BUFFER_VIEW_REC);
STYPE(MAIN_WINDOW_REC);
STYPE(STATUSBAR_REC);
typedef struct SBAR_ITEM_REC SBAR_ITEM_REC;

#include "signal-registry.def"

#endif
