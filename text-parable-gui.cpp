// Define this to avoid architecture-specific headers that cause issues on ARM Macs
#define SDL_DISABLE_IMMINTRIN_H 1

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <fstream>
#include <memory>
#include <sstream>
#include <algorithm>
#include <functional>
#include "json.hpp"

// Define window constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int FONT_SIZE = 16;
const char* FONT_PATH = "font.ttf"; // You'll need to provide this font file
const SDL_Color TEXT_COLOR = {220, 220, 220, 255}; // Light gray
const SDL_Color HIGHLIGHT_COLOR = {100, 200, 255, 255}; // Light blue
const SDL_Color BACKGROUND_COLOR = {30, 30, 40, 255}; // Dark blue-gray

// Define a minimal filesystem alternative for compatibility
namespace fs {
    // Same filesystem implementation from text-parable.cpp
    class path {
    private:
        std::string path_value;
    public:
        path() = default;
        path(const std::string& p) : path_value(p) {}
        path(const char* p) : path_value(p) {}
        
        path parent_path() const {
            size_t pos = path_value.find_last_of("/\\");
            if (pos == std::string::npos) return "";
            return path_value.substr(0, pos);
        }
        
        path operator/(const std::string& other) const {
            if (path_value.empty()) return other;
            if (path_value.back() == '/' || path_value.back() == '\\') 
                return path_value + other;
            return path_value + "/" + other;
        }
        
        std::string string() const { return path_value; }
        
        operator std::string() const { return path_value; }
    };
    
    bool exists(const path& p) {
        std::ifstream file(static_cast<std::string>(p));
        return file.good();
    }
}

using json = nlohmann::json;

// Constants
const fs::path SCRIPT_DIR = fs::path(__FILE__).parent_path();
const fs::path ACHIEVEMENTS_FILE = SCRIPT_DIR / "achievements.json";

// Ending achievements and funny achievements data
// These are the same as in text-parable.cpp
const std::vector<std::pair<std::string, std::string>> ENDING_DATA = {
    {"silent_worker", "Silent_Worker"},
    {"curiosity_cost", "Curiosity_Cost"},
    {"conformity_comfort", "Conformity_Comfort"},
    {"corporate_conspiracy", "Corporate_Conspiracy"},
    {"awakened", "Awakened"},
    {"ignorance_bliss", "Ignorance_Bliss"},
    {"rebellion_unleashed", "Rebellion_Unleashed"},
    {"silent_bystander", "Silent_Bystander"},
    {"eternal_worker", "Eternal_Worker"},
    {"secret_society", "Secret_Society"},
    {"lost_labyrinth", "Lost_Labyrinth"},
    {"desperate_escape", "Desperate_Escape"},
    {"hope_amidst_chaos", "Hope_Amidst_Chaos"}
};

const std::vector<std::pair<std::string, std::string>> FUNNY_ACHIEVEMENTS = {
    {"persistent_knocker", "Persistent_Knocker"},
    {"paperclip_hoarder", "Paperclip_Hoarder"},
    {"chair_spinner", "Chair_Spinner"},
    {"water_cooler_chat", "Water_Cooler_Chat"},
    {"over_caffeinated", "Over_Caffeinated"}
};

// Global achievements map (achievement ID -> bool)
std::map<std::string, bool> achievements;

// Global counter for door knocks
int door_knock_count = 0;

// SDL Globals
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
std::vector<std::string> text_buffer;
int scroll_offset = 0;
int max_lines_on_screen = 0;

// Forward declarations
class StoryNode;
void game_narrative();
void achievements_screen();
std::string title_screen_main_menu();
void render_text_buffer();
void clear_screen();

// SDL Helper functions
bool initialize_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return false;
    }
    
    if (TTF_Init() < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ttf could not initialize! TTF_Error: %s", TTF_GetError());
        return false;
    }
    
    window = SDL_CreateWindow("Text Parable", 
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                             WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window could not be created! SDL_Error: %s", SDL_GetError());
        return false;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Renderer could not be created! SDL_Error: %s", SDL_GetError());
        return false;
    }
    
    font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (!font) {
        // If font not found, try loading from system fonts
        font = TTF_OpenFont("/System/Library/Fonts/Menlo.ttc", FONT_SIZE); // macOS
        
        if (!font)
            font = TTF_OpenFont("C:\\Windows\\Fonts\\consola.ttf", FONT_SIZE); // Windows
            
        if (!font) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load font! TTF_Error: %s", TTF_GetError());
            return false;
        }
    }
    
    // Calculate max visible lines
    int font_height = TTF_FontHeight(font);
    max_lines_on_screen = (WINDOW_HEIGHT - 80) / font_height; // Leave space for UI elements
    
    return true;
}

void clean_up_sdl() {
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void clear_screen() {
    SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, BACKGROUND_COLOR.a);
    SDL_RenderClear(renderer);
}

void present_screen() {
    SDL_RenderPresent(renderer);
}

// Render a single line of text to the screen
void render_text(const std::string& text, int x, int y, SDL_Color color = TEXT_COLOR) {
    SDL_Surface* text_surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (text_surface) {
        SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
        if (text_texture) {
            SDL_Rect dest_rect = {x, y, text_surface->w, text_surface->h};
            SDL_RenderCopy(renderer, text_texture, NULL, &dest_rect);
            SDL_DestroyTexture(text_texture);
        }
        SDL_FreeSurface(text_surface);
    }
}

// Render the current text buffer to the screen
void render_text_buffer() {
    clear_screen();
    
    int line_height = TTF_FontHeight(font);
    int y_pos = 20; // Start with some padding
    
    int start_line = scroll_offset;
    int end_line = std::min(start_line + max_lines_on_screen, static_cast<int>(text_buffer.size()));
    
    for (int i = start_line; i < end_line; i++) {
        render_text(text_buffer[i], 20, y_pos);
        y_pos += line_height;
    }
    
    // Draw scrollbar if needed
    if (text_buffer.size() > max_lines_on_screen) {
        int scrollbar_height = WINDOW_HEIGHT - 40;
        int scrollbar_x = WINDOW_WIDTH - 20;
        int scrollbar_width = 10;
        
        // Background track
        SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
        SDL_Rect track_rect = {scrollbar_x, 20, scrollbar_width, scrollbar_height};
        SDL_RenderFillRect(renderer, &track_rect);
        
        // Handle
        float handle_ratio = static_cast<float>(max_lines_on_screen) / text_buffer.size();
        int handle_height = static_cast<int>(scrollbar_height * handle_ratio);
        int handle_y = 20 + static_cast<int>((scrollbar_height - handle_height) * 
                       (static_cast<float>(scroll_offset) / (text_buffer.size() - max_lines_on_screen)));
        
        SDL_SetRenderDrawColor(renderer, 150, 150, 170, 255);
        SDL_Rect handle_rect = {scrollbar_x, handle_y, scrollbar_width, handle_height};
        SDL_RenderFillRect(renderer, &handle_rect);
    }
    
    present_screen();
}

// Add text to the text buffer
void add_text(const std::string& text) {
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        text_buffer.push_back(line);
    }
    
    // Ensure most recent text is visible
    if (text_buffer.size() > max_lines_on_screen) {
        scroll_offset = text_buffer.size() - max_lines_on_screen;
    }
    
    render_text_buffer();
}

// Simple text input handling (wait for any key)
void wait_for_key() {
    SDL_Event event;
    bool waiting = true;
    while (waiting) {
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0); // Handle window close
            } else if (event.type == SDL_KEYDOWN) {
                waiting = false;
            } else if (event.type == SDL_MOUSEWHEEL) {
                if (event.wheel.y > 0) { // Scroll up
                    scroll_offset = std::max(0, scroll_offset - 1);
                } else if (event.wheel.y < 0) { // Scroll down
                    scroll_offset = std::min(static_cast<int>(text_buffer.size() - max_lines_on_screen), 
                                           scroll_offset + 1);
                }
                if (scroll_offset < 0) scroll_offset = 0;
                render_text_buffer();
            }
        }
        SDL_Delay(10); // Small delay to prevent hogging CPU
    }
}

// Display menu and return selected option index
int display_menu(const std::vector<std::string>& options) {
    int selected_item = 0;
    bool menu_active = true;
    
    int line_height = TTF_FontHeight(font);
    int menu_start_y = WINDOW_HEIGHT / 2;
    
    while (menu_active) {
        clear_screen();
        
        // Draw any current text
        render_text_buffer();
        
        // Draw menu options
        for (size_t i = 0; i < options.size(); i++) {
            int x = (WINDOW_WIDTH - options[i].length() * FONT_SIZE/2) / 2; // Approx centering
            int y = menu_start_y + i * line_height;
            
            if (i == selected_item) {
                render_text("> " + options[i] + " <", x - 20, y, HIGHLIGHT_COLOR);
            } else {
                render_text(options[i], x, y);
            }
        }
        
        present_screen();
        
        // Handle input
        SDL_Event event;
        if (SDL_WaitEvent(&event)) {
            if (event.type == SDL_QUIT) {
                exit(0);
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        selected_item = (selected_item - 1 + options.size()) % options.size();
                        break;
                    case SDLK_DOWN:
                        selected_item = (selected_item + 1) % options.size();
                        break;
                    case SDLK_RETURN:
                        menu_active = false;
                        break;
                }
            }
        }
    }
    
    return selected_item;
}

// Type text with a visual effect
void stream_text(const std::string& text, float delay = 0.02) {
    std::string current_line;
    for (char c : text) {
        if (c == '\n') {
            text_buffer.push_back(current_line);
            current_line.clear();
        } else {
            current_line += c;
        }
        
        // Update display
        if (c == '\n') {
            render_text_buffer();
        } else {
            // Last line update
            int line_height = TTF_FontHeight(font);
            int y_pos = 20 + (text_buffer.size() - scroll_offset) * line_height;
            
            clear_screen();
            for (size_t i = scroll_offset; i < text_buffer.size(); i++) {
                int current_y = 20 + (i - scroll_offset) * line_height;
                render_text(text_buffer[i], 20, current_y);
            }
            render_text(current_line, 20, y_pos);
            present_screen();
        }
        
        // Add delay for typing effect
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay * 1000)));
        }
        
        // Check for key press to skip typing effect
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                delay = 0; // Speed up to instant
            }
        }
    }
    
    // Add the final line if it's not empty
    if (!current_line.empty()) {
        text_buffer.push_back(current_line);
        render_text_buffer();
    }
}

// Load achievements from file
void load_achievements() {
    if (fs::exists(ACHIEVEMENTS_FILE)) {
        try {
            std::ifstream file(static_cast<std::string>(ACHIEVEMENTS_FILE));
            json j;
            file >> j;
            
            for (auto it = j.begin(); it != j.end(); ++it) {
                achievements[it->first] = it->second.bool_value_public();
            }
        } catch (...) {
            // Initialize with default values if loading fails
            for (const auto& pair : ENDING_DATA) {
                achievements[pair.first] = false;
            }
            for (const auto& pair : FUNNY_ACHIEVEMENTS) {
                achievements[pair.first] = false;
            }
        }
    } else {
        // Initialize with default values
        for (const auto& pair : ENDING_DATA) {
            achievements[pair.first] = false;
        }
        for (const auto& pair : FUNNY_ACHIEVEMENTS) {
            achievements[pair.first] = false;
        }
    }
}

// Save achievements to file
void save_achievements() {
    json j;
    for (const auto& pair : achievements) {
        j[pair.first] = pair.second;
    }
    std::ofstream file(static_cast<std::string>(ACHIEVEMENTS_FILE));
    file << j.dump(4); // Pretty print with 4-space indent
}

// STORY NODES - similar to the terminal version but adapted for GUI
class StoryNode {
public:
    std::string description;
    std::map<std::string, std::shared_ptr<StoryNode>> choices;
    std::string ending;
    std::string ending_id;
    
    StoryNode(const std::string& desc, 
              const std::map<std::string, std::shared_ptr<StoryNode>>& ch = {}, 
              const std::string& end = "", 
              const std::string& end_id = "") 
        : description(desc), choices(ch), ending(end), ending_id(end_id) {}
    
    virtual void play() {
        // Clear the text buffer for new scene
        text_buffer.clear();
        scroll_offset = 0;
        
        // Stream the scene description
        stream_text(description + "\n");
        
        // Handle ending if this is an ending node
        if (!ending.empty()) {
            if (!ending_id.empty()) {
                achievements[ending_id] = true;
                save_achievements();
            }
            stream_text("\n--- " + ending + " ---\n");
            add_text("\nPress any key to return to the main menu...");
            wait_for_key();
            return;
        }
        
        // Collect choice options
        std::vector<std::string> choice_texts;
        for (const auto& pair : choices) {
            choice_texts.push_back(pair.first);
        }
        
        // Add prompt for choices
        add_text("\nWhat will you do?");
        
        // Display menu and get selection
        int selected_idx = display_menu(choice_texts);
        
        // Clear the buffer for the next scene
        text_buffer.clear();
        scroll_offset = 0;
        
        // Navigate to the selected choice
        auto it = choices.begin();
        std::advance(it, selected_idx);
        it->second->play();
    }
    
    virtual ~StoryNode() {}
};

// Special node for the door knocking action
class KnockDoorNode : public StoryNode {
public:
    std::shared_ptr<StoryNode> return_node;
    
    KnockDoorNode(std::shared_ptr<StoryNode> ret_node) 
        : StoryNode(""), return_node(ret_node) {}
    
    void play() override {
        door_knock_count++;
        
        // Clear the text buffer
        text_buffer.clear();
        scroll_offset = 0;
        
        stream_text("You knock on the door. It doesn't open.\n");
        
        if (door_knock_count >= 5 && !achievements["persistent_knocker"]) {
            achievements["persistent_knocker"] = true;
            save_achievements();
            stream_text("\nAchievement Unlocked: Persistent_Knocker!\n");
        }
        
        add_text("\nPress any key to return to the boss's office...");
        wait_for_key();
        
        return_node->play();
    }
};

// Node for unlocking funny achievements
class FunnyAchievementNode : public StoryNode {
public:
    std::string achievement_id;
    std::string achievement_name;
    std::string message;
    std::shared_ptr<StoryNode> return_node;
    
    FunnyAchievementNode(const std::string& ach_id, 
                         const std::string& ach_name,
                         const std::string& msg, 
                         std::shared_ptr<StoryNode> ret_node)
        : StoryNode(""), achievement_id(ach_id), achievement_name(ach_name),
          message(msg), return_node(ret_node) {}
    
    void play() override {
        // Clear the text buffer
        text_buffer.clear();
        scroll_offset = 0;
        
        stream_text(message + "\n");
        
        if (!achievements[achievement_id]) {
            achievements[achievement_id] = true;
            save_achievements();
            stream_text("\nAchievement Unlocked: " + achievement_name + "!\n");
        }
        
        add_text("\nPress any key to return...");
        wait_for_key();
        
        return_node->play();
    }
};

// Create the game's story structure
void game_narrative() {
    // Creating story nodes with shared pointers
    auto ending_silent_worker = std::make_shared<StoryNode>(
        "You sit down and surrender to the hypnotic drone of the presentation.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Silent Worker\nYou spent your day in quiet compliance.",
        "silent_worker"
    );
    
    auto ending_curiosity_cost = std::make_shared<StoryNode>(
        "Your curiosity leads you to decode hidden symbols in the projection.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Curiosity's Cost\nSome truths are best left undiscovered.",
        "curiosity_cost"
    );
    
    auto ending_conformity_comfort = std::make_shared<StoryNode>(
        "You dismiss the oddities and blend into the mundane routine.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Conformity's Comfort\nRoutine soothes the mind, even if questions remain.",
        "conformity_comfort"
    );
    
    auto ending_corporate_conspiracy = std::make_shared<StoryNode>(
        "You pore over a dusty file in a hidden drawer, uncovering blueprints of a secret corporate agenda.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Corporate Conspiracy\nThe truth behind the facade is revealed—but at what cost?",
        "corporate_conspiracy"
    );

    // Create rest of the story nodes (meeting room branch)
    auto meeting_room_clues = std::make_shared<StoryNode>(
        "In the meeting room, your eyes wander over peculiar symbols flickering behind the projector.");
    
    meeting_room_clues->choices = {
        {"Investigate the symbols", ending_curiosity_cost},
        {"Ignore them and take your seat", ending_conformity_comfort}
    };
    
    auto meeting_room = std::make_shared<StoryNode>(
        "You enter the meeting room. The narrator instructs you to take a seat as the presentation begins.");
    
    auto return_to_meeting_room = std::make_shared<StoryNode>(
        "Deciding not to meddle with secrets you aren't ready to face, you return to the meeting room.");
    
    auto meeting_room_drawer = std::make_shared<StoryNode>(
        "While seated, you notice a small desk drawer left slightly ajar. "
        "Inside, a dusty file lies hidden, filled with cryptic memos and blueprints.");
    
    meeting_room_drawer->choices = {
        {"Read the file thoroughly", ending_corporate_conspiracy},
        {"Leave it untouched", return_to_meeting_room}
    };
    
    auto water_cooler_achievement = std::make_shared<FunnyAchievementNode>(
        "water_cooler_chat", "Water_Cooler_Chat",
        "You strike up a chat with the lonely water cooler.", meeting_room
    );
    
    // Set up meeting room choices after creating all nodes it references
    meeting_room->choices = {
        {"Sit down and comply", ending_silent_worker},
        {"Look around for clues", meeting_room_clues},
        {"Inspect the desk drawer", meeting_room_drawer},
        {"Chat with the water cooler", water_cooler_achievement}
    };
    
    return_to_meeting_room->choices = {{"Continue", meeting_room}};
    
    // Boss's office branch
    auto ending_awakened = std::make_shared<StoryNode>(
        "Inside, cryptic messages make your heart race as you awaken to a hidden reality.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Awakened\nThe wall's secrets have shattered your perception of reality.",
        "awakened"
    );
    
    auto ending_ignorance_bliss = std::make_shared<StoryNode>(
        "You choose ignorance and sit down, letting routine lull your senses.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Ignorance is Bliss\nSome mysteries are best left unexplored.",
        "ignorance_bliss"
    );
    
    auto ending_rebellion_unleashed = std::make_shared<StoryNode>(
        "You confront shadowy figures outside, sparking a volatile rebellion.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Rebellion Unleashed\nYou shatter the silence with your defiance.",
        "rebellion_unleashed"
    );
    
    auto ending_silent_bystander = std::make_shared<StoryNode>(
        "You retreat silently, forever marked as an observer of hidden truths.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Silent Bystander\nSome secrets remain unchallenged.",
        "silent_bystander"
    );
    
    auto boss_office_inside = std::make_shared<StoryNode>(
        "Inside the boss's office, you find cryptic messages scrawled on the walls.");
    
    boss_office_inside->choices = {
        {"Read the messages", ending_awakened},
        {"Ignore them and sit down", ending_ignorance_bliss}
    };
    
    auto eavesdrop = std::make_shared<StoryNode>(
        "Lingering outside the boss's office, you strain to catch hushed conversations.");
    
    eavesdrop->choices = {
        {"Confront the speakers", ending_rebellion_unleashed},
        {"Retreat silently", ending_silent_bystander}
    };
    
    auto boss_office = std::make_shared<StoryNode>(
        "You approach the boss's office. The door is slightly ajar, inviting yet mysterious.");
    
    auto coffee_achievement = std::make_shared<FunnyAchievementNode>(
        "over_caffeinated", "Over_Caffeinated",
        "You grab a cup of coffee from a nearby machine and feel a surge of energy.", boss_office
    );
    
    // Create the knock door node with a reference to boss_office
    auto knock_door = std::make_shared<KnockDoorNode>(boss_office);
    
    boss_office->choices = {
        {"Push the door open and enter", boss_office_inside},
        {"Knock on the door", knock_door},
        {"Wait outside and eavesdrop", eavesdrop},
        {"Grab a cup of coffee", coffee_achievement}
    };
    
    auto follow_narrator = std::make_shared<StoryNode>(
        "Heeding the narrator's voice, you rise from your desk and step into the unknown corridors.");
    
    follow_narrator->choices = {
        {"Enter the meeting room", meeting_room},
        {"Head to the boss's office", boss_office}
    };
    
    // Disobey Narrator Branch
    auto ending_eternal_worker = std::make_shared<StoryNode>(
        "You remain chained to your desk, lost in monotonous tasks.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Eternal Worker\nThe cycle of routine engulfs you.",
        "eternal_worker"
    );
    
    auto ending_secret_society = std::make_shared<StoryNode>(
        "Following ghostly whispers, you stumble upon a clandestine group plotting escape.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Secret Society\nYou join the underground network of the disillusioned.",
        "secret_society"
    );
    
    auto ending_lost_labyrinth = std::make_shared<StoryNode>(
        "Wandering endlessly, the corridors twist into a maze with no exit.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Lost in the Labyrinth\nYou become forever lost in a maze of sterile halls.",
        "lost_labyrinth"
    );
    
    auto explore_corridors = std::make_shared<StoryNode>(
        "Leaving your desk behind, you step into dim corridors where distant murmurs beckon.");
    
    explore_corridors->choices = {
        {"Follow the sound of whispers", ending_secret_society},
        {"Wander aimlessly", ending_lost_labyrinth}
    };
    
    auto ending_desperate_escape = std::make_shared<StoryNode>(
        "In a burst of determination, you climb out the window, embracing the risk of freedom.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Desperate Escape\nYou risk it all for a chance at escape.",
        "desperate_escape"
    );
    
    auto ending_hope_amidst_chaos = std::make_shared<StoryNode>(
        "You call for help, and amid the chaos, a glimmer of hope emerges.",
        std::map<std::string, std::shared_ptr<StoryNode>>{},
        "Ending: Hope Amidst Chaos\nEven in darkness, hope may yet be found.",
        "hope_amidst_chaos"
    );
    
    auto escape_attempt = std::make_shared<StoryNode>(
        "Refusing to be confined, you decide to leave the building altogether.");
