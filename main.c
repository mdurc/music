
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MINIAUDIO_IMPLEMENTATION
#include "scene_manager.h"

ma_engine engine;

int hash_f(char* file_name){
    int i, hash = 7;
    for(i=0; i<256 && file_name[i]!='\0'; ++i){
        hash = abs(hash*31 + tolower((unsigned char)file_name[i]));
    }
    return hash % MAX_SONGS;
}


void add(Node* songbook[MAX_SONGS], SoundMeta* sound){
    int hash = hash_f(sound->file_name);
    printf("Hash: %d\n", hash);

    // add it to the front of the ll
    Node* temp = malloc(sizeof(Node));
    temp->meta = sound;
    temp->next = songbook[hash];
    songbook[hash] = temp;
}


SoundMeta* find(Node* songbook[MAX_SONGS], char* file_name){
    int hash = hash_f(file_name);
    Node* temp = songbook[hash];
    while(temp){
        if(strcasecmp(temp->meta->file_name, file_name)==0){
            return temp->meta;
        }
        temp = temp->next;
    }
    return NULL;
}


void format_file_size(off_t size, char* buffer) {
    if (size >= 1024 * 1024) {
        snprintf(buffer, 20, "%.2f MB", (double)size / (1024 * 1024));
    } else if (size >= 1024) {
        snprintf(buffer, 20, "%.2f KB", (double)size / 1024);
    } else {
        snprintf(buffer, 20, "%lld B", (long long)size);
    }
}

void parse_sound(const char* filepath, const char* filename, SoundMeta* sound, ma_engine* engine) {
    struct stat file_stat;

    if (stat(filepath, &file_stat) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if (ma_sound_init_from_file(engine, filepath, 0, NULL, NULL, &sound->audio) != MA_SUCCESS) {
        fprintf(stderr, "Sound failed: %s\n", filepath);
        exit(EXIT_FAILURE);
    }
    ma_sound_get_length_in_seconds(&sound->audio, &sound->duration);

    format_file_size(file_stat.st_size, sound->file_size);

    strncpy(sound->file_name, filename, sizeof(sound->file_name) - 1);
    sound->file_name[sizeof(sound->file_name) - 1] = '\0';

    sound->favorite = 0;
    sound->initialized = 1;
}


bool btn_pressed(Vector2 mouse_pos, Rectangle* btn){
    return CheckCollisionPointRec(mouse_pos, *btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}



void delete_list(Node* n){
    if(!n) return;
    delete_list(n->next);
    free(n->meta);
    free(n);
}
void clear_mem(Node* songbook[MAX_SONGS]){
    int i;
    for(i=0;i<MAX_SONGS;++i){
        delete_list(songbook[i]);
        songbook[i] = NULL;
    }
}


void load_songs_from_directory( const char* dir_path, Node* songbook[MAX_SONGS],
                                ma_engine* engine, int* song_count) {
    DIR *dir;
    struct dirent *entry;
    char file_path[1035];
    char filename[256];
    char* file_name_start;
    
    if ((dir = opendir(dir_path)) == NULL) {
        perror("opendir");
        return;
    }
    int i = 0;

    while ((entry = readdir(dir)) != NULL) {
        // find regular files, mp3
        if (entry->d_type == DT_REG) {
            ++i;

            snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

            file_name_start = strrchr(file_path, '/');
            if (file_name_start) file_name_start++;
            else file_name_start = file_path;

            strncpy(filename, file_name_start, sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0';

            if (strlen(filename) > MAX_FNAME_LEN) {
                strncpy(filename + MAX_FNAME_LEN - 3, "...", 4);
            }

            if(find(songbook, filename) == NULL){
                printf("Adding new song #%d: %s, ",i, filename);
                SoundMeta* new_song = malloc(sizeof(SoundMeta));
                if (new_song == NULL) { perror("malloc"); continue; }

                parse_sound(file_path, filename, new_song, engine);
                add(songbook, new_song);
                ++(*song_count);
            }
        }
    }

    closedir(dir);
}


void reload_music_dir(int* song_count, Node* songbook[MAX_SONGS]){
    printf("SONG COUNT: %d\n", *song_count);
    load_songs_from_directory("music/", songbook, &engine, song_count);
    printf("NEW SONG COUNT: %d\n", *song_count);
}

int main(){

    int i;
    Node* songbook[MAX_SONGS];
    Playlist* playlists[MAX_PLAYLISTS];
    for(i=0; i<MAX_SONGS; ++i){
        songbook[i] = NULL;
        if(i<MAX_PLAYLISTS) playlists[i] = NULL;
    }


    const int WIDTH = 800;
    const int HEIGHT = 450;
    float progress=0.0f;
    bool playing = 0;
    bool dragging_scrubber = 0;
    int song_count=0;
    unsigned int sample_rate;


    Rectangle playback_line = { 100, HEIGHT - 50, WIDTH - 200, 10 };
    Vector2 play_btn_center = {WIDTH/2.0f, HEIGHT-80};
    float play_btn_radius = 15.0f;

    Vector2 mouse_pos;

    //  =======
    // MINIAUDIO_IMPLEMENTATION
    SoundMeta* current_song = malloc(sizeof(SoundMeta));
    current_song->initialized = 0;
    ma_result result;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) { 
        fprintf(stderr, "Engine failed");
        exit(EXIT_FAILURE);
     }

    //  =======
    // Opengl Context
    SetTraceLogLevel(LOG_ERROR); // Hide logs in raylib init to console
    InitWindow(WIDTH, HEIGHT, "Rythme");
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetTargetFPS(60);
    Font font = LoadFont("/Users/mdurcan/Library/Fonts/UbuntuMono-B.ttf");


    sample_rate = engine.sampleRate;

    reload_music_dir(&song_count, songbook);

    init_scrn_manager(WIDTH, HEIGHT, &song_count);

    while (!WindowShouldClose()) {
        mouse_pos = GetMousePosition();

        handle_audio(mouse_pos, play_btn_center, play_btn_radius, &playback_line,
                &playing, &progress, &dragging_scrubber, current_song, sample_rate);

        update_scrn_manager(mouse_pos);

        BeginDrawing();
        ClearBackground((Color){20, 20, 20, 255});

        draw_scene(&font, songbook, mouse_pos, &current_song, &playing);
        draw_scrub_player(&font, mouse_pos, play_btn_center, play_btn_radius, &playback_line, progress, playing, current_song);

        EndDrawing();
    }

    clear_mem(songbook);
    ma_engine_uninit(&engine);
    CloseWindow();

    return 0;
}

