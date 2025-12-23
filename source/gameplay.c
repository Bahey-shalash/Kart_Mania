#include "map_selection.h"
#include <nds.h>
#include <string.h>
#include "context.h"
#include "game_types.h"
#include "scorching_sands_TL.h"
#include "scorching_sands_TC.h"
#include "scorching_sands_TR.h"
#include "scorching_sands_ML.h"
#include "scorching_sands_MC.h"
#include "scorching_sands_MR.h"
#include "scorching_sands_BL.h"
#include "scorching_sands_BC.h"
#include "scorching_sands_BR.h"

//=============================================================================
// Private variables
//=============================================================================
static int scrollX = 0;
static int scrollY = 0;
static const int SCROLL_SPEED = 2;

// Map bounds for 1024x1024 with overlapping quadrants
// With 256px offset between quadrants, effective range is 512 in each direction
static const int MAX_SCROLL_X = 768;
static const int MAX_SCROLL_Y = 832;

// Quadrant boundaries (256 pixels between each quadrant start)
static const int QUAD_OFFSET = 256;  // Distance between quadrant starting positions

// Track which quadrant is currently loaded
typedef enum {
    QUAD_TL = 0,  // Top-Left
    QUAD_TC = 1,  // Top-Center
    QUAD_TR = 2,  // Top-Right
    QUAD_ML = 3,  // Middle-Left
    QUAD_MC = 4,  // Middle-Center
    QUAD_MR = 5,  // Middle-Right
    QUAD_BL = 6,  // Bottom-Left
    QUAD_BC = 7,  // Bottom-Center
    QUAD_BR = 8   // Bottom-Right
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
        case QUAD_TC:
            tiles = scorching_sands_TCTiles;
            map = scorching_sands_TCMap;
            tilesLen = scorching_sands_TCTilesLen;
            break;
        case QUAD_TR:
            tiles = scorching_sands_TRTiles;
            map = scorching_sands_TRMap;
            tilesLen = scorching_sands_TRTilesLen;
            break;
        case QUAD_ML:
            tiles = scorching_sands_MLTiles;
            map = scorching_sands_MLMap;
            tilesLen = scorching_sands_MLTilesLen;
            break;
        case QUAD_MC:
            tiles = scorching_sands_MCTiles;
            map = scorching_sands_MCMap;
            tilesLen = scorching_sands_MCTilesLen;
            break;
        case QUAD_MR:
            tiles = scorching_sands_MRTiles;
            map = scorching_sands_MRMap;
            tilesLen = scorching_sands_MRTilesLen;
            break;
        case QUAD_BL:
            tiles = scorching_sands_BLTiles;
            map = scorching_sands_BLMap;
            tilesLen = scorching_sands_BLTilesLen;
            break;
        case QUAD_BC:
            tiles = scorching_sands_BCTiles;
            map = scorching_sands_BCMap;
            tilesLen = scorching_sands_BCTilesLen;
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
    // Quadrants transition at 256px intervals
    // At x=0-255: use left column (TL/ML/BL)
    // At x=256-511: use center column (TC/MC/BC)  
    // At x=512+: use right column (TR/MR/BR)
    
    int quadX = 0;
    int quadY = 0;
    
    // Determine horizontal quadrant (transitions at x=256 and x=512)
    if (x < 256) {
        quadX = 0;  // Left column
    } else if (x < 512) {
        quadX = 1;  // Center column (starts at overlap point)
    } else {
        quadX = 2;  // Right column (starts at second overlap point)
    }
    
    // Determine vertical quadrant (transitions at y=256 and y=512)
    if (y < 256) {
        quadY = 0;  // Top row
    } else if (y < 512) {
        quadY = 1;  // Middle row (starts at overlap point)
    } else {
        quadY = 2;  // Bottom row (starts at second overlap point)
    }
    
    return (QuadrantID)(quadY * 3 + quadX);
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
    // Each quadrant starts at a different position in the 1024x1024 map:
    // TL:(0,0), TC:(256,0), TR:(512,0)
    // ML:(0,256), MC:(256,256), MR:(512,256)  
    // BL:(0,512), BC:(256,512), BR:(512,512)
    
    int localX = scrollX;
    int localY = scrollY;
    
    // Determine which column (0, 1, or 2) and row (0, 1, or 2)
    int col = currentQuadrant % 3;
    int row = currentQuadrant / 3;
    
    // Subtract the quadrant's starting position in the original 1024x1024 image
    localX = scrollX - (col * QUAD_OFFSET);
    localY = scrollY - (row * QUAD_OFFSET);
    
    BG_OFFSET[0].x = localX;
    BG_OFFSET[0].y = localY;
}
