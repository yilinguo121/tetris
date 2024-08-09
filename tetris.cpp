#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <ncurses.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <algorithm>

using namespace std;
using namespace std::chrono;

const int width = 10;
const int height = 20;
const char EMPTY_CHAR = ' ';
const char BORDER_CHAR = '#';  // Border character
const char BLOCK_CHAR = '#';   // Block character

// Tetromino shapes (7 types)
vector<vector<int>> shapes[7] = {
    {{1, 1, 1, 1}},           // I
    {{1, 1, 0}, {0, 1, 1}},   // S
    {{0, 1, 1}, {1, 1, 0}},   // Z
    {{1, 1}, {1, 1}},         // O
    {{1, 1, 1}, {0, 1, 0}},   // T
    {{1, 1, 1}, {1, 0, 0}},   // L
    {{1, 1, 1}, {0, 0, 1}}    // J
};

// Colors for each shape
const int shapeColors[7] = {
    1,  // I - Cyan
    2,  // S - Green
    3,  // Z - Red
    4,  // O - Yellow
    5,  // T - Magenta
    6,  // L - Blue
    7   // J - White
};

struct Game {
    vector<vector<char>> board;
    int x, y, shape;
    vector<vector<int>> currentShape;
    vector<int> nextShapes;
    int score;
    static int highScore;
    milliseconds normalDropInterval;
    milliseconds acceleratedDropInterval;
    milliseconds currentDropInterval;
    steady_clock::time_point lastDropTime;
    bool isAccelerated;

    Game() : board(height, vector<char>(width, EMPTY_CHAR)), x(0), y(0), score(0),
             normalDropInterval(500ms), acceleratedDropInterval(100ms), currentDropInterval(normalDropInterval),
             lastDropTime(steady_clock::now()), isAccelerated(false) {
        srand(time(0));
        for (int i = 0; i < 3; ++i) {
            nextShapes.push_back(rand() % 7);
        }
        newShape();
    }

    // Generate a new shape
    void newShape() {
        shape = nextShapes[0];
        currentShape = shapes[shape];
        nextShapes.erase(nextShapes.begin());
        nextShapes.push_back(rand() % 7);
        x = width / 2 - currentShape[0].size() / 2;
        y = 0;
        if (checkCollision(0, 0)) {
            throw runtime_error("Game Over");
        }
    }

    // Draw the game board and shapes
    void draw() {
        static WINDOW *win = newwin(height + 2, width + 2, 0, 0);  // Game window with border
        static WINDOW *score_win = newwin(3, width + 2, height + 2, 0);  // Score window
        static WINDOW *next_win = newwin(height + 2, 10, 0, width + 2);  // Next shape window

        werase(win);
        werase(score_win);
        werase(next_win);

        // Draw border
        box(win, BORDER_CHAR, BORDER_CHAR);

        // Draw current shape
        for (int i = 0; i < currentShape.size(); ++i) {
            for (int j = 0; j < currentShape[0].size(); ++j) {
                if (currentShape[i][j]) {
                    int boardX = x + j;
                    int boardY = y + i;
                    if (boardY >= 0) {
                        mvwaddch(win, boardY + 1, boardX + 1, BLOCK_CHAR | COLOR_PAIR(shapeColors[shape]));
                    }
                }
            }
        }

        // Draw game board
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                if (board[i][j] != EMPTY_CHAR) {
                    mvwaddch(win, i + 1, j + 1, BLOCK_CHAR | COLOR_PAIR(board[i][j] - '1' + 1));
                }
            }
        }

        // Display score
        mvwprintw(score_win, 1, 1, "Score: %d", score);
        wrefresh(win);
        wrefresh(score_win);

        // Display the next three shapes
        mvwprintw(next_win, 1, 1, "Next:");
        for (int k = 0; k < 3; ++k) {
            int nextShape = nextShapes[k];
            for (int i = 0; i < shapes[nextShape].size(); ++i) {
                for (int j = 0; j < shapes[nextShape][0].size(); ++j) {
                    if (shapes[nextShape][i][j]) {
                        mvwaddch(next_win, 2 + k * 4 + i, 1 + j, BLOCK_CHAR | COLOR_PAIR(shapeColors[nextShape]));
                    }
                }
            }
        }
        wrefresh(next_win);
    }

    // Check for collision with other blocks or the borders
    bool checkCollision(int dx, int dy) {
        for (int i = 0; i < currentShape.size(); ++i) {
            for (int j = 0; j < currentShape[0].size(); ++j) {
                if (currentShape[i][j]) {
                    int nx = x + j + dx;
                    int ny = y + i + dy;
                    if (nx < 0 || nx >= width || ny >= height || (ny >= 0 && board[ny][nx] != EMPTY_CHAR)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // Place the shape on the board
    void placeShape() {
        for (int i = 0; i < currentShape.size(); ++i) {
            for (int j = 0; j < currentShape[0].size(); ++j) {
                if (currentShape[i][j] && y + i >= 0) {
                    board[y + i][x + j] = '1' + shape;
                }
            }
        }
    }

    // Flash the full line before removing it
    void flashLine(int row) {
        for (int k = 0; k < 3; ++k) {  // Flash 3 times
            for (int j = 0; j < width; ++j) {
                mvwaddch(stdscr, row + 1, j + 1, ' ' | COLOR_PAIR(8));  // Use background color
            }
            wrefresh(stdscr);
            this_thread::sleep_for(100ms);

            for (int j = 0; j < width; ++j) {
                mvwaddch(stdscr, row + 1, j + 1, BLOCK_CHAR | COLOR_PAIR(8));  // Use border color
            }
            wrefresh(stdscr);
            this_thread::sleep_for(100ms);
        }
    }

    // Remove full lines from the board
    void removeFullLines() {
        int linesCleared = 0;
        for (int i = 0; i < height; ++i) {
            bool full = true;
            for (int j = 0; j < width; ++j) {
                if (board[i][j] == EMPTY_CHAR) {
                    full = false;
                    break;
                }
            }
            if (full) {
                flashLine(i);  // Flash the line before removing
                board.erase(board.begin() + i);
                board.insert(board.begin(), vector<char>(width, EMPTY_CHAR));
                ++linesCleared;
            }
        }
        switch (linesCleared) {
            case 1: score += 10; break;
            case 2: score += 25; break;
            case 3: score += 50; break;
            case 4: score += 100; break;
            default: break;
        }
    }

    // Check if rotating the shape will cause a collision
    bool checkRotationCollision(const vector<vector<int>>& rotatedShape, int newX, int newY) {
        for (int i = 0; i < rotatedShape.size(); ++i) {
            for (int j = 0; j < rotatedShape[0].size(); ++j) {
                if (rotatedShape[i][j]) {
                    int nx = newX + j;
                    int ny = newY + i;
                    if (nx < 0 || nx >= width || ny >= height || (ny >= 0 && board[ny][nx] != EMPTY_CHAR)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // Rotate the current shape
    void rotate() {
        vector<vector<int>> rotated(currentShape[0].size(), vector<int>(currentShape.size()));
        for (int i = 0; i < currentShape.size(); ++i) {
            for (int j = 0; j < currentShape[0].size(); ++j) {
                rotated[j][currentShape.size() - 1 - i] = currentShape[i][j];
            }
        }

        // Check for collision after rotation
        if (!checkRotationCollision(rotated, x, y)) {
            currentShape = rotated;
        }
    }

    // Update the game state
    bool update() {
        steady_clock::time_point now = steady_clock::now();
        duration<float> elapsed = now - lastDropTime;

        if (elapsed > currentDropInterval) {
            lastDropTime = now;
            if (!checkCollision(0, 1)) {
                ++y;
            } else {
                placeShape();
                removeFullLines();
                newShape();
                if (checkCollision(0, 0)) {
                    return false;  // Game over
                }
            }
        }
        return true;
    }

    // Move the shape horizontally or vertically
    void move(int dx, int dy) {
        if (!checkCollision(dx, dy)) {
            x += dx;
            y += dy;
        }
    }

    // Set accelerated drop speed
    void setAccelerated(bool accelerated) {
        isAccelerated = accelerated;
        currentDropInterval = isAccelerated ? acceleratedDropInterval : normalDropInterval;
    }
};

int Game::highScore = 0;

// Save the score to a file
void saveScore(int score) {
    ofstream file("scores.txt", ios::app);
    if (file.is_open()) {
        file << score << endl;
        file.close();
    } else {
        cerr << "Unable to open scores file for writing." << endl;
    }
}

// Load the scores from the file
vector<int> loadScores() {
    vector<int> scores;
    ifstream file("scores.txt");
    if (file.is_open()) {
        int score;
        while (file >> score) {
            scores.push_back(score);
        }
        file.close();
    } else {
        cerr << "Unable to open scores file for reading." << endl;
    }
    return scores;
}

// Print the top N scores
void printTopScores(const vector<int>& scores, int topN = 3) {
    vector<int> topScores = scores;
    sort(topScores.rbegin(), topScores.rend());
    for (int i = 0; i < min(topN, static_cast<int>(topScores.size())); ++i) {
        mvprintw(2 + i, 0, "%d. %d", i + 1, topScores[i]);
    }
}

// Show the game menu
void showMenu() {
    erase();  // Clear the screen
    mvprintw(0, 0, "=== GAME MENU ===");
    mvprintw(2, 0, "1. Start Game");
    mvprintw(3, 0, "2. View Leaderboard");
    mvprintw(4, 0, "3. Exit");
    mvprintw(6, 0, "Choose an option: ");
    refresh();
}

// View the leaderboard
void viewLeaderboard() {
    vector<int> scores = loadScores();
    clear();  // Clear the screen
    mvprintw(0, 0, "=== LEADERBOARD ===");
    if (scores.empty()) {
        mvprintw(2, 0, "No records.");
    } else {
        printTopScores(scores, 5);
    }
    mvprintw(7, 0, "Press any key to return to menu...");
    refresh();
    while (true) {
        int choice = getch();
        if (choice != -1) {
            break;
        }
    }
}

// Start the game
void startGame() {
    Game game;
    try {
        while (true) {
            game.draw();

            int ch = getch();
            if (ch == 'q') break;  // Quit
            if (ch == 'a') game.move(-1, 0);  // Move left
            if (ch == 'd') game.move(1, 0);   // Move right
            if (ch == 'w') game.rotate();     // Rotate
            if (ch == 's') game.setAccelerated(true);  // Accelerate drop
            if (ch != 's' && game.isAccelerated) game.setAccelerated(false);  // Stop acceleration

            if (!game.update()) {
                break;  // End game if collision detected
            }
        }
    } catch (const exception &e) {
        endwin();  // End ncurses mode
        cout << e.what() << endl;
    }

    endwin();  // End ncurses mode

    // Save the score and reload ncurses for the menu
    saveScore(game.score);
    initscr();
    timeout(10);
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
}

int main() {
    initscr();  // Initialize ncurses
    timeout(10);  // Reduce timeout to speed up input detection
    noecho();  // Do not echo input
    cbreak();  // Disable line buffering
    keypad(stdscr, TRUE);  // Enable special keys
    curs_set(0);  // Hide cursor
    start_color();  // Enable color functionality

    // Define color pairs
    init_pair(1, COLOR_CYAN, COLOR_BLACK);   // I
    init_pair(2, COLOR_GREEN, COLOR_BLACK);  // S
    init_pair(3, COLOR_RED, COLOR_BLACK);    // Z
    init_pair(4, COLOR_YELLOW, COLOR_BLACK); // O
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK); // T
    init_pair(6, COLOR_BLUE, COLOR_BLACK);   // L
    init_pair(7, COLOR_WHITE, COLOR_BLACK);  // J
    init_pair(8, COLOR_WHITE, COLOR_BLACK);  // Border

    while (true) {
        showMenu();
        int choice = getch();

        if (choice == '1') {
            // Start game
            startGame();
        } else if (choice == '2') {
            // View leaderboard
            viewLeaderboard();
        } else if (choice == '3') {
            // Exit game
            break;
        }
    }

    endwin();  // End ncurses mode
    return 0;
}

