#include "device_restart_info.h"

#include <Preferences.h>
#include <esp_system.h>
#include <rtc_s.h>

#define RESTART_NVS_NAMESPACE "restart_info"
#define DEVICE_RESTART_HISTORY_SIZE 10


//=============================================================================
// GLOBAL OBJECT
//=============================================================================

DeviceRestartInfo deviceRestartInfo;


//=============================================================================
// BEGIN
//=============================================================================

void DeviceRestartInfo::begin()
{
    // IMPORTANT:
    // initRTC() must be called before deviceRestartInfo.begin()
    // in main.cpp.

    buildRestartDateTime();

    restartReason = getResetReason();

    // Save this restart into NVS history.
    saveRestartToHistory();


    USBSerial.println();
    USBSerial.println("======================================");
    USBSerial.println("       DEVICE RESTART INFORMATION");
    USBSerial.println("======================================");
    USBSerial.println("Restart Date   : " + lastRestartDate);
    USBSerial.println("Restart Time   : " + lastRestartTime);
    USBSerial.println("Restart Reason : " + restartReason);
    USBSerial.println("======================================");
    USBSerial.println();
}


//=============================================================================
// BUILD DATE + TIME FROM EXISTING RTC
//=============================================================================

void DeviceRestartInfo::buildRestartDateTime()
{
    weather_station_global_structure_t rtcData;

    // Uses your existing RTC function.
    readRTCDateTimeToStruct(&rtcData);


    //-------------------------------------------------------------------------
    // DATE: YYYY-MM-DD
    //-------------------------------------------------------------------------

    lastRestartDate = "20";

    if (rtcData.rtc_year < 10)
        lastRestartDate += "0";

    lastRestartDate += String(rtcData.rtc_year);

    lastRestartDate += "-";

    if (rtcData.rtc_month < 10)
        lastRestartDate += "0";

    lastRestartDate += String(rtcData.rtc_month);

    lastRestartDate += "-";

    if (rtcData.rtc_date < 10)
        lastRestartDate += "0";

    lastRestartDate += String(rtcData.rtc_date);


    //-------------------------------------------------------------------------
    // TIME: HH:MM:SS
    //-------------------------------------------------------------------------

    lastRestartTime = "";

    if (rtcData.rtc_hour < 10)
        lastRestartTime += "0";

    lastRestartTime += String(rtcData.rtc_hour);

    lastRestartTime += ":";

    if (rtcData.rtc_min < 10)
        lastRestartTime += "0";

    lastRestartTime += String(rtcData.rtc_min);

    lastRestartTime += ":";

    if (rtcData.rtc_sec < 10)
        lastRestartTime += "0";

    lastRestartTime += String(rtcData.rtc_sec);
}


//=============================================================================
// ESP32 RESET / RESTART REASON
//=============================================================================

String DeviceRestartInfo::getResetReason() const
{
    esp_reset_reason_t reason = esp_reset_reason();

    switch (reason)
    {
        case ESP_RST_UNKNOWN:
            return "Unknown Reset";

        case ESP_RST_POWERON:
            return "Power On";

        case ESP_RST_EXT:
            return "External Reset";

        case ESP_RST_SW:
            return "Software Reset";

        case ESP_RST_PANIC:
            return "Crash / Panic";

        case ESP_RST_INT_WDT:
            return "Interrupt Watchdog";

        case ESP_RST_TASK_WDT:
            return "Task Watchdog";

        case ESP_RST_WDT:
            return "Watchdog Reset";

        case ESP_RST_DEEPSLEEP:
            return "Deep Sleep";

        case ESP_RST_BROWNOUT:
            return "Brownout / Low Voltage";

        case ESP_RST_SDIO:
            return "SDIO Reset";

        default:
            return "Other Reset";
    }
}


//=============================================================================
// SAVE RESTART HISTORY TO NVS
//
// index 0 = latest restart
// index 1 = previous restart
// ...
// index 9 = oldest stored restart
//=============================================================================

void DeviceRestartInfo::saveRestartToHistory()
{
    Preferences prefs;

    if (!prefs.begin(RESTART_NVS_NAMESPACE, false))
    {
        USBSerial.println(
            "[RESTART] ERROR: Cannot open NVS"
        );

        return;
    }


    uint8_t count = prefs.getUChar("count", 0);

    if (count > DEVICE_RESTART_HISTORY_SIZE)
        count = DEVICE_RESTART_HISTORY_SIZE;


    //-------------------------------------------------------------------------
    // Shift existing records
    //-------------------------------------------------------------------------

    for (int i = DEVICE_RESTART_HISTORY_SIZE - 1; i >= 1; i--)
    {
        String oldDateKey =
            "date" + String(i - 1);

        String oldTimeKey =
            "time" + String(i - 1);

        String oldReasonKey =
            "reason" + String(i - 1);


        String newDateKey =
            "date" + String(i);

        String newTimeKey =
            "time" + String(i);

        String newReasonKey =
            "reason" + String(i);


        String oldDate =
            prefs.getString(
                oldDateKey.c_str(),
                ""
            );

        String oldTime =
            prefs.getString(
                oldTimeKey.c_str(),
                ""
            );

        String oldReason =
            prefs.getString(
                oldReasonKey.c_str(),
                ""
            );


        if (oldDate.length() > 0)
        {
            prefs.putString(
                newDateKey.c_str(),
                oldDate
            );

            prefs.putString(
                newTimeKey.c_str(),
                oldTime
            );

            prefs.putString(
                newReasonKey.c_str(),
                oldReason
            );
        }
    }


    //-------------------------------------------------------------------------
    // Save newest restart at index 0
    //-------------------------------------------------------------------------

    prefs.putString(
        "date0",
        lastRestartDate
    );

    prefs.putString(
        "time0",
        lastRestartTime
    );

    prefs.putString(
        "reason0",
        restartReason
    );


    //-------------------------------------------------------------------------
    // Update count
    //-------------------------------------------------------------------------

    uint8_t newCount = count + 1;

    if (newCount > DEVICE_RESTART_HISTORY_SIZE)
        newCount = DEVICE_RESTART_HISTORY_SIZE;

    prefs.putUChar(
        "count",
        newCount
    );


    prefs.end();
}


//=============================================================================
// ATTACH WEB API
//=============================================================================

void DeviceRestartInfo::attach(WebServer &server)
{
    // Latest restart
    server.on(
        "/restartinfo",
        [this, &server]()
        {
            handleRestartInfo(server);
        }
    );


    // Restart history
    server.on(
        "/restarthistory",
        [this, &server]()
        {
            handleRestartHistory(server);
        }
    );
}


//=============================================================================
// /restartinfo
//
// Returns:
//
// {
//   "date":"2026-08-25",
//   "time":"14:35:22",
//   "reason":"Software Reset"
// }
//=============================================================================

void DeviceRestartInfo::handleRestartInfo(
    WebServer &server
)
{
    String json = "{";

    json += "\"date\":\"";
    json += lastRestartDate;
    json += "\",";

    json += "\"time\":\"";
    json += lastRestartTime;
    json += "\",";

    json += "\"reason\":\"";
    json += restartReason;
    json += "\"";

    json += "}";


    server.send(
        200,
        "application/json",
        json
    );
}


//=============================================================================
// /restarthistory
//
// Returns:
//
// {
//   "count":2,
//   "history":[
//      {
//        "date":"2026-08-25",
//        "time":"14:35:22",
//        "reason":"Software Reset"
//      },
//      ...
//   ]
// }
//=============================================================================

void DeviceRestartInfo::handleRestartHistory(
    WebServer &server
)
{
    Preferences prefs;

    if (!prefs.begin(
            RESTART_NVS_NAMESPACE,
            true
        ))
    {
        server.send(
            500,
            "application/json",
            "{\"error\":\"NVS open failed\"}"
        );

        return;
    }


    uint8_t count =
        prefs.getUChar(
            "count",
            0
        );


    if (count > DEVICE_RESTART_HISTORY_SIZE)
        count = DEVICE_RESTART_HISTORY_SIZE;


    String json = "{";

    json += "\"count\":";
    json += String(count);

    json += ",\"history\":[";


    for (uint8_t i = 0; i < count; i++)
    {
        String dateKey =
            "date" + String(i);

        String timeKey =
            "time" + String(i);

        String reasonKey =
            "reason" + String(i);


        String date =
            prefs.getString(
                dateKey.c_str(),
                ""
            );

        String time =
            prefs.getString(
                timeKey.c_str(),
                ""
            );

        String reason =
            prefs.getString(
                reasonKey.c_str(),
                ""
            );


        if (i > 0)
            json += ",";


        json += "{";

        json += "\"date\":\"";
        json += date;
        json += "\",";

        json += "\"time\":\"";
        json += time;
        json += "\",";

        json += "\"reason\":\"";
        json += reason;
        json += "\"";

        json += "}";
    }


    json += "]}";


    prefs.end();


    server.send(
        200,
        "application/json",
        json
    );
}


//=============================================================================
// WEBPAGE CSS
//=============================================================================

String DeviceRestartInfo::getCss() const
{
    return R"rawliteral(

/* =========================================================
   DEVICE RESTART INFORMATION
   ========================================================= */

.restart-card{
    margin-top:20px;
}

.restart-info-grid{
    display:grid;
    grid-template-columns:repeat(2, 1fr);
    gap:20px;
    margin-top:15px;
}

.restart-info-item{
    background:rgba(255,255,255,0.08);
    border-radius:14px;
    padding:18px;
}

.restart-label{
    font-size:14px;
    color:#dbeafe;
    margin-bottom:8px;
}

.restart-value{
    font-size:22px;
    font-weight:700;
    color:white;
}


/* =========================================================
   HISTORY TABLE
   ========================================================= */

.restart-history-wrapper{
    width:100%;
    overflow-x:auto;
    margin-top:15px;
}

.restart-history-table{
    width:100%;
    border-collapse:collapse;
    color:white;
}

.restart-history-table th,
.restart-history-table td{
    padding:12px;
    text-align:left;
    border-bottom:1px solid rgba(255,255,255,0.15);
}

.restart-history-table th{
    background:rgba(255,255,255,0.10);
    font-weight:600;
}

.restart-history-table td{
    background:rgba(255,255,255,0.04);
}

.restart-history-table tr:hover td{
    background:rgba(255,255,255,0.08);
}


/* =========================================================
   MOBILE
   ========================================================= */

@media(max-width:700px){

    .restart-info-grid{
        grid-template-columns:1fr;
    }

    .restart-value{
        font-size:18px;
    }

    .restart-history-table{
        min-width:600px;
    }
}

)rawliteral";
}


//=============================================================================
// WEBPAGE HTML
//=============================================================================

String DeviceRestartInfo::getHtml() const
{
    return R"rawliteral(

<!-- =========================================================
     DEVICE RESTART INFORMATION
     ========================================================= -->

<div class="card restart-card">

    <h2>🔄 Device Restart Information</h2>

    <div class="restart-info-grid">

        <div class="restart-info-item">

            <div class="restart-label">
                Last Restart
            </div>

            <div
                class="restart-value"
                id="last-restart"
            >
                Loading...
            </div>

        </div>


        <div class="restart-info-item">

            <div class="restart-label">
                Restart Reason
            </div>

            <div
                class="restart-value"
                id="restart-reason"
            >
                Loading...
            </div>

        </div>

    </div>

</div>


<!-- =========================================================
     RESTART HISTORY
     ========================================================= -->

<div class="card restart-card">

    <h2>📋 Restart History</h2>

    <div class="restart-history-wrapper">

        <table class="restart-history-table">

            <thead>

                <tr>
                    <th>#</th>
                    <th>Date</th>
                    <th>Time</th>
                    <th>Reason</th>
                </tr>

            </thead>


            <tbody id="restart-history-body">

                <tr>
                    <td colspan="4">
                        Loading...
                    </td>
                </tr>

            </tbody>

        </table>

    </div>

</div>

)rawliteral";
}


//=============================================================================
// WEBPAGE JAVASCRIPT
//=============================================================================

String DeviceRestartInfo::getJavaScript() const
{
    return R"rawliteral(

/* =========================================================
   DEVICE RESTART INFORMATION
   ========================================================= */

function loadRestartInfo()
{
    fetch('/restartinfo')

        .then(response => response.json())

        .then(data =>
        {
            const lastRestart =
                document.getElementById(
                    "last-restart"
                );

            const restartReason =
                document.getElementById(
                    "restart-reason"
                );


            if (lastRestart)
            {
                lastRestart.innerHTML =
                    data.date + " " + data.time;
            }


            if (restartReason)
            {
                restartReason.innerHTML =
                    data.reason;
            }
        })

        .catch(error =>
        {
            console.log(
                "Restart information error:",
                error
            );


            const lastRestart =
                document.getElementById(
                    "last-restart"
                );

            const restartReason =
                document.getElementById(
                    "restart-reason"
                );


            if (lastRestart)
                lastRestart.innerHTML =
                    "Unavailable";


            if (restartReason)
                restartReason.innerHTML =
                    "Unavailable";
        });
}


/* =========================================================
   RESTART HISTORY
   ========================================================= */

function loadRestartHistory()
{
    fetch('/restarthistory')

        .then(response => response.json())

        .then(data =>
        {
            const tableBody =
                document.getElementById(
                    "restart-history-body"
                );


            if (!tableBody)
                return;


            tableBody.innerHTML = "";


            if (
                !data.history ||
                data.history.length === 0
            )
            {
                tableBody.innerHTML =
                    '<tr>' +
                    '<td colspan="4">' +
                    'No restart history available' +
                    '</td>' +
                    '</tr>';

                return;
            }


            data.history.forEach(
                (item, index) =>
                {
                    const row =
                        document.createElement(
                            "tr"
                        );


                    row.innerHTML =
                        '<td>' +
                        (index + 1) +
                        '</td>' +

                        '<td>' +
                        item.date +
                        '</td>' +

                        '<td>' +
                        item.time +
                        '</td>' +

                        '<td>' +
                        item.reason +
                        '</td>';


                    tableBody.appendChild(row);
                }
            );
        })

        .catch(error =>
        {
            console.log(
                "Restart history error:",
                error
            );


            const tableBody =
                document.getElementById(
                    "restart-history-body"
                );


            if (tableBody)
            {
                tableBody.innerHTML =
                    '<tr>' +
                    '<td colspan="4">' +
                    'Unable to load restart history' +
                    '</td>' +
                    '</tr>';
            }
        });
}


/* =========================================================
   LOAD RESTART DATA
   ========================================================= */

function loadDeviceRestartData()
{
    loadRestartInfo();
    loadRestartHistory();
}


// Initial load
loadDeviceRestartData();


// Refresh every 10 seconds
setInterval(
    loadDeviceRestartData,
    10000
);

)rawliteral";
}


//=============================================================================
// GETTERS
//=============================================================================

const String &DeviceRestartInfo::getDate() const
{
    return lastRestartDate;
}


const String &DeviceRestartInfo::getTime() const
{
    return lastRestartTime;
}


const String &DeviceRestartInfo::getReason() const
{
    return restartReason;
}