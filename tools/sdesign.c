#include "../src/stage.h"
#include <assert.h>
#include <clay/clay.h>
#include <clay/clay_raylib_renderer.c>
#include <raylib.h>
#include <stdio.h>

void go_into_full_screen();
void handle_clay_errors(Clay_ErrorData errorData);
Clay_RenderCommandArray build_ui();
void text(Clay_String str, Clay_Color color, Clay_TextAlignment alligment);
void button(Clay_ElementId id, Clay_SizingAxis x_sizing, Clay_SizingAxis y_sizing, Clay_String label,
            Clay_Color background_color, Clay_Color label_color,
            void (*on_hover)(Clay_ElementId, Clay_PointerData, intptr_t));

void delete_object_action(Clay_ElementId id, Clay_PointerData pd, intptr_t ud);
void copy_object_action(Clay_ElementId id, Clay_PointerData pd, intptr_t ud);
void paste_object_action(Clay_ElementId id, Clay_PointerData pd, intptr_t ud);

void dump_button_action(Clay_ElementId id, Clay_PointerData pd, intptr_t ud);
void save_stage_action(Clay_ElementId id, Clay_PointerData pd, intptr_t ud);
void load_stage_action(Clay_ElementId id, Clay_PointerData pd, intptr_t ud);

static bool running = true;
static size_t selected = 0xffff;
static char *out = NULL;
static const char *saveFilename = "stage_editable.txt";

Camera2D camera = {.zoom = 1, .offset = {960, 540}, .target = {0, 0}, .rotation = 0};

#define SPAWN_SELECT 100

static Stage stage;

#define CLAY_WHITE (Clay_Color){255, 255, 255, 255}

static bool has_copied = false;
static Rectangle copied_rect;
static bool copied_is_platform;

void update_axis_controls() {
    if (selected == 0xffff || selected == SPAWN_SELECT)
        return;
    float moveStep = 5.0f * GetFrameTime();
    Rectangle *obj = NULL;
    if (selected < stage.count) {
        obj = &stage.platforms[selected];
    } else if (selected >= 50 && selected < 50 + stage.count_sp) {
        obj = &stage.spawns[selected - 50];
    }
    if (obj) {
        if (IsKeyDown(KEY_RIGHT))
            obj->x += moveStep;
        if (IsKeyDown(KEY_LEFT))
            obj->x -= moveStep;
        if (IsKeyDown(KEY_DOWN))
            obj->y += moveStep;
        if (IsKeyDown(KEY_UP))
            obj->y -= moveStep;
    }
}

void dump_button_action(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    (void)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        FILE *file = fopen(out, "w");
        if (!file) {
            TraceLog(LOG_ERROR, "Failed to open dump file");
            return;
        }
        fprintf(file, "Stage stage {\n");
        fprintf(file, "  .spawn = (Vector2){%.2f, %.2f},\n", stage.spawn.x, stage.spawn.y);
        fprintf(file, "  .platforms = {\n");
        for (size_t i = 0; i < stage.count; i++) {
            fprintf(file, "    (Rectangle){.x = %.2f, .y = %.2f, .width = %.2f, .height = %.2f},\n",
                    stage.platforms[i].x, stage.platforms[i].y, stage.platforms[i].width, stage.platforms[i].height);
        }
        fprintf(file, "  },\n");
        fprintf(file, "  .count = %zu,\n", stage.count);
        fprintf(file, "  .spawns = {\n");
        for (size_t i = 0; i < stage.count_sp; i++) {
            fprintf(file, "    (Rectangle){.x = %.2f, .y = %.2f, .width = %.2f, .height = %.2f},\n", stage.spawns[i].x,
                    stage.spawns[i].y, stage.spawns[i].width, stage.spawns[i].height);
        }
        fprintf(file, "  },\n");
        fprintf(file, "  .count_sp = %zu\n", stage.count_sp);
        fprintf(file, "}\n");
        fclose(file);
        TraceLog(LOG_INFO, "Stage dumped to %s", out);
    }
}

void save_stage_action(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    (void)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        FILE *file = fopen(saveFilename, "w");
        if (!file) {
            TraceLog(LOG_ERROR, "Failed to open save file");
            return;
        }
        fprintf(file, "%.2f %.2f\n", stage.spawn.x, stage.spawn.y);
        fprintf(file, "%zu\n", stage.count);
        for (size_t i = 0; i < stage.count; i++) {
            fprintf(file, "%.2f %.2f %.2f %.2f\n", stage.platforms[i].x, stage.platforms[i].y, stage.platforms[i].width,
                    stage.platforms[i].height);
        }
        fprintf(file, "%zu\n", stage.count_sp);
        for (size_t i = 0; i < stage.count_sp; i++) {
            fprintf(file, "%.2f %.2f %.2f %.2f\n", stage.spawns[i].x, stage.spawns[i].y, stage.spawns[i].width,
                    stage.spawns[i].height);
        }
        fclose(file);
        TraceLog(LOG_INFO, "Stage saved for editing to %s", saveFilename);
    }
}

void load_stage_action(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    (void)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        FILE *file = fopen(saveFilename, "r");
        if (!file) {
            TraceLog(LOG_ERROR, "Failed to open save file for loading");
            return;
        }
        if (fscanf(file, "%f %f", &stage.spawn.x, &stage.spawn.y) != 2) {
            TraceLog(LOG_ERROR, "Failed to read spawn");
            fclose(file);
            return;
        }
        if (fscanf(file, "%zu", &stage.count) != 1) {
            TraceLog(LOG_ERROR, "Failed to read platform count");
            fclose(file);
            return;
        }
        for (size_t i = 0; i < stage.count; i++) {
            if (fscanf(file, "%f %f %f %f", &stage.platforms[i].x, &stage.platforms[i].y, &stage.platforms[i].width,
                       &stage.platforms[i].height) != 4) {
                TraceLog(LOG_ERROR, "Failed to read platform %zu", i);
                fclose(file);
                return;
            }
        }
        if (fscanf(file, "%zu", &stage.count_sp) != 1) {
            TraceLog(LOG_ERROR, "Failed to read enemy spawn count");
            fclose(file);
            return;
        }
        for (size_t i = 0; i < stage.count_sp; i++) {
            if (fscanf(file, "%f %f %f %f", &stage.spawns[i].x, &stage.spawns[i].y, &stage.spawns[i].width,
                       &stage.spawns[i].height) != 4) {
                TraceLog(LOG_ERROR, "Failed to read enemy spawn %zu", i);
                fclose(file);
                return;
            }
        }
        fclose(file);
        TraceLog(LOG_INFO, "Stage loaded from %s", saveFilename);
    }
}

int main(int argc, char **argv) {
    stage.count = 0;
    stage.spawn = (Vector2){0, 0};

    if (argc < 2) {
        TraceLog(LOG_ERROR, "Provide the dump file to write the stage to");
        return 1;
    }
    out = argv[1];

    InitWindow(1920, 1080, "SDesign");
    SetExitKey(KEY_NULL);
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    SetWindowMinSize(800, 450);

    const int min_mem_clay = Clay_MinMemorySize();
    Clay_Arena clay_arena = Clay_CreateArenaWithCapacityAndMemory(min_mem_clay, malloc(min_mem_clay));
    Clay_Initialize(clay_arena, (Clay_Dimensions){GetScreenWidth(), GetScreenHeight()},
                    (Clay_ErrorHandler){.errorHandlerFunction = handle_clay_errors});
    Font font[1];
    font[0] = LoadFontEx("assets/fonts/iosevka medium.ttf", 32, NULL, 255);
    SetTextureFilter(font[0].texture, TEXTURE_FILTER_BILINEAR);

    Clay_SetMeasureTextFunction(Raylib_MeasureText, font);

    while (!WindowShouldClose() && running) {
        const Vector2 scroll_delta = GetMouseWheelMoveV();
        const Vector2 mouse_pos = GetMousePosition();

        {
            const bool mouse_left = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            const float dt = GetFrameTime();
            Clay_SetLayoutDimensions((Clay_Dimensions){GetScreenWidth(), GetScreenHeight()});
            Clay_SetPointerState((Clay_Vector2){mouse_pos.x, mouse_pos.y}, mouse_left);
            Clay_UpdateScrollContainers(true, (Clay_Vector2){scroll_delta.x, scroll_delta.y}, dt);
        }

        if (IsKeyPressed(KEY_F11)) {
            go_into_full_screen();
        }
        if (IsKeyPressed(KEY_F10)) {
            Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            const Vector2 mouse_delta = GetMouseDelta();
            camera.target = Vector2Subtract(camera.target, Vector2Scale(mouse_delta, 1.0 / camera.zoom));
        }

        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            if (scroll_delta.y != 0) {
                camera.zoom += scroll_delta.y * 0.1;
            }
        }
        camera.zoom = Clamp(camera.zoom, 0.2, 4);

        Vector2 world_mouse_pos = GetScreenToWorld2D(mouse_pos, camera);
        if (CheckCollisionPointCircle(world_mouse_pos, stage.spawn, 25)) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                selected = SPAWN_SELECT;
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                const Vector2 mouse_delta = GetMouseDelta();
                if (selected == SPAWN_SELECT) {
                    stage.spawn.x += mouse_delta.x / camera.zoom;
                    stage.spawn.y += mouse_delta.y / camera.zoom;
                }
            }
        } else {
            for (size_t i = 0; i < stage.count; i++) {
                if (CheckCollisionPointRec(world_mouse_pos, stage.platforms[i])) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        selected = i;
                    }
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                        const Vector2 mouse_delta = GetMouseDelta();
                        if (selected != 0xffff && selected != SPAWN_SELECT) {
                            stage.platforms[selected].x += mouse_delta.x / camera.zoom;
                            stage.platforms[selected].y += mouse_delta.y / camera.zoom;
                        }
                    }
                    if (selected != 0xffff && selected != SPAWN_SELECT) {
                        if (IsKeyDown(KEY_LEFT_SHIFT)) {
                            if (scroll_delta.y != 0) {
                                stage.platforms[selected].width += scroll_delta.y * 10;
                            }
                        }
                        if (IsKeyDown(KEY_LEFT_ALT)) {
                            if (scroll_delta.y != 0) {
                                stage.platforms[selected].height += scroll_delta.y * 10;
                            }
                        }
                    }
                }
            }
            for (size_t i = 0; i < stage.count_sp; i++) {
                if (CheckCollisionPointRec(world_mouse_pos, stage.spawns[i])) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        selected = i + 50;
                    }
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                        const Vector2 mouse_delta = GetMouseDelta();
                        if (selected != 0xffff && selected != SPAWN_SELECT && selected > 49) {
                            stage.spawns[selected - 50].x += mouse_delta.x / camera.zoom;
                            stage.spawns[selected - 50].y += mouse_delta.y / camera.zoom;
                        }
                    }
                    if (selected != 0xffff && selected != SPAWN_SELECT && selected > 49) {
                        if (IsKeyDown(KEY_LEFT_SHIFT)) {
                            if (scroll_delta.y != 0) {
                                stage.spawns[selected - 50].width += scroll_delta.y * 10;
                            }
                        }
                        if (IsKeyDown(KEY_LEFT_ALT)) {
                            if (scroll_delta.y != 0) {
                                stage.spawns[selected - 50].height += scroll_delta.y * 10;
                            }
                        }
                    }
                }
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            selected = 0xffff;
        }

        update_axis_controls();

        BeginDrawing();
        ClearBackground(GetColor(0x181818ff));
        BeginMode2D(camera);
        DrawText("0, 0", 10, 10, 36, WHITE);
        DrawLineEx((Vector2){0, -3000}, (Vector2){0, 3000}, 5, GRAY);
        DrawLineEx((Vector2){-3000, 0}, (Vector2){3000, 0}, 5, GRAY);
        for (size_t i = 0; i < stage.count; i++) {
            DrawRectangleRec(stage.platforms[i], WHITE);
            if (i == selected) {
                DrawRectangleLinesEx(stage.platforms[i], 2, RED);
            }
        }
        for (size_t i = 0; i < stage.count_sp; i++) {
            DrawRectangleRec(stage.spawns[i], GetColor(0x00ff0066));
            if (i + 50 == selected) {
                DrawRectangleLinesEx(stage.spawns[i], 2, RED);
            }
        }
        DrawCircleV(stage.spawn, 25, GREEN);
        DrawRectangle(stage.spawn.x, stage.spawn.y, 32, 96, WHITE);
        if (selected == SPAWN_SELECT) {
            DrawCircleLinesV(stage.spawn, 25, RED);
        }
        EndMode2D();
        Clay_Raylib_Render(build_ui(), font);
        EndDrawing();
    }

    CloseWindow();
}

void go_into_full_screen() {
    if (!IsWindowFullscreen()) {
        SetWindowSize(GetMonitorWidth(0), GetMonitorHeight(0));
        ToggleFullscreen();
    } else {
        ToggleFullscreen();
    }
}

void handle_clay_errors(Clay_ErrorData errorData) {
    TraceLog(LOG_ERROR, "%s", errorData.errorText.chars);
}

void close_button_action(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    (void)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        running = false;
    }
}

void add_platform_button_action(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    (void)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        stage.platforms[stage.count++] = (Rectangle){
            .x = 0,
            .y = 0,
            .width = 64,
            .height = 64,
        };
    }
}

void add_spawn_button_action(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    (void)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        stage.spawns[stage.count_sp++] = (Rectangle){
            .x = 0,
            .y = 0,
            .width = 64,
            .height = 64,
        };
    }
}

void delete_object_action(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    (void)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (selected == SPAWN_SELECT) {
            stage.spawn = (Vector2){0, 0};
        } else if (selected < stage.count) {
            for (size_t i = selected; i < stage.count - 1; i++) {
                stage.platforms[i] = stage.platforms[i + 1];
            }
            stage.count--;
            selected = 0xffff;
        } else if (selected >= 50 && selected < 50 + stage.count_sp) {
            size_t idx = selected - 50;
            for (size_t i = idx; i < stage.count_sp - 1; i++) {
                stage.spawns[i] = stage.spawns[i + 1];
            }
            stage.count_sp--;
            selected = 0xffff;
        }
    }
}

void copy_object_action(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    (void)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        if (selected == SPAWN_SELECT)
            return;
        else if (selected < stage.count) {
            copied_rect = stage.platforms[selected];
            copied_is_platform = true;
            has_copied = true;
        } else if (selected >= 50 && selected < 50 + stage.count_sp) {
            copied_rect = stage.spawns[selected - 50];
            copied_is_platform = false;
            has_copied = true;
        }
    }
}

void paste_object_action(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    (void)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME && has_copied) {
        Vector2 mouse_pos = GetMousePosition();
        Vector2 world_mouse = GetScreenToWorld2D(mouse_pos, camera);
        Rectangle new_rect = copied_rect;
        new_rect.x = world_mouse.x;
        new_rect.y = world_mouse.y;
        if (copied_is_platform) {
            stage.platforms[stage.count++] = new_rect;
            selected = stage.count - 1;
        } else {
            stage.spawns[stage.count_sp++] = new_rect;
            selected = 50 + stage.count_sp - 1;
        }
    }
}

void text(Clay_String str, Clay_Color color, Clay_TextAlignment alligment) {
    CLAY_TEXT(str, CLAY_TEXT_CONFIG({
                       .textAlignment = alligment,
                       .textColor = color,
                       .fontSize = 32,
                       .fontId = 0,
                   }));
}

void button(Clay_ElementId id, Clay_SizingAxis x_sizing, Clay_SizingAxis y_sizing, Clay_String label,
            Clay_Color background_color, Clay_Color label_color,
            void (*on_hover)(Clay_ElementId, Clay_PointerData, intptr_t)) {
    CLAY({.id = id,
          .cornerRadius = {16, 16, 16, 16},
          .layout = {.sizing = {x_sizing, y_sizing},
                     .padding = {16, 16, 16, 16},
                     .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}},
          .backgroundColor = background_color}) {
        Clay_OnHover(on_hover, 0);
        text(label, label_color, CLAY_TEXT_ALIGN_CENTER);
    }
}

Clay_RenderCommandArray build_ui() {
    Clay_BeginLayout();
    CLAY({.id = CLAY_ID("Root"),
          .layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                     .padding = {16, 16, 16, 16},
                     .childGap = 16}}) {
        CLAY({.id = CLAY_ID("Toolbar"),
              .layout = {.layoutDirection = CLAY_LEFT_TO_RIGHT,
                         .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.1)},
                         .padding = {16, 16, 16, 16},
                         .childGap = 16},
              .cornerRadius = {16, 16, 16, 16},
              .backgroundColor = {50, 50, 50, 255}}) {
            button(CLAY_ID("QuitButton"), CLAY_SIZING_PERCENT(0.05), CLAY_SIZING_GROW(0), CLAY_STRING("X"),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, close_button_action);
            // Dump as before using the command-line file.
            button(CLAY_ID("Dump"), CLAY_SIZING_PERCENT(0.05), CLAY_SIZING_GROW(0), CLAY_STRING("Dump"),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, dump_button_action);
            // Save and Load for editable file.
            button(CLAY_ID("SaveStage"), CLAY_SIZING_PERCENT(0.05), CLAY_SIZING_GROW(0), CLAY_STRING("Save"),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, save_stage_action);
            button(CLAY_ID("LoadStage"), CLAY_SIZING_PERCENT(0.05), CLAY_SIZING_GROW(0), CLAY_STRING("Load"),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, load_stage_action);
        }
        CLAY({.id = CLAY_ID("ObjectControls"),
              .cornerRadius = {16, 16, 16, 16},
              .layout = {.sizing = {CLAY_SIZING_PERCENT(0.4), CLAY_SIZING_PERCENT(.1)},
                         .padding = {16, 16, 16, 16},
                         .childGap = 16},
              .backgroundColor = {50, 50, 50, 255}}) {
            button(CLAY_ID("AddPlatform"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Add Plat."),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, add_platform_button_action);
            button(CLAY_ID("AddSpawn"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Add Spawn."),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, add_spawn_button_action);
            button(CLAY_ID("Delete"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Delete"),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, delete_object_action);
            button(CLAY_ID("Copy"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Copy"),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, copy_object_action);
            button(CLAY_ID("Paste"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Paste"),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, paste_object_action);
        }
    }
    return Clay_EndLayout();
}
