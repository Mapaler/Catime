/**
 * @file drawing.c
 * @brief Window drawing functionality implementation
 * 
 * This file implements the drawing-related functionality of the application window,
 * including text rendering, color settings, and window content drawing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "../include/drawing.h"
#include "../include/font.h"
#include "../include/color.h"
#include "../include/timer.h"
#include "../include/config.h"

// Variable imported from window_procedure.c
extern int elapsed_time;

// Using window drawing related constants defined in resource.h

/**
 * @brief Handle window painting
 * @param hwnd Window handle
 * @param ps Paint structure
 * 
 * Process the window's WM_PAINT message, performing the following operations:
 * 1. Create memory DC double buffering to prevent flickering
 * 2. Calculate remaining time/get current time based on mode
 * 3. Dynamically load font resources (supports real-time preview)
 * 4. Parse color configuration (supports HEX/RGB formats)
 * 5. Draw text using double buffering mechanism
 * 6. Automatically adjust window size to fit text content
 */
void HandleWindowPaint(HWND hwnd, PAINTSTRUCT *ps) {
    static char time_text[50];
    HDC hdc = ps->hdc;
    RECT rect;
    GetClientRect(hwnd, &rect);

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    SetGraphicsMode(memDC, GM_ADVANCED);
    SetBkMode(memDC, TRANSPARENT);
    SetStretchBltMode(memDC, HALFTONE);
    SetBrushOrgEx(memDC, 0, 0, NULL);

    // Generate display text based on different modes
    if (CLOCK_SHOW_CURRENT_TIME) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        int hour = tm_info->tm_hour;
        
        if (!CLOCK_USE_24HOUR) {
            if (hour == 0) {
                hour = 12;
            } else if (hour > 12) {
                hour -= 12;
            }
        }

        tm_info->tm_hour = hour;
        // if (CLOCK_SHOW_SECONDS) {
        //     sprintf(time_text, "%d:%02d:%02d", 
        //             hour, tm_info->tm_min, tm_info->tm_sec);
        // } else {
        //     sprintf(time_text, "%d:%02d", 
        //             hour, tm_info->tm_min);
        // }
        char *format;
        if (CLOCK_SHOW_SECONDS) {
            format = "%H:%M:%S";
        } else {
            format = "%H:%M";
        }
        strftime(time_text, sizeof(time_text), format, tm_info);

    } else if (CLOCK_COUNT_UP) {
        // Count-up mode
        int hours = countup_elapsed_time / 3600;
        int minutes = (countup_elapsed_time % 3600) / 60;
        int seconds = countup_elapsed_time % 60;

        // if (hours > 0) {
        //     sprintf(time_text, "%d:%02d:%02d", hours, minutes, seconds);
        // } else if (minutes > 0) {
        //     sprintf(time_text, "%d:%02d", minutes, seconds);
        // } else {
        //     sprintf(time_text, "%d", seconds);
        // }
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        tm_info->tm_hour = hours;
        tm_info->tm_min = minutes;
        tm_info->tm_sec = seconds;

        char* format;
        if (hours > 0 || CLOCK_SHOW_SECONDS) {
            format = "%H:%M:%S";
        }
        else {
            format = "%M:%S";
        }
        strftime(time_text, sizeof(time_text), format, tm_info);
    } else {
        // Countdown mode
        int remaining_time = CLOCK_TOTAL_TIME - countdown_elapsed_time;
        if (remaining_time <= 0) {
            // Timeout reached, decide whether to display content based on conditions
            if (CLOCK_TOTAL_TIME == 0 && countdown_elapsed_time == 0) {
                // This is the case after sleep operation, don't display anything
                time_text[0] = '\0';
            } else if (strcmp(CLOCK_TIMEOUT_TEXT, "0") == 0) {
                time_text[0] = '\0';
            } else if (strlen(CLOCK_TIMEOUT_TEXT) > 0) {
                strncpy(time_text, CLOCK_TIMEOUT_TEXT, sizeof(time_text) - 1);
                time_text[sizeof(time_text) - 1] = '\0';
            } else {
                time_text[0] = '\0';
            }
        } else {
            int hours = remaining_time / 3600;
            int minutes = (remaining_time % 3600) / 60;
            int seconds = remaining_time % 60;
            
            // if (hours > 0) {
            //     sprintf(time_text, "%d:%02d:%02d", hours, minutes, seconds);
            // } else if (minutes > 0) {
            //     sprintf(time_text, "%d:%02d", minutes, seconds);
            // } else {
            //     sprintf(time_text, "%d", seconds);
            // }
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            tm_info->tm_hour = hours;
            tm_info->tm_min = minutes;
            tm_info->tm_sec = seconds;

            char* format;
            if (hours > 0 || CLOCK_SHOW_SECONDS) {
                format = "%H:%M:%S";
            }
            else {
                format = "%M:%S";
            }
            strftime(time_text, sizeof(time_text), format, tm_info);
        }
    }

    const char* fontToUse = IS_PREVIEWING ? PREVIEW_FONT_NAME : FONT_FILE_NAME;
    HFONT hFont = CreateFont(
        -CLOCK_BASE_FONT_SIZE * CLOCK_FONT_SCALE_FACTOR,
        0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,   
        VARIABLE_PITCH | FF_SWISS,
        IS_PREVIEWING ? PREVIEW_INTERNAL_NAME : FONT_INTERNAL_NAME
    );
    HFONT oldFont = (HFONT)SelectObject(memDC, hFont);

    SetTextAlign(memDC, TA_LEFT | TA_TOP);
    SetTextCharacterExtra(memDC, 0);
    SetMapMode(memDC, MM_TEXT);

    DWORD quality = SetICMMode(memDC, ICM_ON);
    SetLayout(memDC, 0);

    const COLORREF COLOR_WHITE = RGB(0xFF, 0xFF, 0xFF);
    const COLORREF COLOR_BLACK = RGB(0, 0, 0);
    int r = 255, g = 255, b = 255;
    const char* colorToUse = IS_COLOR_PREVIEWING ? PREVIEW_COLOR : CLOCK_TEXT_COLOR;
    
    if (strlen(colorToUse) > 0) {
        if (colorToUse[0] == '#') {
            if (strlen(colorToUse) == 7) {
                sscanf(colorToUse + 1, "%02x%02x%02x", &r, &g, &b);
            }
        } else {
            sscanf(colorToUse, "%d,%d,%d", &r, &g, &b);
        }
    }
    
    COLORREF textColor = RGB(r, g, b);
    COLORREF strokeColor = RGB(0xFF - r, 0xFF - g, 0xFF - b);
    int strokeWidth = 2;

    SetTextColor(memDC, textColor);

    if (CLOCK_EDIT_MODE) {
        HBRUSH hBrush = CreateSolidBrush(RGB(20, 20, 20));  // Dark gray background
        FillRect(memDC, &rect, hBrush);
        DeleteObject(hBrush);
    } else {
        HBRUSH hBrush = CreateSolidBrush(COLOR_BLACK);
        FillRect(memDC, &rect, hBrush);
        DeleteObject(hBrush);
    }

    if (strlen(time_text) > 0) {
        SIZE textSize;
        GetTextExtentPoint32(memDC, time_text, strlen(time_text), &textSize);

        if (textSize.cx != (rect.right - rect.left) || 
            textSize.cy != (rect.bottom - rect.top)) {
            RECT windowRect;
            GetWindowRect(hwnd, &windowRect);
            
            SetWindowPos(hwnd, NULL,
                windowRect.left, windowRect.top,
                textSize.cx + WINDOW_HORIZONTAL_PADDING, 
                textSize.cy + WINDOW_VERTICAL_PADDING, 
                SWP_NOZORDER | SWP_NOACTIVATE);
            GetClientRect(hwnd, &rect);
        }

        
        int x = (rect.right - textSize.cx) / 2;
        int y = (rect.bottom - textSize.cy) / 2;


        SetTextColor(memDC, strokeColor);
        // 在周围位置多次绘制文本以形成描边效果
        for (int dx = -strokeWidth; dx <= strokeWidth; dx++)
        {
            for (int dy = -strokeWidth; dy <= strokeWidth; dy++)
            {
                // 跳过中心点（将在后面绘制）
                if (dx == 0 && dy == 0) continue;

                TextOutA(memDC, x + dx, y + dy, time_text, strlen(time_text));
            }
        }
        SetTextColor(memDC, textColor);
        for (int i = 0; i < 8; i++) {
            TextOutA(memDC, x, y, time_text, strlen(time_text));
        }
    }

    BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldFont);
    DeleteObject(hFont);
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}