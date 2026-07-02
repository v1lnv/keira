#include "include/rtc.h"
#include "../../arch/x86/include/asm/io.h"
#include "regs.h"

/* Register addresses and indices are included from regs.h */

/**
 * Read a single byte from a CMOS register.
 */
static uint8_t cmos_read(uint8_t reg) {
    /* Select the register (and disable NMI with bit 7 = 0) */
    outb(CMOS_ADDRESS, reg);
    io_wait();
    return inb(CMOS_DATA);
}

/**
 * Check if the RTC is currently updating its registers.
 * We must not read while an update is in progress.
 */
static int rtc_update_in_progress(void) {
    return cmos_read(RTC_REG_STATUS_A) & 0x80;
}

/**
 * Convert BCD (Binary-Coded Decimal) to binary.
 */
static uint8_t bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

void rtc_init(void) { /* Nothing special needed for basic RTC reading */
}

void rtc_get_time(rtc_time_t *time) {
    /* Wait until an update is NOT in progress */
    while (rtc_update_in_progress())
        ;

    uint8_t second = cmos_read(RTC_REG_SECONDS);
    uint8_t minute = cmos_read(RTC_REG_MINUTES);
    uint8_t hour = cmos_read(RTC_REG_HOURS);
    uint8_t day = cmos_read(RTC_REG_DAY);
    uint8_t month = cmos_read(RTC_REG_MONTH);
    uint8_t year = cmos_read(RTC_REG_YEAR);

    /* Read a second time to make sure values didn't change during read */
    while (rtc_update_in_progress())
        ;
    uint8_t second2 = cmos_read(RTC_REG_SECONDS);
    uint8_t minute2 = cmos_read(RTC_REG_MINUTES);
    uint8_t hour2 = cmos_read(RTC_REG_HOURS);
    uint8_t day2 = cmos_read(RTC_REG_DAY);
    uint8_t month2 = cmos_read(RTC_REG_MONTH);
    uint8_t year2 = cmos_read(RTC_REG_YEAR);

    /* If values differ, read again */
    if (second != second2 || minute != minute2 || hour != hour2 || day != day2 || month != month2 ||
        year != year2) {
        while (rtc_update_in_progress())
            ;
        second = cmos_read(RTC_REG_SECONDS);
        minute = cmos_read(RTC_REG_MINUTES);
        hour = cmos_read(RTC_REG_HOURS);
        day = cmos_read(RTC_REG_DAY);
        month = cmos_read(RTC_REG_MONTH);
        year = cmos_read(RTC_REG_YEAR);
    }

    /* Check Status Register B to see if values are in BCD or binary */
    uint8_t status_b = cmos_read(RTC_REG_STATUS_B);

    if (!(status_b & 0x04)) {
        /* Values are in BCD : convert to binary */
        second = bcd_to_bin(second);
        minute = bcd_to_bin(minute);
        hour = bcd_to_bin(hour & 0x7F) | (hour & 0x80); /* Preserve PM bit */
        day = bcd_to_bin(day);
        month = bcd_to_bin(month);
        year = bcd_to_bin(year);
    }

    /* Handle 12-hour mode */
    if (!(status_b & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }

    time->second = second;
    time->minute = minute;
    time->hour = hour;
    time->day = day;
    time->month = month;
    time->year = 2000 + year; /* Assume 21st century */
}
