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
const char BORDER_CHAR = '#'; // 邊框字符
const char BLOCK_CHAR = '#'; // 方塊字符

vector<vector<int>> shapes[7] = {
    {{1, 1, 1, 1}}, // I
    {{1, 1, 0}, {0, 1, 1}}, // S
    {{0, 1, 1}, {1, 1, 0}}, // Z
    {{1, 1}, {1, 1}}, // O
    {{1, 1, 1}, {0, 1, 0}}, // T
    {{1, 1, 1}, {1, 0, 0}}, // L
    {{1, 1, 1}, {0, 0, 1}}  // J
};

// 每個方塊的顏色
const int shapeColors[7] = {
    1, // I - Cyan
    2, // S - Green
    3, // Z - Red
    4, // O - Yellow
    5, // T - Magenta
    6, // L - Blue
    7  // J - White
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

    void newShape() {
        shape = nextShapes[0];
        currentShape = shapes[shape];
        nextShapes.erase(nextShapes.begin());
        nextShapes.push_back(rand() % 7);
        x = width / 2 - currentShape[0].size() / 2;
        y = 0;
        if (checkCollision(0, 0)) {
            throw runtime_error("遊戲結束");
        }
    }

    void draw() {
        static WINDOW *win = newwin(height + 2, width + 2, 0, 0); // 增加邊框空間
        static WINDOW *score_win = newwin(3, width + 2, height + 2, 0); // 增加邊框空間
        static WINDOW *next_win = newwin(height + 2, 10, 0, width + 2); // 顯示下一個方塊的窗口

        werase(win);
        werase(score_win);
        werase(next_win);

        // 繪製邊框
        for (int i = 0; i < height + 2; ++i) {
            mvwaddch(win, i, 0, BORDER_CHAR | COLOR_PAIR(8)); // 左邊框
            mvwaddch(win, i, width + 1, BORDER_CHAR | COLOR_PAIR(8)); // 右邊框
        }
        for (int j = 0; j < width + 2; ++j) {
            mvwaddch(win, 0, j, BORDER_CHAR | COLOR_PAIR(8)); // 上邊框
            mvwaddch(win, height + 1, j, BORDER_CHAR | COLOR_PAIR(8)); // 下邊框
        }

        // 繪製當前方塊
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

        // 繪製遊戲面板
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                if (board[i][j] != EMPTY_CHAR) {
                    mvwaddch(win, i + 1, j + 1, BLOCK_CHAR | COLOR_PAIR(board[i][j] - '1' + 1));
                }
            }
        }

        // 繪製分數
        mvwprintw(score_win, 1, 1, "Score: %d", score);
        wrefresh(win);
        wrefresh(score_win);

        // 繪製下三個方塊
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

    void placeShape() {
        for (int i = 0; i < currentShape.size(); ++i) {
            for (int j = 0; j < currentShape[0].size(); ++j) {
                if (currentShape[i][j] && y + i >= 0) {
                    board[y + i][x + j] = '1' + shape;
                }
            }
        }
    }

    void flashLine(int row) {
        for (int k = 0; k < 3; ++k) { // 閃爍3次
            for (int j = 0; j < width; ++j) {
                mvwaddch(stdscr, row + 1, j + 1, ' ' | COLOR_PAIR(8)); // 用背景色
            }
            wrefresh(stdscr);
            this_thread::sleep_for(100ms);

            for (int j = 0; j < width; ++j) {
                mvwaddch(stdscr, row + 1, j + 1, BLOCK_CHAR | COLOR_PAIR(8)); // 用邊框色
            }
            wrefresh(stdscr);
            this_thread::sleep_for(100ms);
        }
    }

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
                flashLine(i); // 先閃爍
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

    void rotate() {
        vector<vector<int>> rotated(currentShape[0].size(), vector<int>(currentShape.size()));
        for (int i = 0; i < currentShape.size(); ++i) {
            for (int j = 0; j < currentShape[0].size(); ++j) {
                rotated[j][currentShape.size() - 1 - i] = currentShape[i][j];
            }
        }

        // 檢查旋轉後是否超出範圍或碰撞
        if (!checkRotationCollision(rotated, x, y)) {
            currentShape = rotated;
        }
    }

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
                    return false; // 遊戲結束
                }
            }
        }
        return true;
    }

    void move(int dx, int dy) {
        if (!checkCollision(dx, dy)) {
            x += dx;
            y += dy;
        }
    }

    void setAccelerated(bool accelerated) {
        isAccelerated = accelerated;
        currentDropInterval = isAccelerated ? acceleratedDropInterval : normalDropInterval;
    }
};

int Game::highScore = 0;

void saveScore(int score) {
    ofstream file("scores.txt", ios::app);
    if (file.is_open()) {
        file << score << endl;
        file.close();
    } else {
        cerr << "Unable to open scores file for writing." << endl;
    }
}

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

void printTopScores(const vector<int>& scores, int topN = 3) {
    vector<int> topScores = scores;
    sort(topScores.rbegin(), topScores.rend());
    cout << "Top " << topN << " Scores:" << endl;
    for (int i = 0; i < min(topN, static_cast<int>(topScores.size())); ++i) {
        cout << i + 1 << ". " << topScores[i] << endl;
    }
}

int main() {
    initscr(); // 初始化 ncurses
    timeout(10); // 減少超時以加快輸入檢測
    noecho(); // 不回顯輸入
    cbreak(); // 禁用行緩衝
    keypad(stdscr, TRUE); // 啟用特殊按鍵
    curs_set(0); // 隱藏光標
    start_color(); // 啟動顏色功能

    // 定義顏色對
    init_pair(1, COLOR_CYAN, COLOR_BLACK);   // I
    init_pair(2, COLOR_GREEN, COLOR_BLACK);  // S
    init_pair(3, COLOR_RED, COLOR_BLACK);    // Z
    init_pair(4, COLOR_YELLOW, COLOR_BLACK); // O
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK); // T
    init_pair(6, COLOR_BLUE, COLOR_BLACK);   // L
    init_pair(7, COLOR_WHITE, COLOR_BLACK);  // J
    init_pair(8, COLOR_WHITE, COLOR_BLACK);  // 邊框

    Game game;

    try {
        while (true) {
            game.draw();

            int ch = getch();
            if (ch == 'q') break; // 退出
            if (ch == 'a') game.move(-1, 0); // 左移
            if (ch == 'd') game.move(1, 0);  // 右移
            if (ch == 'w') game.rotate(); // 旋轉
            if (ch == 's') game.setAccelerated(true); // 加速
            if (ch != 's' && game.isAccelerated) game.setAccelerated(false); // 停止加速

            if (!game.update()) {
                break; // 若有碰撞則結束遊戲
            }
        }
    } catch (const exception &e) {
        endwin(); // 結束 ncurses 模式
        cout << e.what() << endl;
    }

    endwin(); // 結束 ncurses 模式

    // 輸出分數並保存
    cout << "Game Over. Your Score: " << game.score << endl;
    saveScore(game.score);

    // 載入所有分數並輸出前三
    vector<int> scores = loadScores();
    printTopScores(scores);

    return 0;
}

