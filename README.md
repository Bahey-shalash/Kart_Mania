# Kart Mania

A Nintendo DS kart racing game with single-player and multiplayer support.

---

## Code Documentation

### Play Again Screen (`ui/play_again.c`)

#### Purpose
Post-race screen allowing players to restart the race or return to the main menu.

#### Features
- **YES/NO button selection** with visual highlighting
- **Multiple input methods**: D-pad, touchscreen, and SELECT button
- **Automatic cleanup**: Stops race timers and cleans up multiplayer sessions when exiting

#### Architecture

**Graphics System:**
- **BG0 (Priority 0)**: Main "Play Again?" screen graphics
- **BG1 (Priority 1)**: Selection highlight layer (blue for YES, red for NO)
- Uses palette indices 240-241 for dynamic highlight tinting

**Input Handling:**

| Input | Action |
|-------|--------|
| LEFT / RIGHT | Select YES / NO |
| UP / DOWN | Toggle selection |
| A Button | Confirm selection |
| Touch | Select button by touching hitbox |
| SELECT | Quit directly to home (skip confirmation) |

**State Management:**
- Tracks current and previous selection to update highlights efficiently
- Only one button can be selected at a time

#### Usage
```c
// Initialize the screen
PlayAgain_Initialize();

// In main loop
while (1) {
    GameState next = PlayAgain_Update();
    if (next == GAMEPLAY) {
        // Restart race
    } else if (next == HOME_PAGE) {
        // Return to menu
    }
}
```

#### Constants (see `game_constants.h`)
- `PA_SELECTION_PAL_BASE` - Palette base index (240)
- `PA_YES_TOUCH_X/Y_MIN/MAX` - YES button touchscreen hitbox
- `PA_NO_TOUCH_X/Y_MIN/MAX` - NO button touchscreen hitbox
- `PA_YES/NO_RECT_X/Y_START/END` - Selection rectangle tile coordinates

#### Design Notes
- **Critical cleanup order**: Race timers must be stopped *before* multiplayer cleanup to avoid race conditions
- **Highlight rendering**: Uses BG layer instead of sprites for smooth, flicker-free highlighting
- **Touch validation**: Coordinates are checked against screen bounds before processing

---

### Settings Screen (`ui/settings.c`)

#### Purpose
Configuration screen for WiFi, music, and sound effects settings with persistent storage.

#### Features
- **Three toggle settings**: WiFi, Music, Sound FX (ON/OFF pills)
- **Save functionality**: Persist settings to storage (START+SELECT to reset to defaults)
- **Back/Home buttons**: Return to main menu
- **Dual-screen UI**: Bitmap graphics on top screen, interactive controls on bottom screen

#### Architecture

**Graphics System:**
- **Main Screen (Top)**: Static bitmap showing settings title/logo
- **Sub Screen (Bottom)**:
  - **BG0 (Priority 0)**: Main settings interface graphics
  - **BG1 (Priority 1)**: Toggle pills (red=OFF, green=ON) and selection highlights
- Uses palette indices 244-249 for selection highlighting
- Uses palette indices 254-255 for toggle pills (red/green)

**Input Handling:**

| Input | Action |
|-------|--------|
| UP / DOWN | Cycle through buttons |
| LEFT / RIGHT | Navigate bottom row (Save/Back/Home) |
| A Button | Toggle setting or activate button |
| Touch | Select by touching text labels or pills |
| START + SELECT + A | Reset to default settings |

**State Management:**
- Tracks current and previous selection for highlighting
- Settings changes are immediate (visual + functional)
- Persists to storage only when Save button is pressed

#### Usage
```c
// Initialize the screen
Settings_Initialize();

// In main loop
while (1) {
    GameState next = Settings_Update();
    if (next == HOME_PAGE) {
        // Return to menu
    }
}
```

#### Constants (see `game_constants.h`)
- `SETTINGS_SELECTION_PAL_BASE` - Palette base index (244)
- `SETTINGS_TOGGLE_START_X/WIDTH` - Toggle pill position and size
- `SETTINGS_WIFI/MUSIC/SOUNDFX_TOGGLE_Y_START/END` - Toggle pill Y coordinates
- `SETTINGS_WIFI/MUSIC/SOUNDFX_RECT_*` - Selection rectangle tile coordinates
- `SETTINGS_SAVE/BACK/HOME_RECT_*` - Button selection rectangle coordinates
- `SETTINGS_*_TEXT/PILL_X/Y_MIN/MAX` - Touch hitbox coordinates

#### Design Notes
- **Dual hitboxes**: Each toggle has two touch areas (text label + pill) for better UX
- **Sound FX timing**: Plays ding sound *before* potentially muting itself
- **Secret reset**: Hold START+SELECT while pressing A on Save to reset to defaults
- **Immediate application**: Settings take effect immediately, but only persist on explicit save
- **Long function split**: Touch input handling split into 4 helper functions for maintainability

---

### Home Page Screen (`ui/home_page.c`)

#### Purpose
Main menu screen with single player, multiplayer, and settings options, featuring animated kart sprite.

#### Features
- **Three menu buttons**: Single Player, Multiplayer, Settings
- **Animated sprite**: Kart scrolling across top screen
- **WiFi check**: Prevents multiplayer if WiFi is disabled in settings
- **Dual-screen UI**: Animated graphics on top, interactive menu on bottom

#### Architecture

**Graphics System:**
- **Main Screen (Top)**: Bitmap background with animated kart sprite (64x64px, scrolls left-to-right)
- **Sub Screen (Bottom)**:
  - **BG0 (Priority 0)**: Main menu graphics
  - **BG1 (Priority 1)**: Selection highlight layer
- Uses palette indices 251-253 for button highlights
- VBlank interrupt for smooth sprite animation

**Input Handling:**

| Input | Action |
|-------|--------|
| UP / DOWN | Cycle through menu buttons |
| A Button | Activate selected button |
| Touch | Select button by touching hitbox |

**State Management:**
- Tracks current and previous selection for highlighting
- Multiplayer button checks WiFi settings before proceeding
- Handles multiplayer init failure with REINIT_HOME state

#### Usage
```c
// Initialize the screen
HomePage_Initialize();

// In VBlank ISR
HomePage_OnVBlank();  // Animate kart sprite

// In main loop
while (1) {
    GameState next = HomePage_Update();
    if (next == MAPSELECTION) {
        // Go to map selection
    } else if (next == MULTIPLAYER_LOBBY) {
        // Go to multiplayer lobby
    } else if (next == SETTINGS) {
        // Go to settings
    } else if (next == REINIT_HOME) {
        // Multiplayer init failed, reinit home
    }
}

// When exiting
HomePage_Cleanup();  // Free sprite graphics
```

#### Constants (see `game_constants.h`)
- `HOME_HL_PAL_BASE` - Palette base index (251)
- `HOME_MENU_X/WIDTH/HEIGHT/SPACING/Y_START` - Menu layout parameters
- `HOME_HIGHLIGHT_TILE_X/WIDTH/HEIGHT` - Highlight tile parameters
- `HOME_KART_INITIAL_X/Y/OFFSCREEN_X` - Kart sprite animation parameters

#### Design Notes
- **WiFi validation**: Plays ding sound and stays on home page if WiFi disabled for multiplayer
- **Sprite cleanup**: Must call `HomePage_Cleanup()` before state transition to free OAM graphics
- **VBlank callback**: `HomePage_OnVBlank()` called from timer ISR for smooth animation
- **Array-driven hitboxes**: Touch detection uses array lookup for maintainability
- **Reinit state**: REINIT_HOME allows multiplayer failure recovery without losing home page state

---

## Getting started

To make it easy for you to get started with GitLab, here's a list of recommended next steps.

Already a pro? Just edit this README.md and make it your own. Want to make it easy? [Use the template at the bottom](#editing-this-readme)!

## Add your files

- [ ] [Create](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#create-a-file) or [upload](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#upload-a-file) files
- [ ] [Add files using the command line](https://docs.gitlab.com/topics/git/add_files/#add-files-to-a-git-repository) or push an existing Git repository with the following command:

```
cd existing_repo
git remote add origin https://gitlab.epfl.ch/shalash/kart-mania.git
git branch -M main
git push -uf origin main
```

## Integrate with your tools

- [ ] [Set up project integrations](https://gitlab.epfl.ch/shalash/kart-mania/-/settings/integrations)

## Collaborate with your team

- [ ] [Invite team members and collaborators](https://docs.gitlab.com/ee/user/project/members/)
- [ ] [Create a new merge request](https://docs.gitlab.com/ee/user/project/merge_requests/creating_merge_requests.html)
- [ ] [Automatically close issues from merge requests](https://docs.gitlab.com/ee/user/project/issues/managing_issues.html#closing-issues-automatically)
- [ ] [Enable merge request approvals](https://docs.gitlab.com/ee/user/project/merge_requests/approvals/)
- [ ] [Set auto-merge](https://docs.gitlab.com/user/project/merge_requests/auto_merge/)

## Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/)
- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!). Thanks to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.
