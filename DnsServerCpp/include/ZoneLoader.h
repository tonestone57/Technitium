#ifndef ZONE_LOADER_H
#define ZONE_LOADER_H

#include "ZoneManager.h"
#include <string>

class ZoneLoader {
public:
    static bool load(ZoneManager& zoneManager, const std::string& filename);
};

#endif // ZONE_LOADER_H
