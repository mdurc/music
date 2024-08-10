
#include <raylib.h>

#define MINIAUDIO_IMPLEMENTATION
#include "libs/miniaudio.h"

typedef struct {
    float duration;
    char title[128];
    char artist[128];
} meta_data;

int main(){
    //  =======
    // MINIAUDIO_IMPLEMENTATION
    ma_result result;
    ma_engine engine;
    ma_sound sound;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) { return result; }
    result = ma_sound_init_from_file(&engine, "song.mp3", 0, NULL, NULL, &sound);
    if (result != MA_SUCCESS) { return result; }

    //  =======

    const int WIDTH = 800;
    const int HEIGHT = 450;

    SetTraceLogLevel(LOG_ERROR); // Hide logs in raylib init to console
    InitWindow(WIDTH, HEIGHT, "Rythme");
    SetTargetFPS(60);

    Rectangle play_btn = { WIDTH/2.0 - 50, HEIGHT/2.0 - 25, 100, 50 };
    bool play_btn_hover = 0;
    bool playing = 0;

    while(!WindowShouldClose()){
        Vector2 mouse_pos = GetMousePosition();
        play_btn_hover = CheckCollisionPointRec(mouse_pos, play_btn);

        if (play_btn_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if(!playing){
                ma_sound_start(&sound);
            }else{
                ma_sound_stop(&sound);
            }
            playing = !playing;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (play_btn_hover) {
            DrawRectangleRec(play_btn, LIGHTGRAY);
        } else {
            DrawRectangleRec(play_btn, GRAY);
        }
        
        DrawText("Play", play_btn.x + 25, play_btn.y + 15, 20, BLACK);

        EndDrawing();
    }

    ma_engine_uninit(&engine);
    CloseWindow();

    return 0;
}
