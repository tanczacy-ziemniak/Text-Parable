#include <ncurses.h>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <fstream>
#include <memory>
#include "json.hpp" // Using the simplified json library

// Define a minimal filesystem alternative for compatibility
namespace fs {
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

// Ending achievements data: (ID, Display Name)
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

// Funny achievements: (ID, Display Name)
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

// Forward declarations
class StoryNode;
void game_narrative();
void achievements_screen();
std::string title_screen_main_menu();

// Safe version of addstr that won't crash on terminal boundary issues
void safe_addstr(int y, int x, const std::string& text, int attr = 0) {
    try {
        if (attr)
            attron(attr);
        
        move(y, x);
        printw("%s", text.c_str());
        
        if (attr)
            attroff(attr);
    } catch (...) {
        // Ignore any errors
    }
}

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

void save_achievements() {
    json j;
    for (const auto& pair : achievements) {
        j[pair.first] = pair.second;
    }
    std::ofstream file(static_cast<std::string>(ACHIEVEMENTS_FILE));
    file << j.dump(4); // Pretty print with 4-space indent
}

// Stream text with typewriter effect
int stream_text(const std::string& text, float delay = 0.02, int start_y = 0, int start_x = 0) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int y = start_y, x = start_x;
    
    for (char c : text) {
        if (c == '\n') {
            y++;
            x = start_x;
        } else {
            if (x >= max_x) {
                y++;
                x = start_x;
            }
            if (y >= max_y) {
                break;
            }
            try {
                mvaddch(y, x, c);
            } catch (...) {
                // Ignore errors
            }
            x++;
        }
        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay * 1000)));
    }
    return y;
}

// Display a menu of choices
int display_menu(const std::vector<std::string>& choices, int start_y) {
    int current_idx = 0;
    int n_choices = choices.size();
    std::string header_text = "Use the UP and DOWN arrow keys to navigate and ENTER to select:";
    int h, w;
    getmaxyx(stdscr, h, w);
    int menu_lines = 1 + n_choices; // header plus one line per choice
    
    while (true) {
        for (int i = 0; i < menu_lines; i++) {
            move(start_y + i, 0);
            clrtoeol();
        }
        int header_x = (w - header_text.length()) / 2;
        safe_addstr(start_y, header_x, header_text);
        
        for (int i = 0; i < n_choices; i++) {
            int choice_x = (w - choices[i].length()) / 2;
            if (i == current_idx)
                safe_addstr(start_y + 1 + i, choice_x, choices[i], A_REVERSE);
            else
                safe_addstr(start_y + 1 + i, choice_x, choices[i]);
        }
        refresh();
        
        int key = getch();
        if (key == KEY_UP) {
            current_idx = (current_idx - 1 + n_choices) % n_choices;
        } else if (key == KEY_DOWN) {
            current_idx = (current_idx + 1) % n_choices;
        } else if (key == '\n' || key == '\r') {
            return current_idx;
        }
    }
}

// Base class for story nodes
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
        clear();
        int final_y = stream_text(description + "\n", 0.02, 0, 0);
        safe_addstr(final_y + 1, 0, "Press any key to see your choices...");
        refresh();
        getch();
        
        if (!ending.empty()) {
            if (!ending_id.empty()) {
                achievements[ending_id] = true;
                save_achievements();
            }
            stream_text("\n--- " + ending + " ---\n");
            safe_addstr(0, 0, "\nPress any key to return to the main menu...");
            refresh();
            getch();
            return;
        }
        
        std::vector<std::string> choice_texts;
        for (const auto& pair : choices) {
            choice_texts.push_back(pair.first);
        }
        
        int menu_start_y = final_y + 3;
        int selected_idx = display_menu(choice_texts, menu_start_y);
        clear();
        
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
        clear();
        stream_text("You knock on the door. It doesn't open.\n", 0.02, 0, 0);
        
        if (door_knock_count >= 5 && !achievements["persistent_knocker"]) {
            achievements["persistent_knocker"] = true;
            save_achievements();
            stream_text("\nAchievement Unlocked: Persistent_Knocker!\n", 0.02, 0, 0);
        }
        
        safe_addstr(0, 0, "\nPress any key to return to the boss's office...");
        refresh();
        getch();
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
        clear();
        stream_text(message + "\n", 0.02, 0, 0);
        
        if (!achievements[achievement_id]) {
            achievements[achievement_id] = true;
            save_achievements();
            stream_text("\nAchievement Unlocked: " + achievement_name + "!\n", 0.02, 0, 0);
        }
        
        safe_addstr(0, 0, "\nPress any key to return...");
        refresh();
        getch();
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
    
    escape_attempt->choices = {
        {"Climb out the window", ending_desperate_escape},
        {"Call for help", ending_hope_amidst_chaos}
    };
    
    auto stay_at_desk = std::make_shared<StoryNode>(
        "You decide to defy the call, remaining at your desk despite the emptiness around you.");
    
    auto paperclips_achievement = std::make_shared<FunnyAchievementNode>(
        "paperclip_hoarder", "Paperclip_Hoarder",
        "You absent-mindedly fidget with a pile of paperclips.", stay_at_desk
    );
    
    auto chair_achievement = std::make_shared<FunnyAchievementNode>(
        "chair_spinner", "Chair_Spinner",
        "You spin in your chair, laughing at your own dizziness.", stay_at_desk
    );
    
    stay_at_desk->choices = {
        {"Keep working mindlessly", ending_eternal_worker},
        {"Eventually, explore the corridors", explore_corridors},
        {"Attempt to leave the building", escape_attempt},
        {"Fidget with paperclips", paperclips_achievement},
        {"Spin in your chair", chair_achievement}
    };
    
    // Create start node
    auto start = std::make_shared<StoryNode>(
        "Stanley wakes up at his desk in an eerily empty office. A calm yet authoritative narrator echoes:\n"
        "'It is time to work... or is it?'");
    
    start->choices = {
        {"Follow the narrator's instructions", follow_narrator},
        {"Disobey and remain at your desk", stay_at_desk}
    };
    
    // Start the game
    start->play();
}

void achievements_screen() {
    clear();
    int h, w;
    getmaxyx(stdscr, h, w);
    
    // Check if window is too small
    if (h < 10 || w < 30) {
        clear();
        try {
            mvprintw(0, 0, "Window too small");
            mvprintw(1, 0, "Press any key to return");
            refresh();
        } catch (...) {
            // Ignore errors
        }
        getch();
        return;
    }
    
    // Display loading message
    try {
        mvprintw(0, 0, "Loading achievements...");
        refresh();
    } catch (...) {
        // Ignore errors
    }
    
    // Build the list of lines to display
    std::vector<std::string> lines;
    lines.push_back("");
    
    std::string footer = "Use UP/DOWN to scroll; press any other key to return to the main menu.";
    int footer_x = (w - footer.length()) / 2;
    std::string centered_footer = std::string(footer_x, ' ') + footer;
    lines.push_back(centered_footer);
    
    lines.push_back("");
    
    std::string header = "Achievements";
    int header_x = (w - header.length()) / 2;
    std::string centered_header = std::string(header_x, ' ') + header;
    lines.push_back(centered_header);
    
    lines.push_back("");
    
    std::string endings = "Endings Achieved:";
    int endings_x = (w - endings.length()) / 2;
    std::string centered_endings = std::string(endings_x, ' ') + endings;
    lines.push_back(centered_endings);
    
    lines.push_back("");
    
    for (const auto& pair : ENDING_DATA) {
        std::string line;
        if (achievements[pair.first]) {
            line = pair.second;
        } else {
            line = "??????????";
        }
        int line_x = (w - line.length()) / 2;
        lines.push_back(std::string(line_x, ' ') + line);
    }
    
    lines.push_back("");
    
    std::string other = "Other Achievements:";
    int other_x = (w - other.length()) / 2;
    std::string centered_other = std::string(other_x, ' ') + other;
    lines.push_back(centered_other);
    
    lines.push_back("");
    
    for (const auto& pair : FUNNY_ACHIEVEMENTS) {
        std::string line;
        if (achievements[pair.first]) {
            line = pair.second;
        } else {
            line = "??????????";
        }
        int line_x = (w - line.length()) / 2;
        lines.push_back(std::string(line_x, ' ') + line);
    }
    
    lines.push_back("");
    
    // Create a pad for scrolling
    int pad_height = lines.size();
    WINDOW* pad = newpad(pad_height, w);
    
    for (int idx = 0; idx < lines.size(); idx++) {
        try {
            mvwprintw(pad, idx, 0, "%s", lines[idx].c_str());
        } catch (...) {
            // Ignore errors
        }
    }
    
    // Clear the screen before showing pad
    clear();
    refresh();
    
    int offset = 0;
    
    // Function to refresh the pad safely
    auto refresh_pad = [&]() {
        try {
            int visible_height = std::min(h-1, pad_height-offset);
            if (visible_height > 0) {
                prefresh(pad, offset, 0, 0, 0, visible_height-1, w-1);
            }
        } catch (...) {
            try {
                prefresh(pad, offset, 0, 0, 0, h-1, w-1);
            } catch (...) {
                try {
                    prefresh(pad, offset, 0, 0, 0, 0, 0);
                } catch (...) {
                    // Ignore all errors
                }
            }
        }
    };
    
    // Initial display
    refresh_pad();
    
    // Wait for user input
    while (true) {
        int key = getch();
        if (key == KEY_UP) {
            offset = std::max(0, offset - 1);
            refresh_pad();
        } else if (key == KEY_DOWN) {
            offset = std::min(pad_height - h, offset + 1);
            offset = std::max(0, offset);
            refresh_pad();
        } else {
            break;
        }
    }
    
    // Clean up
    delwin(pad);
}

std::string title_screen_main_menu() {
    clear();
    int h, w;
    getmaxyx(stdscr, h, w);
    
    std::string title = "Text Parable";
    std::string subtitle = "A Stanley Parable-Inspired Text Adventure";
    std::string credits = "Made by: tanczacy-ziemniak";
    
    safe_addstr(h/2 - 4, (w - title.length()) / 2, title, A_BOLD);
    safe_addstr(h/2 - 3, (w - subtitle.length()) / 2, subtitle);
    safe_addstr(h/2 - 2, (w - credits.length()) / 2, credits);
    refresh();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::vector<std::string> options = {"Start Game", "Achievements", "Exit"};
    int menu_start_y = h/2;
    int choice_idx = display_menu(options, menu_start_y);
    
    return options[choice_idx];
}

int main() {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0); // Hide cursor
    
    // Load achievements
    load_achievements();
    
    // Main game loop
    while (true) {
        clear();
        std::string option = title_screen_main_menu();
        if (option == "Start Game") {
            game_narrative();
        } else if (option == "Achievements") {
            achievements_screen();
        } else if (option == "Exit") {
            break;
        }
    }
    
    // Clean up ncurses
    endwin();
    return 0;
}
