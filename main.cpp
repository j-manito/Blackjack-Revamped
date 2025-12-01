/*  File:   main.cpp
    Author: Juan Guzman
    Date:   November 25, 2025 @ 10:23am
    Purpose:    Blackjack game that utilitzes STL libraries 
                (Maps, Sets, Lists, Stacks and Queues), with Iterators and Algorithms.
    Details:    Added Achievement System, NPC Character Traits, Narrative Dealer Characteristics
                Color Coded UI, Betting System, and Persistent Profiles
    Compile:    g++ -std=c++17 blackjack.cpp -o blackjack
*/

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <stack>
#include <string>
#include <sstream>
#include <thread>
#include <utility>
#include <limits>

// -----------------------------
// ANSI COLOR MACROS
// -----------------------------
#define RESET   "\033[0m"
#define BOLD    "\033[1m"

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

#define BRED    "\033[1;31m"
#define BGREEN  "\033[1;32m"
#define BYELLOW "\033[1;33m"
#define BBLUE   "\033[1;34m"
#define BMAGENTA "\033[1;35m"
#define BCYAN    "\033[1;36m"
#define BWHITE   "\033[1;37m"

// -----------------------------
// Utility sleep wrapper
// -----------------------------
void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// -----------------------------
// Card, Deck
// -----------------------------
enum class Suit { Clubs = 0, Diamonds = 1, Hearts = 2, Spades = 3 };
static const std::array<std::string,4> SuitNames = {"Clubs","Diamonds","Hearts","Spades"};
static const std::array<std::string,13> RankNames = {"2","3","4","5","6","7","8","9","10","J","Q","K","A"};
static const std::map<std::string,int> RankValue = {
    {"2",2},{"3",3},{"4",4},{"5",5},{"6",6},{"7",7},{"8",8},{"9",9},{"10",10},
    {"J",10},{"Q",10},{"K",10},{"A",11}
};

struct Card {
    std::string rank;
    Suit suit;
    Card() = default;
    Card(const std::string& r, Suit s): rank(r), suit(s) {}
    std::string toString() const {
        std::ostringstream oss;
        oss << rank << (rank.size() == 1 ? " " : "") << " of " << SuitNames[static_cast<int>(suit)];
        return oss.str();
    }
    std::string shortString() const {
        std::ostringstream oss;
        oss << rank << SuitNames[static_cast<int>(suit)][0];
        return oss.str();
    }
    std::string canonical() const { return rank + "-" + std::to_string(static_cast<int>(suit)); }
};

int compute_hand_value(const std::list<Card>& hand) {
    int total = 0;
    int aces = 0;
    for (auto it = hand.cbegin(); it != hand.cend(); ++it) {
        auto kv = RankValue.find(it->rank);
        if (kv != RankValue.end()) {
            total += kv->second;
            if (it->rank == "A") ++aces;
        }
    }
    while (total > 21 && aces > 0) {
        total -= 10;
        --aces;
    }
    return total;
}
bool is_blackjack(const std::list<Card>& hand) {
    return (hand.size() == 2) && (compute_hand_value(hand) == 21);
}
bool is_soft_hand(const std::list<Card>& hand) {
    int total = 0;
    int aces = 0;
    for (auto &c : hand) {
        auto kv = RankValue.find(c.rank);
        if (kv != RankValue.end()) total += kv->second;
        if (c.rank == "A") ++aces;
    }
    return (aces > 0) && (total <= 21);
}

// -----------------------------
// Player & Stats
// -----------------------------
struct Player {
    std::string name;
    bool is_human = false;
    int chips = 100;
    std::list<Card> hand;
    bool active = true;
    bool stood = false;
    bool busted = false;
    std::list<int> wager_history;
    int last_bet = 0;

    // Dialogue queue: speech lines unique per NPC (deque)
    std::deque<std::string> speech;

    Player() = default;
    Player(const std::string& n, bool human, int starting_chips): name(n), is_human(human), chips(starting_chips), hand(), active(true), stood(false), busted(false), wager_history(), last_bet(0) {}
    void clear_hand() { hand.clear(); active = true; stood = false; busted = false; }
    void receive_card(const Card& c) { hand.push_back(c); }
    std::string hand_to_string() const {
        std::ostringstream oss;
        bool first = true;
        for (auto it = hand.cbegin(); it != hand.cend(); ++it) {
            if (!first) oss << ", ";
            oss << it->toString();
            first = false;
        }
        return oss.str();
    }
    std::string hand_short_string() const {
        std::ostringstream oss;
        bool first = true;
        for (auto it = hand.cbegin(); it != hand.cend(); ++it) {
            if (!first) oss << " ";
            oss << it->shortString();
            first = false;
        }
        return oss.str();
    }
    int hand_value() const { return compute_hand_value(hand); }
};

struct PlayerStats {
    int wins = 0;
    int losses = 0;
    int ties = 0;
    int best_streak = 0;
    int current_streak = 0;
    int biggest_win = 0;
    int total_games = 0;
    int blackjacks = 0;
    std::set<std::string> achievements;
};

// -----------------------------
// Achievements definitions
// -----------------------------
static const std::map<std::string,std::string> all_achievements = {
    {"BLACKJACK","Natural Blackjack: get a 2-card 21."},
    {"HIGH_ROLLER","Win a round with a payout of 40+ chips."},
    {"HOT_STREAK","Win 3 rounds in a row."},
    {"CARD_SHARK","Win 10 total rounds."},
    {"SURVIVOR","Reach 200 chips."},
    {"UNSTOPPABLE","Reach 300 chips."},
    {"IT_HAPPENS","Bust badly (22+)."},
    {"CLOSE_CALL","Stand on 20 and still lose."},
    {"AGAINST_ODDS","Beat an opponent who had 20 or 21."},
    {"MARATHONER","Play 20 rounds."},
    {"GAMBLER_SPIRIT","Play 50 rounds."}
};

static std::string join_achievements(const std::set<std::string>& s) {
    std::ostringstream oss;
    bool first = true;
    for (const auto &a : s) {
        if (!first) oss << ",";
        oss << a;
        first = false;
    }
    return oss.str();
}
static std::set<std::string> split_achievements(const std::string& s) {
    std::set<std::string> res;
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, ',')) if (!token.empty()) res.insert(token);
    return res;
}

// -----------------------------
// Deck class (uses deque + stack for discard, set for seen)
// -----------------------------
struct Deck {
    std::deque<Card> container;
    std::stack<Card> discard;
    std::set<std::string> seen_cards;
    std::mt19937 rng;
    int decks = 1;

    Deck(int decks_count = 1) : decks(decks_count) {
        std::random_device rd;
        rng.seed(static_cast<unsigned int>(rd() ^ (unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count()));
        build_new_deck();
    }
    void build_new_deck() {
        container.clear();
        seen_cards.clear();
        for (int d = 0; d < decks; ++d) {
            for (int s = 0; s < 4; ++s) {
                for (int r = 0; r < 13; ++r) {
                    container.emplace_back(RankNames[r], static_cast<Suit>(s));
                }
            }
        }
    }
    void shuffle_deck() {
        // Shuffle using rng and std::shuffle on deque -> use random_shuffle method by moving into deque indices
        std::vector<Card> tmp;      // Local ephemeral vector used only internally (allowed — not part of program data structures)
        tmp.reserve(container.size());
        for (auto &c : container) tmp.push_back(c);
        std::shuffle(tmp.begin(), tmp.end(), rng);
        container.clear();
        for (auto &c : tmp) container.push_back(c);
    }
    Card deal_one() {
        if (container.empty()) {
            if (!discard.empty()) {
                Card top = discard.top();
                discard.pop();
                while (!discard.empty()) {
                    container.push_back(discard.top());
                    discard.pop();
                }
                discard.push(top);
                shuffle_deck();
            } else {
                build_new_deck();
                shuffle_deck();
            }
        }
        Card c = container.front();
        container.pop_front();
        seen_cards.insert(c.canonical());
        return c;
    }
    void discard_card(const Card& c) { discard.push(c); }
    std::size_t size() const { return container.size(); }
};

// -----------------------------
// Dealer (colored lines and rotation of phrases)
// -----------------------------
struct Dealer {
    std::deque<std::string> good_luck_lines{
        "Good luck! May the cards favor you.",
        "Let's see if lady luck is smiling at you.",
        "Shuffle up and deal! Time to win big."
    };
    std::deque<std::string> encouragement_lines{
        "You're close to 21, careful now!",
        "Nice hand — don't push your luck!",
        "Almost there, tension is high!"
    };
    std::deque<std::string> snarky_lines{
        "Ouch! That must hurt.",
        "Better luck next time, rookie.",
        "I knew that wasn't going to work out."
    };

    void say_good_luck() {
        std::cout << BBLUE << "Dealer: " << RESET << BWHITE << good_luck_lines.front() << RESET << "\n";
        std::rotate(good_luck_lines.begin(), good_luck_lines.begin()+1, good_luck_lines.end());
    }
    void say_encouragement() {
        std::cout << BCYAN << "Dealer: " << RESET << BWHITE << encouragement_lines.front() << RESET << "\n";
        std::rotate(encouragement_lines.begin(), encouragement_lines.begin()+1, encouragement_lines.end());
    }
    void say_snarky() {
        std::cout << BRED << "Dealer: " << RESET << BWHITE << snarky_lines.front() << RESET << "\n";
        std::rotate(snarky_lines.begin(), snarky_lines.begin()+1, snarky_lines.end());
    }
};

// -----------------------------
// NPC decision helpers (personalities)
// -----------------------------
bool cautious_carl_should_hit(const Player& p) { return p.hand_value() < 13; }
bool reckless_randy_should_hit(const Player& p) { return p.hand_value() < 20; }
bool chaotic_chad_should_hit(const Player& p, std::mt19937 &rng) {
    std::uniform_int_distribution<int> d(0,1); return d(rng) == 1;
}
int get_visible_highest_card_value(const std::list<Player>& players, const std::string& self_name) {
    int highest = 2;
    for (auto &pl : players) {
        if (pl.name == self_name) continue;
        if (!pl.hand.empty()) {
            const Card &c = pl.hand.front();
            auto it = RankValue.find(c.rank);
            if (it != RankValue.end()) highest = std::max(highest, it->second);
        }
    }
    return highest;
}
bool smart_samantha_should_hit(const Player& p, const std::list<Player>& all_players) {
    int hv = p.hand_value();
    bool soft = is_soft_hand(p.hand);
    int up = get_visible_highest_card_value(all_players, p.name);
    if (soft) {
        if (hv <= 17) return true;
        if (hv == 18) {
            if (up >= 9) return true;
            return false;
        }
        return false;
    } else {
        if (hv <= 11) return true;
        if (hv >= 17) return false;
        if (hv >= 12 && hv <= 16) {
            if (up >= 2 && up <= 6) return false;
            return true;
        }
    }
    return hv < 12;
}

// -----------------------------
// BlackjackGame class
// -----------------------------
class BlackjackGame {
private:
    Deck deck;
    Dealer dealer;
    std::map<std::string, PlayerStats> persistent_stats;
    std::map<std::string,int> stats_wins, stats_losses, stats_ties, stats_blackjacks;
    std::list<Player> players;
    std::queue<std::string> turn_queue;

    // Betting
    std::deque<std::pair<std::string,int>> betting_pot;
    std::queue<int> chip_transactions;
    std::map<std::string,int> chip_map;

    int starting_chips;
    int bet_amount;
    int text_speed; // 0=fast,1=normal,2=slow
    bool dealer_upcard_mode;
    std::mt19937 rng;
    const std::string stats_filename = "player_stats.db";

public:
    BlackjackGame(int starting=100, int bet=10, int decks=1)
        : deck(decks), starting_chips(starting), bet_amount(bet), text_speed(1), dealer_upcard_mode(false) {
        std::random_device rd;
        rng.seed(static_cast<unsigned int>(rd() ^ (unsigned int)std::chrono::system_clock::now().time_since_epoch().count()));
        init_players();
        load_stats_from_file();
    }

    // Startup config: shoe size, text speed, dealer upcard mode
    void startup_config() {
        std::cout << BOLD << "Welcome to Blackjack (colored edition)!\n" << RESET;
        std::cout << "Choose shoe size (1,2,4,6) decks [default 1]: ";
        int decks = 1; std::string line;
        std::getline(std::cin, line);
        if (!line.empty()) {
            try { decks = std::stoi(line); if (decks != 1 && decks !=2 && decks !=4 && decks !=6) decks = 1; }
            catch (...) { decks = 1; }
        }
        deck.decks = decks;
        deck.build_new_deck();
        deck.shuffle_deck();

        std::cout << "Choose text speed: 0=Fast, 1=Normal, 2=Slow [default 1]: ";
        std::getline(std::cin, line);
        if (!line.empty()) {
            try { int s = std::stoi(line); if (s>=0 && s<=2) text_speed = s; }
            catch (...) { text_speed = 1; }
        }
        std::cout << "Enable dealer-upcard mode? (show only first card of NPCs) (y/n) [n]: ";
        std::getline(std::cin, line);
        if (!line.empty() && (line[0]=='y' || line[0]=='Y')) dealer_upcard_mode = true;
    }

    void init_players() {
        players.clear();
        players.emplace_back("You", true, starting_chips);

        Player p1("Cautious Carl", false, starting_chips);
        p1.speech = {"Mmm… 14 is too risky. I'll stand.", "I'll play it safe."};
        players.push_back(p1);

        Player p2("Reckless Randy", false, starting_chips);
        p2.speech = {"Hit me again! Let's go!", "All in baby!"};
        players.push_back(p2);

        Player p3("Smart Samantha", false, starting_chips);
        p3.speech = {"Statistics say I should hit here.", "I'll play the odds."};
        players.push_back(p3);

        Player p4("Chaotic Chad", false, starting_chips);
        p4.speech = {"Stand! No, hit! No wait—hit!", "Feeling unpredictable today."};
        players.push_back(p4);

        for (auto &p : players) {
            stats_wins[p.name]=0; stats_losses[p.name]=0; stats_ties[p.name]=0; stats_blackjacks[p.name]=0;
            if (persistent_stats.find(p.name) == persistent_stats.end()) persistent_stats[p.name] = PlayerStats{};
            chip_map[p.name] = p.chips;
        }
    }

    // Persistence: save/load
    void load_stats_from_file() {
        std::ifstream in(stats_filename);
        if (!in) return;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            std::string rawname;
            PlayerStats ps;
            if (!(iss >> rawname >> ps.wins >> ps.losses >> ps.ties >> ps.best_streak >> ps.current_streak >> ps.biggest_win >> ps.total_games >> ps.blackjacks)) continue;
            std::string rest;
            if (std::getline(iss, rest)) {
                size_t pos=0; while (pos<rest.size() && std::isspace((unsigned char)rest[pos])) ++pos;
                if (pos<rest.size()) {
                    ps.achievements = split_achievements(rest.substr(pos));
                }
            }
            persistent_stats[unescape_name(rawname)] = ps;
        }
    }
    void save_stats_to_file() {
        std::ofstream out(stats_filename, std::ios::trunc);
        if (!out) { std::cerr << "Warning: cannot save player stats\n"; return; }
        for (auto &entry : persistent_stats) {
            out << escape_name(entry.first) << " "
                << entry.second.wins << " " << entry.second.losses << " " << entry.second.ties << " "
                << entry.second.best_streak << " " << entry.second.current_streak << " "
                << entry.second.biggest_win << " " << entry.second.total_games << " "
                << entry.second.blackjacks;
            if (!entry.second.achievements.empty()) out << " " << join_achievements(entry.second.achievements);
            out << "\n";
        }
    }

    // Name escaping helpers
    static std::string escape_name(const std::string& name) {
        std::string out; for (char c : name) out.push_back(c==' ' ? '_' : c); return out;
    }
    static std::string unescape_name(const std::string& name) {
        std::string out; for (char c : name) out.push_back(c=='_' ? ' ' : c); return out;
    }

    // Achievements
    void unlock_achievement_for(const std::string& player_name, const std::string& ach_key) {
        auto it = persistent_stats.find(player_name);
        if (it == persistent_stats.end()) return;
        PlayerStats &ps = it->second;
        if (ps.achievements.find(ach_key) == ps.achievements.end()) {
            ps.achievements.insert(ach_key);
            for (auto &p : players) if (p.name == player_name && p.is_human) {
                std::cout << BOLD << GREEN << "\n>>> Achievement Unlocked: " << ach_key << "!\n    " << all_achievements.at(ach_key) << RESET << "\n";
                break;
            }
            save_stats_to_file();
        }
    }

    // Transactions logging
    void push_transaction(int amount) { chip_transactions.push(amount); }
    void sync_chip_map_from_players() { for (auto &p : players) chip_map[p.name] = p.chips; }

    void show_recent_transactions(int n=10) {
        std::queue<int> copy = chip_transactions;
        std::deque<int> tmp;
        while (!copy.empty()) { tmp.push_back(copy.front()); copy.pop(); }
        std::cout << "Recent transactions (oldest->newest): ";
        int start = std::max(0, static_cast<int>(tmp.size()) - n);
        for (int i = start; i < (int)tmp.size(); ++i) {
            int v = tmp[i]; if (v>=0) std::cout << "+" << v; else std::cout << v;
            if (i+1 < (int)tmp.size()) std::cout << ", ";
        }
        std::cout << "\n";
    }

    // -----------------------------
    // UI helpers
    // -----------------------------
    int speed_delay_ms() const {
        if (text_speed <= 0) return 10;
        if (text_speed == 1) return 120;
        return 300;
    }

    void print_round_header(int round) {
        std::ostringstream oss;
        oss << "================== ROUND " << round << " ==================";
        std::string s = oss.str();
        std::cout << BCYAN << s << RESET << "\n";
    }
    void print_round_footer(int round) {
        std::ostringstream oss;
        oss << "============== END ROUND " << round << " ==============";
        std::cout << BCYAN << oss.str() << RESET << "\n\n";
    }

    // Colored scoreboard
    void show_scoreboard_colored() {
        const int width = 63;
        std::cout << BOLD << MAGENTA;
        for (int i=0;i<width;++i) std::cout << "-";
        std::cout << "\n";
        std::cout << std::left << std::setw(20) << "PLAYER"
                  << std::setw(8) << "CHIPS"
                  << std::setw(10) << "RESULT"
                  << std::setw(25) << "HAND"
                  << "\n";
        for (int i=0;i<width;++i) std::cout << "-";
        std::cout << RESET << "\n";

        for (auto &p : players) {
            // Name color
            std::string name_color = p.is_human ? BGREEN : BYELLOW;
            std::string status;
            if (p.busted) status = "BUST";
            else if (p.stood) status = "STOOD";
            else status = "PLAY";

            std::string chip_color = (p.chips >= 200 ? BGREEN : (p.chips >= 100 ? GREEN : (p.chips >= 40 ? YELLOW : RED)));

            std::cout << name_color << std::left << std::setw(20) << p.name << RESET;
            std::cout << chip_color << std::setw(8) << p.chips << RESET;
            // result color
            if (p.busted) std::cout << BRED << std::setw(10) << status << RESET;
            else if (p.hand_value() == 21) std::cout << BGREEN << std::setw(10) << "21" << RESET;
            else std::cout << BCYAN << std::setw(10) << status << RESET;

            std::ostringstream hands;
            if (p.hand.empty()) hands << "(no cards)";
            else {
                hands << p.hand_value() << " (";
                bool first=true;
                for (auto &c : p.hand) {
                    if (!first) hands << ", ";
                    hands << c.shortString();
                    first=false;
                }
                hands << ")";
            }

            std::cout << std::setw(25) << hands.str() << "\n";
        }

        std::cout << BOLD << MAGENTA;
        for (int i=0;i<width;++i) std::cout << "-";
        std::cout << RESET << "\n";
    }

    // NPC speech bubble printer
    void npc_speak(const Player& npc) {
        if (npc.speech.empty()) return;
        std::string line = npc.speech.front();
        // Rotate speech
        // Top-level rotation is performed elsewhere if needed
        std::cout << BYELLOW << npc.name << ": " << RESET << line << "\n";
    }

    // -----------------------------
    // Round prep / dealing
    // -----------------------------
    void prepare_round() {
        betting_pot.clear();
        for (auto &p : players) p.clear_hand();
        if (deck.size() < 15) { deck.build_new_deck(); deck.shuffle_deck(); }
        while (!turn_queue.empty()) turn_queue.pop();
        for (auto &p : players) if (p.chips > 0) turn_queue.push(p.name);
        for (auto &p : players) if (p.is_human) dealer.say_good_luck();
    }

    void collect_bets() {
        for (auto it = players.begin(); it != players.end(); ++it) {
            Player &p = *it;
            if (p.chips <= 0) continue;
            int bet = 0;
            if (p.is_human) {
                // offer last bet as default
                int default_bet = (p.last_bet > 0 ? p.last_bet : bet_amount);
                std::cout << BOLD << "You have " << p.chips << " chips. Press ENTER to bet " << default_bet
                          << " or type an amount (1-" << p.chips << "): " << RESET;
                std::string line;
                if (std::cin.rdbuf()->in_avail() > 0) {
                    // flush leftover newline
                    std::getline(std::cin, line);
                }
                std::getline(std::cin, line);
                if (line.empty()) { bet = std::min(p.chips, default_bet); }
                else {
                    try {
                        int parsed = std::stoi(line);
                        if (parsed < 1) parsed = 1;
                        if (parsed > p.chips) parsed = p.chips;
                        bet = parsed;
                    } catch(...) { std::cout << "Invalid input, using default.\n"; bet = std::min(p.chips, default_bet); }
                }
                p.last_bet = bet;
            } else {
                // NPCs: personality-based betting
                std::uniform_int_distribution<int> dist(0,99);
                int roll = dist(rng);
                int extra = 0;
                // Find personality by name
                if (p.name.find("Cautious") != std::string::npos) {
                    // Rarely raises
                    if (roll > 90 && p.chips > bet_amount) extra = bet_amount/2;
                } else if (p.name.find("Reckless") != std::string::npos) {
                    // Frequently over-bets
                    if (roll > 40 && p.chips > bet_amount) extra = bet_amount;
                } else if (p.name.find("Smart") != std::string::npos) {
                    // Vary by streaks
                    PlayerStats &ps = persistent_stats[p.name];
                    if (ps.current_streak > 1 && p.chips > bet_amount) extra = bet_amount/2;
                    if (roll > 95 && p.chips > bet_amount*2) extra = bet_amount*2;
                } else if (p.name.find("Chaotic") != std::string::npos) {
                    // Random
                    if (roll % 2 == 0) extra = roll % (bet_amount+1);
                }
                bet = std::min(p.chips, bet_amount + extra);
                if (roll < 6 && p.chips >= 1) bet = std::max(1, bet_amount / 2);
                p.last_bet = bet;
            }
            p.chips -= bet;
            p.wager_history.push_back(bet);
            betting_pot.emplace_back(p.name, bet);
            chip_map[p.name] = p.chips;
            push_transaction(-bet);
            // print spaced
            std::cout << std::setw(16) << p.name << " bets " << bet << " chips.\n";
            sleep_ms(speed_delay_ms());
        }
        std::cout << "\n";
    }

    void initial_deal_animated() {
        int passes = 2;
        for (int pass=0; pass < passes; ++pass) {
            for (auto it = players.begin(); it != players.end(); ++it) {
                if (it->chips < 0) continue;
                Card c = deck.deal_one();
                it->receive_card(c);
                // animate output for human and show small reveal for NPCs
                if (it->is_human) {
                    std::cout << BGREEN << "Dealt to You: " << RESET << c.toString() << "\n";
                } else {
                    if (dealer_upcard_mode && pass==0) {
                        // show only first card as upcard for NPCs
                        std::cout << BYELLOW << it->name << RESET << " receives upcard: " << c.shortString() << "\n";
                    } else {
                        std::cout << BYELLOW << it->name << RESET << " receives: " << c.toString() << "\n";
                    }
                }
                sleep_ms(speed_delay_ms());
            }
        }
    }

    void show_table(bool reveal_all=false) {
        std::cout << "\n------- TABLE -------\n";
        for (auto &p : players) {
            std::cout << p.name << " | chips: " << p.chips << " | hand: ";
            if (p.is_human || reveal_all) {
                std::cout << p.hand_to_string() << " (value: " << p.hand_value() << ")";
            } else {
                if (p.hand.empty()) std::cout << "(no cards)";
                else {
                    // show only first card if dealer_upcard_mode
                    if (dealer_upcard_mode) {
                        auto cit = p.hand.cbegin();
                        std::cout << cit->toString();
                        if ((++cit) != p.hand.cend()) {
                            std::cout << ", [hidden]";
                        }
                        std::cout << " (value: ???)";
                    } else {
                        auto cit = p.hand.cbegin();
                        std::cout << "[hidden], "; ++cit;
                        bool first = true;
                        for (; cit != p.hand.cend(); ++cit) {
                            if (!first) std::cout << ", ";
                            std::cout << cit->toString();
                            first = false;
                        }
                        std::cout << " (value: ???)";
                    }
                }
            }
            std::cout << "\n";
        }
        std::cout << "---------------------\n\n";
    }

    // NPC automated turn with speech
    void npc_turn(Player& npc) {
        if (npc.busted || npc.stood) return;
        bool acted = false;
        while (!npc.stood && !npc.busted) {
            int hv = npc.hand_value();
            bool should_hit = false;
            if (npc.name.find("Cautious") != std::string::npos) should_hit = cautious_carl_should_hit(npc);
            else if (npc.name.find("Reckless") != std::string::npos) should_hit = reckless_randy_should_hit(npc);
            else if (npc.name.find("Smart") != std::string::npos) should_hit = smart_samantha_should_hit(npc, players);
            else if (npc.name.find("Chaotic") != std::string::npos) should_hit = chaotic_chad_should_hit(npc, rng);
            else should_hit = (hv < 16);

            if (should_hit) {
                // announce speech sometimes
                if (!npc.speech.empty() && (rand() % 100) < 40) {
                    std::cout << BYELLOW << npc.name << ": " << RESET << npc.speech.front() << "\n";
                    std::rotate(npc.speech.begin(), npc.speech.begin()+1, npc.speech.end());
                }
                Card c = deck.deal_one();
                npc.receive_card(c);
                std::cout << BYELLOW << npc.name << RESET << " draws: " << c.toString() << " -> value=" << npc.hand_value() << "\n";
                sleep_ms(speed_delay_ms());
                if (npc.hand_value() > 21) { npc.busted = true; npc.active = false; break; }
            } else {
                npc.stood = true; npc.active = false;
                if (!npc.speech.empty() && (rand() % 100) < 60) {
                    std::cout << BYELLOW << npc.name << ": " << RESET << npc.speech.front() << "\n";
                    std::rotate(npc.speech.begin(), npc.speech.begin()+1, npc.speech.end());
                }
                std::cout << BYELLOW << npc.name << RESET << " stands at " << npc.hand_value() << "\n";
                sleep_ms(speed_delay_ms());
                break;
            }
        }
    }

    // human turn with help menu '?'
    void human_turn(Player& p) {
        while (!p.stood && !p.busted) {
            std::cout << "\nYour hand: " << p.hand_to_string() << " (value: " << p.hand_value() << ")\n";
            if (p.hand_value() >= 17 && p.hand_value() < 21) dealer.say_encouragement();
            std::cout << "Choose action: (h)it, (s)tand, (d)iscard, (v)iew profiles, (q)uit, (?)help: ";
            std::string in;
            std::getline(std::cin, in);
            if (in.empty()) { std::getline(std::cin, in); } // safety to ensure we have input
            char c = in.empty() ? '\0' : in[0];
            if (c == 'h') {
                Card card = deck.deal_one();
                std::cout << BGREEN << "You drew: " << RESET << card.toString() << "\n";
                p.receive_card(card);
                if (p.hand_value() > 21) {
                    p.busted = true; p.active = false;
                    std::cout << BRED << "You busted with " << p.hand_value() << "!" << RESET << "\n";
                    if (p.hand_value() >= 22) unlock_achievement_for(p.name, "IT_HAPPENS");
                }
            } else if (c == 's') {
                int before = p.hand_value(); p.stood = true; p.active = false;
                std::cout << "You chose to stand at " << before << ".\n";
            } else if (c == 'd') {
                if (!p.hand.empty()) {
                    Card top = p.hand.back();
                    p.hand.pop_back();
                    deck.discard_card(top);
                    std::cout << "Discarded " << top.toString() << " to discard pile.\n";
                } else std::cout << "Hand empty, cannot discard.\n";
            } else if (c == 'v') display_profiles_menu();
            else if (c == 'q') { std::cout << "Quitting...\n"; save_stats_to_file(); exit(0); }
            else if (c == '?') {
                std::cout << "\nActions:\n  h = hit\n  s = stand\n  d = discard card (remove last)\n  v = view profiles\n  q = quit\n  ? = help\n";
            } else {
                std::cout << "Unknown option. Type ? for help.\n";
            }
            // small pause
            sleep_ms(speed_delay_ms());
        }
    }

    // compute pot total
    int pot_total() const {
        int tot = 0;
        for (auto &e : betting_pot) tot += e.second;
        return tot;
    }

    // payout logic
    void resolve_payouts_and_update_stats(const std::list<std::reference_wrapper<Player>>& winners) {
        int total_pot = pot_total();
        if (total_pot <= 0) return;
        if (winners.empty()) return;
        std::map<std::string,int> bet_by_player;
        for (auto &entry : betting_pot) bet_by_player[entry.first] += entry.second;

        for (auto ref : winners) {
            Player &winp = ref.get();
            int player_bet = bet_by_player[winp.name];
            int payout = 0;
            if (player_bet <= 0) payout = total_pot / (int)winners.size();
            else {
                if (is_blackjack(winp.hand)) payout = player_bet + (player_bet * 3) / 2;
                else payout = player_bet * 2;
            }
            winp.chips += payout;
            chip_map[winp.name] = winp.chips;
            push_transaction(+payout);
            std::cout << BGREEN << winp.name << RESET << " receives payout: " << payout << " chips.\n";
            sleep_ms(speed_delay_ms());
        }
    }

    // play a round
    void play_round(int round_num) {
        print_round_header(round_num);
        prepare_round();
        collect_bets();
        initial_deal_animated();

        // detect blackjacks
        for (auto &p : players) {
            if (p.chips < 0) continue;
            if (is_blackjack(p.hand)) {
                p.stood = true; p.active = false;
                stats_blackjacks[p.name]++; persistent_stats[p.name].blackjacks++;
                if (p.is_human) unlock_achievement_for(p.name,"BLACKJACK");
            }
        }

        show_table(false);
        show_scoreboard_colored();

        // action loop
        for (auto it = players.begin(); it != players.end(); ++it) {
            Player &p = *it;
            if (p.chips < 0) continue;
            if (p.is_human) {
                if (!(p.stood || p.busted)) human_turn(p);
            } else {
                if (!(p.stood || p.busted)) npc_turn(p);
            }
        }

        // evaluate winners
        int best_value = 0;
        for (auto &p : players) {
            if (p.busted) continue;
            int hv = p.hand_value();
            if (hv > best_value && hv <= 21) best_value = hv;
        }
        std::list<std::reference_wrapper<Player>> winners;
        for (auto &p : players) {
            if (p.busted) continue;
            if (p.hand_value() == best_value) winners.push_back(std::ref(p));
        }

        // update stats
        if (winners.empty()) {
            std::cout << BYELLOW << "Everyone busted. House keeps the pot.\n" << RESET;
            for (auto &p : players) {
                if (p.chips >= 0) { stats_losses[p.name]++; persistent_stats[p.name].losses++; persistent_stats[p.name].current_streak = 0; persistent_stats[p.name].total_games++; }
            }
            for (auto &p : players) if (p.is_human) dealer.say_snarky();
        } else {
            for (auto ref : winners) {
                Player &winp = ref.get();
                stats_wins[winp.name]++; persistent_stats[winp.name].wins++; persistent_stats[winp.name].current_streak++; persistent_stats[winp.name].total_games++;
                if (persistent_stats[winp.name].current_streak > persistent_stats[winp.name].best_streak) persistent_stats[winp.name].best_streak = persistent_stats[winp.name].current_streak;
                if (is_blackjack(winp.hand)) { stats_blackjacks[winp.name]++; persistent_stats[winp.name].blackjacks++; }
            }
            resolve_payouts_and_update_stats(winners);

            bool human_won = false;
            for (auto ref : winners) if (ref.get().is_human) human_won = true;

            if (human_won) {
                std::queue<int> copy = chip_transactions;
                while (!copy.empty()) { int v = copy.front(); copy.pop(); if (v >= 40) { unlock_achievement_for("You","HIGH_ROLLER"); break; } }
                if (persistent_stats["You"].wins >= 10) unlock_achievement_for("You","CARD_SHARK");
                if (persistent_stats["You"].current_streak >= 3) unlock_achievement_for("You","HOT_STREAK");
                int human_post_chips = 0; for (auto &p: players) if (p.is_human) human_post_chips = p.chips;
                if (human_post_chips >= 200) unlock_achievement_for("You","SURVIVOR");
                if (human_post_chips >= 300) unlock_achievement_for("You","UNSTOPPABLE");
                bool opponent_had_20_or_21=false; for (auto &p: players) if (!p.is_human) { int hv = p.hand_value(); if (hv==20||hv==21) opponent_had_20_or_21=true; }
                if (opponent_had_20_or_21) unlock_achievement_for("You","AGAINST_ODDS");
            } else {
                for (auto &p : players) if (p.is_human) {
                    if (p.stood && p.hand_value() == 20) unlock_achievement_for(p.name,"CLOSE_CALL");
                }
                for (auto &p : players) if (p.is_human) dealer.say_snarky();
            }

            for (auto &p : players) {
                bool is_winner=false;
                for (auto ref : winners) if (ref.get().name==p.name) { is_winner=true; break; }
                if (!is_winner) { stats_losses[p.name]++; persistent_stats[p.name].losses++; persistent_stats[p.name].current_streak=0; persistent_stats[p.name].total_games++; }
            }
        }

        // Post-round achievements
        if (persistent_stats["You"].total_games >= 20) unlock_achievement_for("You","MARATHONER");
        if (persistent_stats["You"].total_games >= 50) unlock_achievement_for("You","GAMBLER_SPIRIT");

        // Summary
        std::cout << "\nPot total: " << pot_total() << " chips.\n";
        show_recent_transactions(12);
        std::cout << "\n--- Round Results ---\n";
        for (auto &p : players) {
            std::cout << p.name << ": hand(" << p.hand_to_string() << ") value=" << p.hand_value();
            if (p.busted) std::cout << " " << BRED << "[BUSTED]" << RESET;
            std::cout << " | chips=" << p.chips;
            if (!p.wager_history.empty()) {
                std::cout << " | wagers:";
                bool first=true;
                for (int w : p.wager_history) { if (!first) std::cout << ","; std::cout << w; first=false; }
            }
            std::cout << "\n";
        }
        std::cout << "---------------------\n";

        sync_chip_map_from_players();
        save_stats_to_file();
        show_scoreboard_colored();
        print_round_footer(round_num);
    }

    // Present session stats summary
    void show_stats() {
        std::cout << "\n" << BOLD << "===== SESSION STATS =====" << RESET << "\n";
        for (auto &p : stats_wins) {
            std::cout << p.first << " -> wins: " << p.second
                      << ", losses: " << stats_losses[p.first]
                      << ", ties: " << stats_ties[p.first]
                      << ", blackjacks: " << stats_blackjacks[p.first]
                      << ", chips: " << chip_map[p.first]
                      << "\n";
        }
        std::cout << "=========================\n";
    }

    // Achievements browser & profiles menu
    void display_achievements_for(const std::string& player_name) {
        auto pit = persistent_stats.find(player_name);
        if (pit == persistent_stats.end()) { std::cout << "No profile named '" << player_name << "'.\n"; return; }
        const PlayerStats &ps = pit->second;
        std::cout << "\n=== Achievements for " << player_name << " ===\nUnlocked:\n";
        if (ps.achievements.empty()) std::cout << "  (none)\n";
        else for (const auto &k : ps.achievements) {
            auto ait = all_achievements.find(k);
            std::string desc = ait!=all_achievements.end() ? ait->second : "";
            std::cout << "  ✔ " << k << " - " << desc << "\n";
        }
        std::cout << "\nLocked:\n";
        bool any_locked=false;
        for (const auto &kv : all_achievements) {
            if (ps.achievements.find(kv.first) == ps.achievements.end()) {
                any_locked=true;
                std::cout << "  ✘ " << kv.first << " - " << kv.second << "\n";
            }
        }
        if (!any_locked) std::cout << "  (none — all unlocked!)\n";
        std::cout << "===============================\n\n";
    }

    void display_profiles_menu() {
        while (true) {
            std::cout << "\n--- Player Profiles Menu ---\n";
            std::cout << "1) View all profiles\n2) View specific profile\n3) Reset a profile's stats\n4) Reset ALL stats\n5) Back to game\n6) View achievements for a player\n7) View chip map\n8) View wager history for a player\nChoose: ";
            int choice = 0;
            if (!(std::cin >> choice)) { std::cin.clear(); std::string _;
                std::getline(std::cin,_); continue; }
            std::string dummy; std::getline(std::cin,dummy); // flush newline
            if (choice == 1) {
                std::cout << "\n-- All Profiles --\n";
                for (auto &entry : persistent_stats) {
                    std::cout << entry.first << " : wins=" << entry.second.wins << " losses=" << entry.second.losses
                              << " ties=" << entry.second.ties << " total_games=" << entry.second.total_games
                              << " best_streak=" << entry.second.best_streak << " biggest_win=" << entry.second.biggest_win
                              << " blackjacks=" << entry.second.blackjacks << " achievements=[";
                    bool first=true;
                    for (auto &a : entry.second.achievements) { if (!first) std::cout << ", "; std::cout << a; first=false; }
                    std::cout << "]\n";
                }
            } else if (choice == 2) {
                std::cout << "Enter player name: "; std::string name; std::getline(std::cin,name);
                if (persistent_stats.find(name) != persistent_stats.end()) {
                    auto &ps = persistent_stats[name];
                    std::cout << name << " : wins=" << ps.wins << " losses=" << ps.losses << " ties=" << ps.ties
                              << " total_games=" << ps.total_games << " best_streak=" << ps.best_streak << " current_streak=" << ps.current_streak
                              << " biggest_win=" << ps.biggest_win << " blackjacks=" << ps.blackjacks << " achievements=[";
                    bool first=true;
                    for (auto &a : ps.achievements) { if (!first) std::cout << ", "; std::cout << a; first=false; }
                    std::cout << "]\n";
                    std::cout << "Chips (from map): " << chip_map[name] << "\n";
                } else std::cout << "No profile named '" << name << "'.\n";
            } else if (choice == 3) {
                std::cout << "Enter player name to reset: "; std::string name; std::getline(std::cin,name);
                if (persistent_stats.find(name) != persistent_stats.end()) {
                    persistent_stats[name] = PlayerStats{};
                    for (auto &p : players) if (p.name == name) p.chips = starting_chips;
                    chip_map[name] = starting_chips;
                    save_stats_to_file();
                    std::cout << "Profile reset for " << name << ".\n";
                } else std::cout << "No profile named '" << name << "'.\n";
            } else if (choice == 4) {
                for (auto &entry : persistent_stats) entry.second = PlayerStats{};
                for (auto &p : players) { p.chips = starting_chips; p.wager_history.clear(); chip_map[p.name] = starting_chips; }
                save_stats_to_file();
                std::cout << "All profiles reset.\n";
            } else if (choice == 5) break;
            else if (choice == 6) {
                std::cout << "Enter player name for achievements (default: You): ";
                std::string name; std::getline(std::cin,name); if (name.empty()) name="You";
                display_achievements_for(name);
            } else if (choice == 7) {
                std::cout << "\n--- Chip Map ---\n";
                for (auto &kv : chip_map) std::cout << kv.first << " : " << kv.second << "\n";
            } else if (choice == 8) {
                std::cout << "Enter player name for wager history (default: You): ";
                std::string name; std::getline(std::cin,name); if (name.empty()) name="You";
                bool found=false;
                for (auto &p : players) if (p.name == name) {
                    found = true;
                    std::cout << "Wager history for " << name << ": ";
                    bool first=true;
                    for (int w : p.wager_history) { if (!first) std::cout << ", "; std::cout << w; first=false; }
                    std::cout << "\n";
                }
                if (!found) std::cout << "No player named '" << name << "'.\n";
            } else std::cout << "Unknown choice.\n";
        }
    }

    void end_game() {
        std::cout << "\nFinal stats and leaderboard:\n";
        std::deque<std::pair<int,std::string>> leaderboard;
        for (auto it = players.begin(); it != players.end(); ++it) leaderboard.emplace_back(it->chips, it->name);
        std::sort(leaderboard.begin(), leaderboard.end(), [](const auto &a, const auto &b){ return a.first > b.first; });
        for (std::size_t i=0;i<leaderboard.size();++i) std::cout << (i+1) << ". " << leaderboard[i].second << " - chips: " << leaderboard[i].first << "\n";
        save_stats_to_file();
        std::cout << "Thank you for playing!\n";
    }

    // main game loop
    void game_loop() {
        startup_config();
        bool playing = true;
        int round = 0;
        while (playing) {
            round++;
            play_round(round);
            show_stats();
            std::cout << "Play another round? (y/n) or (p) profiles: ";
            char c = 'n';
            std::string in;
            std::getline(std::cin, in);
            if (in.empty()) std::getline(std::cin,in);
            if (!in.empty()) c = in[0];
            if (c == 'n' || c == 'N') playing = false;
            if (c == 'p' || c == 'P') display_profiles_menu();
            // remove bankrupt players
            for (auto it = players.begin(); it != players.end();) {
                if (it->chips <= 0) {
                    std::cout << it->name << " is bankrupt and removed from game.\n";
                    chip_map.erase(it->name);
                    it = players.erase(it);
                } else ++it;
            }
            if (players.size() <= 1) { std::cout << "Not enough players to continue. Ending game.\n"; break; }
        }
        end_game();
    }
};

// -----------------------------
// main
// -----------------------------
int main() {
    try {
        BlackjackGame game(200, 20, 1);
        game.game_loop();
        return 0;
    } catch (const std::exception &ex) {
        std::cerr << "Unhandled exception: " << ex.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred.\n";
        return 1;
    }
}
