# GSM Failure Matrix

| Failure condition | Detection | Current response | New response (Stage 2) | Timeout | Recovery level |
|---|---|---|---|---|---|
| TCP send failure | `AT+QISEND` returns `ERROR` or prompt `>` times out, or missing `SEND OK` | Returns `false` for TX loop | Increments `consecutive_failures`. Closes socket. | 3s prompt, 10s TX | Level 1: Retry next cycle. |
| TCP socket closed / Server drops | `AT+QISEND` fails | Fails on next send | Triggers failure logic | Varies | Level 1: Normal `AT+QICLOSE`. |
| PDP context lost | `AT+QIOPEN` fails | Returns early, waits 30s next loop | Fails immediately. Tracks failure. | 15s | Level 2: At 3 fails -> `AT+QIDEACT=1`. |
| Network registration lost | `AT+QIOPEN` fails | Endless failure loop, reboot required | Tracks failure | 15s | Level 3: At 5 fails -> Full `gsm_Init()` to re-register software layer. |
| Modem not responding | `AT` command timeout | Blocks until timeout | Tracks failure | Up to 15s | Level 4: At 7 fails -> `gsm_Hardware_Reset()` toggles `RESET_N` pin LOW on GPIO 15 for 200ms. |
| UART/parser desync | `gsmSendAT()` timeout | Fails condition | Fails condition, tracked. Added buffer bounding to limit fragmentation. | Command specific | Recovers organically or escalates to `gsm_Init()`. |

**Hardware Reset Mechanics:**
The modem is now explicitly tied to `GPIO 15` representing `RESET_N`. At 7 consecutive failed transmission cycles (with intervals defined by the user config), the ESP32 physically cycles this pin to trigger a hard EC200U reboot, wait 5 seconds, and cleanly re-initializes UART configuration. Subsequent failures do not trigger an immediate reset loop; they use a modulo scale to retry Level 2/3/4 progressively.
