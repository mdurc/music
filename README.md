# music player in c

## Features

- **Download Songs**
  - Download songs directly within the app from a YouTube link.
  - Paste the code after `?v=` from `/watch?v=vZi8ET9k11g` into the app: `vZi8ET9k11g`
  - Use `Ctrl+V` to paste.
  - Access the download feature through the download button on the sidebar.

- **Queue Management**
  - Add songs to the queue by right-clicking on a song in the list.
  - Right-clicking on a queued song removes it from the queue.

- **Song Playback**
  - Scroll through downloaded songs and play any by left-clicking.
  - Skip songs using the on-screen arrows to play the next song in the queue or a random song if the queue is empty.
  - If a song finishes, the next in the queue or a random song from the playlist plays.

- **Search**
  - Search for songs within your current playlist.
  - The current playlist is displayed as a scrollable list on the home screen.

- **Playlist Management**
  - Create new playlists easily.
  - Add or remove songs from a playlist by pressing the right arrow key, when hovering over a song, and selecting an option from the popup. Exit any popup with `<Esc>`.
  - A default playlist, `All Songs`, holds all songs and is non-removable.
  - Right-click any custom playlist to delete it.
  - Create new playlists with the onscreen `+` button, with a maximum playlist limit.
  - Acces the playlists through the library icon on the sidebar

- **Playlist Persistence**
  - Custom playlists are saved in `playlists.txt` and automatically loaded when you reopen the program.

- **Resizing UI**
  - The UI resizes when the window size changes (not extensively).

## Compile and Run

```bash
gcc main.c scrubber.c scene_manager.c -lraylib && ./a.out
# or
gcc *.c -lraylib && ./a.out
```
