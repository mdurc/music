
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MINIAUDIO_IMPLEMENTATION
#include "scene_manager.h"

ma_engine engine;


SoundMeta* find(AllSongs* songbook, char* file_name){
    int i;
    SoundMeta* temp;
    for(i=0;i<songbook->size;++i){
        if(strcasecmp(songbook->songs[i]->file_name, file_name) == 0 &&
    strlen(songbook->songs[i]->file_name) == strlen(file_name)){
            return songbook->songs[i];
        }
    }
    return NULL;
}


void remove_from_queue(Queue* queue, int i){
    queue->songs[i] = NULL;
    for(;i<queue->size-1;++i){
        queue->songs[i] = queue->songs[i+1];
    }
    --queue->size;
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
}


// TODO: is this necessary
bool btn_pressed(Vector2 mouse_pos, Rectangle* btn){
    return CheckCollisionPointRec(mouse_pos, *btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}



// TODO: do this
void clear_mem(AllSongs* songbook){
    int i;
    for(i=0;i<MAX_SONGS;++i){
        //free(songbook[i]);
    }
}


void load_songs_from_directory( const char* dir_path, AllSongs* songbook,
                                ma_engine* engine) {
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
                printf("Adding new song #%d: %s\n",i, filename);
                SoundMeta* new_song = malloc(sizeof(SoundMeta));
                if (new_song == NULL) { perror("malloc"); continue; }

                parse_sound(file_path, filename, new_song, engine);
                songbook->songs[songbook->size] = new_song;
                ++songbook->size;
            }
        }
    }

    closedir(dir);
}


void reload_music_dir(AllSongs* songbook){
    printf("SONG COUNT: %d\n", songbook->size);
    load_songs_from_directory("music/", songbook, &engine);
    printf("NEW SONG COUNT: %d\n", songbook->size);
}

int main(){

    int i;
    AllSongs songbook;
    Queue queue;
    Playlist* playlists[MAX_PLAYLISTS];


    const int WIDTH = 800;
    const int HEIGHT = 450;
    float progress=0.0f;
    bool dragging_scrubber = 0;
    unsigned int sample_rate;


    Rectangle playback_line = { 100, HEIGHT - 50, WIDTH - 200, 10 };
    Vector2 play_btn_center = {WIDTH/2.0f, HEIGHT-80};
    float play_btn_radius = 15.0f;

    Vector2 mouse_pos;

    //  =======
    // MINIAUDIO_IMPLEMENTATION
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

    // INITILIZE
    for(i=0; i<MAX_SONGS; ++i){
        songbook.songs[i] = NULL;
        if(i<MAX_QUEUE) queue.songs[i] = NULL;
        if(i<MAX_PLAYLISTS) playlists[i] = NULL;
    }
    songbook.size = 0;
    songbook.current_song = NULL;
    songbook.playing = false;
    queue.size = 0;
    reload_music_dir(&songbook);

    init_scrn_manager(WIDTH, HEIGHT);



    while (!WindowShouldClose()) {
        mouse_pos = GetMousePosition();

        handle_audio(mouse_pos, play_btn_center, play_btn_radius, &playback_line,
                &songbook.playing, &progress, &dragging_scrubber, songbook.current_song, sample_rate);

        update_scrn_manager(mouse_pos);

        BeginDrawing();
        ClearBackground((Color){20, 20, 20, 255});

        draw_scene(&font, &songbook, &queue, mouse_pos);
        draw_scrub_player(&font, mouse_pos, play_btn_center, play_btn_radius, &playback_line, progress, songbook.playing, songbook.current_song);

        EndDrawing();
    }

    clear_mem(&songbook);
    ma_engine_uninit(&engine);
    CloseWindow();

    return 0;
}

