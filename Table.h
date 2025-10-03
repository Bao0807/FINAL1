#pragma once
#include "Queue.h"
#include "Menu.h"
using namespace std;

const int NUM_TABLES = 10;
string gTableStatus[NUM_TABLES];
int gTableOwner[NUM_TABLES];
const int WIDTH = 20;
const int HEIGHT = 7;

void initTables();
void displayTables();
int chooseTable();
bool anyOrderForTable(Queue &q, int tableNo);

void clearScreen()
{
    cout << "\x1b[2J\x1b[H\x1b[0m";
    cout.flush();
}

void initTables()
{
    for (int i = 0; i < NUM_TABLES; i++)
    {
        gTableStatus[i] = "Empty";
        gTableOwner[i] = 0;
    }
}

// Vẽ 1 ô bàn với 2 dòng text: TABLE + trạng thái
void makeSquare(string box[], const string &text1, const string &text2, bool isFull)
{
    string topBottom = "+" + string(WIDTH - 2, '-') + "+";
    box[0] = topBottom;

    for (int i = 0; i < HEIGHT - 2; i++)
    {
        string line = "|";
        string color = isFull ? "\x1b[41m" : "\x1b[42m"; // Full = đỏ, Empty = xanh

        if (i == 0)
        { // dòng in TABLE
            int space = (WIDTH - 2 - text1.size()) / 2;
            line += color + string(space, ' ') + text1 + string(WIDTH - 2 - space - text1.size(), ' ') + "\x1b[0m";
        }
        else if (i == 3)
        { // dòng in trạng thái
            int space = (WIDTH - 2 - text2.size()) / 2;
            line += color + string(space, ' ') + text2 + string(WIDTH - 2 - space - text2.size(), ' ') + "\x1b[0m";
        }
        else
        {
            line += color + string(WIDTH - 2, ' ') + "\x1b[0m";
        }
        line += "|";
        box[i + 1] = line;
    }

    box[HEIGHT - 1] = topBottom;
}

void displayTables()
{
    string boxes[NUM_TABLES][HEIGHT];

    // Tạo toàn bộ 10 ô (TABLE + trạng thái)
    for (int i = 0; i < NUM_TABLES; i++)
    {
        string tname = "TABLE " + to_string(i + 1);
        string status = gTableStatus[i]; // lấy Empty / Full
        bool full = (status == "Full");
        makeSquare(boxes[i], tname, status, full);
    }

    int rows = 2, cols = 5, idx = 0;
    for (int r = 0; r < rows; r++)
    {
        for (int line = 0; line < HEIGHT; line++)
        {
            for (int c = 0; c < cols && idx + c < NUM_TABLES; c++)
            {
                cout << boxes[idx + c][line] << "  ";
            }
            cout << "\n";
        }
        cout << "\n";
        idx += cols;
    }
}

int chooseTable()
{
    while (true)
    {
        // mỗi lần chọn bàn hiển thị trên màn hình sạch
        clearScreen();
        displayTables();
        int t = 0;
        cout << "Choose table (1-" << NUM_TABLES << "): ";
        if (!(cin >> t))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input.\n";
            continue;
        }
        if (t < 1 || t > NUM_TABLES)
        {
            cout << "Out of range.\n";
            continue;
        }
        if (gTableStatus[t - 1] == "Full")
        {
            cout << "Table is Full.\n";
            continue;
        }
        return t;
    }
}

bool anyOrderForTable(Queue &q, int tableNo)
{
    if (tableNo < 1 || tableNo > NUM_TABLES)
        return false;
    // nếu có owner đã gán -> có order liên quan
    if (gTableOwner[tableNo - 1] != 0)
        return true;

    for (int i = 0; i < q.count; i++)
    {
        int idx = (q.front + i) % MAX;
        if (q.orders[idx].tableNumber == tableNo)
            return true;
    }
    return false;
}