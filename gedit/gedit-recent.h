

#ifndef __GEDIT_RECENT_H__
#define __GEDIT_RECENT_H__

#include "recent-files/egg-recent-model.h"


EggRecentModel * gedit_recent_get_model (void);
void               gedit_recent_init (void);

#endif
