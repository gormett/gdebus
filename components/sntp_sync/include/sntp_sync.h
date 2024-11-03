#ifndef SNTP_SYNC_H
#define SNTP_SYNC_H

/**
 * @brief Initialize SNTP with a specific timezone
 *
 * @param timezone Timezone string (e.g., "CET-1CEST,M3.5.0,M10.5.0/3")
 */
void initialize_sntp(const char* timezone);

void func(void);

#endif  // SNTP_SYNC_H
