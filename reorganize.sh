#!/bin/bash
# Script to reorganize source files into folders using git mv

cd "$(dirname "$0")/source" || exit 1

# Create folder structure
mkdir -p core
mkdir -p math
mkdir -p gameplay/items
mkdir -p graphics
mkdir -p ui
mkdir -p network
mkdir -p audio
mkdir -p storage

# Move core files
git mv main.c core/
git mv context.c context.h core/
git mv game_types.h core/
git mv game_constants.h core/
git mv timer.c timer.h core/

# Move math files
git mv fixedmath2d.c fixedmath2d.h math/
git mv Car.c Car.h math/

# Move gameplay files
git mv gameplay.c gameplay.h gameplay/
git mv gameplay_logic.c gameplay_logic.h gameplay/
git mv wall_collision.c wall_collision.h gameplay/
git mv terrain_detection.c terrain_detection.h gameplay/

# Move items files (nested under gameplay)
git mv items.c Items.h gameplay/items/
git mv item_navigation.c item_navigation.h gameplay/items/

# Move graphics files
git mv graphics.c graphics.h graphics/
git mv color.h graphics/

# Move UI files
git mv home_page.c home_page.h ui/
git mv map_selection.c map_selection.h ui/
git mv settings.c settings.h ui/
git mv play_again.c play_again.h ui/
git mv multiplayer_lobby.c multiplayer_lobby.h ui/

# Move network files
git mv multiplayer.c multiplayer.h network/
git mv WiFi_minilib.c WiFi_minilib.h network/

# Move audio files
git mv sound.c sound.h audio/

# Move storage files
git mv storage.c storage.h storage/
git mv storage_pb.c storage_pb.h storage/

echo "✅ Source reorganization complete!"
echo ""
echo "New structure:"
echo "source/"
echo "├── core/          (main, context, types, constants, timer)"
echo "├── math/          (fixedmath2d, Car)"
echo "├── gameplay/      (gameplay, race logic, collision, terrain)"
echo "│   └── items/     (Items, item_navigation)"
echo "├── graphics/      (graphics, color)"
echo "├── ui/            (all menu screens)"
echo "├── network/       (multiplayer, WiFi)"
echo "├── audio/         (sound)"
echo "└── storage/       (storage, storage_pb)"
