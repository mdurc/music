
#ifndef SHARED_H
#define SHARED_H
#include <raylib.h>
#define MINIAUDIO_IMPLEMENTATION
#include "libs/miniaudio.h"

typedef struct{
    // Meta data
    char file_name[256];
    char mod_date[256];
    char file_size[64];
    //float sample_rate; // in Hz
    float duration;

    ma_sound audio;
} sound_meta;


// For hashtable of songs
typedef struct Node {
    sound_meta* meta;
    struct Node* next; // for collisions, a linked list
} Node;


#define MAX_SONGS 1000
int hash_f(char* file_name);
void add(Node* songbook[MAX_SONGS], sound_meta* sound);
sound_meta* find(Node* songbook[MAX_SONGS], char* file_name);
void parse_sound(const char* filepath, sound_meta* sound, ma_engine* engine);

inline bool btn_pressed(Vector2 mouse_pos, Rectangle* btn){ return CheckCollisionPointRec(mouse_pos, *btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON); }


void draw_scrub_player(Font* font, Vector2 mouse_pos, Vector2 play_btn_center,
        float play_btn_radius, Rectangle* playback_line, float progress,
        bool playing, sound_meta* sound);
void check_play_btn_pressed(Vector2 mouse_pos, Vector2 play_btn_center,
        float play_btn_radius, bool* playing, ma_sound* aud);
void check_adjust_scrubber(Vector2 mouse_pos, Rectangle* playback_line, bool* dragging_scrubber,
            bool playing, sound_meta* sound, unsigned int sample_rate);
void handle_audio(Vector2 mouse_pos, Vector2 play_btn_center, float play_btn_radius,
        Rectangle* playback_line, bool* playing, float* progress,
        bool* dragging_scrubber, sound_meta* sound, unsigned int sample_rate);

#endif
