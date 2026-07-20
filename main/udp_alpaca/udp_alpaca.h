#pragma once

/*
 * Start the Alpaca Discovery UDP listener.
 *
 * Runs as a background FreeRTOS task listening on port 32227.
 * When a discovery probe ("alpacadiscovery1") is received, the task
 * responds with a JSON document advertising the Alpaca REST API port
 * so ASCOM / Alpaca clients can auto-discover this device.
 */
void udp_alpaca_start(void);
