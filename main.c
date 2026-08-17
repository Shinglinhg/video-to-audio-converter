#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define Rectangle Win32_Rectangle
#define CloseWindow Win32_CloseWindow
#define ShowCursor Win32_ShowCursor
#define LoadImageA Win32_LoadImageA
#define DrawTextA Win32_DrawTextA
#define DrawTextExA Win32_DrawTextExA

#include <windows.h>
#include <commdlg.h>

#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef LoadImageA
#undef DrawTextA
#undef DrawTextExA

#undef DrawText
#undef DrawTextEx
#undef LoadImage

#define GUBBY_SPEED 150
#define GUBBY_SCALE 2
#define UI_FONT_SIZE 30

static char message[1024] = "Selecione um video";
static char error_message[512] = "";

void draw_wrapped_text(Font font, const char *text, Vector2 pos, float font_size, float spacing, Color color)
{
    char line[512] = "";
    char word[128];

    float y = pos.y;
    int word_start = 0;

    for (int i = 0; ; i++) {
        if (text[i] == ' ' || text[i] == '\n' || text[i] == '\0') {
            int len = i - word_start;

            memcpy(word, text + word_start, len);
            word[len] = '\0';

            char test[512];
            snprintf(test, sizeof(test), "%s%s%s", line, line[0] ? " " : "", word);

            if (MeasureTextEx(font, test, font_size, spacing).x > 780) {
                DrawTextEx(font, line, (Vector2){pos.x, y}, font_size, spacing, color);
                y += font_size + 8;

                strcpy(line, word);
            } else {
                strcpy(line, test);
            }

            if (text[i] == '\n') {
                DrawTextEx(font, line, (Vector2){pos.x, y}, font_size, spacing, color);
                y += font_size + 8;
                line[0] = '\0';
            }

            word_start = i + 1;

            if (text[i] == '\0')
                break;
        }
    }

    if (line[0] != '\0')
        DrawTextEx(font, line, (Vector2){pos.x, y}, font_size, spacing, color);
}

const char *open_file_dialog(void)
{
    static char filename[MAX_PATH * 4];

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ZeroMemory(filename, sizeof(filename));

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = sizeof(filename);

    ofn.lpstrFilter =
        "Video Files\0*.mp4;*.mkv;*.avi;*.mov;*.webm\0"
        "All Files\0*.*\0";

    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
        return filename;

    return NULL;
}


void make_mp3_path(const char *input, char *output, size_t size)
{
    strncpy(output, input, size);
    output[size - 1] = '\0';

    char *dot = strrchr(output, '.');

    if (dot)
        strcpy(dot, ".mp3");
    else
        strcat(output, ".mp3");
}

void render_frame(Font font, Texture2D gubby, float* gubby_rotation) {
        BeginDrawing();
        ClearBackground((Color){30, 30, 30, 255});

        draw_wrapped_text(
            font,
            error_message[0] ? error_message : message,
            (Vector2){10, 10},
            UI_FONT_SIZE,
            1,
            error_message[0] ? RED : WHITE
        );
        DrawTexturePro(
        gubby,
        (Rectangle){0,0, gubby.width, gubby.height},
        (Rectangle){GetScreenWidth()/2, GetScreenHeight() - ((gubby.height * GUBBY_SCALE)), gubby.width * GUBBY_SCALE, gubby.height * GUBBY_SCALE},
        (Vector2){(gubby.width * GUBBY_SCALE) / 2, (gubby.height * GUBBY_SCALE) / 2},
        *gubby_rotation,
        WHITE
        );
        *gubby_rotation += GetFrameTime() * GUBBY_SPEED;
        EndDrawing();        
}

void format_display_path(const char *path, char *output, size_t size)
{
    const size_t max_chars = 55;
    size_t length = strlen(path);

    if (length <= max_chars) {
        strncpy(output, path, size);
        output[size - 1] = '\0';
        return;
    }

    const char *filename = strrchr(path, '\\');

    if (!filename)
        filename = path;
    else
        filename++;

    if (strlen(filename) + 10 >= max_chars) {
        snprintf(output, size, "...%s", filename);
        return;
    }

    size_t remaining = max_chars - strlen(filename) - 10;

    snprintf(
        output,
        size,
        "%.*s...\\%s",
        (int)remaining,
        path,
        filename
    );
}


int ffmpeg_convert(const char *input_path, const char *output_path)
{
    char ffmpeg_path[MAX_PATH * 4];

    snprintf(
        ffmpeg_path,
        sizeof(ffmpeg_path),
        "%sffmpeg.exe",
        GetApplicationDirectory()
    );

    char command_line[8192];

    snprintf(
        command_line,
        sizeof(command_line),
        "\"%s\" -y -i \"%s\" -vn -codec:a libmp3lame -q:a 2 \"%s\"",
        ffmpeg_path,
        input_path,
        output_path
    );

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);

    if (!CreateProcessA(NULL, command_line, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        snprintf(
            error_message,
            sizeof(error_message),
            "Nao foi possivel iniciar ffmpeg (%lu)",
            GetLastError()
        );

        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exit_code != 0) {
        snprintf(
            error_message,
            sizeof(error_message),
            "FFmpeg falhou (%lu)",
            exit_code
        );

        return 1;
    }

    return 0;
}


int main(void)
{
    InitWindow(800, 600, "Conversor de video 30000 pro mat :D");

    Font font = LoadFontEx("./ComicMono.ttf", UI_FONT_SIZE, NULL, 0);
    Texture2D gubby = LoadTexture(TextFormat("%s/%s", GetApplicationDirectory(), "data.png"));
    float gubby_rotation = .0f;

    render_frame(font, gubby, &gubby_rotation);
    
    const char *input = open_file_dialog();

    if (input) {
        char output[MAX_PATH * 4];
        char display_output[256];

        make_mp3_path(input, output, sizeof(output));
        format_display_path(output, display_output, sizeof(display_output));

        snprintf(
            message,
            sizeof(message),
            "Convertendo:\n%s\n\nOutput:\n%s",
            input,
            display_output
        );

        ffmpeg_convert(input, output);

        if (error_message[0] == '\0') {
            snprintf(
                message,
                sizeof(message),
                "Convertido!\n\nSalvo como:\n%s",
                display_output
            );
        }
    } else {
        snprintf(error_message, sizeof(error_message), "Nenhum Arquivo Selecionado.");
    }
    
    while (!WindowShouldClose()) {
        render_frame(font, gubby, &gubby_rotation);
    }

    UnloadFont(font);
    CloseWindow();

    return 0;
}
