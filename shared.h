
#ifndef SHARED_H
#define SHARED_H
#include <raylib.h>
#include "libs/miniaudio.h"


#define MAX_PLAYLISTS 10
#define MAX_QUEUE 10
#define MAX_SONGS 1000
#define MAX_FNAME_LEN 16

typedef struct{
    // Meta data
    char file_name[256];
    char file_size[64];
    float duration;
    bool favorite;

    ma_sound audio;
} SoundMeta;


typedef struct {
    SoundMeta* songs[MAX_SONGS];
    SoundMeta* current_song; // points to a song inside of songs[MAX_SONGS]
    int size;
    bool playing;
} AllSongs;

typedef struct {
    SoundMeta* songs[MAX_QUEUE];
    int size;
} Queue;

typedef struct {
    char name[MAX_FNAME_LEN];
    SoundMeta* songs[MAX_SONGS];
    int size;
} Playlist;



SoundMeta* find(AllSongs* songbook, char* file_name);
void parse_sound(const char* filepath, const char* filename, SoundMeta* sound, ma_engine* engine);

void remove_from_queue(Queue* queue, int i);

void reload_music_dir(AllSongs* songbook);


bool btn_pressed(Vector2 mouse_pos, Rectangle* btn);

void draw_scrub_player(Font* font, Vector2 mouse_pos, Vector2 play_btn_center,
        float play_btn_radius, Rectangle* playback_line, float progress,
        bool playing, SoundMeta* sound);
void check_play_btn_pressed(Vector2 mouse_pos, Vector2 play_btn_center,
        float play_btn_radius, bool* playing, ma_sound* aud);
void check_adjust_scrubber(Vector2 mouse_pos, Rectangle* playback_line, bool* dragging_scrubber,
            bool playing, SoundMeta* sound, unsigned int sample_rate);
void handle_audio(Vector2 mouse_pos, Vector2 play_btn_center, float play_btn_radius,
        Rectangle* playback_line, bool* playing, float* progress,
        bool* dragging_scrubber, SoundMeta* sound, unsigned int sample_rate);

#endif
