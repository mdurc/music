#include <math.h>
#include <stdio.h>
#include <unistd.h>


#include "shared.h"

void draw_scrub_player(Font* font, Vector2 mouse_pos, Vector2 play_btn_center, float play_btn_radius, Rectangle* playback_line, float progress, bool playing, SoundMeta* sound){
    char time_stamps[10];
    char* play_text;
    Vector2 text_size;
    float dur = sound ? sound->duration: 0.0f;
    if(CheckCollisionPointCircle(mouse_pos, play_btn_center, play_btn_radius)){
        DrawCircleV(play_btn_center, play_btn_radius, LIGHTGRAY);
    }else{
        DrawCircleV(play_btn_center, play_btn_radius, GRAY);
    }
    play_text = !playing ? ">" : "~~";
    text_size = MeasureTextEx(*font, play_text, 20, 1);
    DrawTextEx(*font, play_text, (Vector2){play_btn_center.x - text_size.x/2.0f, play_btn_center.y - text_size.y/2.0f}, 20, 1, BLACK);

    text_size = MeasureTextEx(*font, "<-", 20, 1);
    DrawTextEx(*font, "<-", (Vector2){play_btn_center.x - play_btn_radius*2 - text_size.x, play_btn_center.y - text_size.y/2.0f}, 20, 1, WHITE);
    DrawTextEx(*font, "->", (Vector2){play_btn_center.x + play_btn_radius*2, play_btn_center.y - text_size.y/2.0f}, 20, 1, WHITE);

    DrawRectangleRounded(*playback_line, 0.5f, 8, GRAY);
    DrawRectangleRounded((Rectangle){playback_line->x, playback_line->y, playback_line->width * progress, playback_line->height}, 0.5f, 8, WHITE);


    snprintf(time_stamps, sizeof(time_stamps), "%d:%02d", (int)(progress*dur)/60, (int)(progress*dur) % 60);
    DrawTextEx(*font, time_stamps, (Vector2){playback_line->x + 11, playback_line->y + 15}, 20, 1, WHITE);
    snprintf(time_stamps, sizeof(time_stamps), "%d:%02d", (int)dur/60, (int)dur % 60);
    DrawTextEx(*font, time_stamps, (Vector2){playback_line->x + playback_line->width - 55, playback_line->y + 15}, 20, 1, WHITE);

    play_text = sound ? sound->file_name : "N/A";
    DrawTextEx(*font, TextFormat("%s", play_text), (Vector2){playback_line->x+10, playback_line->y-30}, 20, 1, WHITE);
    //DrawTextEx(*font, TextFormat("%s", sound->file_size), (Vector2){playback_line->x+130, playback_line->y-30}, 20, 1, WHITE);
    //play_text = sound ? sound->mod_date : "N/A"; // not using exiftool for mod date anymore
    //DrawTextEx(*font, TextFormat("%s", play_text), (Vector2){playback_line->width-20, playback_line->y-30}, 20, 1, WHITE);
}

void check_play_btn_pressed(Vector2 mouse_pos, Vector2 play_btn_center, float play_btn_radius, bool* playing, ma_sound* aud){
    if(CheckCollisionPointCircle(mouse_pos, play_btn_center, play_btn_radius) &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
        if (!(*playing)) {
            ma_sound_start(aud);
        } else {
            ma_sound_stop(aud);
        }
        *playing = !(*playing);
    }
}

void check_adjust_scrubber(Vector2 mouse_pos, Rectangle* playback_line, bool* dragging_scrubber,
            bool playing, SoundMeta* sound, unsigned int sample_rate){
    float new_time;
    if (*dragging_scrubber || (btn_pressed(mouse_pos,playback_line))) {
        if(ma_sound_is_playing(&sound->audio)) ma_sound_stop(&sound->audio);
        new_time = (mouse_pos.x - playback_line->x) / playback_line->width * sound->duration;
        ma_sound_seek_to_pcm_frame(&sound->audio, fmin(new_time*sample_rate, sound->duration*sample_rate));
        *dragging_scrubber = 1;
    }
    if (*dragging_scrubber && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        *dragging_scrubber = 0;
        if(playing){
            usleep(400000); // let the audio sync up if the scrubber was pressed and released quickly
            ma_sound_start(&sound->audio);
        }
    }

}

void handle_audio(Vector2 mouse_pos, Vector2 play_btn_center, float play_btn_radius,
        Rectangle* playback_line, bool* playing, float* progress, bool* dragging_scrubber, SoundMeta* sound, unsigned int sample_rate){
    if(sound==NULL) return;

    check_play_btn_pressed(mouse_pos, play_btn_center, play_btn_radius, playing, &sound->audio);

    check_adjust_scrubber(mouse_pos, playback_line, dragging_scrubber,
            *playing, sound, sample_rate);

    // update the progress
    ma_sound_get_cursor_in_seconds(&sound->audio, progress);
    *progress = *progress / sound->duration;
    
    // TODO: check if progress is complete
}
