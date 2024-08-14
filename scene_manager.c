#include "scene_manager.h"
#include <CoreAudio/CoreAudio.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared.h"


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


bool is_popup_open = false;

void draw_sidebar();
void draw_home(Font* font, AllSongs* songbook, AllSongs* queue, Playlist* playlists[MAX_PLAYLISTS], Vector2 mouse_pos);
void draw_library(Font* font, AllSongs* songbook, Playlist* playlists[MAX_PLAYLISTS], Vector2 mouse_pos);
void draw_download(Font* font, AllSongs* songbook, Playlist* playlist[MAX_PLAYLISTS], Vector2 mouse_pos);



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

void draw_scene(Font* font, AllSongs* songbook, AllSongs* queue, Playlist* playlists[MAX_PLAYLISTS], Vector2 mouse_pos){
    draw_sidebar();
    switch (current_scene) {
        case SCENE_HOME:
            draw_home(font, songbook, queue, playlists, mouse_pos);
            break;
        case SCENE_LIBRARY:
            draw_library(font, songbook, playlists, mouse_pos);
            break;
        case SCENE_DOWNLOAD:
            draw_download(font, songbook, playlists, mouse_pos);
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
        if(strMatch(playlist->song_names[i], name)){
            return 1;
        }
    }
    return 0;
}

// include songbook for current_song and is_playing.
// song_list_to_show is for what songs are shown on the list
void song_scroll(Font* font, SoundMeta** current_song, AllSongs* song_list, AllSongs* queue, Playlist* playlists[MAX_PLAYLISTS],
        Vector2 mouse_pos, int x, int y, int ITEM_HEIGHT, int VISIBLE_ITEMS, bool disable_scroll) {
    const float scrollSpeed = 4.0f;

    int num_songs_shown = playlists[song_list->playlist]->size;

    float max_scroll = num_songs_shown;

    int gap = 5;
    int i, j, startY, visible_item_len = VISIBLE_ITEMS*(ITEM_HEIGHT+gap);
    int temp_ind;
    SoundMeta* temp;

    max_scroll = max_scroll*(ITEM_HEIGHT+gap) - visible_item_len + ITEM_HEIGHT;
    if (max_scroll < 0) max_scroll = 0;

    if(!disable_scroll && num_songs_shown >= VISIBLE_ITEMS) g_scrollOffset -= GetMouseWheelMove() * scrollSpeed;

    if (!disable_scroll && num_songs_shown>=VISIBLE_ITEMS && g_scrollOffset < 0) g_scrollOffset = 0;
    if (!disable_scroll && num_songs_shown>=VISIBLE_ITEMS && g_scrollOffset > max_scroll) g_scrollOffset = max_scroll;

    startY = (disable_scroll || num_songs_shown<VISIBLE_ITEMS)? 0: -g_scrollOffset;

    for(i=0; i<song_list->size; ++i){
        temp = song_list->songs[i];

        if(!found_in_playlist(playlists[song_list->playlist], temp->file_name)) {
            if(disable_scroll){
                printf("Cannot find %s inside: ", temp->file_name);
                for(j=0;j<playlists[0]->size;++j){
                    printf("%s, -- ", playlists[0]->song_names[j]);
                }
                printf("\n");
            }
            continue;
        }

        if (startY >= -ITEM_HEIGHT && startY < visible_item_len-ITEM_HEIGHT) {

            Rectangle rect = { g_width / 4.0f + x, startY + 10 + y, disable_scroll?130:400, ITEM_HEIGHT };

            if(CheckCollisionPointRec(mouse_pos, rect)){
                DrawRectangleRec(rect, GRAY);

                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                    printf("Changing song to: %s\n", temp->file_name);
                    if(current_song && *current_song){
                        ma_sound_stop(&(*current_song)->audio);
                    }
                    *current_song = temp;

                    // restart the song and then play it
                    ma_sound_seek_to_pcm_frame(&(*current_song)->audio, 0);
                    ma_sound_start(&(*current_song)->audio);
                    song_list->playing = 1;
                    
                    // if it was in the queue, remove it
                    temp_ind = find(queue, temp->file_name);
                    if(temp_ind!=-1) remove_from_queue(queue, temp_ind);
                    
                }else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && queue!=NULL){
                    // Check if the song we just pressed already exists in the queue, if so, remove
                    for(j=0;j<queue->size;++j){
                        if(strMatch(queue->songs[j]->file_name, temp->file_name)){
                            printf("Removing %s from queue\n", temp->file_name);
                            remove_from_queue(queue, j);
                            j=-1;
                            break;
                        }
                    }
                    // ADD TO QUEUE
                    if(j!=-1){
                        printf("Added song to queue in slot %d: %s\n", queue->size, temp->file_name);
                        queue->songs[queue->size] = temp;
                        ++queue->size;
                    }
                }
            } else {
                DrawRectangleRec(rect, DARKGRAY);
            }
            DrawTextEx(*font, temp->file_name, 
                    (Vector2){g_width / 4.0f + 10 + x, startY + ITEM_HEIGHT/2.0 + y + (disable_scroll?5:0)}, 
                    ITEM_HEIGHT/2.0, 1, WHITE);
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
void draw_download(Font* font, AllSongs* songbook, Playlist* playlist[MAX_PLAYLISTS], Vector2 mouse_pos) {
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

            reload_music_dir(songbook, playlist[1]);
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

void draw_library(Font* font, AllSongs* songbook, Playlist* playlists[MAX_PLAYLISTS], Vector2 mouse_pos) {
    int i, key;
    int square_size = 80;
    int padding = 20;
    int x = 200;
    int y = 100;
    int font_size = 15;
    bool removed_playlist_inframe = 0;

    Rectangle square;
    Vector2 text_size, text_position;

    // START at 1 because playlist 0 is a PLACEHOLDER
    for(i=1; i<MAX_PLAYLISTS && playlists[i]!=NULL; ++i){
        //printf("Name: %s, size:%d, index: %d\n", playlists[i].name, playlists[i].size, i);
        square.x = x; square.y = y;
        square.width = square_size; square.height = square_size;

        text_size = MeasureTextEx(*font, playlists[i]->name, font_size, 1);
        text_position.x = square.x + (square.width - text_size.x) / 2;
        text_position.y = square.y + (square.height - text_size.y) / 2;

        DrawRectangleRounded(square, 0.5f, 8, DARKGRAY);
        DrawTextEx(*font, playlists[i]->name, text_position, font_size, 1, RAYWHITE);

        if (CheckCollisionPointRec(mouse_pos, square)) {
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                songbook->playlist = i;
                printf("Moving to playlist #%d\n", i);
            }else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
                //TODO: instead of right clicking everywhere, just add an elipses that opens an options popup to take different actions
                if(strMatch(playlists[i]->name, "All Songs")){
                    printf("Cannot remove \"All Songs\" playlist\n");
                }else{
                    remove_from_playlist(playlists, i);
                    printf("Removing playlist #%d\n", i);

                    // if we just deleted the playlist that we were on, just go back to All Songs.
                    // All Songs should just be index 1 of playlists. And it cannot be removed
                    if(songbook->playlist == i) songbook->playlist = 1;

                    // we just removed the current index, so do --i and for-loop will do ++i to stay at same index
                    --i;
                    removed_playlist_inframe = 1;
                }
            }
        }

        x += square_size + padding;
        if (x + square_size > GetScreenWidth()) {
            x = padding;
            y += square_size + padding;
        }
    }


    // for adding a new playlist
    // first check if we have already maxed out
    for(i=0;i<MAX_PLAYLISTS && playlists[i]!=NULL;++i);

    // if we just removed one, just wait until the next frame to add a new playlist
    // this way the + button just seemlessly replaces the previously last playlist square
    if(removed_playlist_inframe || i==MAX_PLAYLISTS){
        //printf("Maximum playlist count has been reached: %d\n", i-1);
        return;
    }

    square.x = x; square.y = y;
    square.width = square_size; square.height = square_size;
    text_size = MeasureTextEx(*font, "+", font_size, 1);
    text_position.x = square.x + (square.width - text_size.x) / 2;
    text_position.y = square.y + (square.height - text_size.y) / 2;

    DrawRectangleRounded(square, 0.5f, 8, DARKGRAY);
    DrawTextEx(*font, "+", text_position, font_size, 1, RAYWHITE);
    if (CheckCollisionPointRec(mouse_pos, square) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        printf("Adding new playlist\n");
        is_popup_open = true;
        input_buf[0]='\0';
        letter_count = 0;
    }

    if (is_popup_open) {
        square.x = g_width/2.0 - 150;
        square.y = g_height/2.0 - 50;
        square.width = 300;
        square.height = 100;
        DrawRectangleRounded(square, 0.1f, 8, GRAY);

        DrawTextEx(*font, "Enter playlist name:", (Vector2){square.x + 10, square.y + 10}, 20, 1, RAYWHITE);
        DrawTextEx(*font, input_buf, (Vector2){square.x + 10, square.y + 40}, 20, 1, RAYWHITE);

        key = GetCharPressed();

        // check if more characters have been pressed on the same frame
        assert(MAX_PLAYNAME < sizeof(input_buf)-1); // we dont want to overflow input_buf
        while (key > 0){
            // only allow keys within: [32..125]
            if ((key >= 32) && (key <= 125) && (letter_count < MAX_PLAYNAME)){
                input_buf[letter_count] = (char)key;
                input_buf[++letter_count] = '\0';
            }

            // check next character in the queue
            key = GetCharPressed();
        }

        // Handle backspace
        if (IsKeyPressed(KEY_BACKSPACE) && letter_count > 0) {
            input_buf[--letter_count] = '\0';
        }

        // Handle submission (e.g., pressing Enter)
        if (IsKeyPressed(KEY_ENTER)) {
            printf("Playlist name entered: %s\n", input_buf);
            // see if there already exists a playlist with that name
            for(i=0;i<MAX_PLAYLISTS && playlists[i]!=NULL;++i){
                if(strMatch(playlists[i]->name, input_buf)){
                    printf("Playlist name already exists: %s\n", input_buf);
                    // return before allocating and leaving the popup
                    return;
                }
            }

            // find the last playlist index
            // Should still be stored in variable "i" from : for(i=0;i<MAX_PLAYLISTS && playlists[i]!=NULL;++i);
            playlists[i] = malloc(sizeof(Playlist));

            // init new playlist at index i
            strncpy(playlists[i]->name, input_buf, sizeof(playlists[i]->name) - 1);
            playlists[i]->name[sizeof(playlists[i]->name)-1] = '\0';
            playlists[i]->size = 0;
            playlists[i]->every_song = 0;

            is_popup_open = false;
        }

        // TODO: diasable raylib exit application on ESC
        if(IsKeyPressed(KEY_ESCAPE)) is_popup_open = false;
    }
}



void draw_home(Font* font, AllSongs* songbook, AllSongs* queue, Playlist* playlists[MAX_PLAYLISTS], Vector2 mouse_pos) {

    int i, temp_ind, match_count=0;
    char temp[MAX_FNAME_LEN+1];
    SoundMeta* first_match;
    SoundMeta* search_song;
    char* current_song_name;
    size_t song_name_size;

    // index 0 of playlist should be allocated in main
    // put the matches of the search into the playlist index 0
    // pull from whatever the current playlist is
    assert(playlists[0]!=NULL);
    playlists[0]->size = 0;
    if (letter_count > 0) {
        for(i=0;i<playlists[songbook->playlist]->size;++i){
            assert(sizeof(temp) >= sizeof(playlists[songbook->playlist]->song_names[i]));
            strcpy(temp, playlists[songbook->playlist]->song_names[i]);
            if (strcasestr(temp, input_buf) != NULL) {
                // i do not want memory overflow ever again
                assert(sizeof(playlists[0]->song_names[playlists[0]->size]) >= sizeof(temp));
                strcpy(playlists[0]->song_names[playlists[0]->size], temp);
                ++playlists[0]->size;
                if(++match_count == 1){
                    temp_ind = find(songbook,temp);
                    assert(temp_ind != -1); // it should always be found
                    first_match = songbook->songs[temp_ind];
                }
            }
        }
        // maximum of 5 results at a time
        // Songbook is added because it carries the "current song" that might be changed
        playlists[0]->every_song = playlists[0]->size == songbook->size;

        temp_ind = songbook->playlist;
        songbook->playlist = 0;
        song_scroll(font, &songbook->current_song, songbook, queue, playlists, mouse_pos, -70, 100, 40, 5, false);
        songbook->playlist = temp_ind;
    }else{
        song_scroll(font, &songbook->current_song, songbook, queue, playlists , mouse_pos, -70, 100, 40, 5, false);
    }

    // DRAW QUEUE
    // first fill playlist[0] with queue
    playlists[0]->size = 0;
    playlists[0]->every_song = 0;
    for(i=0;i<queue->size;++i){

        // alias for the playlist name that we want to populate
        current_song_name = playlists[0]->song_names[playlists[0]->size];
        song_name_size = sizeof(playlists[0]->song_names[playlists[0]->size]);

        // populate and ensure memory safety
        assert(song_name_size >= sizeof(queue->songs[i]->file_name));

        // strncpy anyways, even after doing the assert
        strncpy(current_song_name, queue->songs[i]->file_name, song_name_size - 1);
        current_song_name[song_name_size - 1] = '\0';

        ++playlists[0]->size;
    }

    // plug queue in, instead of songbooks, so that we only look for queue songs:
    queue->playlist = 0;
    song_scroll(font, &songbook->current_song, queue, queue, playlists, mouse_pos, 400, 100, 20, 5, true);


    Rectangle search_bar = {(g_width-400)/2.0f, 20, 400, 40};
    // check if KEY_ENTER was pressed
    // TODO: ONLY SEARCH WITHIN THE CURRENT PLAYLIST
    if(create_search_bar(&search_bar, mouse_pos)){
        printf("Searching for: %s\n", input_buf);
        if(!found_in_playlist(playlists[songbook->playlist], input_buf)){
            printf("Not found\n");
            search_song = NULL;
        }else{
            temp_ind = find(songbook, input_buf);
            search_song = temp_ind!=-1 ? songbook->songs[temp_ind] : NULL;
        }


        if((search_song || match_count==1) && songbook->current_song && ma_sound_is_playing(&songbook->current_song->audio)){
            ma_sound_stop(&songbook->current_song->audio);
        }

        if(search_song){
            printf("Found\n");
            songbook->current_song = search_song;
        }else if(match_count==1){
            printf("Playing top pick\n");
            songbook->current_song = first_match;
        }else{
            printf("Not found\n");
        }

        if(search_song || match_count==1){
            // restart the song and then play it
            ma_sound_seek_to_pcm_frame(&songbook->current_song->audio, 0);
            ma_sound_start(&songbook->current_song->audio);
            songbook->playing = 1;
        }
    }


    // Drawings:
    draw_search_bar(font, &search_bar);
}
