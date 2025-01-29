#ifndef BUS_API_H
#define BUS_API_H

#include "departure_info.h"

// Returns NULL if failed to get departure info
departure_info_t* get_bus_departures(void);

#endif  // BUS_API_H