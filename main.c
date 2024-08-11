
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>


#include "scrubber.c"
#include "shared.h"


int hash_f(char* file_name){
    int i, hash = 7;
    for(i=0; i<256 && file_name[i]!='\0'; ++i){
        hash = abs(hash*31 + file_name[i]);
    }
    return hash % MAX_SONGS;
}


void add(Node* songbook[MAX_SONGS], sound_meta* sound){
    int hash = hash_f(sound->file_name);
    printf("Hash: %d\n", hash);

    // add it to the front of the ll
    Node* temp = malloc(sizeof(Node));
    temp->meta = sound;
    temp->next = songbook[hash];
    songbook[hash] = temp;
}


sound_meta* find(Node* songbook[MAX_SONGS], char* file_name){
    int hash = hash_f(file_name);
    Node* temp = songbook[hash];
    while(temp){
        if(strcmp(temp->meta->file_name, file_name)==0){
            return temp->meta;
        }
        temp = temp->next;
    }
    return NULL;
}


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


void load_songs_from_directory(const char* dir_path, Node* songbook[MAX_SONGS], ma_engine* engine) {
    DIR *dir;
    struct dirent *entry;
    char file_path[256];
    
    if ((dir = opendir(dir_path)) == NULL) {
        perror("opendir");
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // find regular files, mp3
        if (entry->d_type == DT_REG) {

            snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);
                sound_meta *new_song = malloc(sizeof(sound_meta));
                if (new_song == NULL) {
                    perror("malloc");
                    continue;
                }
                
                parse_sound(file_path, new_song, engine);
                printf("Added song: %s, ", new_song->file_name);
                add(songbook, new_song);
        }
    }
    
    closedir(dir);
}

int main(){

    int i;
    Node* songbook[MAX_SONGS];
    for(i=0; i<MAX_SONGS; ++i){
        songbook[i] = NULL;
    }

    const int WIDTH = 800;
    const int HEIGHT = 450;
    float new_time, progress;
    bool playing = 0;
    bool dragging_scrubber = 0;
    unsigned int sample_rate;


    Rectangle playback_line = { 100, HEIGHT - 50, WIDTH - 200, 10 };
    Vector2 play_btn_center = {WIDTH/2.0f, HEIGHT-80};
    float play_btn_radius = 15.0f;

    Vector2 mouse_pos;

    //  =======
    // MINIAUDIO_IMPLEMENTATION
    sound_meta* current_song;
    ma_result result;
    ma_engine engine;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) { 
        fprintf(stderr, "Engine failed");
        exit(EXIT_FAILURE);
     }

    //  =======
    // Opengl Context
    SetTraceLogLevel(LOG_ERROR); // Hide logs in raylib init to console
    InitWindow(WIDTH, HEIGHT, "Rythme");
    SetTargetFPS(60);
    Font font = LoadFont("/Users/mdurcan/Library/Fonts/UbuntuMono-B.ttf");

    load_songs_from_directory("music/", songbook, &engine);
    current_song = find(songbook, "Lemon.mp3");

    sample_rate = engine.sampleRate;

    while (!WindowShouldClose()) {
        mouse_pos = GetMousePosition();

        handle_audio(mouse_pos, play_btn_center, play_btn_radius, &playback_line,
                &playing, &progress, &dragging_scrubber, current_song, sample_rate);

        BeginDrawing();
        ClearBackground((Color){20, 20, 20, 255});

        draw_scrub_player(&font, mouse_pos, play_btn_center, play_btn_radius, &playback_line, progress, playing, current_song);

        EndDrawing();
    }

    clear_mem(songbook);
    ma_engine_uninit(&engine);
    CloseWindow();

    return 0;
}

