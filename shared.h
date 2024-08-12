
#ifndef SHARED_H
#define SHARED_H
#include <raylib.h>
#include "libs/miniaudio.h"


#define MAX_PLAYLISTS 10
#define MAX_SONGS 1000
#define MAX_FNAME_LEN 16

typedef struct{
    // Meta data
    char file_name[256];
    char file_size[64];
    float duration;
    bool favorite;
    bool initialized;

    ma_sound audio;
} SoundMeta;



typedef struct {
    char name[MAX_FNAME_LEN];
    char* song_file_names[MAX_SONGS]; // Array of file names or indices
    int song_count;
} Playlist;


// For hashtable of songs
typedef struct Node {
    SoundMeta* meta;
    struct Node* next; // for collisions, a linked list
} Node;


int hash_f(char* file_name);
void add(Node* songbook[MAX_SONGS], SoundMeta* sound);
SoundMeta* find(Node* songbook[MAX_SONGS], char* file_name);
void parse_sound(const char* filepath, const char* filename, SoundMeta* sound, ma_engine* engine);


void reload_music_dir(int* song_count, Node* songbook[MAX_SONGS]);


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
