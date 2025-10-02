#pragma once
#include <iostream>
#include <fstream>
#include <limits>
#include <sstream>
// thêm libs cho timer
#include <thread>
#include <atomic>
#include <chrono>
#include "Queue.h"
#include "Menu.h"

using namespace std;

// helper clear screen dùng ANSI
void clearScreen()
{
    cout << "\x1b[2J\x1b[H\x1b[0m";
    cout.flush();
}

// ======= Globals =======
const int NUM_TABLES = 10;
string gTableStatus[NUM_TABLES];
int gTableOwner[NUM_TABLES];

// ===== timer globals (không dùng static/inline) =====
std::thread g_timerThread;
std::atomic<bool> g_timerRunning(false);
int g_secondsPer_minute = 60; // số giây tương ứng 1 "phút" mô phỏng
Queue *g_queuePtr = nullptr;  // con trỏ tới queue dùng trong thread

// Declarations
string formatPrice(long long price);
int getLastOrderID();
void initTables();
void displayTables();
int chooseTable();
bool anyOrderForTable(Queue &q, int tableNo);

Order *findOrderByID(Queue &q, int id);
bool removeOrderByID(Queue &q, int id, Order &out);

void printBill(const Order &o);
void saveBillToFile(const Order &o);

const char *itemState(const OrderDetail &d);
void writeProgressTxt(const Order &o);
void snapshotReadyOrdersToBill(Queue &q);
void updateRevenue(const Order &o);
void viewRevenueSummary();

void addOrder(Queue &q, int &idCounter);
void displayQueue(Queue &q);
void editOrder(Queue &q);
void deleteOrder(Queue &q);
void advanceTime(Queue &q, int seconds);
void searchByTable(Queue &q, int tableNo);
void payment(Queue &q);
void manageData(Queue &q, int &idCounter);

// ======= Timer functions (new) =======
// start background timer: mỗi vòng là 1 giây mô phỏng
void startTimer(Queue *q)
{
    if (g_timerRunning.load())
        return;
    g_queuePtr = q;
    g_timerRunning.store(true);
    g_timerThread = std::thread([]()
                                {
        while (g_timerRunning.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (g_queuePtr) {
                // giảm 1 giây cho tất cả đơn trong queue
                advanceTime(*g_queuePtr, 1);
                // kiểm tra và xử lý đơn hoàn thành
                int toRemove[MAX];
                int remCount = 0;
                for (int i = 0; i < g_queuePtr->count; ++i) {
                    int idx = (g_queuePtr->front + i) % MAX;
                    Order &o = g_queuePtr->orders[idx];
                    if (o.itemCount <= 0) continue;
                    bool allZero = true;
                    for (int j = 0; j < o.itemCount; ++j) {
                        if (o.items[j].remainingTime > 0) { allZero = false; break; }
                    }
                    if (allZero && o.status != "Ready" && o.status != "Saved") {
                        if (remCount < MAX) toRemove[remCount++] = o.id;
                    }
                }
                for (int k = 0; k < remCount; ++k) {
                    Order rem;
                    if (removeOrderByID(*g_queuePtr, toRemove[k], rem)) {
                        rem.status = "Ready"; // hoàn thành, chưa thanh toán
                        saveBillToFile(rem);
                        writeProgressTxt(rem);
                        cout << "Order ID " << rem.id << " finished -> moved out of queue and saved (Ready).\n";
                    }
                }
            }
        } });
}
// dừng timer
void stopTimer()
{
    if (!g_timerRunning.load())
        return;
    g_timerRunning.store(false);
    if (g_timerThread.joinable())
        g_timerThread.join();
}

// ======= Definitions =======
string formatPrice(long long price)
{
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%lld", (long long)price);
    string s = tmp, out;
    out.reserve(s.size() + s.size() / 3 + 2);
    int c = 0;
    for (int i = (int)s.size() - 1; i >= 0; --i)
    {
        out.insert(out.begin(), s[i]);
        c++;
        if (c == 3 && i > 0)
        {
            out.insert(out.begin(), '.');
            c = 0;
        }
    }
    return out;
}

int getLastOrderID()
{
    ifstream fin("bill.txt");
    if (!fin)
        return 0;
    string line;
    int last = 0;
    while (getline(fin, line))
    {
        if (line.rfind("OrderID:", 0) == 0)
        {
            int id = atoi(line.c_str() + 8);
            if (id > last)
                last = id;
        }
    }
    fin.close();
    return last;
}

void initTables()
{
    for (int i = 0; i < NUM_TABLES; i++)
    {
        gTableStatus[i] = "Empty";
        gTableOwner[i] = 0;
    }
}

void displayTables()
{
    cout << "\n--- Table Status ---\n";
    for (int i = 0; i < NUM_TABLES; i++)
        cout << "Table " << (i + 1) << ": " << gTableStatus[i] << "\n";
    cout << "---------------------\n";
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

Order *findOrderByID(Queue &q, int id)
{
    for (int i = 0; i < q.count; i++)
    {
        int idx = (q.front + i) % MAX;
        if (q.orders[idx].id == id)
            return &q.orders[idx];
    }
    return 0;
}

bool removeOrderByID(Queue &q, int id, Order &out)
{
    if (isEmpty(q))
        return false;
    int foundIndex = -1;
    // tìm vị trí (offset i trong queue)
    for (int i = 0; i < q.count; i++)
    {
        int idx = (q.front + i) % MAX;
        if (q.orders[idx].id == id)
        {
            foundIndex = i;
            break;
        }
    }
    if (foundIndex == -1)
        return false;

    // sao chép order ra ngoài
    int src = (q.front + foundIndex) % MAX;
    out = q.orders[src];

    // dịch trái các phần tử sau foundIndex
    for (int i = foundIndex; i < q.count - 1; i++)
    {
        int from = (q.front + i + 1) % MAX;
        int to = (q.front + i) % MAX;
        q.orders[to] = q.orders[from];
    }
    // cập nhật rear và count
    q.rear = (q.rear - 1 + MAX) % MAX;
    q.count--;

    // xóa mọi gTableOwner trùng order.id (nếu có merge)
    for (int t = 0; t < NUM_TABLES; t++)
    {
        if (gTableOwner[t] == out.id)
        {
            gTableOwner[t] = 0;
            // nếu không còn order nào cho bàn đó thì chuyển status về Empty
            if (!anyOrderForTable(q, t + 1))
                gTableStatus[t] = "Empty";
        }
    }

    // nếu primary table của out không còn order, cập nhật trạng thái
    if (!anyOrderForTable(q, out.tableNumber))
        gTableStatus[out.tableNumber - 1] = "Empty";

    return true;
}

void printBill(const Order &o)
{
    // Tạo chuỗi các bàn: bàn chính + các bàn được merge (gTableOwner == o.id)
    int tables[NUM_TABLES];
    int tcount = 0;
    if (o.tableNumber >= 1 && o.tableNumber <= NUM_TABLES)
        tables[tcount++] = o.tableNumber;

    for (int t = 0; t < NUM_TABLES; ++t)
    {
        if (gTableOwner[t] == o.id)
        {
            int tn = t + 1;
            bool already = false;
            for (int k = 0; k < tcount; ++k)
                if (tables[k] == tn)
                {
                    already = true;
                    break;
                }
            if (!already)
                tables[tcount++] = tn;
        }
    }

    std::ostringstream tboss;
    for (int i = 0; i < tcount; ++i)
    {
        if (i)
            tboss << " + ";
        tboss << tables[i];
    }
    std::string tableStr = tboss.str();
    if (tableStr.empty())
        tableStr = std::to_string(o.tableNumber);

    // Giao diện bill theo yêu cầu (ANSI colors)
    cout << "\x1b[36m\n=================================================================\n"
         << "                          RESTAURANT BILL           \n"
         << "=================================================================\n\x1b[0m";
    cout << "OrderID:\x1b[33m" << o.id << "\x1b[0m\n";
    cout << "Customer:\x1b[32m" << o.customerName << "\x1b[0m\n";
    cout << "Table:\x1b[33m" << tableStr << "\x1b[0m\n";
    cout << "-----------------------------------------------------------------\n";
    cout << "\x1b[35mNo\tFood Name\t\tQuantity\tPrice\x1b[0m\n";
    for (int i = 0; i < o.itemCount; i++)
    {
        cout << (i + 1) << ".\t" << o.items[i].foodName << "\t\t"
             << o.items[i].quantity << "\t\t" << formatPrice(o.items[i].subtotal) << "\n";
    }
    cout << "-----------------------------------------------------------------\n";
    cout << "\x1b[31mTOTAL: " << formatPrice(o.total) << " VND\x1b[0m\n";
    cout << "Status: " << o.status << "\n";
    cout << "\x1b[36m=================================================================\x1b[0m\n";
}

// thay đổi saveBillToFile để ghi chuỗi bàn (đã merge) rõ ràng
void saveBillToFile(const Order &o)
{
    // build table string (không dùng vector)
    int tables[NUM_TABLES];
    int tcount = 0;
    if (o.tableNumber >= 1 && o.tableNumber <= NUM_TABLES)
        tables[tcount++] = o.tableNumber;
    for (int t = 0; t < NUM_TABLES; ++t)
    {
        if (gTableOwner[t] == o.id)
        {
            int tn = t + 1;
            bool already = false;
            for (int k = 0; k < tcount; ++k)
                if (tables[k] == tn)
                {
                    already = true;
                    break;
                }
            if (!already)
                tables[tcount++] = tn;
        }
    }
    std::ostringstream oss;
    for (int i = 0; i < tcount; ++i)
    {
        if (i)
            oss << " + ";
        oss << tables[i];
    }
    string tableStr = oss.str();
    if (tableStr.empty())
        tableStr = to_string(o.tableNumber);

    ofstream fout("bill.txt", ios::app);
    fout << "=====================================\n";
    fout << "           RESTAURANT BILL           \n";
    fout << "=====================================\n";
    fout << "OrderID:   " << o.id << "\n";
    fout << "Customer:  " << o.customerName << "\n";
    fout << "Table:     " << tableStr << "\n";
    fout << "-------------------------------------\n";
    fout << "Food        Qty     Price\n";
    fout << "-------------------------------------\n";
    for (int i = 0; i < o.itemCount; i++)
    {
        fout << o.items[i].foodName << "\t" << o.items[i].quantity << "\t" << o.items[i].subtotal << "\n";
    }
    fout << "-------------------------------------\n";
    fout << "TOTAL: " << o.total << " VND\n";
    fout << "Status: " << o.status << "\n";
    fout << "=====================================\n\n";
    fout.close();
}

const char *itemState(const OrderDetail &d)
{
    int full = d.prepTime * d.quantity;
    if (d.remainingTime <= 0)
        return "done";
    if (d.remainingTime >= full)
        return "wait";
    return "cook";
}

void writeProgressTxt(const Order &o)
{
    ofstream fout("progress.txt", ios::app);
    fout << "OrderID: " << o.id << " | Customer: " << o.customerName
         << " | Table: " << o.tableNumber << " | Status: " << o.status << "\n";
    for (int i = 0; i < o.itemCount; i++)
    {
        const OrderDetail &d = o.items[i];
        fout << " - " << d.foodName << " x" << d.quantity
             << " [" << itemState(d) << "]"
             << " remaining=" << d.remainingTime << "m\n";
    }
    fout << "-------------------------------------\n";
    fout.close();
}

void snapshotReadyOrdersToBill(Queue &q)
{
    for (int i = 0; i < q.count; i++)
    {
        int idx = (q.front + i) % MAX;
        Order &o = q.orders[idx];
        if (o.status == "Ready" && !o.progressLogged)
        {
            Order snap = o;
            snap.status = "Pending";
            saveBillToFile(snap);
            o.progressLogged = true;
        }
    }
}

void updateRevenue(const Order &o)
{
    ofstream fout("revenue.txt", ios::app);
    fout << "OrderID: " << o.id << " | Total: " << o.total << " | Items: ";
    for (int i = 0; i < o.itemCount; i++)
    {
        if (i)
            fout << ", ";
        fout << o.items[i].foodName << "x" << o.items[i].quantity;
    }
    fout << "\n";
    fout.close();
}

void viewRevenueSummary()
{
    ifstream fin("revenue.txt");
    long long grand = 0;
    struct C
    {
        string name;
        int qty;
    } cnt[128];
    int cc = 0;
    for (int i = 0; i < 128; i++)
    {
        cnt[i].qty = 0;
    }
    if (fin)
    {
        string line;
        while (getline(fin, line))
        {
            size_t p = line.find("Total:");
            if (p != string::npos)
            {
                long long t = atoll(line.c_str() + p + 6);
                grand += t;
            }
            p = line.find("Items:");
            if (p != string::npos)
            {
                string s = line.substr(p + 6);
                string token;
                for (size_t i = 0; i <= s.size(); ++i)
                {
                    char ch = (i < s.size() ? s[i] : ',');
                    if (ch == ',')
                    {
                        size_t start = 0;
                        while (start < token.size() && token[start] == ' ')
                            start++;
                        if (start < token.size())
                        {
                            size_t x = token.rfind('x');
                            if (x != string::npos)
                            {
                                string name = token.substr(start, x - start);
                                int q = atoi(token.c_str() + x + 1);
                                bool f = false;
                                for (int k = 0; k < cc; k++)
                                    if (cnt[k].name == name)
                                    {
                                        cnt[k].qty += q;
                                        f = true;
                                        break;
                                    }
                                if (!f && cc < 128)
                                {
                                    cnt[cc].name = name;
                                    cnt[cc].qty = q;
                                    cc++;
                                }
                            }
                        }
                        token.clear();
                    }
                    else
                        token.push_back(ch);
                }
            }
        }
        fin.close();
    }
    cout << "\n=== REVENUE SUMMARY ===\n";
    cout << "Total revenue: " << formatPrice(grand) << " VND\n";
    cout << "Items sold:\n";
    for (int i = 0; i < cc; i++)
        cout << " - " << cnt[i].name << ": " << cnt[i].qty << "\n";
    cout << "=======================\n";
}

// thay đổi addOrder: sau enqueue đặt trạng thái Cooking và khởi động timer nếu chưa chạy
void addOrder(Queue &q, int &idCounter)
{
    clearScreen();
    Order o;
    o.id = ++idCounter;
    cout << "Customer name: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, o.customerName);

    int tableNum = chooseTable();
    o.tableNumber = tableNum;
    o.tableStatus = "Full";
    gTableStatus[o.tableNumber - 1] = "Full";
    // gán owner cho bàn chính
    gTableOwner[o.tableNumber - 1] = o.id;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    clearScreen();
    char more = 'n';
    do
    {
        if (o.itemCount >= MAX_ITEMS)
        {
            cout << "Reached max items!\n";
            break;
        }

        displayMenu();
        cout << "Choose food (number or exact name): ";
        string inputFood;
        getline(cin, inputFood);
        MenuItem sel = getMenuItem(inputFood);
        if (!sel.available || sel.price == 0)
        {
            cout << "Invalid choice or sold out.\n";
            continue;
        }

        OrderDetail d;
        d.foodName = sel.foodName;
        cout << "Quantity: ";
        if (!(cin >> d.quantity) || d.quantity <= 0)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid quantity.\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        d.price = sel.price;
        d.subtotal = d.price * d.quantity;
        d.prepTime = sel.prepTime;
        d.remainingTime = sel.prepTime * d.quantity;

        o.items[o.itemCount++] = d;
        o.total += d.subtotal;

        cout << "Add more food? (y/n): ";
        cin >> more;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    } while (more == 'y' || more == 'Y');

    // mặc định khi add xong đặt Pending nhưng theo yêu cầu bắt đầu đếm ngay -> đặt Cooking
    o.status = "Cooking";

    if (!enqueue(q, o))
    {
        cout << "Queue full! Reverting table.\n";
        gTableStatus[o.tableNumber - 1] = "Empty";
        gTableOwner[o.tableNumber - 1] = 0;
        return;
    }

    // đảm bảo timer chạy (không tạo nhiều thread vì startTimer kiểm tra flag)
    startTimer(&q);

    cout << "Added order ID " << o.id << "\n";
    Order *f = &q.orders[q.rear];

    // Lặp menu hành động sau khi add: cho phép Edit rồi quay lại actions
    while (true)
    {
        clearScreen();
        cout << "Added order ID " << f->id << "\n";
        cout << "Actions after Add: 1-Delete  2-Edit  3-Temp bill (COOK)  4-Save (hold)  0-Back\nChoose: ";
        int act;
        if (!(cin >> act))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid.\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (act == 1)
        {
            // Delete the newly added order
            Order rem;
            if (removeOrderByID(q, f->id, rem))
            {
                cout << "Deleted.\n";
                // trạng thái & owner đã được xử lý trong removeOrderByID
            }
            else
                cout << "Delete failed.\n";
            break;
        }
        else if (act == 2)
        {
            // Edit in-place (không hỏi ID) rồi quay lại menu actions
            cout << "Current customer: " << f->customerName << "\n";
            cout << "New name (or '.' keep): ";
            string name;
            getline(cin, name);
            if (name != ".")
                f->customerName = name;

            // Thay đổi bàn: đưa 2 lựa chọn Move hoặc Add(merge)
            cout << "Table action: 1-Move to another table  2-Add another table (merge)  0-Keep current\nChoose: ";
            int tc;
            if (!(cin >> tc))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                tc = 0;
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (tc == 1)
            {
                int old = f->tableNumber;
                int nt = chooseTable();
                // gỡ owner của các bàn đang trỏ tới order này (bao gồm old)
                for (int t = 0; t < NUM_TABLES; t++)
                    if (gTableOwner[t] == f->id)
                        gTableOwner[t] = 0;
                // gán bàn mới là primary
                f->tableNumber = nt;
                gTableStatus[nt - 1] = "Full";
                gTableOwner[nt - 1] = f->id;
                // nếu bàn cũ không còn order thì empty
                if (!anyOrderForTable(q, old))
                    gTableStatus[old - 1] = "Empty";
            }
            else if (tc == 2)
            {
                cout << "Select table to ADD (merge) with current order: ";
                int addt;
                if (!(cin >> addt))
                {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    addt = -1;
                }
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                if (addt >= 1 && addt <= NUM_TABLES)
                {
                    if (gTableOwner[addt - 1] == 0 && !anyOrderForTable(q, addt))
                    {
                        gTableOwner[addt - 1] = f->id;
                        gTableStatus[addt - 1] = "Full";
                        cout << "Merged table " << addt << " into order " << f->id << ".\n";
                    }
                    else
                    {
                        cout << "Table not available to merge.\n";
                    }
                }
                else
                    cout << "Invalid table.\n";
            }
            // tiếp tục edit món nếu muốn
            cout << "Add more items? (y/n): ";
            char c2;
            cin >> c2;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (c2 == 'y' || c2 == 'Y')
            {
                char mm = 'n';
                do
                {
                    if (f->itemCount >= MAX_ITEMS)
                    {
                        cout << "Reached max items!\n";
                        break;
                    }
                    displayMenu();
                    cout << "Choose food (number or name): ";
                    string in;
                    getline(cin, in);
                    MenuItem sel = getMenuItem(in);
                    if (!sel.available || sel.price == 0)
                    {
                        cout << "Invalid.\n";
                        continue;
                    }
                    OrderDetail d;
                    d.foodName = sel.foodName;
                    cout << "Quantity: ";
                    if (!(cin >> d.quantity) || d.quantity <= 0)
                    {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Invalid quantity.\n";
                        continue;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    d.price = sel.price;
                    d.subtotal = d.price * d.quantity;
                    d.prepTime = sel.prepTime;
                    d.remainingTime = sel.prepTime * d.quantity;
                    f->items[f->itemCount++] = d;
                    f->total += d.subtotal;
                    cout << "Add more? (y/n): ";
                    cin >> mm;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                } while (mm == 'y' || mm == 'Y');
            }
            cout << "Edit done.\n";
            // quay lại actions
        }
        else if (act == 3)
        {
            // Temp bill cho đơn vừa thêm -> chuyển trạng thái Cooking (nếu chưa)
            f->status = "Cooking";
            writeProgressTxt(*f);
            printBill(*f);
            cout << "[Temp bill] COOK & progress logged.\n";
            // pause để user đọc bill trước khi vòng lặp clear màn hình
            {
                cout << "\nPress Enter to continue...";
                string _tmp;
                getline(cin, _tmp);
            }
            // đảm bảo timer chạy
            startTimer(&q);
        }
        else if (act == 4)
        {
            f->isOnHold = true;
            f->status = "Saved";
            cout << "Saved (on hold).\n";
        }
        else if (act == 0)
        {
            break;
        }
        else
        {
            cout << "Invalid.\n";
        }
    }
}

void displayQueue(Queue &q)
{
    // Lặp menu hành động (vẫn cho phép Delete / Edit / Temp bill / Save / Back)
    while (true)
    {
        clearScreen();
        cout << "\n===== ORDERS IN QUEUE =====\n";
        if (q.count == 0)
        {
            cout << "No orders in queue.\n";
        }
        else
        {
            for (int i = 0; i < q.count; i++)
        {
            int idx = (q.front + i) % MAX;
            Order &o = q.orders[idx];
            cout << "OrderID: " << o.id
                 << " | Name: " << o.customerName
                 << " | Table: " << o.tableNumber
                 << " | Status: " << o.status
                 << " | Total: " << formatPrice(o.total) << "\n";
            for (int j = 0; j < o.itemCount; j++)
            {
                cout << "  - " << o.items[j].foodName
                     << " x" << o.items[j].quantity
                     << " | remain " << o.items[j].remainingTime << "s"
                     << " [" << itemState(o.items[j]) << "]\n";
            }
            cout << "\n";
        }
    }

        cout << "Actions in Display: 1-Delete  2-Edit  3-Temp bill (COOK)  4-Save (hold)  0-Back\nChoose: ";
        int act;
        if (!(cin >> act))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid.\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (act == 1)
        {
            deleteOrder(q);
            break;
        }
        else if (act == 2)
        {
            editOrder(q);
        }
        else if (act == 3)
        {
            cout << "Enter ID to temp-bill: ";
            int id;
            cin >> id;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            Order *f = findOrderByID(q, id);
            if (!f)
            {
                cout << "Not found.\n";
            }
            else
            {
                f->status = "Cooking";
                writeProgressTxt(*f);
                printBill(*f);
                cout << "[Temp bill] COOK & progress logged.\n";
                cout << "\nPress Enter to continue...";
                string _tmp;
                getline(cin, _tmp);
            }
        }
        else if (act == 4)
        {
            cout << "Enter ID to save (hold): ";
            int id;
            cin >> id;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            Order *f = findOrderByID(q, id);
            if (!f)
            {
                cout << "Not found.\n";
            }
            else
            {
                f->isOnHold = true;
                f->status = "Saved";
                cout << "Saved (on hold).\n";
            }
        }
        else if (act == 0)
        {
            break;
        }
        else
        {
            cout << "Invalid.\n";
        }
    }
}

void editOrder(Queue &q)
{
    int id;
    cout << "Enter ID to edit: ";
    cin >> id;
    Order *f = findOrderByID(q, id);
    if (!f)
    {
        cout << "Not found.\n";
        return;
    }
    cout << "Current customer: " << f->customerName << "\n";
    cout << "New name (or '.' keep): ";
    string name;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, name);
    if (name != ".")
        f->customerName = name;

    cout << "Change table? (y/n): ";
    char c;
    cin >> c;
    if (c == 'y' || c == 'Y')
    {
        int old = f->tableNumber;
        int nt = chooseTable();
        f->tableNumber = nt;
        gTableStatus[nt - 1] = "Full";
        bool hasOther = false;
        for (int i = 0; i < q.count; i++)
        {
            int idx = (q.front + i) % MAX;
            if (q.orders[idx].id != f->id && q.orders[idx].tableNumber == old)
            {
                hasOther = true;
                break;
            }
        }
        if (!hasOther && old >= 1 && old <= NUM_TABLES)
            gTableStatus[old - 1] = "Empty";
    }

    cout << "Add more items? (y/n): ";
    char c2;
    cin >> c2;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (c2 == 'y' || c2 == 'Y')
    {
        char more = 'n';
        do
        {
            if (f->itemCount >= MAX_ITEMS)
            {
                cout << "Reached max items!\n";
                break;
            }
            displayMenu();
            cout << "Choose food (number or name): ";
            string in;
            getline(cin, in);
            MenuItem sel = getMenuItem(in);
            if (!sel.available || sel.price == 0)
            {
                cout << "Invalid.\n";
                continue;
            }
            OrderDetail d;
            d.foodName = sel.foodName;
            cout << "Quantity: ";
            if (!(cin >> d.quantity) || d.quantity <= 0)
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid quantity.\n";
                continue;
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            d.price = sel.price;
            d.subtotal = d.price * d.quantity;
            d.prepTime = sel.prepTime;
            d.remainingTime = sel.prepTime * d.quantity;
            f->items[f->itemCount++] = d;
            f->total += d.subtotal;
            cout << "Add more? (y/n): ";
            cin >> more;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        } while (more == 'y' || more == 'Y');
    }
    cout << "Edit done.\n";
}

void deleteOrder(Queue &q)
{
    int id;
    cout << "Enter ID to delete: ";
    cin >> id;
    Order rem;
    if (removeOrderByID(q, id, rem))
    {
        cout << "Deleted.\n";
        int t = rem.tableNumber;
        if (t >= 1 && t <= NUM_TABLES && !anyOrderForTable(q, t))
            gTableStatus[t - 1] = "Empty";
    }
    else
        cout << "Not found.\n";
}

void advanceTime(Queue &q, int seconds)
{
    if (seconds <= 0)
        return;
    for (int i = 0; i < q.count; ++i)
    {
        int idx = (q.front + i) % MAX;
        Order &o = q.orders[idx];
        bool anyRunning = false;
        for (int j = 0; j < o.itemCount; ++j)
        {
            OrderDetail &d = o.items[j];
            if (d.remainingTime > 0)
            {
                // remainingTime lưu đơn vị giây
                if (d.remainingTime > seconds)
                    d.remainingTime -= seconds;
                else
                    d.remainingTime = 0;
            }
            if (d.remainingTime > 0)
                anyRunning = true;
        }
        if (o.itemCount > 0)
        {
            if (anyRunning)
                o.status = "Cooking";
            else
                o.status = "Ready";
        }
    }
}

void searchByTable(Queue &q, int tableNo)
{
    cout << "\n-- Orders at Table " << tableNo << " --\n";
    bool any = false;
    for (int i = 0; i < q.count; i++)
    {
        int idx = (q.front + i) % MAX;
        const Order &o = q.orders[idx];
        if (o.tableNumber == tableNo)
        {
            any = true;
            printBill(o);
        }
    }
    if (!any)
        cout << "No orders.\n";
}

void payment(Queue &q)
{
    int tableNo;
    cout << "Enter table number: ";
    cin >> tableNo;
    if (tableNo < 1 || tableNo > NUM_TABLES)
    {
        cout << "Invalid table.\n";
        return;
    }
    int ids[32];
    int n = 0;
    cout << "Orders:\n";
    for (int i = 0; i < q.count; i++)
    {
        int idx = (q.front + i) % MAX;
        Order &o = q.orders[idx];
        if (o.tableNumber == tableNo)
        {
            cout << " - ID " << o.id << " [" << o.status << "] Total=" << formatPrice(o.total) << "\n";
            if (n < 32)
                ids[n++] = o.id;
        }
    }
    if (n == 0)
    {
        cout << "No orders.\n";
        return;
    }
    int id;
    cout << "Choose Order ID to pay: ";
    cin >> id;
    bool ok = false;
    for (int i = 0; i < n; i++)
        if (ids[i] == id)
        {
            ok = true;
            break;
        }
    if (!ok)
    {
        cout << "ID not in this table.\n";
        return;
    }
    cout << "Payment method (1-Cash, 2-QR, 3-Card): ";
    int pm;
    cin >> pm;
    (void)pm;

    Order rem;
    if (!removeOrderByID(q, id, rem))
    {
        cout << "Not found.\n";
        return;
    }
    rem.status = "Completed";
    printBill(rem);
    saveBillToFile(rem);
    updateRevenue(rem);
    cout << "Payment done & saved.\n";
    int t = rem.tableNumber;
    if (t >= 1 && t <= NUM_TABLES && !anyOrderForTable(q, t))
        gTableStatus[t - 1] = "Empty";
}

void manageData(Queue &q, int &idCounter)
{
    int c;
    do
    {
        clearScreen();
        cout << "\n===== MANAGE DATA =====\n";
        cout << "1. Add Order\n";
        cout << "2. Display All\n";
        cout << "0. Back\n";
        cout << "Choose: ";
        if (!(cin >> c))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            c = -1;
            continue;
        }
        // sau khi chọn, làm sạch màn hình trước khi thực hiện action
        clearScreen();
        switch (c)
        {
        case 1:
            addOrder(q, idCounter);
            break;
        case 2:
            displayQueue(q);
            break;
        case 0:
            break;
        default:
            cout << "Invalid.\n";
            break;
        }
    } while (c != 0);
}
