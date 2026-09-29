#include <archive.h>
#include <archive_entry.h>
#include <ctype.h>
#include <curl/curl.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "net.h"

#define ZIP_URL "https://codeload.github.com/FlagBrew/Sharkive/zip/refs/heads/master"
#define ZIP_TMP "sdmc:/sharkive-tmp.zip"
#define PREFIX "Sharkive-master/3ds/"
#define CHEAT_DIR "sdmc:/cheats"
// ponytail: codeload sends chunked (no Content-Length), so the download bar uses this estimate.
#define EXPECTED_ZIP 800000.0f
#define SOC_SIZE 0x100000

volatile State g_state = ST_IDLE;
volatile float g_progress;
volatile u32 g_bytes, g_count;
char g_msg[160];

static volatile bool abort_req;
static Thread thread;
static u32 *soc_buf;

static void fail(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_msg, sizeof g_msg, fmt, ap);
    va_end(ap);
    g_state = ST_ERROR;
}

static int on_progress(void *p, curl_off_t total, curl_off_t now, curl_off_t ut, curl_off_t un) {
    g_bytes = now;
    float f = total > 0 ? (float)now / total : now / EXPECTED_ZIP;
    g_progress = f > 0.99f ? 0.99f : f;
    return abort_req; // non-zero aborts the transfer
}

static bool download(void) {
    FILE *f = fopen(ZIP_TMP, "wb");
    if (!f) { fail("Cannot create %s.\nIs the SD card write-protected?", ZIP_TMP); return false; }
    setvbuf(f, NULL, _IOFBF, 64 * 1024);

    char err[CURL_ERROR_SIZE] = "";
    CURL *c = curl_easy_init();
    if (!c) { fclose(f); fail("curl_easy_init failed"); return false; }
    curl_easy_setopt(c, CURLOPT_URL, ZIP_URL);
    curl_easy_setopt(c, CURLOPT_USERAGENT, "SharkiveUpdater-3DS");
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(c, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(c, CURLOPT_LOW_SPEED_LIMIT, 1L); // give up after 30 s without data
    curl_easy_setopt(c, CURLOPT_LOW_SPEED_TIME, 30L);
    curl_easy_setopt(c, CURLOPT_CAINFO, "romfs:/cacert.pem");
    curl_easy_setopt(c, CURLOPT_BUFFERSIZE, 64L * 1024);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, f); // default write callback is fwrite
    curl_easy_setopt(c, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(c, CURLOPT_XFERINFOFUNCTION, on_progress);
    curl_easy_setopt(c, CURLOPT_ERRORBUFFER, err);

    CURLcode r = curl_easy_perform(c);
    long code = 0;
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(c);
    int close_err = fclose(f);

    if (r == CURLE_OK && close_err == 0) return true;
    if (abort_req) fail("Cancelled.");
    else if (r == CURLE_HTTP_RETURNED_ERROR) fail("Server error: HTTP %ld", code);
    else if (r == CURLE_WRITE_ERROR || r == CURLE_OK) fail("Write error during download.\nIs the SD card full?");
    else if (r == CURLE_PEER_FAILED_VERIFICATION) fail("Invalid TLS certificate.\nCheck system date/time.");
    else fail("Download failed:\n%s", err[0] ? err : curl_easy_strerror(r));
    return false;
}

// Luma3DS reads sdmc:/cheats/<16 uppercase hex>.txt
static bool is_cheat_name(const char *s) {
    if (strlen(s) != 20 || strcmp(s + 16, ".txt")) return false;
    for (int i = 0; i < 16; i++)
        if (!isxdigit((unsigned char)s[i]) || islower((unsigned char)s[i])) return false;
    return true;
}

// Write to <dst>.tmp first so a failure never leaves a half-written cheat file behind.
// Returns 1 = ok, 0 = SD write error, -1 = corrupt archive data.
static int write_entry(struct archive *a, const char *name) {
    static char buf[16 * 1024];
    char dst[64], tmp[72];
    snprintf(dst, sizeof dst, CHEAT_DIR "/%s", name);
    snprintf(tmp, sizeof tmp, "%s.tmp", dst);

    FILE *f = fopen(tmp, "wb");
    if (!f) return 0;
    int ok = 1;
    la_ssize_t n;
    while ((n = archive_read_data(a, buf, sizeof buf)) > 0)
        if (fwrite(buf, 1, n, f) != (size_t)n) { ok = 0; break; }
    if (n < 0) ok = -1;
    if (fclose(f) && ok > 0) ok = 0;
    // ponytail: FAT rename won't overwrite, so the old file is removed first (tiny gap if rename then fails)
    if (ok > 0) { remove(dst); ok = rename(tmp, dst) == 0; }
    if (ok <= 0) remove(tmp);
    return ok;
}

static void extract(void) {
    struct stat st;
    if (stat(ZIP_TMP, &st) || st.st_size == 0) { fail("Download is empty."); return; }
    mkdir(CHEAT_DIR, 0777);

    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);
    if (archive_read_open_filename(a, ZIP_TMP, 64 * 1024) != ARCHIVE_OK) {
        fail("Invalid ZIP:\n%s", archive_error_string(a));
        archive_read_free(a);
        return;
    }

    struct archive_entry *e;
    int r;
    size_t plen = strlen(PREFIX);
    while ((r = archive_read_next_header(a, &e)) == ARCHIVE_OK || r == ARCHIVE_WARN) {
        const char *p = archive_entry_pathname(e);
        if (p && !strncmp(p, PREFIX, plen) && is_cheat_name(p + plen)) {
            int w = write_entry(a, p + plen);
            if (w < 0) { fail("Corrupt ZIP:\n%s", archive_error_string(a)); break; }
            if (!w) { fail("Write error on %s.\nIs the SD card full?", p + plen); break; }
            g_count++;
        }
        g_progress = (float)archive_filter_bytes(a, -1) / st.st_size;
        if (abort_req) { fail("Cancelled."); break; }
    }
    if (g_state != ST_ERROR && r != ARCHIVE_EOF) fail("Corrupt ZIP:\n%s", archive_error_string(a));
    if (g_state != ST_ERROR && g_count == 0) fail("No cheats found in archive.\nDid the repo layout change?");
    archive_read_free(a);
}

static void worker(void *arg) {
    u32 wifi = 0;
    if (ACU_GetWifiStatus(&wifi) || !wifi) { fail("No Wi-Fi connection.\nPlease connect to the internet."); return; }

    if (download()) {
        g_progress = 0;
        g_state = ST_EXTRACT;
        extract();
    }
    remove(ZIP_TMP);
    if (g_state != ST_ERROR) {
        g_progress = 1;
        snprintf(g_msg, sizeof g_msg, "%lu cheat files installed!", g_count);
        g_state = ST_DONE;
    }
}

void net_init(void) {
    soc_buf = memalign(0x1000, SOC_SIZE);
    if (soc_buf && R_FAILED(socInit(soc_buf, SOC_SIZE))) { free(soc_buf); soc_buf = NULL; }
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

static void join(void) {
    if (!thread) return;
    threadJoin(thread, U64_MAX);
    threadFree(thread);
    thread = NULL;
}

void net_start(void) {
    if (g_state == ST_DOWNLOAD || g_state == ST_EXTRACT) return;
    join();
    abort_req = false;
    g_progress = 0;
    g_bytes = g_count = 0;
    if (!soc_buf) { fail("Network init (soc) failed."); return; }
    g_state = ST_DOWNLOAD;

    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    // Lower priority than the UI (higher number); the UI sleeps in vsync so the worker still gets time.
    thread = threadCreate(worker, NULL, 0x20000, prio + 1, -2, false);
    if (!thread) fail("Could not start worker thread.");
}

void net_cancel(void) { abort_req = true; }

void net_exit(void) {
    abort_req = true;
    join();
    curl_global_cleanup();
    if (soc_buf) { socExit(); free(soc_buf); }
}
