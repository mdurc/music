
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#define MINIAUDIO_IMPLEMENTATION
#include "libs/miniaudio.h"


Font font;

typedef struct{
    // Meta data
    char file_name[256];
    char mod_date[256];
    char file_size[64];
    //float sample_rate; // in Hz
    float duration;

    ma_sound audio;
} sound_meta;

void parse_sound(const char* filepath, sound_meta* sound, ma_engine* engine) {
    FILE *fp;
    char path[1035];
    char command[512];

    snprintf(command, sizeof(command), "exiftool -FileSize -FileModifyDate -FileName %s", filepath);

    fp = popen(command, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }

    if (ma_sound_init_from_file(engine, filepath, 0, NULL, NULL, &sound->audio)!= MA_SUCCESS) { 
        fprintf(stderr, "Sound failed\n");
        exit(EXIT_FAILURE);
    }
    ma_sound_get_length_in_seconds(&sound->audio, &sound->duration);

    // defaults
    strcpy(sound->file_size, "0.0 MB");
    strcpy(sound->file_name, "Unknown");
    strcpy(sound->mod_date, "Unknown");

    if (fgets(path, sizeof(path)-1, fp)) sscanf(path, "File Size : %[^\n]", sound->file_size);
    if (fgets(path, sizeof(path)-1, fp)) sscanf(path, "File Modification Date/Time : %[^ ]", sound->mod_date);
    if (fgets(path, sizeof(path)-1, fp)) sscanf(path, "File Name : %[^\n]", sound->file_name);

    pclose(fp);
}


// TODO: Hashtable of songs by artist
bool btn_pressed(Vector2 mouse_pos, Rectangle* btn){
    return CheckCollisionPointRec(mouse_pos, *btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}


void draw_scrub_player(Vector2 mouse_pos, Vector2 play_btn_center, float play_btn_radius, Rectangle* playback_line, float progress, bool playing, sound_meta* sound){
    char time_stamps[10];
    char* play_text;
    Vector2 text_size;
    if(CheckCollisionPointCircle(mouse_pos, play_btn_center, play_btn_radius)){
        DrawCircleV(play_btn_center, play_btn_radius, LIGHTGRAY);
    }else{
        DrawCircleV(play_btn_center, play_btn_radius, GRAY);
    }
    play_text = !playing ? ">" : "~~";
    text_size = MeasureTextEx(font, play_text, 20, 1);
    DrawTextEx(font, play_text, (Vector2){play_btn_center.x - text_size.x/2.0f, play_btn_center.y - text_size.y/2.0f}, 20, 1, BLACK);

    text_size = MeasureTextEx(font, "<-", 20, 1);
    DrawTextEx(font, "<-", (Vector2){play_btn_center.x - play_btn_radius*2 - text_size.x, play_btn_center.y - text_size.y/2.0f}, 20, 1, WHITE);
    DrawTextEx(font, "->", (Vector2){play_btn_center.x + play_btn_radius*2, play_btn_center.y - text_size.y/2.0f}, 20, 1, WHITE);

    DrawRectangleRounded(*playback_line, 0.5f, 8, GRAY);
    DrawRectangleRounded((Rectangle){playback_line->x, playback_line->y, playback_line->width * progress, playback_line->height}, 0.5f, 8, WHITE);


    snprintf(time_stamps, sizeof(time_stamps), "%d:%02d", (int)(progress*sound->duration)/60, (int)(progress*sound->duration) % 60);
    DrawTextEx(font, time_stamps, (Vector2){playback_line->x - 50, playback_line->y - 5}, 20, 1, WHITE);
    snprintf(time_stamps, sizeof(time_stamps), "%d:%02d", (int)sound->duration/60, (int)sound->duration % 60);
    DrawTextEx(font, time_stamps, (Vector2){playback_line->x + playback_line->width + 10, playback_line->y - 5}, 20, 1, WHITE);

    DrawTextEx(font, TextFormat("%s", sound->file_name), (Vector2){playback_line->x+10, playback_line->y-30}, 20, 1, WHITE);
    //DrawTextEx(font, TextFormat("%s", sound->file_size), (Vector2){playback_line->x+130, playback_line->y-30}, 20, 1, WHITE);
    DrawTextEx(font, TextFormat("%s", sound->mod_date), (Vector2){playback_line->width-20, playback_line->y-30}, 20, 1, WHITE);
}

void check_play_btn_pressed(Vector2 mouse_pos, Vector2 play_btn_center, float play_btn_radius, bool* playing, ma_sound* aud){
    if(CheckCollisionPointCircle(mouse_pos, play_btn_center, play_btn_radius) &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
        if (!(*playing)) {
            ma_sound_start(aud);
        } else {
            ma_sound_stop(aud);
        }
        *playing = !(*playing);
    }
}

void check_adjust_scrubber(Vector2 mouse_pos, Rectangle* playback_line, bool* dragging_scrubber,
            bool playing, sound_meta* sound, unsigned int sample_rate){
    float new_time;
    if (*dragging_scrubber || (btn_pressed(mouse_pos,playback_line))) {
        if(ma_sound_is_playing(&sound->audio)) ma_sound_stop(&sound->audio);
        new_time = (mouse_pos.x - playback_line->x) / playback_line->width * sound->duration;
        ma_sound_seek_to_pcm_frame(&sound->audio, fmin(new_time*sample_rate, sound->duration*sample_rate));
        *dragging_scrubber = 1;
    }
    if (*dragging_scrubber && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        *dragging_scrubber = 0;
        if(playing){
            usleep(400000); // let the audio sync up if the scrubber was pressed and released quickly
            ma_sound_start(&sound->audio);
        }
    }

}


void handle_audio(Vector2 mouse_pos, Vector2 play_btn_center, float play_btn_radius,
        Rectangle* playback_line, bool* playing, float* progress, bool* dragging_scrubber, sound_meta* sound, unsigned int sample_rate){

    check_play_btn_pressed(mouse_pos, play_btn_center, play_btn_radius, playing, &sound->audio);

    check_adjust_scrubber(mouse_pos, playback_line, dragging_scrubber,
            *playing, sound, sample_rate);

    // update the progress
    ma_sound_get_cursor_in_seconds(&sound->audio, progress);
    *progress = *progress / sound->duration;
    
    // TODO: check if progress is complete
}

int main(int argc, char** argv){
    if(argc<2){
        fprintf(stderr, "Usage: ./a.out <song>\n");
        exit(EXIT_FAILURE);
    }

    const int WIDTH = 800;
    const int HEIGHT = 450;
    float new_time, progress;
    bool playing = 0; // there is ma_sound_is_playing(&sound);
    bool dragging_scrubber = 0;

    Vector2 mouse_pos;

    Rectangle playback_line = { 100, HEIGHT - 50, WIDTH - 200, 10 };
    Vector2 play_btn_center = {WIDTH/2.0f, HEIGHT-80};
    float play_btn_radius = 15.0f;


    //  =======
    // MINIAUDIO_IMPLEMENTATION
    ma_result result;
    ma_engine engine;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) { 
        fprintf(stderr, "Engine failed");
        exit(EXIT_FAILURE);
     }

    //  =======

    SetTraceLogLevel(LOG_ERROR); // Hide logs in raylib init to console
    InitWindow(WIDTH, HEIGHT, "Rythme");
    SetTargetFPS(60);
    font = LoadFont("/Users/mdurcan/Library/Fonts/UbuntuMono-B.ttf");

    sound_meta* sound = malloc(sizeof(sound_meta));
    parse_sound(argv[1], sound, &engine);
    unsigned int sample_rate = engine.sampleRate;

    while (!WindowShouldClose()) {
        mouse_pos = GetMousePosition();

        handle_audio(mouse_pos, play_btn_center, play_btn_radius, &playback_line,
                &playing, &progress, &dragging_scrubber, sound, sample_rate);

        BeginDrawing();
        ClearBackground((Color){20, 20, 20, 255});

        draw_scrub_player(mouse_pos, play_btn_center, play_btn_radius, &playback_line, progress, playing, sound);

        EndDrawing();
    }

    ma_engine_uninit(&engine);
    CloseWindow();

    return 0;
}

