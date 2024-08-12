#include "scene_manager.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define SIDEBAR_WIDTH 60
#define ICON_SIZE 40
#define ITEM_HEIGHT 40
#define MAX_INPUT_CHARS MAX_FNAME_LEN-3 // 3 elipses included for MAX_FNAME_LEN

Rectangle home = {10, 10, ICON_SIZE, ICON_SIZE};
Rectangle lib = {10, 60, ICON_SIZE, ICON_SIZE};
Rectangle search = {10, 110, ICON_SIZE, ICON_SIZE};
SceneType current_scene = SCENE_HOME;

int g_width, g_height;

float g_scrollOffset = 0.0f;
int* song_count;


// For search
char input_buf[MAX_INPUT_CHARS + 1] = "\0"; // plus one for null terminating character
int letter_count = 0, frame_count=0;
bool mouse_on_search = false;

void draw_sidebar();
void draw_home(Font font, Node* songbook[MAX_SONGS], Vector2 mouse_pos, SoundMeta** current_song, bool* playing);
void draw_library(Font font, Node* songbook[MAX_SONGS]);
void draw_search(Font font, Node* songbook[MAX_SONGS], Vector2 mouse_pos, SoundMeta** current_song, bool* playing);



void init_scrn_manager(const int WIDTH, const int HIEGHT, int* total_songs){
    g_width = WIDTH; g_height = HIEGHT; 
    song_count = total_songs;
}

void update_scrn_manager(Vector2 mouse_pos){
    if (btn_pressed(mouse_pos, &home)) {
        g_scrollOffset = 0;
        current_scene = SCENE_HOME;
        printf("Changed to home scene\n");
    }else if (btn_pressed(mouse_pos, &lib)) {
        current_scene = SCENE_LIBRARY;
        printf("Changed to favorites scene\n");
    }else if (btn_pressed(mouse_pos, &search)) {
        g_scrollOffset = 0;
        current_scene = SCENE_SEARCH;
        printf("Changed to search scene\n");
    }
}

void draw_scene(Font font, Node* songbook[MAX_SONGS], Vector2 mouse_pos, SoundMeta** current_song, bool* playing){
    draw_sidebar();
    switch (current_scene) {
        case SCENE_HOME:
            draw_home(font, songbook, mouse_pos, current_song, playing);
            break;
        case SCENE_LIBRARY:
            draw_library(font, songbook);
            break;
        case SCENE_SEARCH:
            draw_search(font, songbook, mouse_pos, current_song, playing);
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


void song_scroll(Font font, Node* songbook[MAX_SONGS], int len_book, int total_songs, bool chaining,
        Vector2 mouse_pos, SoundMeta** current_song, bool* playing, int x, int y, int VISIBLE_ITEMS) {
    const float scrollSpeed = 4.0f;

    float max_scroll = total_songs;
    int gap = 5;
    int i, startY, visible_item_len = VISIBLE_ITEMS*(ITEM_HEIGHT+gap);
    Node* temp;

    max_scroll = max_scroll*(ITEM_HEIGHT+gap) - visible_item_len + ITEM_HEIGHT;
    if (max_scroll < 0) max_scroll = 0;

    if(total_songs >= VISIBLE_ITEMS) g_scrollOffset -= GetMouseWheelMove() * scrollSpeed;

    if (g_scrollOffset < 0) g_scrollOffset = 0;
    if (g_scrollOffset > max_scroll) g_scrollOffset = max_scroll;

    startY = -g_scrollOffset;

    for(i=0; i<len_book; ++i){
        temp = songbook[i];
        while(temp){
            if (startY >= -ITEM_HEIGHT && startY < visible_item_len-ITEM_HEIGHT) {

                Rectangle rect = { g_width / 4.0f + x, startY + 10 + y, g_width / 2.0f, ITEM_HEIGHT };

                if(btn_pressed(mouse_pos, &rect)){
                    printf("Changing song to: %s\n", temp->meta->file_name);
                    ma_sound_stop(&(*current_song)->audio);
                    *current_song = temp->meta;

                    // restart the song and then play it
                    ma_sound_seek_to_pcm_frame(&(*current_song)->audio, 0);
                    ma_sound_start(&(*current_song)->audio);
                    *playing = 1;
                }
                
                if(CheckCollisionPointRec(mouse_pos, rect)){
                    DrawRectangleRec(rect, GRAY);
                } else {
                    DrawRectangleRec(rect, DARKGRAY);
                }
                DrawTextEx(font, temp->meta->file_name, 
                    (Vector2){g_width / 4.0f + 10 + x, startY + 20 + y}, 
                    20, 2, WHITE);
            }
            startY += ITEM_HEIGHT+gap;

            if(chaining) temp = temp->next;
            else temp = NULL;
        }

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

void draw_home(Font font, Node* songbook[MAX_SONGS], Vector2 mouse_pos, SoundMeta** current_song, bool* playing) {
    song_scroll(font, songbook, MAX_SONGS, *song_count, true, mouse_pos, current_song, playing, 0, 0, 8);
}

void draw_library(Font font, Node* songbook[MAX_SONGS]) {
}

void draw_search(Font font, Node* songbook[MAX_SONGS], Vector2 mouse_pos, SoundMeta** current_song, bool* playing) {

    int i, key, matches;
    SoundMeta* song_p;
    Node* temp;
    Node** match_list;

    Rectangle search_bar = {(g_width-400)/2.0f, 20, 400, 40};
    mouse_on_search = CheckCollisionPointRec(mouse_pos, search_bar);

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
        }else if(IsKeyPressed(KEY_ENTER)){
            printf("Searching for: %s\n", input_buf);
            song_p = find(songbook, input_buf);
            if(song_p){
                printf("Found\n");
                ma_sound_stop(&(*current_song)->audio);
                *current_song = song_p;

                // restart the song and then play it
                ma_sound_seek_to_pcm_frame(&(*current_song)->audio, 0);
                ma_sound_start(&(*current_song)->audio);
                *playing = 1;
            }else{
                printf("Not found\n");
            }
        }
    }
    else{
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        frame_count = 0;
    }


    matches = 0;
    match_list = NULL;

    if (letter_count > 0) {
        for(i=0;i<MAX_SONGS;++i){
            temp = songbook[i];
            while (temp) {
                if (strcasestr(temp->meta->file_name, input_buf) != NULL) {
                    match_list = realloc(match_list, (matches + 1) * sizeof(Node*));
                    match_list[matches] = temp;
                    ++matches;
                }
                temp = temp->next;
            }
        }
    }

    // maximum of 5 results at a time
    song_scroll(font, match_list, matches, matches, false, mouse_pos, current_song, playing, 0, 100, 5);


    // Drawings:
    DrawRectangleRounded(search_bar, 0.5f, 8, GRAY);
    
    if(letter_count == 0){
        DrawTextEx(font, "Hover to type", 
                (Vector2){search_bar.x+10, search_bar.y+10}, 
                20, 
                1, 
                DARKGRAY
                );
    }

    if (mouse_on_search) DrawRectangleRoundedLines(search_bar, 0.5f, 8, 2, DARKBLUE);
    else DrawRectangleRoundedLines(search_bar, 0.5f, 8, 2, DARKGRAY);

    DrawTextEx(font, input_buf, (Vector2){search_bar.x+10, search_bar.y+10}, 20, 1, DARKBLUE);
    DrawTextEx(font, TextFormat("INPUT CHARS: %i/%i", letter_count, MAX_INPUT_CHARS), (Vector2){315, search_bar.y+55}, 20, 1, DARKGRAY);

    if (mouse_on_search){
        if (letter_count < MAX_INPUT_CHARS){
            if (((frame_count/20)%2) == 0) DrawText("_", search_bar.x+MeasureTextEx(font, input_buf,20, 1).x+12, search_bar.y+15, 20, DARKBLUE);
        }
        else DrawTextEx(font, "Press BACKSPACE to delete chars...", (Vector2){230, 300}, 20, 1, GRAY);
    }
}
