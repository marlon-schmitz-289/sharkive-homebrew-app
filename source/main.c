#include <citro2d.h>
#include <stdio.h>
#include <time.h>
#include "net.h"

#define TOP_W 400
#define BOT_W 320
#define SCR_H 240

#define RGB(r, g, b) C2D_Color32(r, g, b, 0xFF)
#define COL_TEXT   RGB(0x3C, 0x3C, 0x3C)
#define COL_SUB    RGB(0x80, 0x86, 0x8C)
#define COL_ACCENT RGB(0x1E, 0xA0, 0xE6) // HOME-menu blue
#define COL_OK     RGB(0x3C, 0xB4, 0x4A)
#define COL_ERR    RGB(0xE0, 0x40, 0x40)
#define COL_WHITE  RGB(0xFF, 0xFF, 0xFF)
#define COL_TRACK  RGB(0xDD, 0xE1, 0xE6)
#define COL_OFF    RGB(0xB8, 0xBE, 0xC4)

static C2D_TextBuf tbuf;

static struct { u8 pct; bool charging, wifi; u8 bars; } sys;

// wrap > 0 word-wraps at that width
static void textw(const char *s, float x, float y, float scale, u32 col, u32 align, float wrap) {
    C2D_Text t;
    C2D_TextParse(&t, tbuf, s);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor | align | (wrap > 0 ? C2D_WordWrap : 0), x, y, 0, scale, scale, col, wrap);
}

static void text(const char *s, float x, float y, float scale, u32 col, u32 align) {
    textw(s, x, y, scale, col, align, 0);
}

static float text_width(const char *s, float scale, float *h) {
    C2D_Text t;
    float w;
    C2D_TextParse(&t, tbuf, s);
    C2D_TextGetDimensions(&t, scale, scale, &w, h);
    return w;
}

static void rrect(float x, float y, float w, float h, float r, u32 c) {
    C2D_DrawRectSolid(x + r, y, 0, w - 2 * r, h, c);
    C2D_DrawRectSolid(x, y + r, 0, w, h - 2 * r, c);
    C2D_DrawCircleSolid(x + r, y + r, 0, r, c);
    C2D_DrawCircleSolid(x + w - r, y + r, 0, r, c);
    C2D_DrawCircleSolid(x + r, y + h - r, 0, r, c);
    C2D_DrawCircleSolid(x + w - r, y + h - r, 0, r, c);
}

static void panel(float x, float y, float w, float h) {
    rrect(x + 1, y + 3, w, h, 10, C2D_Color32(0, 0, 0, 0x22)); // shadow
    rrect(x, y, w, h, 10, COL_WHITE);
}

// Light HOME-menu style background with faint diagonal stripes.
static void background(float w) {
    u32 a = RGB(0xF7, 0xF8, 0xFA), b = RGB(0xD8, 0xDD, 0xE3);
    C2D_DrawRectangle(0, 0, 0, w, SCR_H, a, a, b, b);
    u32 s = C2D_Color32(0xFF, 0xFF, 0xFF, 0x70);
    for (float x = -SCR_H; x < w; x += 16)
        C2D_DrawLine(x, SCR_H, s, x + SCR_H, 0, s, 4, 0);
}

static void progress_bar(float x, float y, float w, float f, u32 col) {
    rrect(x, y, w, 14, 7, COL_TRACK);
    if (f > 0) rrect(x, y, 14 + (w - 14) * (f > 1 ? 1 : f), 14, 7, col);
}

static void wifi_icon(float x, float y) {
    for (int i = 0; i < 3; i++) {
        float h = 4 + i * 4;
        u32 c = sys.wifi && sys.bars > i ? COL_ACCENT : COL_OFF;
        C2D_DrawRectSolid(x + i * 6, y + 12 - h, 0, 4, h, c);
    }
    if (!sys.wifi) C2D_DrawLine(x - 1, y, COL_ERR, x + 17, y + 12, COL_ERR, 2, 0);
}

static void battery_icon(float x, float y) {
    rrect(x, y, 26, 13, 3, COL_TEXT);
    C2D_DrawRectSolid(x + 26, y + 4, 0, 2, 5, COL_TEXT);
    C2D_DrawRectSolid(x + 2, y + 2, 0, 22, 9, COL_WHITE);
    u32 c = sys.charging ? COL_OK : sys.pct <= 15 ? COL_ERR : COL_ACCENT;
    C2D_DrawRectSolid(x + 3, y + 3, 0, 20 * sys.pct / 100.0f, 7, c);
    if (sys.charging) { // lightning bolt
        u32 yel = RGB(0xFF, 0xD2, 0x00);
        C2D_DrawTriangle(x + 15, y + 1, yel, x + 9, y + 7, yel, x + 13, y + 7, yel, 0);
        C2D_DrawTriangle(x + 13, y + 6, yel, x + 17, y + 6, yel, x + 11, y + 12, yel, 0);
    }
}

static void status_bar(void) {
    u32 a = C2D_Color32(0xFF, 0xFF, 0xFF, 0xE0);
    C2D_DrawRectSolid(0, 0, 0, TOP_W, 22, a);
    C2D_DrawRectSolid(0, 22, 0, TOP_W, 1, COL_TRACK);

    char buf[32];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(buf, sizeof buf, "%H:%M", tm);
    text(buf, 8, 3, 0.55f, COL_TEXT, C2D_AlignLeft);
    strftime(buf, sizeof buf, "%d.%m.%Y", tm);
    text(buf, 64, 3, 0.55f, COL_SUB, C2D_AlignLeft);

    battery_icon(TOP_W - 36, 5);
    wifi_icon(TOP_W - 62, 5);
}

static void draw_top(void) {
    background(TOP_W);
    status_bar();
    panel(24, 36, TOP_W - 48, 190);
    text("Sharkive Updater", TOP_W / 2, 48, 0.9f, COL_ACCENT, C2D_AlignCenter);
    text("Cheats for Luma3DS (Rosalina)", TOP_W / 2, 78, 0.5f, COL_SUB, C2D_AlignCenter);
    C2D_DrawRectSolid(48, 100, 0, TOP_W - 96, 1, COL_TRACK);

    char buf[64];
    float cx = TOP_W / 2, bx = 48, bw = TOP_W - 96;
    switch (g_state) {
    case ST_IDLE:
        text("Press \uE000 to download the cheat database\nfrom FlagBrew/Sharkive.", cx, 124, 0.55f, COL_TEXT, C2D_AlignCenter);
        break;
    case ST_DOWNLOAD:
        text("Downloading...", cx, 118, 0.6f, COL_TEXT, C2D_AlignCenter);
        progress_bar(bx, 152, bw, g_progress, COL_ACCENT);
        snprintf(buf, sizeof buf, "%lu KB", g_bytes / 1024);
        text(buf, cx, 176, 0.5f, COL_SUB, C2D_AlignCenter);
        break;
    case ST_EXTRACT:
        text("Extracting cheats...", cx, 118, 0.6f, COL_TEXT, C2D_AlignCenter);
        progress_bar(bx, 152, bw, g_progress, COL_ACCENT);
        snprintf(buf, sizeof buf, "%lu files", g_count);
        text(buf, cx, 176, 0.5f, COL_SUB, C2D_AlignCenter);
        break;
    case ST_DONE:
        progress_bar(bx, 152, bw, 1, COL_OK);
        textw(g_msg, cx, 118, 0.6f, COL_OK, C2D_AlignCenter, bw);
        text("Enable cheats in-game via the Rosalina menu\n(L + D-Pad Down + SELECT).", cx, 176, 0.45f, COL_SUB, C2D_AlignCenter);
        break;
    case ST_ERROR:
        textw(g_msg, cx, 120, 0.55f, COL_ERR, C2D_AlignCenter, bw);
        text("\uE000 Try again", cx, 186, 0.45f, COL_SUB, C2D_AlignCenter);
        break;
    }
}

// Big A button on the bottom screen; also tappable.
#define BTN_X 20
#define BTN_Y 40
#define BTN_W (BOT_W - 40)
#define BTN_H 72
#define EXIT_Y 132
#define EXIT_H 44

static bool busy(void) { return g_state == ST_DOWNLOAD || g_state == ST_EXTRACT; }

static void draw_bottom(void) {
    background(BOT_W);
    text("Sharkive Updater", BOT_W / 2, 10, 0.5f, COL_SUB, C2D_AlignCenter);

    // A button: 3D edge + lighter top half, "A" badge and label centered as one group
    bool b = busy();
    u32 c = b ? COL_OFF : COL_ACCENT;
    rrect(BTN_X, BTN_Y + 5, BTN_W, BTN_H, 18, b ? RGB(0x98, 0x9E, 0xA4) : RGB(0x14, 0x78, 0xB4));
    rrect(BTN_X, BTN_Y, BTN_W, BTN_H, 18, c);
    rrect(BTN_X + 4, BTN_Y + 4, BTN_W - 8, BTN_H / 2, 14, b ? RGB(0xC6, 0xCB, 0xD0) : RGB(0x4C, 0xB6, 0xEE));
    rrect(BTN_X + 4, BTN_Y + 18, BTN_W - 8, BTN_H - 22, 14, c);

    const char *label = b ? "\uE001 Cancel" : "Update cheats";
    float h, lw = text_width(label, 0.7f, &h);
    float gx = (BOT_W - (36 + 12 + lw)) / 2, cy = BTN_Y + BTN_H / 2;
    C2D_DrawCircleSolid(gx + 18, cy, 0, 18, COL_WHITE);
    float ah, aw = text_width("A", 0.75f, &ah);
    text("A", gx + 18 - aw / 2, cy - ah / 2, 0.75f, c, C2D_AlignLeft);
    text(label, gx + 48, cy - h / 2, 0.7f, COL_WHITE, C2D_AlignLeft);

    panel(BTN_X, EXIT_Y, BTN_W, EXIT_H);
    float sw = text_width("Exit", 0.55f, &h);
    float sx = (BOT_W - (58 + 10 + sw)) / 2;
    rrect(sx, 144, 58, 20, 10, COL_TEXT);
    text("START", sx + 29, 146, 0.45f, COL_WHITE, C2D_AlignCenter);
    text("Exit", sx + 68, 154 - h / 2, 0.55f, COL_TEXT, C2D_AlignLeft);

    if (g_state == ST_ERROR) text("Error - see top screen", BOT_W / 2, 186, 0.5f, COL_ERR, C2D_AlignCenter);

    text("Target: sd:/cheats/<TitleID>.txt", BOT_W / 2, 212, 0.45f, COL_SUB, C2D_AlignCenter);
}

static void poll_system(bool mcu_ok) {
    u8 v = 0;
    if (mcu_ok && R_SUCCEEDED(MCUHWC_GetBatteryLevel(&v))) sys.pct = v > 100 ? 100 : v;
    else if (R_SUCCEEDED(PTMU_GetBatteryLevel(&v))) sys.pct = v * 20; // 0-5 bars
    if (R_SUCCEEDED(PTMU_GetBatteryChargeState(&v))) sys.charging = v;
    u32 w = 0;
    sys.wifi = R_SUCCEEDED(ACU_GetWifiStatus(&w)) && w;
    sys.bars = osGetWifiStrength();
}

int main(void) {
    gfxInitDefault();
    romfsInit();
    ptmuInit();
    acInit();
    bool mcu_ok = R_SUCCEEDED(mcuHwcInit()); // needs Luma3DS service access, falls back to PTMU
    osSetSpeedupEnable(true);
    net_init();

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    tbuf = C2D_TextBufNew(1024);

    for (u32 frame = 0; aptMainLoop(); frame++) {
        hidScanInput();
        u32 down = hidKeysDown();
        bool tapped = false, exit_tapped = false;
        if (down & KEY_TOUCH) {
            touchPosition t;
            hidTouchRead(&t);
            bool in_x = t.px >= BTN_X && t.px < BTN_X + BTN_W;
            tapped = in_x && t.py >= BTN_Y && t.py < BTN_Y + BTN_H;
            exit_tapped = in_x && t.py >= EXIT_Y && t.py < EXIT_Y + EXIT_H;
        }
        if ((down & KEY_START) || exit_tapped) break;
        if ((down & KEY_A) || tapped) net_start();
        if (down & KEY_B) net_cancel();
        aptSetSleepAllowed(!busy()); // lid-close sleep would kill Wi-Fi mid-download; only IPCs on change
        if (frame % 60 == 0) poll_system(mcu_ok);

        C2D_TextBufClear(tbuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, COL_WHITE);
        C2D_SceneBegin(top);
        draw_top();
        C2D_TargetClear(bot, COL_WHITE);
        C2D_SceneBegin(bot);
        draw_bottom();
        C3D_FrameEnd(0);
    }

    net_exit();
    C2D_TextBufDelete(tbuf);
    C2D_Fini();
    C3D_Fini();
    if (mcu_ok) mcuHwcExit();
    acExit();
    ptmuExit();
    romfsExit();
    gfxExit();
    return 0;
}
