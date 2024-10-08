#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "shared.h"

typedef enum {
    SCENE_HOME,
    SCENE_LIBRARY,
    SCENE_DOWNLOAD
} SceneType;


typedef struct {
    Rectangle rect;
    SoundMeta* meta;
} SongButton;

void init_scrn_manager(const int WIDTH, const int HIEGHT);
void update_scrn_manager(Vector2 mouse_pos);
void draw_scene(Font* font, AllSongs* songbook, AllSongs* queue, Playlist* playlists[MAX_PLAYLISTS], Vector2 mouse_pos);
bool found_in_playlist(Playlist* playlist, char name[MAX_FNAME_LEN]);
void remove_song_from_playlist(Playlist* playlist, char* song_to_remove);
void unload_textures();

#endif
