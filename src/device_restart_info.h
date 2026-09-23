#ifndef DEVICE_RESTART_INFO_H
#define DEVICE_RESTART_INFO_H

#include <Arduino.h>
#include <WebServer.h>
#include "debug_serial.h"

/*
 * DeviceRestartInfo
 *
 * Captures the RTC date/time and ESP32 reset reason at boot,
 * and provides a web API:
 *
 *   GET /restartinfo
 *
 * Example JSON:
 * {
 *   "date":"2026-08-25",
 *   "time":"14:35:22",
 *   "reason":"Software Reset"
 * }
 */

class DeviceRestartInfo
{
public:
    // Call once from setup(), after initRTC().
    void begin();

    // Registers GET /restartinfo on the supplied WebServer.
    void attach(WebServer &server);

    // Optional getters.
    const String &getDate() const;
    const String &getTime() const;
    const String &getReason() const;
    // Webpage components
    String getCss() const;
    String getHtml() const;
    String getJavaScript() const;

private:
    String lastRestartDate;
    String lastRestartTime;
    String restartReason;

    String getResetReason() const;
    void buildRestartDateTime();
    void saveRestartToHistory();

    void handleRestartInfo(WebServer &server);
    void handleRestartHistory(WebServer &server);
};

// Global module object.
extern DeviceRestartInfo deviceRestartInfo;

#endif
