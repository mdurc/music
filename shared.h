
#ifndef SHARED_H
#define SHARED_H
#include <raylib.h>


// MAKE SURE THAT THIS IS WHERE YOURE STORING DATA
#include "/Users/mdurcan/personal/git_projects/rythme/libs/miniaudio.h"
#define MUSIC_DIR "/Users/mdurcan/personal/git_projects/rythme/music/"
#define PLAYLIST_FILE "/Users/mdurcan/personal/git_projects/rythme/playlists.txt"


#define MAX_PLAYLISTS 6 // Will only actually allow MAX_PLAYLISTS-1 playlists bc index 0 is a placeholder
#define MAX_PLAYNAME 9
#define MAX_SONGS 1000
#define MAX_FNAME_LEN 16
#define ORIG_WIDTH 950
#define ORIG_HEIGHT 550

typedef struct{
    // Meta data
    char file_name[MAX_FNAME_LEN+1]; // for null term
    char file_size[64];
    float duration;
    bool favorite;

    ma_sound audio;
} SoundMeta;


typedef struct {
    SoundMeta* songs[MAX_SONGS];
    SoundMeta* current_song; // points to a song inside of songs[MAX_SONGS]
    int size, playlist; // both should start at 0
    bool playing;
} AllSongs;

typedef struct {
    char name[MAX_PLAYNAME + 1]; // for null term
    char song_names[MAX_SONGS][MAX_FNAME_LEN+1];
    int size;

    // for if we want the playlist to be every single song downloaded,
    // we wont have to add every song name into the song_names
    bool every_song;
} Playlist;


bool strMatch(char* a, char* b);

int find(AllSongs* songbook, char* file_name);
void parse_sound(const char* filepath, const char* filename, SoundMeta* sound, ma_engine* engine);

void remove_from_queue(AllSongs* queue, int i);
void remove_playlist(Playlist* playlist[MAX_PLAYLISTS], int i);

void reload_music_dir(AllSongs* songbook, Playlist* all_songs);


bool btn_pressed(Vector2 mouse_pos, Rectangle* btn);

void draw_scrub_player(Font* font, Vector2 mouse_pos, Vector2 play_btn_center,
        float play_btn_radius, Vector2 left_arrow_pos, Vector2 right_arrow_pos, Rectangle* playback_line, float progress,
        bool playing, SoundMeta* sound);
void check_adjust_scrubber(Vector2 mouse_pos, Rectangle* playback_line, bool* dragging_scrubber,
            bool playing, SoundMeta* sound, unsigned int sample_rate);
void handle_audio(Vector2 mouse_pos, Vector2 play_btn_center, float play_btn_radius, Vector2 left_arrow_pos, Vector2 right_arrow_pos,
        Rectangle* playback_line, bool* playing, float* progress,
        bool* dragging_scrubber, SoundMeta* sound, unsigned int sample_rate);

#endif
