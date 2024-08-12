#include "scene_manager.h"
#include <stdio.h>

#define SIDEBAR_WIDTH 60
#define ICON_SIZE 40
#define ITEM_HEIGHT 40
#define VISIBLE_ITEMS 8

Rectangle home = {10, 10, ICON_SIZE, ICON_SIZE};
Rectangle lib = {10, 60, ICON_SIZE, ICON_SIZE};
Rectangle search = {10, 110, ICON_SIZE, ICON_SIZE};
SceneType current_scene = SCENE_HOME;

int g_width, g_height, btn_count=0;

float g_scrollOffset = 0.0f;

int* song_count;
SongButton song_btns[VISIBLE_ITEMS]; // only give the option to press the VISIBLE_ITEMS

void draw_sidebar();
void draw_home(Font font, Node* songbook[MAX_SONGS], Vector2 mouse_pos);
void draw_library(Font font, Node* songbook[MAX_SONGS]);
void draw_search(Font font, Node* songbook[MAX_SONGS]);



void init_scrn_manager(const int WIDTH, const int HIEGHT, int* total_songs){
    g_width = WIDTH; g_height = HIEGHT; 
    song_count = total_songs;
}

void update_scrn_manager(Vector2 mouse_pos, SoundMeta** current_song, bool* playing){
    int i;
    if (btn_pressed(mouse_pos, &home)) {
        current_scene = SCENE_HOME;
        printf("Changed to home scene\n");
    }else if (btn_pressed(mouse_pos, &lib)) {
        current_scene = SCENE_LIBRARY;
        printf("Changed to favorites scene\n");
    }else if (btn_pressed(mouse_pos, &search)) {
        current_scene = SCENE_SEARCH;
        printf("Changed to search scene\n");
    }else if(current_scene == SCENE_HOME){
        // Check for button presses to switch song
        for(i=0;i<btn_count;++i){
            if(btn_pressed(mouse_pos, &song_btns[i].rect)){
                printf("Changing song to: %s\n", song_btns[i].meta->file_name);
                ma_sound_stop(&(*current_song)->audio);
                *current_song = song_btns[i].meta;

                // restart the song and then play it
                ma_sound_seek_to_pcm_frame(&(*current_song)->audio, 0);
                ma_sound_start(&(*current_song)->audio);
                *playing = 1;
                return;
            }
        }
    }
}

void draw_scene(Font font, Node* songbook[MAX_SONGS], Vector2 mouse_pos){
    draw_sidebar();
    switch (current_scene) {
        case SCENE_HOME:
            draw_home(font, songbook, mouse_pos);
            break;
        case SCENE_LIBRARY:
            draw_library(font, songbook);
            break;
        case SCENE_SEARCH:
            draw_search(font, songbook);
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

void draw_home(Font font, Node* songbook[MAX_SONGS], Vector2 mouse_pos) {
    const float scrollSpeed = 4.0f;

    float max_scroll = *song_count, check_has_moved;
    int gap = 5;
    int i, startY, visible_item_len = VISIBLE_ITEMS*(ITEM_HEIGHT+gap);
    Node* temp;

    max_scroll = max_scroll*(ITEM_HEIGHT+gap) - visible_item_len + ITEM_HEIGHT;
    if (max_scroll < 0) max_scroll = 0;

    check_has_moved = g_scrollOffset;
    g_scrollOffset -= GetMouseWheelMove() * scrollSpeed;
    if(check_has_moved!=g_scrollOffset) btn_count = 0;

    if (g_scrollOffset < 0) g_scrollOffset = 0;
    if (g_scrollOffset > max_scroll) g_scrollOffset = max_scroll;

    startY = -g_scrollOffset;

    for(i=0; i<MAX_SONGS; ++i){
        temp = songbook[i];
        while(temp){
            if (startY >= -ITEM_HEIGHT && startY < visible_item_len-ITEM_HEIGHT) {

                Rectangle rect = { g_width / 4.0f, startY + 10, g_width / 2.0f, ITEM_HEIGHT };

                // btn_count gets reset every time you scroll
                if(btn_count < VISIBLE_ITEMS){
                    song_btns[btn_count].rect = rect;
                    song_btns[btn_count].meta = temp->meta;
                    ++btn_count;
                }
                if(CheckCollisionPointRec(mouse_pos, rect)){
                    DrawRectangleRec(rect, GRAY);
                } else {
                    DrawRectangleRec(rect, DARKGRAY);
                }
                
                DrawTextEx(font, temp->meta->file_name, 
                    (Vector2){g_width / 4.0f + 10, startY + 20}, 
                    20, 2, WHITE);
            }
            startY += ITEM_HEIGHT+gap;
            temp = temp->next;
        }
    }

    Rectangle perimeter = {
        g_width / 4.0f - 13-ITEM_HEIGHT,     // x coord
        -ITEM_HEIGHT,                        // y coord
        g_width / 2.0f + 24 + ITEM_HEIGHT*2, // width
        visible_item_len+19+ITEM_HEIGHT      // height
    };
    DrawRectangleLinesEx(perimeter, ITEM_HEIGHT+8 /* thickness */, (Color){20, 20, 20, 255});
}

void draw_library(Font font, Node* songbook[MAX_SONGS]) {
}

void draw_search(Font font, Node* songbook[MAX_SONGS]) {
}
