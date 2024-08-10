
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#define MINIAUDIO_IMPLEMENTATION
#include "libs/miniaudio.h"


typedef struct {
    float duration;
    char title[128];
    char artist[128];
} meta_data;

void parse_sound(ma_sound* sound, meta_data* meta) {
    ma_sound_get_length_in_seconds(sound, &meta->duration);
    snprintf(meta->title, sizeof(meta->title), "Unknown Title");
    snprintf(meta->artist, sizeof(meta->artist), "Unknown Artist");
}

int main(int argc, char** argv){
    if(argc<2){
        fprintf(stderr, "Usage: ./a.out <song>\n");
        exit(EXIT_FAILURE);
    }

    const int WIDTH = 800;
    const int HEIGHT = 450;
    float current_time, new_time, progress;
    bool play_btn_hover = 0;
    bool playing = 0; // there is ma_sound_is_playing(&sound);
    bool dragging_scrubber = 0;

    Vector2 mouse_pos;

    Rectangle playback_line = { 100, HEIGHT - 50, WIDTH - 200, 10 };
    Rectangle play_btn = { WIDTH/2.0 - 50, HEIGHT/2.0 - 25, 100, 50 };

    meta_data meta;

    //  =======
    // MINIAUDIO_IMPLEMENTATION
    ma_result result;
    ma_engine engine;
    ma_sound sound;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) { 
        fprintf(stderr, "Engine failed");
        exit(EXIT_FAILURE);
     }
    result = ma_sound_init_from_file(&engine, argv[1], 0, NULL, NULL, &sound);
    if (result != MA_SUCCESS) { 
        fprintf(stderr, "Sound failed\n");
        exit(EXIT_FAILURE);
     }

    //  =======


    SetTraceLogLevel(LOG_ERROR); // Hide logs in raylib init to console
    InitWindow(WIDTH, HEIGHT, "Rythme");
    SetTargetFPS(60);

    parse_sound(&sound, &meta);

    while (!WindowShouldClose()) {
        mouse_pos = GetMousePosition();
        play_btn_hover = CheckCollisionPointRec(mouse_pos, play_btn);

        if (play_btn_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (!playing) {
                ma_sound_start(&sound);
            } else {
                ma_sound_stop(&sound);
            }
            playing = !playing;
        }

        if (dragging_scrubber || (CheckCollisionPointRec(mouse_pos, playback_line) &&
                                  IsMouseButtonPressed(MOUSE_LEFT_BUTTON))) {
            if(ma_sound_is_playing(&sound)) ma_sound_stop(&sound);
            new_time = (mouse_pos.x - playback_line.x) / playback_line.width * meta.duration;
            ma_sound_seek_to_pcm_frame(&sound, fmin(new_time*engine.sampleRate, meta.duration*engine.sampleRate));
            dragging_scrubber = 1;
        }
        if ( dragging_scrubber && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            dragging_scrubber = 0;
            if(playing) ma_sound_start(&sound);
        }

        ma_sound_get_cursor_in_seconds(&sound, &current_time);
        progress = current_time / meta.duration;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (play_btn_hover) {
            DrawRectangleRec(play_btn, LIGHTGRAY);
        } else {
            DrawRectangleRec(play_btn, GRAY);
        }
        
        DrawText(!playing? "Play": "Pause", play_btn.x + 25, play_btn.y + 15, 20, BLACK);

        DrawRectangleRec(playback_line, DARKGRAY);
        DrawRectangle(playback_line.x, playback_line.y - 5, playback_line.width * progress, playback_line.height + 10, SKYBLUE);

        DrawText(TextFormat("Title: %s", meta.title), 10, 10, 20, BLACK);
        DrawText(TextFormat("Artist: %s", meta.artist), 10, 40, 20, BLACK);
        DrawText(TextFormat("Time: %.2f / %.2f", current_time, meta.duration), 10, 70, 20, BLACK);

        EndDrawing();
    }

    ma_engine_uninit(&engine);
    CloseWindow();

    return 0;
}
