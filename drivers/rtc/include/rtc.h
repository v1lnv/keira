#ifndef RTC_H
#define RTC_H

#include <stdint.h>

/**
 * Keira Kernel: Real-Time Clock (CMOS RTC) Driver
 *
 * Reads date and time from the CMOS RTC chip via ports 0x70/0x71.
 */

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

/**
 * Initialize the RTC driver.
 */
void rtc_init(void);

/**
 * Read the current date and time from the CMOS RTC.
 *
 * @param time Pointer to rtc_time_t struct to fill.
 */
void rtc_get_time(rtc_time_t *time);

#endif /* RTC_H */
