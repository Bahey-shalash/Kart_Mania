#!/bin/bash
# Script to fix #include paths after reorganization

cd "$(dirname "$0")/source" || exit 1

echo "🔧 Fixing include paths..."

# Function to update includes in a file
fix_includes_in_file() {
    local file=$1
    local dir=$(dirname "$file")

    # Calculate relative path from current directory to source root
    local depth=$(echo "$dir" | tr -cd '/' | wc -c)
    local prefix=""
    for ((i=0; i<depth; i++)); do
        prefix="../$prefix"
    done

    # Core files (context.h, game_types.h, game_constants.h, timer.h)
    sed -i '' 's|#include "context\.h"|#include "'${prefix}'core/context.h"|g' "$file"
    sed -i '' 's|#include "game_types\.h"|#include "'${prefix}'core/game_types.h"|g' "$file"
    sed -i '' 's|#include "game_constants\.h"|#include "'${prefix}'core/game_constants.h"|g' "$file"
    sed -i '' 's|#include "timer\.h"|#include "'${prefix}'core/timer.h"|g' "$file"

    # Math files (fixedmath2d.h, Car.h)
    sed -i '' 's|#include "fixedmath2d\.h"|#include "'${prefix}'math/fixedmath2d.h"|g' "$file"
    sed -i '' 's|#include "Car\.h"|#include "'${prefix}'math/Car.h"|g' "$file"

    # Gameplay files
    sed -i '' 's|#include "gameplay\.h"|#include "'${prefix}'gameplay/gameplay.h"|g' "$file"
    sed -i '' 's|#include "gameplay_logic\.h"|#include "'${prefix}'gameplay/gameplay_logic.h"|g' "$file"
    sed -i '' 's|#include "wall_collision\.h"|#include "'${prefix}'gameplay/wall_collision.h"|g' "$file"
    sed -i '' 's|#include "terrain_detection\.h"|#include "'${prefix}'gameplay/terrain_detection.h"|g' "$file"

    # Items files (nested under gameplay)
    sed -i '' 's|#include "Items\.h"|#include "'${prefix}'gameplay/items/Items.h"|g' "$file"
    sed -i '' 's|#include "item_navigation\.h"|#include "'${prefix}'gameplay/items/item_navigation.h"|g' "$file"

    # Graphics files
    sed -i '' 's|#include "graphics\.h"|#include "'${prefix}'graphics/graphics.h"|g' "$file"
    sed -i '' 's|#include "color\.h"|#include "'${prefix}'graphics/color.h"|g' "$file"

    # UI files
    sed -i '' 's|#include "home_page\.h"|#include "'${prefix}'ui/home_page.h"|g' "$file"
    sed -i '' 's|#include "map_selection\.h"|#include "'${prefix}'ui/map_selection.h"|g' "$file"
    sed -i '' 's|#include "settings\.h"|#include "'${prefix}'ui/settings.h"|g' "$file"
    sed -i '' 's|#include "play_again\.h"|#include "'${prefix}'ui/play_again.h"|g' "$file"
    sed -i '' 's|#include "multiplayer_lobby\.h"|#include "'${prefix}'ui/multiplayer_lobby.h"|g' "$file"

    # Network files
    sed -i '' 's|#include "multiplayer\.h"|#include "'${prefix}'network/multiplayer.h"|g' "$file"
    sed -i '' 's|#include "WiFi_minilib\.h"|#include "'${prefix}'network/WiFi_minilib.h"|g' "$file"

    # Audio files
    sed -i '' 's|#include "sound\.h"|#include "'${prefix}'audio/sound.h"|g' "$file"

    # Storage files
    sed -i '' 's|#include "storage\.h"|#include "'${prefix}'storage/storage.h"|g' "$file"
    sed -i '' 's|#include "storage_pb\.h"|#include "'${prefix}'storage/storage_pb.h"|g' "$file"
}

# Find all .c and .h files and fix their includes
find . -type f \( -name "*.c" -o -name "*.h" \) | while read file; do
    echo "  Fixing: $file"
    fix_includes_in_file "$file"
done

echo ""
echo "✅ Include paths fixed!"
echo ""
echo "⚠️  Note: Files in the same directory will have relative paths like:"
echo "   - core/main.c includes core/context.h → #include \"context.h\""
echo "   - ui/home_page.c includes core/game_types.h → #include \"../core/game_types.h\""
