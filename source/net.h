#pragma once
#include <3ds.h>

typedef enum { ST_IDLE, ST_DOWNLOAD, ST_EXTRACT, ST_DONE, ST_ERROR } State;

// Written by the worker thread, read by the UI. g_msg is filled before g_state flips to DONE/ERROR.
extern volatile State g_state;
extern volatile float g_progress; // 0..1
extern volatile u32 g_bytes;      // downloaded bytes
extern volatile u32 g_count;      // cheat files written
extern char g_msg[160];

void net_init(void);
void net_exit(void);  // aborts and joins a running update
void net_start(void); // no-op while an update is running
void net_cancel(void); // worker turns this into ST_ERROR "Abgebrochen."
