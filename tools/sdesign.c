#include <clay/clay.h>
#include <clay/clay_raylib_renderer.c>
#include <raylib.h>

void go_into_full_screen();
Clay_RenderCommandArray build_ui();
void handle_clay_errors(Clay_ErrorData errorData);

static bool running = true;
static Rectangle objects[50] = {0};
static size_t object_count = 0;
static size_t selected = 0xffff;

int main() {
    InitWindow(1920, 1080, "SDesign");
    SetExitKey(KEY_NULL);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(800, 450);

    Camera2D camera = {.zoom = 1, .offset = {960, 540}, .target = {0, 0}, .rotation = 0};

    const int min_mem_clay = Clay_MinMemorySize();
    Clay_Arena clay_arena = Clay_CreateArenaWithCapacityAndMemory(min_mem_clay, malloc(min_mem_clay));
    Clay_Initialize(clay_arena, (Clay_Dimensions){GetScreenWidth(), GetScreenHeight()},
                    (Clay_ErrorHandler){.errorHandlerFunction = handle_clay_errors});

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
        for (size_t i = 0; i < object_count; i++) {
            if (CheckCollisionPointRec(world_mouse_pos, objects[i])) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    selected = i;
                }

                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    if (selected != 0xffff) {
                        const Vector2 mouse_delta = GetMouseDelta();
                        objects[selected].x += mouse_delta.x / camera.zoom;
                        objects[selected].y += mouse_delta.y / camera.zoom;
                    }
                }

                if (IsKeyDown(KEY_LEFT_SHIFT)) {
                    if (scroll_delta.y != 0) {
                        objects[selected].width += scroll_delta.y * 10;
                    }
                }

                if (IsKeyDown(KEY_LEFT_ALT)) {
                    if (scroll_delta.y != 0) {
                        objects[selected].height += scroll_delta.y * 10;
                    }
                }
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                bool clicked_on_rectangle = false;
                for (size_t i = 0; i < object_count; i++) {
                    if (CheckCollisionPointRec(world_mouse_pos, objects[i])) {
                        clicked_on_rectangle = true;
                        break;
                    }
                }
                if (!clicked_on_rectangle) {
                    selected = 0xffff;
                }
            }
        }

        // Render
        BeginDrawing();
        ClearBackground(GetColor(0x181818ff));
        BeginMode2D(camera);
        DrawText("0, 0", 10, 10, 36, WHITE);
        DrawLineEx((Vector2){0, -3000}, (Vector2){0, 3000}, 5, GRAY);
        DrawLineEx((Vector2){-3000, 0}, (Vector2){3000, 0}, 5, GRAY);

        for (size_t i = 0; i < object_count; i++) {
            DrawRectangleRec(objects[i], WHITE);
            if (i == selected) {
                DrawRectangleLinesEx(objects[i], 2, RED);
            }
        }

        EndMode2D();
        Clay_Raylib_Render(build_ui(), NULL);
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
        printf("{\n");
        for (size_t i = 0; i < object_count; i++) {
            printf("(Rectangle){\n");
            printf("\t.x = %f\n", objects[i].x);
            printf("\t.y = %f\n", objects[i].y);
            printf("\t.width = %f\n", objects[i].width);
            printf("\t.height = %f\n", objects[i].height);
            printf("},\n");
        }
        printf("}\n");
    }
}

void add_object_button_action(Clay_ElementId e_id, Clay_PointerData pd, intptr_t ud) {
    (void)e_id;
    (void)ud;
    if (pd.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        objects[object_count++] = (Rectangle){
            .x = 0,
            .y = 0,
            .width = 64,
            .height = 64,
        };
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
            CLAY({.id = CLAY_ID("QuitButton"),
                  .cornerRadius = {16, 16, 16, 16},
                  .layout = {.sizing = {CLAY_SIZING_PERCENT(0.05), CLAY_SIZING_GROW(0)}},
                  .backgroundColor = {20, 20, 20, 255}}) {
                Clay_OnHover(close_button_action, 0);
            }
            CLAY({.id = CLAY_ID("DumpButton"),
                  .cornerRadius = {16, 16, 16, 16},
                  .layout = {.sizing = {CLAY_SIZING_PERCENT(0.05), CLAY_SIZING_GROW(0)}},
                  .backgroundColor = {20, 20, 20, 255}}) {
                Clay_OnHover(dump_button_action, 0);
            }
        }
        CLAY({.id = CLAY_ID("ObjectList"),
              .layout =
                  {
                      .layoutDirection = CLAY_TOP_TO_BOTTOM,
                      .sizing = {CLAY_SIZING_PERCENT(0.2), CLAY_SIZING_GROW(0)},
                      .padding = {16, 16, 16, 16},
                      .childGap = 16,
                  },
              .cornerRadius = {16, 16, 16, 16},
              .backgroundColor = {50, 50, 50, 255},
              .scroll = {.vertical = true}}) {
            for (size_t i = 0; i < object_count; i++) {
                CLAY({
                    .layout = {.layoutDirection = CLAY_LEFT_TO_RIGHT,
                               .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(100)},
                               .padding = {16, 16, 16, 16}},
                    .cornerRadius = {16, 16, 16, 16},
                    .backgroundColor = {100, 100, 100, 255},
                }) {
                }
            }
        }
        CLAY({.id = CLAY_ID("ObjectBar"),
              .layout = {.layoutDirection = CLAY_LEFT_TO_RIGHT,
                         .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.1)},
                         .padding = {16, 16, 16, 16}},
              .cornerRadius = {16, 16, 16, 16},
              .backgroundColor = {50, 50, 50, 255}}) {
            CLAY({.id = CLAY_ID("AddObject"),
                  .layout = {.layoutDirection = CLAY_LEFT_TO_RIGHT,
                             .sizing = {CLAY_SIZING_PERCENT(0.1), CLAY_SIZING_GROW(0)},
                             .padding = {16, 16, 16, 16}},
                  .cornerRadius = {16, 16, 16, 16},
                  .backgroundColor = {20, 20, 20, 255}}) {
                Clay_OnHover(add_object_button_action, 0);
            }
        }
    }
    return Clay_EndLayout();
}
