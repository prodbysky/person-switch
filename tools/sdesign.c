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

static bool running = true;
static size_t selected = 0xffff;
static char *out = NULL;
Camera2D camera = {.zoom = 1, .offset = {960, 540}, .target = {0, 0}, .rotation = 0};

#define SPAWN_SELECT 100

static struct {
    Vector2 spawn;
    Rectangle platforms[50];
    Rectangle enemy_spawns[20];
    size_t count;
    size_t count_sp;
} stage;

#define CLAY_WHITE (Clay_Color){255, 255, 255, 255}

int main(int argc, char **argv) {
    stage.count = 0;
    stage.spawn = (Vector2){0, 0};

    if (argc < 2) {
        TraceLog(LOG_ERROR, "Provide the output file to write the stage to");
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
                if (CheckCollisionPointRec(world_mouse_pos, stage.enemy_spawns[i])) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        selected = i + 50;
                    }

                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                        const Vector2 mouse_delta = GetMouseDelta();
                        if (selected != 0xffff && selected != SPAWN_SELECT && selected > 49) {
                            stage.enemy_spawns[selected - 50].x += mouse_delta.x / camera.zoom;
                            stage.enemy_spawns[selected - 50].y += mouse_delta.y / camera.zoom;
                        }
                    }

                    if (selected != 0xffff && selected != SPAWN_SELECT && selected > 49) {
                        if (IsKeyDown(KEY_LEFT_SHIFT)) {
                            if (scroll_delta.y != 0) {
                                stage.enemy_spawns[selected - 50].width += scroll_delta.y * 10;
                            }
                        }
                        if (IsKeyDown(KEY_LEFT_ALT)) {
                            if (scroll_delta.y != 0) {
                                stage.enemy_spawns[selected - 50].height += scroll_delta.y * 10;
                            }
                        }
                    }
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            bool clicked_on_anything = false;
            bool b = true;
            if (CheckCollisionPointCircle(GetScreenToWorld2D(GetMousePosition(), camera), stage.spawn, 25)) {
                clicked_on_anything = true;
                b = false;
            }
            for (size_t i = 0; i < stage.count && b; i++) {
                if (CheckCollisionPointRec(world_mouse_pos, stage.platforms[i])) {
                    clicked_on_anything = true;
                    b = false;
                }
            }
            for (size_t i = 0; i < stage.count_sp && b; i++) {
                if (CheckCollisionPointRec(world_mouse_pos, stage.enemy_spawns[i])) {
                    clicked_on_anything = true;
                    b = false;
                }
            }
            if (!clicked_on_anything) {
                selected = 0xffff;
            }
        }

        // Render
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
            DrawRectangleRec(stage.enemy_spawns[i], GetColor(0x00ff0066));
            if (i == selected) {
                DrawRectangleLinesEx(stage.enemy_spawns[i], 2, RED);
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

void dump_button_action(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    (void)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        FILE *file = fopen(out, "w");
        fprintf(file, "Stage stage {\n");
        fprintf(file, ".spawn = (Vector2){%.2f, %.2f},\n", stage.spawn.x, stage.spawn.y);
        fprintf(file, ".platforms = {");
        for (size_t i = 0; i < stage.count; i++) {
            fprintf(file, "(Rectangle){\n");
            fprintf(file, "\t.x = %.2f,\n", stage.platforms[i].x);
            fprintf(file, "\t.y = %.2f,\n", stage.platforms[i].y);
            fprintf(file, "\t.width = %.2f,\n", stage.platforms[i].width);
            fprintf(file, "\t.height = %.2f,\n", stage.platforms[i].height);
            fprintf(file, "},\n");
        }
        fprintf(file, "},\n");
        fprintf(file, ".count = %zu\n,", stage.count);

        fprintf(file, ".spawns = {");
        for (size_t i = 0; i < stage.count_sp; i++) {
            fprintf(file, "(Rectangle){\n");
            fprintf(file, "\t.x = %.2f,\n", stage.enemy_spawns[i].x);
            fprintf(file, "\t.y = %.2f,\n", stage.enemy_spawns[i].y);
            fprintf(file, "\t.width = %.2f,\n", stage.enemy_spawns[i].width);
            fprintf(file, "\t.height = %.2f,\n", stage.enemy_spawns[i].height);
            fprintf(file, "},\n");
        }
        fprintf(file, "},\n");
        fprintf(file, ".count_sp = %zu\n", stage.count_sp);
        fprintf(file, "}");
        fclose(file);
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
        stage.enemy_spawns[stage.count_sp++] = (Rectangle){
            .x = 0,
            .y = 0,
            .width = 64,
            .height = 64,
        };
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
            button(CLAY_ID("DumpButton"), CLAY_SIZING_PERCENT(0.05), CLAY_SIZING_GROW(0), CLAY_STRING("Save"),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, dump_button_action);
        }
        CLAY({
            .id = CLAY_ID("ObjectControls"),
            .cornerRadius = {16, 16, 16, 16},
            .layout =
                {
                    .sizing = {CLAY_SIZING_PERCENT(0.2), CLAY_SIZING_PERCENT(.1)},
                    .padding = {16, 16, 16, 16},
                    .childGap = 16,
                },
            .backgroundColor = {50, 50, 50, 255},
        }) {
            button(CLAY_ID("AddPlatform"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Add Plat."),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, add_platform_button_action);
            button(CLAY_ID("AddSpawn"), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0), CLAY_STRING("Add Spawn."),
                   (Clay_Color){20, 20, 20, 255}, CLAY_WHITE, add_spawn_button_action);
        }
    }
    return Clay_EndLayout();
}
