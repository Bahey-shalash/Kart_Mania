#include "map_selection.h"
#include <nds.h>
#include <string.h>
#include "context.h"
#include "game_types.h"
#include "scorching_sands_TL.h"
#include "scorching_sands_TR.h"
#include "scorching_sands_BL.h"
#include "scorching_sands_BR.h"

//=============================================================================
// Private variables
//=============================================================================
static int scrollX = 0;
static int scrollY = 0;
static const int SCROLL_SPEED = 2;

// Map bounds for 768x768 with overlapping quadrants
// With 256px overlap, effective range is 768-256 = 512 in each direction
static const int MAX_SCROLL_X = 512;
static const int MAX_SCROLL_Y = 512;

// Overlap boundaries (256 pixels from each edge of 512)
static const int OVERLAP_START = 256;  // When to start transition
static const int OVERLAP_END = 512;    // End of current quadrant

// Track which quadrants are currently loaded
typedef enum {
    QUAD_TL = 0,
    QUAD_TR = 1,
    QUAD_BL = 2,
    QUAD_BR = 3
} QuadrantID;

static QuadrantID currentQuadrant = QUAD_TL;

//=============================================================================
// Private function prototypes
//=============================================================================
static void configureGraphics_MAIN_GAMEPLAY(void);
static void configBG_Main_GAMEPLAY(void);
static void updateScroll(void);
static void loadQuadrant(QuadrantID quad);
static QuadrantID determineQuadrant(int x, int y);

//=============================================================================
// Public API implementation
//=============================================================================
void Gameplay_initialize(void) {
    configureGraphics_MAIN_GAMEPLAY();
    configBG_Main_GAMEPLAY();
    
    // Start at the top-left
    scrollX = 0;
    scrollY = 0;
    currentQuadrant = QUAD_TL;
}

GameState Gameplay_update(void) {
    scanKeys();
    int keys = keysHeld();
    
    // Handle D-pad input for scrolling (diagonal movement supported)
    if (keys & KEY_LEFT) {
        scrollX -= SCROLL_SPEED;
        if (scrollX < 0) scrollX = 0;
    }
    if (keys & KEY_RIGHT) {
        scrollX += SCROLL_SPEED;
        if (scrollX > MAX_SCROLL_X) scrollX = MAX_SCROLL_X;
    }
    if (keys & KEY_UP) {
        scrollY -= SCROLL_SPEED;
        if (scrollY < 0) scrollY = 0;
    }
    if (keys & KEY_DOWN) {
        scrollY += SCROLL_SPEED;
        if (scrollY > MAX_SCROLL_Y) scrollY = MAX_SCROLL_Y;
    }
    
    updateScroll();
    
    return GAMEPLAY;
}

//=============================================================================
// GRAPHICS SETUP
//=============================================================================
static void configureGraphics_MAIN_GAMEPLAY(void) {
    REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

static void configBG_Main_GAMEPLAY(void) {
    Map SelectedMap = GameContext_GetMap();
    
    switch(SelectedMap){
        case ScorchingSands:
            // Configure only BG0
            BGCTRL[0] = BG_64x64 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_PRIORITY(3);
            
            // Load palette (same for all quadrants)
            dmaCopy(scorching_sands_TLPal, BG_PALETTE, scorching_sands_TLPalLen);
            
            // Load initial quadrant (Top-Left)
            loadQuadrant(QUAD_TL);
            break;
            
        case AlpinRush:
            break;
        case NeonCircuit:
            break;
    }
}

static void loadQuadrant(QuadrantID quad) {
    const unsigned short* tiles = NULL;
    const unsigned short* map = NULL;
    unsigned int tilesLen = 0;
    
    switch(quad) {
        case QUAD_TL:
            tiles = scorching_sands_TLTiles;
            map = scorching_sands_TLMap;
            tilesLen = scorching_sands_TLTilesLen;
            break;
        case QUAD_TR:
            tiles = scorching_sands_TRTiles;
            map = scorching_sands_TRMap;
            tilesLen = scorching_sands_TRTilesLen;
            break;
        case QUAD_BL:
            tiles = scorching_sands_BLTiles;
            map = scorching_sands_BLMap;
            tilesLen = scorching_sands_BLTilesLen;
            break;
        case QUAD_BR:
            tiles = scorching_sands_BRTiles;
            map = scorching_sands_BRMap;
            tilesLen = scorching_sands_BRTilesLen;
            break;
    }
    
    // Load tiles
    dmaCopy(tiles, BG_TILE_RAM(1), tilesLen);
    
    // Load map (64x64 tile map for 512x512 image)
    for(int i=0; i<32; i++) {
        dmaCopy(&map[i*64], &BG_MAP_RAM(0)[i*32], 64);
        dmaCopy(&map[i*64+32], &BG_MAP_RAM(1)[i*32], 64);
        dmaCopy(&map[(i+32)*64], &BG_MAP_RAM(2)[i*32], 64);
        dmaCopy(&map[(i+32)*64+32], &BG_MAP_RAM(3)[i*32], 64);
    }
}

static QuadrantID determineQuadrant(int x, int y) {
    // Check if we're in overlap regions and should transition
    // Overlap starts at 256px (middle of each quadrant)
    
    // Horizontal position determines left vs right quadrants
    bool inRightOverlap = (x >= OVERLAP_START);
    
    // Vertical position determines top vs bottom quadrants
    bool inBottomOverlap = (y >= OVERLAP_START);
    
    // Determine which quadrant to load based on position
    if (!inRightOverlap && !inBottomOverlap) {
        return QUAD_TL;  // Top-left region (0-255, 0-255)
    }
    else if (inRightOverlap && !inBottomOverlap) {
        return QUAD_TR;  // Top-right region (256-512, 0-255)
    }
    else if (!inRightOverlap && inBottomOverlap) {
        return QUAD_BL;  // Bottom-left region (0-255, 256-512)
    }
    else {
        return QUAD_BR;  // Bottom-right region (256-512, 256-512)
    }
}

static void updateScroll(void) {
    // Determine which quadrant we should be viewing based on scroll position
    QuadrantID newQuadrant = determineQuadrant(scrollX, scrollY);
    
    // If we've moved into a new quadrant's territory, load it
    if (newQuadrant != currentQuadrant) {
        loadQuadrant(newQuadrant);
        currentQuadrant = newQuadrant;
    }
    
    // Calculate local scroll offset within current quadrant
    // The offset into the currently loaded 512x512 quadrant
    int localX = scrollX;
    int localY = scrollY;
    
    // Adjust scroll position based on which quadrant we're in
    // Since quadrants overlap by 256px, we need to offset appropriately
    switch(currentQuadrant) {
        case QUAD_TL:
            // TL starts at (0,0) in the 768x768 map
            // No adjustment needed
            break;
        case QUAD_TR:
            // TR starts at (256,0) in the 768x768 map
            // When scrollX >= 256, we want to show from the beginning of TR
            localX = scrollX - OVERLAP_START;
            break;
        case QUAD_BL:
            // BL starts at (0,256) in the 768x768 map
            localY = scrollY - OVERLAP_START;
            break;
        case QUAD_BR:
            // BR starts at (256,256) in the 768x768 map
            localX = scrollX - OVERLAP_START;
            localY = scrollY - OVERLAP_START;
            break;
    }
    
    BG_OFFSET[0].x = localX;
    BG_OFFSET[0].y = localY;
}