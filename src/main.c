#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/touch.h>
#include <vita2d.h>

#define SCREEN_WIDTH 960.0f
#define SCREEN_HEIGHT 544.0f
#define WIDGET_COUNT 8
#define GRID_COLUMNS 5
#define GRID_ROWS 3
#define CONFIG_DIR "ux0:data/ona-launcher"
#define CONFIG_PATH CONFIG_DIR "/config.ini"
#define TEXT_COLOR RGBA8(45, 50, 62, 255)

typedef enum {
    SCREEN_MENU = 0,
    SCREEN_PLACEHOLDER
} Screen;

static const char *const widget_labels[WIDGET_COUNT] = {
    "Games", "Homebrew", "Favorites", "Recent", "Screenshots",
    "Settings", "Storage", "About"
};

static void save_config(int selected) {
    SceUID fd;
    char config[64];
    int length;

    sceIoMkdir(CONFIG_DIR, 0777);
    fd = sceIoOpen(CONFIG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if (fd < 0) {
        return;
    }

    length = snprintf(config, sizeof(config), "selected=%d\n", selected);
    if (length > 0) {
        sceIoWrite(fd, config, (unsigned int)length);
    }
    sceIoClose(fd);
}

static int load_config(void) {
    SceUID fd;
    char config[64];
    int selected = 0;
    int length;

    fd = sceIoOpen(CONFIG_PATH, SCE_O_RDONLY, 0);
    if (fd < 0) {
        return selected;
    }

    length = sceIoRead(fd, config, sizeof(config) - 1);
    sceIoClose(fd);
    if (length <= 0) {
        return selected;
    }

    config[length] = '\0';
    if (sscanf(config, "selected=%d", &selected) != 1 ||
        selected < 0 || selected >= WIDGET_COUNT) {
        selected = 0;
    }
    return selected;
}

static int touch_selection(const SceTouchData *touch) {
    static const int widget_x[WIDGET_COUNT] = {0, 2, 4, 0, 3, 0, 1, 4};
    static const int widget_y[WIDGET_COUNT] = {0, 0, 0, 1, 1, 2, 2, 2};
    static const int widget_w[WIDGET_COUNT] = {2, 2, 1, 3, 2, 1, 3, 1};
    static const int widget_h[WIDGET_COUNT] = {1, 1, 1, 1, 1, 1, 1, 1};
    int column;
    int row;
    int index;
    int cell_width = 154;
    int cell_height = 78;
    int grid_x = 74;
    int grid_y = 132;

    if (touch->reportNum < 1) {
        return -1;
    }

    column = ((int)touch->report[0].x - grid_x) / cell_width;
    row = ((int)touch->report[0].y - grid_y) / cell_height;
    if (column < 0 || column >= GRID_COLUMNS || row < 0 || row >= GRID_ROWS) {
        return -1;
    }

    for (index = 0; index < WIDGET_COUNT; index++) {
        if (column >= widget_x[index] &&
            column < widget_x[index] + widget_w[index] &&
            row >= widget_y[index] &&
            row < widget_y[index] + widget_h[index]) {
            return index;
        }
    }
    return -1;
}

static void draw_round_rect(float x, float y, float width, float height,
                            float radius, unsigned int color) {
    float diameter = radius * 2.0f;

    vita2d_draw_rectangle(x + radius, y, width - diameter, height, color);
    vita2d_draw_rectangle(x, y + radius, width, height - diameter, color);
    vita2d_draw_fill_circle(x + radius, y + radius, radius, color);
    vita2d_draw_fill_circle(x + width - radius, y + radius, radius, color);
    vita2d_draw_fill_circle(x + radius, y + height - radius, radius, color);
    vita2d_draw_fill_circle(x + width - radius, y + height - radius, radius,
                            color);
}

static void draw_round_panel(float x, float y, float width, float height,
                             float radius, unsigned int shadow,
                             unsigned int border, unsigned int fill) {
    draw_round_rect(x + 4.0f, y + 5.0f, width, height, radius, shadow);
    draw_round_rect(x, y, width, height, radius, border);
    draw_round_rect(x + 3.0f, y + 3.0f, width - 6.0f, height - 6.0f,
                    radius - 3.0f, fill);
}

static void draw_widget(vita2d_pgf *font, int x, int y, int width, int height,
                        unsigned int color, const char *label,
                        const char *detail, int selected) {
    unsigned int border = selected ? RGBA8(104, 170, 167, 255)
                                   : RGBA8(177, 184, 191, 255);
    unsigned int shadow = RGBA8(173, 181, 190, 100);
    unsigned int icon = selected ? RGBA8(111, 190, 186, 255)
                                 : RGBA8(223, 229, 235, 255);

    draw_round_panel((float)x, (float)y, (float)width, (float)height, 14.0f,
                     shadow, border, color);
    draw_round_rect((float)(x + 16), (float)(y + 16), 48.0f,
                    (float)(height - 32), 10.0f, icon);
    draw_round_rect((float)(x + 25), (float)(y + 25), 30.0f,
                    (float)(height - 50), 7.0f,
                    selected ? RGBA8(255, 255, 255, 220)
                             : RGBA8(255, 255, 255, 190));
    vita2d_pgf_draw_text(font, x + 80, y + 34, TEXT_COLOR, 0.95f, label);
    vita2d_pgf_draw_text(font, x + 80, y + 55, RGBA8(100, 105, 117, 255),
                         0.62f, detail);
}

static void draw_menu(vita2d_pgf *font, int selected) {
    static const int widget_x[WIDGET_COUNT] = {0, 2, 4, 0, 3, 0, 1, 4};
    static const int widget_y[WIDGET_COUNT] = {0, 0, 0, 1, 1, 2, 2, 2};
    static const int widget_w[WIDGET_COUNT] = {2, 2, 1, 3, 2, 1, 3, 1};
    static const int widget_h[WIDGET_COUNT] = {1, 1, 1, 1, 1, 1, 1, 1};
    static const unsigned int widget_colors[WIDGET_COUNT] = {
        RGBA8(203, 230, 227, 255), RGBA8(250, 224, 179, 255),
        RGBA8(220, 214, 238, 255), RGBA8(232, 220, 211, 255),
        RGBA8(224, 235, 220, 255), RGBA8(226, 236, 237, 255),
        RGBA8(219, 228, 238, 255), RGBA8(235, 225, 233, 255)
    };
    static const char *const widget_details[WIDGET_COUNT] = {
        "Your collection", "Vita applications", "Quick access", "Last played",
        "Visual memories", "Personalize Ona", "Free space", "Prototype"
    };
    int dot_x;
    int dot_y;
    int index;
    const int grid_x = 74;
    const int grid_y = 132;
    const int cell_width = 154;
    const int cell_height = 78;

    vita2d_draw_rectangle(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT,
                          RGBA8(244, 246, 247, 255));
    for (dot_y = 0; dot_y < 544; dot_y += 16) {
        for (dot_x = 0; dot_x < 960; dot_x += 16) {
            vita2d_draw_rectangle((float)dot_x, (float)dot_y, 2.0f, 2.0f,
                                  RGBA8(216, 222, 226, 110));
        }
    }

    draw_round_panel(20.0f, 18.0f, 912.0f, 66.0f, 28.0f,
                     RGBA8(173, 181, 190, 100), RGBA8(177, 184, 191, 255),
                     RGBA8(255, 255, 255, 235));
    draw_round_panel(332.0f, 28.0f, 420.0f, 46.0f, 20.0f,
                     RGBA8(190, 197, 203, 70), RGBA8(205, 212, 218, 255),
                     RGBA8(236, 241, 244, 255));
    vita2d_pgf_draw_text(font, 378, 59, TEXT_COLOR, 1.15f,
                         "ONA  HOME");
    vita2d_pgf_draw_text(font, 48, 58, RGBA8(91, 98, 109, 255), 0.72f,
                         "SELECT  OPTIONS");
    vita2d_pgf_draw_text(font, 780, 58, RGBA8(91, 98, 109, 255), 0.72f,
                         "START  PROFILE");
    draw_round_rect(880.0f, 34.0f, 28.0f, 28.0f, 14.0f,
                    RGBA8(111, 190, 186, 255));
    vita2d_pgf_draw_text(font, 889, 56, RGBA8(255, 255, 255, 255), 0.72f,
                         "O");

    draw_round_panel(42.0f, 100.0f, 864.0f, 350.0f, 28.0f,
                     RGBA8(173, 181, 190, 100), RGBA8(177, 184, 191, 255),
                     RGBA8(255, 255, 255, 220));
    draw_round_rect(58.0f, 116.0f, 832.0f, 318.0f, 20.0f,
                    RGBA8(247, 249, 249, 255));

    for (index = 0; index < WIDGET_COUNT; index++) {
        int width = widget_w[index] * cell_width - 10;
        int height = widget_h[index] * cell_height - 10;
        int x = grid_x + widget_x[index] * cell_width;
        int y = grid_y + widget_y[index] * cell_height;

        draw_widget(font, x, y, width, height, widget_colors[index],
                    widget_labels[index], widget_details[index],
                    selected == index);
    }

    vita2d_draw_rectangle(446.0f, 474.0f, 68.0f, 4.0f,
                          RGBA8(111, 190, 186, 255));
    vita2d_draw_rectangle(522.0f, 474.0f, 12.0f, 4.0f,
                          RGBA8(190, 197, 203, 255));
    vita2d_draw_rectangle(542.0f, 474.0f, 12.0f, 4.0f,
                          RGBA8(190, 197, 203, 255));
    vita2d_draw_rectangle(562.0f, 474.0f, 12.0f, 4.0f,
                          RGBA8(190, 197, 203, 255));

    draw_round_panel(28.0f, 488.0f, 218.0f, 38.0f, 14.0f,
                     RGBA8(173, 181, 190, 80), RGBA8(190, 197, 203, 255),
                     RGBA8(255, 255, 255, 220));
    vita2d_pgf_draw_text(font, 44, 512, RGBA8(77, 84, 95, 255), 0.66f,
                         "D-PAD  MOVE");
    draw_round_panel(714.0f, 488.0f, 218.0f, 38.0f, 14.0f,
                     RGBA8(173, 181, 190, 80), RGBA8(190, 197, 203, 255),
                     RGBA8(255, 255, 255, 220));
    vita2d_pgf_draw_text(font, 730, 512, RGBA8(77, 84, 95, 255), 0.66f,
                         "X  SELECT   TRIANGLE  OPTIONS");
}

static void draw_placeholder(vita2d_pgf *font, int selected) {
    vita2d_draw_rectangle(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT,
                          RGBA8(17, 22, 35, 255));
    vita2d_pgf_draw_text(font, 52.0f, 61.0f, TEXT_COLOR, 1.4f,
                         widget_labels[selected]);
    vita2d_pgf_draw_text(font, 80.0f, 230.0f, TEXT_COLOR, 1.1f,
                         "This section is ready for the next prototype step.");
    vita2d_pgf_draw_text(font, 80.0f, 280.0f, TEXT_COLOR, 0.9f,
                         "Press Circle to return to the main menu.");
    vita2d_pgf_draw_text(font, 80.0f, 492.0f, TEXT_COLOR, 0.8f,
                         "START: exit");
}

int main(void) {
    SceCtrlData pad;
    SceTouchData touch;
    vita2d_pgf *font;
    Screen screen = SCREEN_MENU;
    int selected = load_config();
    int previous_selected = selected;
    bool running = true;

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    vita2d_init();
    vita2d_set_clear_color(RGBA8(17, 22, 35, 255));
    font = vita2d_load_default_pgf();
    if (font == NULL) {
        vita2d_fini();
        sceKernelExitProcess(1);
        return 1;
    }

    while (running) {
        memset(&pad, 0, sizeof(pad));
        memset(&touch, 0, sizeof(touch));
        sceCtrlPeekBufferPositive(0, &pad, 1);
        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);

        if ((pad.buttons & SCE_CTRL_START) != 0) {
            running = false;
        } else if (screen == SCREEN_MENU) {
            if ((pad.buttons & SCE_CTRL_UP) != 0) {
                selected = (selected + WIDGET_COUNT - 1) % WIDGET_COUNT;
            } else if ((pad.buttons & SCE_CTRL_DOWN) != 0) {
                selected = (selected + 1) % WIDGET_COUNT;
            } else if ((pad.buttons & SCE_CTRL_CROSS) != 0) {
                screen = SCREEN_PLACEHOLDER;
            } else {
                int touched = touch_selection(&touch);
                if (touched >= 0) {
                    selected = touched;
                }
            }
        } else if ((pad.buttons & SCE_CTRL_CIRCLE) != 0) {
            screen = SCREEN_MENU;
        }

        if (selected != previous_selected) {
            save_config(selected);
            previous_selected = selected;
        }

        vita2d_start_drawing();
        vita2d_clear_screen();
        if (screen == SCREEN_MENU) {
            draw_menu(font, selected);
        } else {
            draw_placeholder(font, selected);
        }
        vita2d_end_drawing();
        vita2d_swap_buffers();
        sceDisplayWaitVblankStart();
    }

    save_config(selected);
    vita2d_free_pgf(font);
    vita2d_fini();
    sceKernelExitProcess(0);
    return 0;
}
