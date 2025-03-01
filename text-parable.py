#!/usr/bin/env python3
import curses
import time
import json
import os

# Ensure the achievements file is stored in the same directory as this script.
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ACHIEVEMENTS_FILE = os.path.join(SCRIPT_DIR, "achievements.json")

# List of ending achievements (ID, Display Name).
ENDING_DATA = [
    ("silent_worker", "Silent Worker"),
    ("curiosity_cost", "Curiosity's Cost"),
    ("conformity_comfort", "Conformity's Comfort"),
    ("corporate_conspiracy", "Corporate Conspiracy"),
    ("awakened", "Awakened"),
    ("ignorance_bliss", "Ignorance is Bliss"),
    ("rebellion_unleashed", "Rebellion Unleashed"),
    ("silent_bystander", "Silent Bystander"),
    ("eternal_worker", "Eternal Worker"),
    ("secret_society", "Secret Society"),
    ("lost_labyrinth", "Lost in the Labyrinth"),
    ("desperate_escape", "Desperate Escape"),
    ("hope_amidst_chaos", "Hope Amidst Chaos")
]

# Funny achievements (ID, Display Name).
FUNNY_ACHIEVEMENTS = [
    ("persistent_knocker", "Persistent Knocker"),
    ("paperclip_hoarder", "Paperclip Hoarder"),
    ("chair_spinner", "Chair Spinner"),
    ("water_cooler_chat", "Water Cooler Conversationalist"),
    ("over_caffeinated", "Over Caffeinated")
]

# Global achievements dictionary (achievement ID -> bool).
achievements = {}

# Global counter for door knocks.
door_knock_count = 0

def load_achievements():
    global achievements
    if os.path.exists(ACHIEVEMENTS_FILE):
        try:
            with open(ACHIEVEMENTS_FILE, "r") as f:
                achievements = json.load(f)
        except Exception:
            achievements = {ending_id: False for ending_id, _ in ENDING_DATA}
            for ach_id, _ in FUNNY_ACHIEVEMENTS:
                achievements[ach_id] = False
    else:
        achievements = {ending_id: False for ending_id, _ in ENDING_DATA}
        for ach_id, _ in FUNNY_ACHIEVEMENTS:
            achievements[ach_id] = False

def save_achievements():
    global achievements
    with open(ACHIEVEMENTS_FILE, "w") as f:
        json.dump(achievements, f)

def stream_text(stdscr, text, delay=0.02, start_y=0, start_x=0):
    """
    Streams text with a typewriter effect.
    Returns the final y coordinate after printing.
    """
    y, x = start_y, start_x
    for char in text:
        if char == "\n":
            y += 1
            x = start_x
        else:
            stdscr.addstr(y, x, char)
            x += 1
        stdscr.refresh()
        time.sleep(delay)
    return y

def display_menu(stdscr, choices, start_y):
    """
    Displays a menu of choices centered on the screen.
    Navigation is done with the arrow keys and selection with Enter.
    Returns the index of the chosen option.
    """
    current_idx = 0
    n_choices = len(choices)
    header_text = "Use the UP and DOWN arrow keys to navigate and ENTER to select:"
    h, w = stdscr.getmaxyx()
    menu_lines = 1 + n_choices  # header plus one line per choice

    while True:
        # Clear only the menu area.
        for i in range(menu_lines):
            stdscr.move(start_y + i, 0)
            stdscr.clrtoeol()
        header_x = (w - len(header_text)) // 2
        stdscr.addstr(start_y, header_x, header_text)
        for i, choice in enumerate(choices):
            choice_x = (w - len(choice)) // 2
            if i == current_idx:
                stdscr.addstr(start_y + 1 + i, choice_x, choice, curses.A_REVERSE)
            else:
                stdscr.addstr(start_y + 1 + i, choice_x, choice)
        stdscr.refresh()
        key = stdscr.getch()
        if key == curses.KEY_UP:
            current_idx = (current_idx - 1) % n_choices
        elif key == curses.KEY_DOWN:
            current_idx = (current_idx + 1) % n_choices
        elif key in [10, 13]:  # Enter key
            return current_idx

def title_screen_main_menu(stdscr):
    """
    Displays the title screen with game title, subtitle, and credits.
    Then shows the main menu options centered on the screen and returns the selected option.
    """
    stdscr.clear()
    h, w = stdscr.getmaxyx()
    title = "Text Parable"
    subtitle = "A Stanley Parable-Inspired Text Adventure"
    credits = "Made by: tanczacy-ziemniak"
    stdscr.addstr(h//2 - 4, (w - len(title)) // 2, title, curses.A_BOLD)
    stdscr.addstr(h//2 - 3, (w - len(subtitle)) // 2, subtitle)
    stdscr.addstr(h//2 - 2, (w - len(credits)) // 2, credits)
    stdscr.refresh()
    time.sleep(1)  # Brief pause for effect
    
    # Main menu options.
    options = ["Start Game", "Achievements", "Exit"]
    menu_start_y = h//2
    choice_idx = display_menu(stdscr, options, menu_start_y)
    return options[choice_idx]

def achievements_screen(stdscr):
    """
    Displays the achievements screen. Endings and funny achievements are shown in two sections.
    For each achievement, if it is unlocked, display:
      Achievement_ID (in Title Case), Achieved
    Otherwise, display:
      Achievement_ID (in Title Case), ??????
    """
    stdscr.clear()
    h, w = stdscr.getmaxyx()
    title = "Achievements"
    stdscr.addstr(1, (w - len(title)) // 2, title, curses.A_BOLD | curses.A_UNDERLINE)
    
    y = 3
    header_endings = "Endings Achieved:"
    stdscr.addstr(y, (w - len(header_endings)) // 2, header_endings, curses.A_BOLD)
    y += 2
    for ach_id, _ in ENDING_DATA:
        display_status = "Achieved" if achievements.get(ach_id, False) else "??????"
        text_line = f"{ach_id.title()}, {display_status}"
        stdscr.addstr(y, (w - len(text_line)) // 2, text_line)
        y += 1
    y += 2
    header_funny = "Other Achievements:"
    stdscr.addstr(y, (w - len(header_funny)) // 2, header_funny, curses.A_BOLD)
    y += 2
    for ach_id, _ in FUNNY_ACHIEVEMENTS:
        display_status = "Achieved" if achievements.get(ach_id, False) else "??????"
        text_line = f"{ach_id.title()}, {display_status}"
        stdscr.addstr(y, (w - len(text_line)) // 2, text_line)
        y += 1

    prompt = "Press any key to return to the main menu..."
    stdscr.addstr(y+1, (w - len(prompt)) // 2, prompt)
    stdscr.refresh()
    stdscr.getch()

class StoryNode:
    def __init__(self, description, choices=None, ending=None, ending_id=None):
        """
        :param description: The narrative text for this scene.
        :param choices: A dictionary mapping choice texts to StoryNode objects.
        :param ending: The ending message (if terminal).
        :param ending_id: Unique identifier for the ending.
        """
        self.description = description
        self.choices = choices or {}
        self.ending = ending
        self.ending_id = ending_id

    def play(self, stdscr):
        stdscr.clear()
        final_y = stream_text(stdscr, self.description + "\n", delay=0.02, start_y=0, start_x=0)
        stdscr.addstr(final_y + 1, 0, "Press any key to see your choices...")
        stdscr.refresh()
        stdscr.getch()

        if self.ending:
            # Record the ending achievement.
            if self.ending_id:
                achievements[self.ending_id] = True
                save_achievements()
            stream_text(stdscr, f"\n--- {self.ending} ---\n")
            stdscr.addstr("\nPress any key to return to the main menu...")
            stdscr.refresh()
            stdscr.getch()
            return  # Return to the main menu

        menu_start_y = final_y + 3
        selected_idx = display_menu(stdscr, list(self.choices.keys()), menu_start_y)
        stdscr.clear()
        next_node = list(self.choices.values())[selected_idx]
        next_node.play(stdscr)

class KnockDoorNode(StoryNode):
    """
    A special node for knocking on the door.
    Increments a global counter and, upon reaching five knocks, unlocks the "Persistent Knocker" achievement.
    Then returns the player to the boss's office.
    """
    def __init__(self, return_node):
        self.return_node = return_node

    def play(self, stdscr):
        global door_knock_count
        door_knock_count += 1
        stdscr.clear()
        stream_text(stdscr, "You knock on the door. It doesn't open.\n", 0.02, 0, 0)
        if door_knock_count >= 5 and not achievements.get("persistent_knocker", False):
            achievements["persistent_knocker"] = True
            save_achievements()
            stream_text(stdscr, "\nAchievement Unlocked: Persistent Knocker!\n", 0.02, 0, 0)
        stdscr.addstr("\nPress any key to return to the boss's office...")
        stdscr.refresh()
        stdscr.getch()
        self.return_node.play(stdscr)

class FunnyAchievementNode(StoryNode):
    """
    A node for unlocking funny achievements.
    Displays a message, unlocks the achievement if not already unlocked, then returns to the designated node.
    """
    def __init__(self, achievement_id, achievement_name, message, return_node):
        self.achievement_id = achievement_id
        self.achievement_name = achievement_name
        self.message = message
        self.return_node = return_node

    def play(self, stdscr):
        stdscr.clear()
        stream_text(stdscr, self.message + "\n", 0.02, 0, 0)
        if not achievements.get(self.achievement_id, False):
            achievements[self.achievement_id] = True
            save_achievements()
            stream_text(stdscr, f"\nAchievement Unlocked: {self.achievement_name.title()}!\n", 0.02, 0, 0)
        stdscr.addstr("\nPress any key to return...")
        stdscr.refresh()
        stdscr.getch()
        self.return_node.play(stdscr)

def game_narrative(stdscr):
    """
    Builds and starts the story. After an ending is reached, the function returns to the main menu.
    """

    # --- Follow Narrator Branch ---

    # Meeting Room Endings
    ending_silent_worker = StoryNode(
        description="You sit down and surrender to the hypnotic drone of the presentation.",
        ending="Ending: Silent Worker\nYou spent your day in quiet compliance.",
        ending_id="silent_worker"
    )
    ending_curiosity_cost = StoryNode(
        description="Your curiosity leads you to decode hidden symbols in the projection.",
        ending="Ending: Curiosity's Cost\nSome truths are best left undiscovered.",
        ending_id="curiosity_cost"
    )
    ending_conformity_comfort = StoryNode(
        description="You dismiss the oddities and blend into the mundane routine.",
        ending="Ending: Conformity's Comfort\nRoutine soothes the mind, even if questions remain.",
        ending_id="conformity_comfort"
    )
    ending_corporate_conspiracy = StoryNode(
        description="You pore over a dusty file in a hidden drawer, uncovering blueprints of a secret corporate agenda.",
        ending="Ending: Corporate Conspiracy\nThe truth behind the facade is revealed—but at what cost?",
        ending_id="corporate_conspiracy"
    )

    # Intermediate nodes for Meeting Room Branch
    meeting_room_clues = StoryNode(
        description="In the meeting room, your eyes wander over peculiar symbols flickering behind the projector.",
        choices={
            "Investigate the symbols": ending_curiosity_cost,
            "Ignore them and take your seat": ending_conformity_comfort
        }
    )
    return_to_meeting_room = StoryNode(
        description="Deciding not to meddle with secrets you aren't ready to face, you return to the meeting room.",
        choices={"Continue": None}  # Will link later.
    )
    meeting_room_drawer = StoryNode(
        description=(
            "While seated, you notice a small desk drawer left slightly ajar. "
            "Inside, a dusty file lies hidden, filled with cryptic memos and blueprints."
        ),
        choices={
            "Read the file thoroughly": ending_corporate_conspiracy,
            "Leave it untouched": return_to_meeting_room
        }
    )
    # New funny option in the meeting room.
    # "Chat with the water cooler" unlocks the Water Cooler Conversationalist achievement.
    meeting_room = StoryNode(
        description="You enter the meeting room. The narrator instructs you to take a seat as the presentation begins.",
        choices={
            "Sit down and comply": ending_silent_worker,
            "Look around for clues": meeting_room_clues,
            "Inspect the desk drawer": meeting_room_drawer,
            "Chat with the water cooler": FunnyAchievementNode("water_cooler_chat", "water cooler chat",
                                                              "You strike up a chat with the lonely water cooler.", None)
        }
    )
    # Link the return option back to meeting_room.
    return_to_meeting_room.choices["Continue"] = meeting_room
    # For the funny node above, set its return_node to meeting_room.
    meeting_room.choices["Chat with the water cooler"].return_node = meeting_room

    # --- Boss's Office Branch ---
    ending_awakened = StoryNode(
        description="Inside, cryptic messages make your heart race as you awaken to a hidden reality.",
        ending="Ending: Awakened\nThe wall’s secrets have shattered your perception of reality.",
        ending_id="awakened"
    )
    ending_ignorance_bliss = StoryNode(
        description="You choose ignorance and sit down, letting routine lull your senses.",
        ending="Ending: Ignorance is Bliss\nSome mysteries are best left unexplored.",
        ending_id="ignorance_bliss"
    )
    # New branch via eavesdropping.
    ending_rebellion_unleashed = StoryNode(
        description="You confront shadowy figures outside, sparking a volatile rebellion.",
        ending="Ending: Rebellion Unleashed\nYou shatter the silence with your defiance.",
        ending_id="rebellion_unleashed"
    )
    ending_silent_bystander = StoryNode(
        description="You retreat silently, forever marked as an observer of hidden truths.",
        ending="Ending: Silent Bystander\nSome secrets remain unchallenged.",
        ending_id="silent_bystander"
    )
    boss_office_inside = StoryNode(
        description="Inside the boss's office, you find cryptic messages scrawled on the walls.",
        choices={
            "Read the messages": ending_awakened,
            "Ignore them and sit down": ending_ignorance_bliss
        }
    )
    eavesdrop = StoryNode(
        description="Lingering outside the boss's office, you strain to catch hushed conversations.",
        choices={
            "Confront the speakers": ending_rebellion_unleashed,
            "Retreat silently": ending_silent_bystander
        }
    )
    # Boss office now includes the knock action and a new funny option.
    boss_office = StoryNode(
        description="You approach the boss's office. The door is slightly ajar, inviting yet mysterious.",
        choices={
            "Push the door open and enter": boss_office_inside,
            "Knock on the door": None,  # Will set to a KnockDoorNode.
            "Wait outside and eavesdrop": eavesdrop,
            "Grab a cup of coffee": FunnyAchievementNode("over_caffeinated", "over caffeinated",
                                                          "You grab a cup of coffee from a nearby machine and feel a surge of energy.", None)
        }
    )
    # Set the "Knock on the door" option.
    boss_office.choices["Knock on the door"] = KnockDoorNode(boss_office)
    # Set the return for the coffee node.
    boss_office.choices["Grab a cup of coffee"].return_node = boss_office

    follow_narrator = StoryNode(
        description="Heeding the narrator's voice, you rise from your desk and step into the unknown corridors.",
        choices={
            "Enter the meeting room": meeting_room,
            "Head to the boss's office": boss_office
        }
    )

    # --- Disobey Narrator Branch ---
    ending_eternal_worker = StoryNode(
        description="You remain chained to your desk, lost in monotonous tasks.",
        ending="Ending: Eternal Worker\nThe cycle of routine engulfs you.",
        ending_id="eternal_worker"
    )
    ending_secret_society = StoryNode(
        description="Following ghostly whispers, you stumble upon a clandestine group plotting escape.",
        ending="Ending: Secret Society\nYou join the underground network of the disillusioned.",
        ending_id="secret_society"
    )
    ending_lost_labyrinth = StoryNode(
        description="Wandering endlessly, the corridors twist into a maze with no exit.",
        ending="Ending: Lost in the Labyrinth\nYou become forever lost in a maze of sterile halls.",
        ending_id="lost_labyrinth"
    )
    explore_corridors = StoryNode(
        description="Leaving your desk behind, you step into dim corridors where distant murmurs beckon.",
        choices={
            "Follow the sound of whispers": ending_secret_society,
            "Wander aimlessly": ending_lost_labyrinth
        }
    )
    ending_desperate_escape = StoryNode(
        description="In a burst of determination, you climb out the window, embracing the risk of freedom.",
        ending="Ending: Desperate Escape\nYou risk it all for a chance at escape.",
        ending_id="desperate_escape"
    )
    ending_hope_amidst_chaos = StoryNode(
        description="You call for help, and amid the chaos, a glimmer of hope emerges.",
        ending="Ending: Hope Amidst Chaos\nEven in darkness, hope may yet be found.",
        ending_id="hope_amidst_chaos"
    )
    escape_attempt = StoryNode(
        description="Refusing to be confined, you decide to leave the building altogether.",
        choices={
            "Climb out the window": ending_desperate_escape,
            "Call for help": ending_hope_amidst_chaos
        }
    )
    # New funny options in the Disobey branch.
    # "Fidget with paperclips" unlocks the Paperclip Hoarder achievement.
    # "Spin in your chair" unlocks the Chair Spinner achievement.
    stay_at_desk = StoryNode(
        description="You decide to defy the call, remaining at your desk despite the emptiness around you.",
        choices={
            "Keep working mindlessly": ending_eternal_worker,
            "Eventually, explore the corridors": explore_corridors,
            "Attempt to leave the building": escape_attempt,
            "Fidget with paperclips": FunnyAchievementNode("paperclip_hoarder", "paperclip hoarder",
                                                            "You absent-mindedly fidget with a pile of paperclips.", None),
            "Spin in your chair": FunnyAchievementNode("chair_spinner", "chair spinner",
                                                         "You spin in your chair, laughing at your own dizziness.", None)
        }
    )
    # Set the return for the funny nodes to stay_at_desk.
    stay_at_desk.choices["Fidget with paperclips"].return_node = stay_at_desk
    stay_at_desk.choices["Spin in your chair"].return_node = stay_at_desk

    # --- Starting Node ---
    start = StoryNode(
        description=(
            "Stanley wakes up at his desk in an eerily empty office. A calm yet authoritative narrator echoes:\n"
            "'It is time to work... or is it?'"
        ),
        choices={
            "Follow the narrator's instructions": follow_narrator,
            "Disobey and remain at your desk": stay_at_desk
        }
    )

    start.play(stdscr)

def main(stdscr):
    curses.curs_set(0)
    load_achievements()
    while True:
        stdscr.clear()
        option = title_screen_main_menu(stdscr)
        if option == "Start Game":
            game_narrative(stdscr)
        elif option == "Achievements":
            achievements_screen(stdscr)
        elif option == "Exit":
            break

if __name__ == "__main__":
    curses.wrapper(main)
