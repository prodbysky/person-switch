#include <clay/clay.h>
#include <clay/clay_raylib_renderer.c>
#include <raylib.h>

void go_into_full_screen();
Clay_RenderCommandArray build_ui();
void handle_clay_errors(Clay_ErrorData errorData);

int main() {
    InitWindow(1920, 1080, "SDesign");
    SetExitKey(KEY_NULL);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(800, 450);

    const int min_mem_clay = Clay_MinMemorySize();
    Clay_Arena clay_arena = Clay_CreateArenaWithCapacityAndMemory(min_mem_clay, malloc(min_mem_clay));
    Clay_Initialize(clay_arena, (Clay_Dimensions){GetScreenWidth(), GetScreenHeight()},
                    (Clay_ErrorHandler){.errorHandlerFunction = handle_clay_errors});

    while (!WindowShouldClose()) {
        {
            const Vector2 mouse_pos = GetMousePosition();
            const Vector2 scroll_delta = GetMouseWheelMoveV();
            const bool mouse_left = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            const float dt = GetFrameTime();
            Clay_SetLayoutDimensions((Clay_Dimensions){GetScreenWidth(), GetScreenHeight()});
            Clay_SetPointerState((Clay_Vector2){mouse_pos.x, mouse_pos.y}, mouse_left);
            Clay_UpdateScrollContainers(true, (Clay_Vector2){scroll_delta.x, scroll_delta.y}, dt);
        }

        if (IsKeyPressed(KEY_F11)) {
            go_into_full_screen();
        }
        BeginDrawing();
        ClearBackground(GetColor(0x181818ff));
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
                         .padding = {16, 16, 16, 16}},
              .cornerRadius = {16, 16, 16, 16},
              .backgroundColor = {50, 50, 50, 255}}) {
            CLAY({.id = CLAY_ID("QuitButton"),
                  .cornerRadius = {16, 16, 16, 16},
                  .layout = {.sizing = {CLAY_SIZING_PERCENT(0.05), CLAY_SIZING_GROW(0)}},
                  .backgroundColor = {20, 20, 20, 255}}) {
            }
        }
        CLAY({.id = CLAY_ID("ObjectList"),
              .layout = {.layoutDirection = CLAY_LEFT_TO_RIGHT,
                         .sizing = {CLAY_SIZING_PERCENT(0.2), CLAY_SIZING_GROW(0)},
                         .padding = {16, 16, 16, 16}},
              .cornerRadius = {16, 16, 16, 16},
              .backgroundColor = {50, 50, 50, 255}}) {
        }
        CLAY({.id = CLAY_ID("ObjectBar"),
              .layout = {.layoutDirection = CLAY_LEFT_TO_RIGHT,
                         .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.1)},
                         .padding = {16, 16, 16, 16}},
              .cornerRadius = {16, 16, 16, 16},
              .backgroundColor = {50, 50, 50, 255}}) {
        }
    }
    return Clay_EndLayout();
}
