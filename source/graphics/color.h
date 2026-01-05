/**
 * File: color.h
 * -------------
 * Description: Common ARGB15 color constants shared across UI and gameplay
 *              rendering. Centralizes palette choices for highlights, toggles,
 *              and menu accents.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */
#ifndef COLOR_H
#define COLOR_H

#define BLACK RGB15(0, 0, 0)
#define WHITE RGB15(31, 31, 31)
#define RED RGB15(31, 0, 0)
#define GREEN RGB15(0, 31, 0)
#define BLUE RGB15(0, 0, 31)
#define YELLOW RGB15(31, 31, 0)
#define DARK_GREEN RGB15(0, 20, 0)
#define DARK_GRAY RGB15(20, 20, 20)
#define TEAL RGB15(0, 20, 31)
#define MENU_BUTTON_HIGHLIGHT_COLOR RGB15(10, 15, 22)
#define MENU_HIGHLIGHT_OFF_COLOR BLACK
#define TOGGLE_ON_COLOR RGB15(6, 26, 6)
#define TOGGLE_OFF_COLOR RGB15(28, 4, 4)
#define SETTINGS_SELECT_COLOR RGB15(20, 20, 20)
#define SP_SELECT_COLOR RGB15(20, 20, 20)

#endif  // COLOR_H
