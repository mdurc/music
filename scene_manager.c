#include "scene_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define SIDEBAR_WIDTH 60
#define ICON_SIZE 40
#define MAX_INPUT_CHARS 43 // size of youtube url

Rectangle home = {10, 10, ICON_SIZE, ICON_SIZE};
Rectangle lib = {10, 60, ICON_SIZE, ICON_SIZE};
Rectangle search = {10, 110, ICON_SIZE, ICON_SIZE};
SceneType current_scene = SCENE_HOME;

int g_width, g_height;

float g_scrollOffset = 0.0f;

// For search
char input_buf[MAX_INPUT_CHARS + 1] = "\0"; // plus one for null terminating character
char output_buf[20][1024];
int output_line=0;


int letter_count = 0, frame_count=0;
bool mouse_on_search = false;
bool flag_download_complete = false;
bool flag_download_failed = false;

void draw_sidebar();
void draw_home(Font* font, AllSongs* songbook, AllSongs* queue, Playlist* playlists, Vector2 mouse_pos);
void draw_library(Font* font, AllSongs* songbook, Playlist* playlists, Vector2 mouse_pos);
void draw_download(Font* font, AllSongs* songbook, Vector2 mouse_pos);



void init_scrn_manager(const int WIDTH, const int HIEGHT){ g_width = WIDTH; g_height = HIEGHT; }

void update_scrn_manager(Vector2 mouse_pos){
    if (btn_pressed(mouse_pos, &home)) {
        g_scrollOffset = letter_count = 0;
        input_buf[0] = '\0';
        current_scene = SCENE_HOME;
        printf("Changed to home scene\n");
    }else if (btn_pressed(mouse_pos, &lib)) {
        g_scrollOffset = letter_count = 0;
        input_buf[0] = '\0';
        current_scene = SCENE_LIBRARY;
        printf("Changed to library scene\n");
    }else if (btn_pressed(mouse_pos, &search)) {
        g_scrollOffset = letter_count = 0;
        input_buf[0] = '\0';
        current_scene = SCENE_DOWNLOAD;
        printf("Changed to download scene\n");
    }
}

void draw_scene(Font* font, AllSongs* songbook, AllSongs* queue, Playlist* playlists, Vector2 mouse_pos){
    draw_sidebar();
    switch (current_scene) {
        case SCENE_HOME:
            draw_home(font, songbook, queue, playlists, mouse_pos);
            break;
        case SCENE_LIBRARY:
            draw_library(font, songbook, playlists, mouse_pos);
            break;
        case SCENE_DOWNLOAD:
            draw_download(font, songbook, mouse_pos);
            break;
        default:
            break;
    }
}



void draw_sidebar(){
    DrawRectangle(0, 0, SIDEBAR_WIDTH, g_height, (Color){50, 50, 50, 255});

    // Home, favorites, search
    DrawRectangleRec(home, (Color){200, 0, 0, 255});
    DrawRectangleRec(lib, (Color){0, 200, 0, 255});
    DrawRectangleRec(search, (Color){0, 0, 200, 255});
}


bool found_in_playlist(Playlist* playlist, char name[MAX_FNAME_LEN]){
    if(playlist->every_song) return 1;

    int i;
    for(i=0; i<playlist->size; ++i){
        if( strcmp(playlist->song_names[i], name)==0 &&
            strlen(playlist->song_names[i]) == strlen(name)){
            return 1;
        }
    }
    return 0;
}

// include songbook for current_song and is_playing.
// song_list_to_show is for what songs are shown on the list
void song_scroll(Font* font, AllSongs* songbook, AllSongs* queue, Playlist* playlists,
        Vector2 mouse_pos, int x, int y, int ITEM_HEIGHT, int VISIBLE_ITEMS, bool disable_scroll) {
    const float scrollSpeed = 4.0f;

    int num_songs_shown = playlists[songbook->playlist].size;

    float max_scroll = num_songs_shown;

    int gap = 5;
    int i, startY, visible_item_len = VISIBLE_ITEMS*(ITEM_HEIGHT+gap);
    int temp_ind;
    SoundMeta* temp;

    max_scroll = max_scroll*(ITEM_HEIGHT+gap) - visible_item_len + ITEM_HEIGHT;
    if (max_scroll < 0) max_scroll = 0;

    if(num_songs_shown >= VISIBLE_ITEMS) g_scrollOffset -= GetMouseWheelMove() * scrollSpeed;

    if (num_songs_shown>=VISIBLE_ITEMS && g_scrollOffset < 0) g_scrollOffset = 0;
    if (num_songs_shown>=VISIBLE_ITEMS && g_scrollOffset > max_scroll) g_scrollOffset = max_scroll;

    startY = (disable_scroll || num_songs_shown<VISIBLE_ITEMS)? 0: -g_scrollOffset;

    for(i=0; i<songbook->size; ++i){
        temp = songbook->songs[i];

        // !disable_scroll means that we are in the queue. We dont want to validate the queue
        if(!found_in_playlist(&playlists[songbook->playlist], temp->file_name)) continue;

        if (startY >= -ITEM_HEIGHT && startY < visible_item_len-ITEM_HEIGHT) {

            Rectangle rect = { g_width / 4.0f + x, startY + 10 + y, disable_scroll?130:400, ITEM_HEIGHT };

            if(CheckCollisionPointRec(mouse_pos, rect)){
                DrawRectangleRec(rect, GRAY);

                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                    printf("Changing song to: %s\n", temp->file_name);
                    if(songbook->current_song){
                        ma_sound_stop(&songbook->current_song->audio);
                    }
                    songbook->current_song = temp;

                    // restart the song and then play it
                    ma_sound_seek_to_pcm_frame(&songbook->current_song->audio, 0);
                    ma_sound_start(&songbook->current_song->audio);
                    songbook->playing = 1;
                    
                    // if it was in the queue, remove it
                    temp_ind = find(queue, temp->file_name);
                    if(temp_ind!=-1) remove_from_queue(queue, temp_ind);
                    
                }else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && queue!=NULL){
                    // ADD TO QUEUE
                    for(i=0;i<queue->size;++i){
                        if(queue->songs[i]->file_name == temp->file_name){
                            printf("Removing %s from queue\n", temp->file_name);
                            remove_from_queue(queue, i);
                            i=-1;
                            break;
                        }
                    }
                    if(i!=-1){
                        printf("Added song to queue: %s\n", temp->file_name);
                        queue->songs[queue->size] = temp;
                        ++queue->size;
                    }
                }
            } else {
                DrawRectangleRec(rect, DARKGRAY);
            }
            DrawTextEx(*font, temp->file_name, 
                    (Vector2){g_width / 4.0f + 10 + x, startY + ITEM_HEIGHT/2.0 + y + (disable_scroll?5:0)}, 
                    ITEM_HEIGHT/2.0, 2, WHITE);
        }
        startY += ITEM_HEIGHT+gap;
        // we found all the songs we need for now
        if(startY > visible_item_len-ITEM_HEIGHT){ break; }
    }

    Rectangle perimeter = {
        g_width / 4.0f - 13-ITEM_HEIGHT + x,     // x coord
        y-ITEM_HEIGHT,                        // y coord
        g_width / 2.0f + 24 + ITEM_HEIGHT*2, // width
        visible_item_len+19+ITEM_HEIGHT      // height
    };
    DrawRectangleLinesEx(perimeter, ITEM_HEIGHT+8 /* thickness */, (Color){20, 20, 20, 255});
}


bool create_search_bar(Rectangle* search_bar,  Vector2 mouse_pos){
    int key, clipboard_len;
    const char* clipboard_text;

    mouse_on_search = CheckCollisionPointRec(mouse_pos, *search_bar);
    if(mouse_on_search){
        SetMouseCursor(MOUSE_CURSOR_IBEAM);
        ++frame_count;

        key = GetCharPressed();

        // check if more characters have been pressed on the same frame
        while (key > 0){
            // only allow keys within: [32..125]
            if ((key >= 32) && (key <= 125) && (letter_count < MAX_INPUT_CHARS)){
                input_buf[letter_count] = (char)key;
                input_buf[letter_count+1] = '\0';
                ++letter_count;
            }

            // check next character in the queue
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)){
            --letter_count;
            if(letter_count < 0) letter_count = 0;
            input_buf[letter_count] = '\0';
        }else if (IsKeyPressed(KEY_V) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))) {
            clipboard_text = GetClipboardText();
            if (clipboard_text) {
                clipboard_len = strlen(clipboard_text);
                if (letter_count + clipboard_len < MAX_INPUT_CHARS) {
                    strcat(input_buf, clipboard_text);
                    letter_count += clipboard_len;
                }
            }
        }else if(IsKeyPressed(KEY_ENTER)){
            return true;
        }
    }
    else{
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        frame_count = 0;
    }
    return false;
}

void draw_search_bar(Font* font, Rectangle* search_bar){
    DrawRectangleRounded(*search_bar, 0.5f, 8, GRAY);
    
    if(letter_count == 0){
        DrawTextEx(*font, "Hover to type", 
                (Vector2){search_bar->x+10, search_bar->y+10}, 
                20, 
                1, 
                DARKGRAY
                );
    }

    if (mouse_on_search) DrawRectangleRoundedLines(*search_bar, 0.5f, 8, 2, DARKBLUE);
    else DrawRectangleRoundedLines(*search_bar, 0.5f, 8, 2, DARKGRAY);

    DrawTextEx(*font, input_buf, (Vector2){search_bar->x+10, search_bar->y+10}, 20, 1, DARKBLUE);
    //DrawTextEx(*font, TextFormat("INPUT CHARS: %i/%i", letter_count, MAX_INPUT_CHARS), (Vector2){315, search_bar->y+55}, 20, 1, DARKGRAY);

    if (mouse_on_search){
        if (letter_count < MAX_INPUT_CHARS){
            if (((frame_count/20)%2) == 0) DrawText("_", search_bar->x+MeasureTextEx(*font, input_buf,20, 1).x+12, search_bar->y+15, 20, DARKBLUE);
        }
        else DrawTextEx(*font, "Press BACKSPACE to delete chars...", (Vector2){230, 300}, 20, 1, GRAY);
    }
}



// TODO: download playlist and add to playlists
void draw_download(Font* font, AllSongs* songbook, Vector2 mouse_pos) {
    int i, y_offset;
    char cmd[150 + MAX_INPUT_CHARS];
    Rectangle search_bar = {(g_width-400)/2.0f, 60, 400, 40};
    if(create_search_bar(&search_bar, mouse_pos)){
        // try to download this char using cmd:
        snprintf(cmd, sizeof(cmd), "yt-dlp -o '%%(title)s.%%(ext)s' -x --audio-format mp3 --audio-quality 0 \"https://www.youtube.com/watch?v=%s\" -P \"music/\"", input_buf);
        printf("Attempting command: %s\n", cmd);

        output_line = 0;
        FILE *fp = popen(cmd, "r");
        while (fgets(output_buf[output_line], sizeof(output_buf[output_line]), fp) != NULL) { ++output_line; }


        if(output_line == 10){
            printf("Command Succeeded\n");
            flag_download_complete = 1;
            flag_download_failed = 0;

            reload_music_dir(songbook);
        }else{
            flag_download_complete = 0;
            flag_download_failed = 1;
            printf("Command Failed\n");
        }
    }


    DrawTextEx(*font, "Download Youtube Songs", (Vector2){search_bar.x+45, 20}, 25, 1, DARKGRAY);
    draw_search_bar(font, &search_bar);

    y_offset = 80;
    for(i=0;i<output_line;++i){
        DrawTextEx(*font, output_buf[i], (Vector2){search_bar.x-120, search_bar.y + y_offset}, 15, 1, DARKGRAY);
        y_offset += 20;
    }
    if(flag_download_complete){
        DrawTextEx(*font, "Download Complete", (Vector2){search_bar.x+95, search_bar.y+50}, 20, 1, GRAY);
    }else if(flag_download_failed){
        DrawTextEx(*font, "Download Failed", (Vector2){search_bar.x+110, search_bar.y+50}, 20, 1, GRAY);
    }
}

void draw_library(Font* font, AllSongs* songbook, Playlist* playlists, Vector2 mouse_pos) {
    int i;
    int square_size = 80;
    int padding = 20;
    int x = 200;
    int y = 100;
    int font_size = 15;

    Rectangle square;
    Vector2 text_size, text_position;

    // START at 1 because playlist 0 is a PLACEHOLDER
    for(i=1; i<MAX_PLAYLISTS && playlists[i].size>0; ++i){
        square.x = x; square.y = y;
        square.width = square_size; square.height = square_size;

        text_size = MeasureTextEx(*font, playlists[i].name, font_size, 1);
        text_position.x = square.x + (square.width - text_size.x) / 2;
        text_position.y = square.y + (square.height - text_size.y) / 2;

        DrawRectangleRounded(square, 0.5f, 8, DARKGRAY);
        DrawTextEx(*font, playlists[i].name, text_position, font_size, 1, RAYWHITE);

        if (CheckCollisionPointRec(mouse_pos, square) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            songbook->playlist = i;
            printf("Moving to playlist #%d\n", i);
        }

        x += square_size + padding;
        if (x + square_size > GetScreenWidth()) {
            x = padding;
            y += square_size + padding;
        }
    }

    // for adding a new playlist
    square.x = x; square.y = y;
    square.width = square_size; square.height = square_size;
    text_size = MeasureTextEx(*font, "+", font_size, 1);
    text_position.x = square.x + (square.width - text_size.x) / 2;
    text_position.y = square.y + (square.height - text_size.y) / 2;

    DrawRectangleRounded(square, 0.5f, 8, DARKGRAY);
    DrawTextEx(*font, "+", text_position, font_size, 1, RAYWHITE);
    if (CheckCollisionPointRec(mouse_pos, square) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        printf("Adding new playlist\n");
        //TODO: promt with a popup for playlist name
    }
}



void draw_home(Font* font, AllSongs* songbook, AllSongs* queue, Playlist* playlists, Vector2 mouse_pos) {

    int i, temp_ind, match_count=0;
    SoundMeta* temp;
    SoundMeta* first_match;

    playlists[0].size = 0;
    if (letter_count > 0) {
        for(i=0;i<songbook->size;++i){
            temp = songbook->songs[i];
            if (strcasestr(temp->file_name, input_buf) != NULL) {
                strcpy(playlists[0].song_names[playlists[0].size], temp->file_name);
                ++playlists[0].size;
                if(++match_count == 1) first_match = temp;
            }
        }
        // maximum of 5 results at a time
        // Songbook is added because it carries the "current song" that might be changed
        playlists[0].every_song = playlists[0].size == songbook->size;

        temp_ind = songbook->playlist;
        songbook->playlist = 0;
        song_scroll(font, songbook, queue, playlists, mouse_pos, -70, 100, 40, 5, false);
        songbook->playlist = temp_ind;
    }else{
        song_scroll(font, songbook, queue, playlists , mouse_pos, -70, 100, 40, 5, false);
    }

    // DRAW QUEUE
    playlists[0].size = 0;
    playlists[0].every_song = 0;
    for(i=0;i<queue->size;++i){
        strcpy(playlists[0].song_names[playlists[0].size], queue->songs[i]->file_name);
        ++playlists[0].size;
    }
    temp_ind = songbook->playlist;
    songbook->playlist = 0;
    song_scroll(font, songbook, queue, playlists, mouse_pos, 400, 100, 20, 5, true);
    songbook->playlist = temp_ind;


    Rectangle search_bar = {(g_width-400)/2.0f, 20, 400, 40};
    // check if KEY_ENTER was pressed
    if(create_search_bar(&search_bar, mouse_pos)){
        printf("Searching for: %s\n", input_buf);
        temp_ind = find(songbook, input_buf);
        temp = temp_ind!=-1 ? songbook->songs[temp_ind] : NULL;

        if((temp || match_count==1) && songbook->current_song && ma_sound_is_playing(&songbook->current_song->audio)){
            ma_sound_stop(&songbook->current_song->audio);
        }

        if(temp){
            printf("Found\n");
            songbook->current_song = temp;
        }else if(match_count==1){
            printf("Playing top pick\n");
            songbook->current_song = first_match;
        }else{
            printf("Not found\n");
        }

        if(temp || match_count==1){
            // restart the song and then play it
            ma_sound_seek_to_pcm_frame(&songbook->current_song->audio, 0);
            ma_sound_start(&songbook->current_song->audio);
            songbook->playing = 1;
        }
    }


    // Drawings:
    draw_search_bar(font, &search_bar);
}
