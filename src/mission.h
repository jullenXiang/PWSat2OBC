#ifndef SRC_MISSION_H_
#define SRC_MISSION_H_

#pragma once

#include "mission/antenna_task.hpp"
#include "mission/main.hpp"
#include "mission/sail.hpp"
#include "mission/time.hpp"
#include "state/struct.h"

namespace mission
{
    typedef MissionLoop<SystemState, TimeTask, antenna::AntennaTask, SailTask> ObcMission;
}

/** @} */

#endif /* SRC_MISSION_H_ */
