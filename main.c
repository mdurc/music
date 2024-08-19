
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MINIAUDIO_IMPLEMENTATION
#include "scene_manager.h"

ma_engine engine;

bool strMatch(char* a, char* b){ return strcmp(a,b)==0 && strlen(a)==strlen(b); }

// return index in songbook
int find(AllSongs* songbook, char* file_name){
    int i;
    for(i=0;i<songbook->size;++i){
        if(strcasecmp(songbook->songs[i]->file_name, file_name) == 0 &&
    strlen(songbook->songs[i]->file_name) == strlen(file_name)){
            return i;
        }
    }
    return -1;
}


void remove_from_queue(AllSongs* queue, int i){
    queue->songs[i] = NULL;
    for(;i<queue->size-1;++i){
        queue->songs[i] = queue->songs[i+1];
    }
    queue->songs[queue->size-1] = NULL;
    --queue->size;
}


void remove_playlist(Playlist* playlist[MAX_PLAYLISTS], int i){
    free(playlist[i]);
    playlist[i] = NULL;
    for(;i<MAX_PLAYLISTS-1;++i){
        playlist[i] = playlist[i+1];
    }
    playlist[MAX_PLAYLISTS-1] = NULL;
}


void format_file_size(off_t size, char* buf) {
    if (size >= 1024 * 1024) {
        snprintf(buf, 20, "%.2f MB", (double)size / (1024 * 1024));
    } else if (size >= 1024) {
        snprintf(buf, 20, "%.2f KB", (double)size / 1024);
    } else {
        snprintf(buf, 20, "%lld B", (long long)size);
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

    assert(sizeof(sound->file_name) >= sizeof(filename));
    strcpy(sound->file_name, filename);
    sound->file_name[sizeof(sound->file_name) - 1] = '\0';

    sound->favorite = 0;
}


bool btn_pressed(Vector2 mouse_pos, Rectangle* btn){
    return CheckCollisionPointRec(mouse_pos, *btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}



void clear_mem(AllSongs* songbook, Playlist* playlists[MAX_PLAYLISTS]){
    int i;
    for(i=0;i<songbook->size;++i){
        ma_sound_uninit(&songbook->songs[i]->audio);
        free(songbook->songs[i]);
        songbook->songs[i] = NULL;
    }
    for(i=0;i<MAX_PLAYLISTS && playlists[i]!=NULL;++i){
        free(playlists[i]);
        playlists[i] = NULL;
    }
}


void load_songs_from_directory( const char* dir_path, AllSongs* songbook,
                                ma_engine* engine, Playlist* all_songs) {
    DIR *dir;
    struct dirent *entry;
    char file_path[1035];
    char filename[MAX_FNAME_LEN+1];
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

            strncpy(filename, file_name_start, MAX_FNAME_LEN);
            filename[sizeof(filename) - 1] = '\0'; // should be room: MAX_FNAME_LEN + 1 is the size

            if(find(songbook, filename) == -1){
                printf("Adding new song #%d: %s\n",i, filename);
                songbook->songs[songbook->size] = malloc(sizeof(SoundMeta));
                if (songbook->songs[songbook->size] == NULL) { perror("malloc"); continue; }

                parse_sound(file_path, filename, songbook->songs[songbook->size], engine);
                ++songbook->size;

                assert(sizeof(all_songs->song_names[all_songs->size]) >= sizeof(filename));
                strcpy(all_songs->song_names[all_songs->size], filename);
                ++all_songs->size;
            }
        }
    }

    closedir(dir);
}


void reload_music_dir(AllSongs* songbook, Playlist* all_songs){
    printf("SONG COUNT: %d\n", songbook->size);
    load_songs_from_directory(MUSIC_DIR, songbook, &engine, all_songs);
    printf("NEW SONG COUNT: %d\n", songbook->size);
}


void next_in_queue(AllSongs* songbook, AllSongs* queue, Playlist* playlists[MAX_PLAYLISTS]){
    if(queue->size > 0){
        printf("Playing next song in queue: %s\n", queue->songs[0]->file_name);

        if(songbook->current_song!=NULL && ma_sound_is_playing(&songbook->current_song->audio)) ma_sound_stop(&songbook->current_song->audio);
        songbook->current_song = queue->songs[0];

        ma_sound_seek_to_pcm_frame(&songbook->current_song->audio, 0);
        if(songbook->playing){ usleep(400000); ma_sound_start(&songbook->current_song->audio); }

        remove_from_queue(queue, 0);
    }else if(songbook->size > 0){
        if(songbook->current_song!=NULL && ma_sound_is_playing(&songbook->current_song->audio)) ma_sound_stop(&songbook->current_song->audio);

        // find random song in current playlist
        char* temp = playlists[songbook->playlist]->song_names[rand()%playlists[songbook->playlist]->size];
        int index = find(songbook, temp);
        if(index == -1){
            printf("Song is no longer downloaded. Removing: %s\n", temp);
            remove_song_from_playlist(playlists[songbook->playlist], temp);
            return;
        }
        songbook->current_song = songbook->songs[index];

        // restart song before playing
        ma_sound_seek_to_pcm_frame(&songbook->current_song->audio, 0);
        if(songbook->playing){ usleep(400000); ma_sound_start(&songbook->current_song->audio); }

        printf("Playing random song: %s\n", songbook->current_song->file_name);
    }else{
        printf("No songs to go to\n");
    }
}

void handle_arrows(Vector2 mouse_pos, Vector2 text_size, Vector2 left_arrow_pos, Vector2 right_arrow_pos, AllSongs* songbook, AllSongs* queue, Playlist* playlists[MAX_PLAYLISTS]){
    bool left_arrow_pressed, right_arrow_pressed;
    float new_time;

    left_arrow_pressed = (mouse_pos.x >= left_arrow_pos.x) && (mouse_pos.x <= left_arrow_pos.x + text_size.x) &&
                              (mouse_pos.y >= left_arrow_pos.y) && (mouse_pos.y <= left_arrow_pos.y + text_size.y);
    right_arrow_pressed = (mouse_pos.x >= right_arrow_pos.x) && (mouse_pos.x <= right_arrow_pos.x + text_size.x) &&
                               (mouse_pos.y >= right_arrow_pos.y) && (mouse_pos.y <= right_arrow_pos.y + text_size.y);
    if (left_arrow_pressed && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if(ma_sound_is_playing(&songbook->current_song->audio)) ma_sound_stop(&songbook->current_song->audio);
        ma_sound_seek_to_pcm_frame(&songbook->current_song->audio, 0.0f);
        if(songbook->playing){ usleep(400000); ma_sound_start(&songbook->current_song->audio); }
    }
    else if (right_arrow_pressed && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        next_in_queue(songbook, queue, playlists);
    }
}

void save_data(Playlist* playlists[MAX_PLAYLISTS]) {
    int i, j;
    FILE *file = fopen(PLAYLIST_FILE, "w");
    if (file == NULL) {
        perror("Failed to open file for writing");
        return;
    }

    // only save from index 2 on
    for(i=2; i<MAX_PLAYLISTS; i++){
        if (playlists[i] != NULL) {
            printf("Saving data of playlist: %s\n", playlists[i]->name);
            fprintf(file, "%s\n", playlists[i]->name);
            fprintf(file, "%d\n", playlists[i]->size);
            fprintf(file, "%d\n", playlists[i]->every_song);

            for(j=0; j<playlists[i]->size; ++j) {
                fprintf(file, "%s\n", playlists[i]->song_names[j]);
            }
            fprintf(file, "--\n");
        }
    }
    fclose(file);
}

void clean_playlists(Playlist* playlists[MAX_PLAYLISTS], AllSongs* songbook){
    int i, j;

    // start at the saved custom playlists, index 2
    for(i=2;i<MAX_PLAYLISTS && playlists[i]!=NULL; ++i){
        printf("Cleaning playlist: %s\n", playlists[i]->name);
        for(j=0;j<playlists[i]->size;){
            if(find(songbook, playlists[i]->song_names[j]) == -1){
                printf("Song is no longer downloaded. Removing: %s\n", playlists[i]->song_names[j]);
                remove_song_from_playlist(playlists[i], playlists[i]->song_names[j]);
            }else{
                ++j;
            }
        }
    }
}

void load_data(Playlist* playlists[MAX_PLAYLISTS]) {
    FILE *file = fopen(PLAYLIST_FILE, "r");
    if (file == NULL) {
        perror("Failed to open file for reading");
        return;
    }

    // buf will be used for both sizes
    assert(MAX_FNAME_LEN >= MAX_PLAYNAME);
    char buf[MAX_FNAME_LEN+2]; // one for newline and another for null


    // Note: start allocating for playlists at index 2.
    // Index 0 and 1 are saved for placeholder and All Songs.
    int i, playlist_ind = 2;
    size_t str_sz;

    while (fgets(buf, sizeof(buf), file) != NULL && playlist_ind < MAX_PLAYLISTS) {
        playlists[playlist_ind] = malloc(sizeof(Playlist));

        buf[strlen(buf)-1] = '\0'; // remove \n bc it counts as a part of strln
        str_sz = sizeof(playlists[playlist_ind]->name);
        strncpy(playlists[playlist_ind]->name, buf, str_sz-1);
        playlists[playlist_ind]->name[str_sz-1] = '\0';

        fgets(buf, sizeof(buf), file);
        playlists[playlist_ind]->size = atoi(buf);

        fgets(buf, sizeof(buf), file);
        playlists[playlist_ind]->every_song = atoi(buf);

        for(i=0; i<playlists[playlist_ind]->size; ++i) {
            fgets(buf, sizeof(buf), file);
            buf[strlen(buf)-1] = '\0'; // remove \n bc it counts as a part of strln
            str_sz = sizeof(playlists[playlist_ind]->song_names[i]);
            strncpy(playlists[playlist_ind]->song_names[i], buf, str_sz-1);
            playlists[playlist_ind]->song_names[i][str_sz-1] = '\0';
        }

        // read the separator : "--\n"
        fgets(buf, sizeof(buf), file);
        ++playlist_ind;
    }
    fclose(file);
}


int main(){
    srand(time(NULL));

    int i;
    // potentially put songbook and queue on heap
    AllSongs songbook = { .songs = {NULL}, .current_song = NULL, .size=0, .playlist=1, .playing=0 };
    AllSongs queue = { .songs = {NULL}, .current_song = NULL, .size=0, .playlist=0, .playing=0 };

    // start them all un-initialized. Only malloc once a new playlist is required.
    Playlist* playlists[MAX_PLAYLISTS] = {NULL};
    load_data(playlists);


    int WIDTH = ORIG_WIDTH;
    int HEIGHT = ORIG_HEIGHT;
    float progress=0.0f;
    bool dragging_scrubber = 0;
    unsigned int sample_rate;


    Rectangle playback_line = {.height = 10, .x = 100};
    Vector2 play_btn_center;
    float play_btn_radius = 15.0f;

    Vector2 mouse_pos;
    Vector2 prev_mouse = {0,0};
    Vector2 resize_start;
    Vector2 window_pos = {100, 100};
    bool drag_window = 0;

    //  =======
    // MINIAUDIO_IMPLEMENTATION
    ma_result result;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) { 
        fprintf(stderr, "Engine failed");
        exit(EXIT_FAILURE);
    }

    // =======
    // Opengl Context
    SetTraceLogLevel(LOG_ERROR); // Hide logs in raylib init to console
    InitWindow(WIDTH, HEIGHT, "Rythme");
    //SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    SetWindowPosition((int)window_pos.x, (int)window_pos.y);
    SetTargetFPS(60);
    SetExitKey(0);
    Font font = LoadFont("/Users/mdurcan/Library/Fonts/UbuntuMono-B.ttf");


    Vector2 text_size = MeasureTextEx(font, "->", 20, 1);
    Vector2 left_arrow_pos;

    Vector2 right_arrow_pos;


    sample_rate = engine.sampleRate;


    // playlists[0] is reserved for queue and match_list
    playlists[0] = malloc(sizeof(Playlist));
    playlists[1] = malloc(sizeof(Playlist));
    reload_music_dir(&songbook, playlists[1]);



    // Add a playlist with all your songs cannot be removed
    strcpy(playlists[1]->name, "All Songs");
    playlists[1]->every_song = 1;


    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D)) {
            break;
        }

        WIDTH = GetScreenWidth();
        HEIGHT = GetScreenHeight();
        if(HEIGHT > 800){
            HEIGHT = 800;
            SetWindowSize(WIDTH, HEIGHT);
        }else if(HEIGHT < 500){
            HEIGHT = 500;
            SetWindowSize(WIDTH, HEIGHT);
        }else if(WIDTH < ORIG_WIDTH){
            WIDTH = ORIG_WIDTH;
            SetWindowSize(WIDTH, HEIGHT);
        }


        // UPDATE UI
        playback_line.y = (HEIGHT - 50);
        playback_line.width = (WIDTH - 200);
        play_btn_center.x=WIDTH/2.0f;
        play_btn_center.y=HEIGHT-80;
        left_arrow_pos.x = (play_btn_center.x - play_btn_radius * 2 - text_size.x);
        left_arrow_pos.y = (play_btn_center.y - text_size.y / 2.0f);
        right_arrow_pos.x = (play_btn_center.x + play_btn_radius * 2);
        right_arrow_pos.y = (play_btn_center.y - text_size.y / 2.0f);


        // check for window movement
        mouse_pos = GetMousePosition();
        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !drag_window){
            if(CheckCollisionPointRec(mouse_pos, (Rectangle){ 0, 0, WIDTH, 20 })){
                drag_window = true;
                resize_start = mouse_pos;
            }
        }

        if (drag_window){
            // Get rid of any drift if the mouse isn't moving
            if(mouse_pos.x != prev_mouse.x && mouse_pos.y != prev_mouse.y){
                window_pos.x += (mouse_pos.x - resize_start.x);
                window_pos.y += (mouse_pos.y - resize_start.y);
                SetWindowPosition((int)window_pos.x, (int)window_pos.y);
            }
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) drag_window = false;
        }
        prev_mouse = mouse_pos;


        init_scrn_manager(WIDTH, HEIGHT);


        handle_audio(mouse_pos, play_btn_center, play_btn_radius, left_arrow_pos, right_arrow_pos, &playback_line,
                &songbook.playing, &progress, &dragging_scrubber, songbook.current_song, sample_rate);
        handle_arrows(mouse_pos, text_size, left_arrow_pos, right_arrow_pos, &songbook, &queue, playlists);

        if((int)progress == 1) next_in_queue(&songbook, &queue, playlists);

        update_scrn_manager(mouse_pos);

        BeginDrawing();
        ClearBackground((Color){20, 20, 20, 255});

        draw_scene(&font, &songbook, &queue, playlists, mouse_pos);
        draw_scrub_player(&font, mouse_pos, play_btn_center, play_btn_radius, left_arrow_pos, right_arrow_pos, &playback_line, progress, songbook.playing, songbook.current_song);

        EndDrawing();
    }

    clean_playlists(playlists, &songbook);
    save_data(playlists);
    clear_mem(&songbook, playlists);
    UnloadFont(font);
    unload_textures();
    ma_engine_uninit(&engine);
    CloseWindow();

    return 0;
}

