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

void init_scrn_manager(const int WIDTH, const int HIEGHT, int* total_songs);
void update_scrn_manager(Vector2 mouse_pos);
void draw_scene(Font* font, Node* songbook[MAX_SONGS], Queue* queue, Vector2 mouse_pos, SoundMeta** current_song, bool* playing);

#endif
