#pragma once
#include <iostream>
#include <fstream>
#include <limits>
#include <sstream>
#include <thread>
#include <atomic>
#include <chrono>
#include "Queue.h"
#include "Menu.h"
#include "Table.h"

using namespace std;

// ===== timer globals (không dùng static/inline) =====
thread g_timerThread;
atomic<bool> g_timerRunning(false);
int g_secondsPer_minute = 60; // số giây tương ứng 1 "phút" mô phỏng
Queue *g_queuePtr = nullptr;  // con trỏ tới queue dùng trong thread

// Declarations
string formatPrice(long long price);
int getLastOrderID();

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

void enter();

// ======= Timer functions (new) =======
// start background timer: mỗi vòng là 1 giây mô phỏng
void startTimer(Queue *q)
{
    if (g_timerRunning.load())
        return;
    g_queuePtr = q;
    g_timerRunning.store(true);
    g_timerThread = thread([]()
    {
        while (g_timerRunning.load())
        {
            this_thread::sleep_for(chrono::seconds(1));
            if (!g_queuePtr) continue;

            // giảm 1 giây cho ORDER đầu tiên có status == "Cooking"
            advanceTime(*g_queuePtr, 1);

            // kiểm tra các order hoàn thành tổng thời gian -> đổi sang Ready và ghi bill
            for (int i = 0; i < g_queuePtr->count; ++i)
            {
                int idx = (g_queuePtr->front + i) % MAX;
                Order &o = g_queuePtr->orders[idx];
                if (o.status == "Cooking" && o.totalRemainingTime == 0)
                {
                    o.status = "Ready";
                    saveBillToFile(o);
                    writeProgressTxt(o);
                    cout << "Order ID " << o.id << " finished -> saved as Ready.\n";
                }
            }
        }
    });
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

// ======= advanceTime (đơn vị: giây) =======
// Thay đổi: chỉ giảm tổng thời gian cho order đầu tiên trong queue có status == "Cooking"
void advanceTime(Queue &q, int seconds)
{
    if (seconds <= 0 || q.count == 0) return;

    // Tìm order đầu tiên trong queue có status == "Cooking"
    int targetIdx = -1;
    for (int i = 0; i < q.count; ++i)
    {
        int idx = (q.front + i) % MAX;
        if (q.orders[idx].status == "Cooking")
        {
            targetIdx = idx;
            break;
        }
    }

    if (targetIdx == -1)
        return; // không có order đang nấu -> không giảm

    Order &o = q.orders[targetIdx];
    if (o.totalRemainingTime > 0)
    {
        o.totalRemainingTime -= seconds;
        if (o.totalRemainingTime < 0) o.totalRemainingTime = 0;
    }

    if (o.itemCount > 0)
    {
        if (o.totalRemainingTime > 0)
            o.status = "Cooking";
        else
            o.status = "Ready";
    }
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
    // build table string (bàn chính + các bàn được merge (gTableOwner == o.id))
    int tables[NUM_TABLES];
    int tcount = 0;

    ostringstream tboss;
    for (int i = 0; i < tcount; ++i)
    {
        if (i)
            tboss << " + ";
        tboss << tables[i];
    }
    string tableStr = tboss.str();
    if (tableStr.empty())
        tableStr = to_string(o.tableNumber);

    // Header (ANSI colors)
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

    // Progress section: CHỈ MỘT DÒNG — tổng thời gian còn lại
    cout << "\n\x1b[33mPROGRESS: \x1b[0m";
    if (o.itemCount == 0)
    {
        cout << "  (No items)\n";
    }
    else
    {
        if (o.totalRemainingTime > 0)
            cout << o.totalRemainingTime << "s remaining...\n";
        else
            cout << "\x1b[32mDONE\x1b[0m\n";
    }

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
    ostringstream oss;
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
         << " | Table: " << o.tableNumber << " | Status: " << o.status
         << " | Remaining(total): " << o.totalRemainingTime << "s\n";
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
    enter();
}

// thay o.status = "Cooking" khi thêm -> đặt mặc định Pending, temp-bill sẽ bật Cooking
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
            enter();
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

    // TÍNH TỔNG THỜI GIAN CÒN LẠI CHO ORDER (dựa trên các món đã nhập)
    o.totalRemainingTime = 0;
    for (int j = 0; j < o.itemCount; j++) {
        o.totalRemainingTime += o.items[j].remainingTime;
    }

    // mặc định khi add xong đặt Pending (không tự chạy timer nếu chưa temp-bill)
    o.status = "Pending";

    if (!enqueue(q, o))
    {
        cout << "Queue full! Reverting table.\n";
        gTableStatus[o.tableNumber - 1] = "Empty";
        gTableOwner[o.tableNumber - 1] = 0;
        return;
    }

    // đảm bảo timer chạy
    startTimer(&q);

    cout << "Added order ID " << o.id << "\n";
    Order *f = &q.orders[q.rear];

    // Lặp menu hành động sau khi add: cho phép Edit rồi quay lại actions
    while (true)
    {
        clearScreen();
        cout << "Added order ID " << f->id << "\n";
        cout << "Actions after Add: 1-Delete  2-Edit  3-Temp bill (COOK)  0-Back\nChoose: ";
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

            // Thay đổi bàn
            cout << "Move to another table(y/n): ";
            char tc;
            if (!(cin >> tc))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                tc = 'N';
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (tc == 'Y' || tc == 'y')
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

                // Sau khi thêm món mới trong Edit -> cập nhật tổng thời gian
                f->totalRemainingTime = 0;
                for (int j = 0; j < f->itemCount; j++) {
                    f->totalRemainingTime += f->items[j].remainingTime;
                }
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
    // Lặp menu hành động (vẫn cho phép Delete / Edit / Temp bill / Back)
    while (true)
    {
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
                     << " | Total: " << formatPrice(o.total)
                     << " | Remain: " << o.totalRemainingTime << "s\n";
            }
        }

        cout << "Actions in Display: 1-Delete  2-Edit  3-Temp bill (COOK) 4-Reset  0-Back\nChoose: ";
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
        if (act == 4)
        {
            clearScreen();
            displayTables();
            continue;
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

        // Cập nhật tổng thời gian sau khi edit
        f->totalRemainingTime = 0;
        for (int j = 0; j < f->itemCount; j++) {
            f->totalRemainingTime += f->items[j].remainingTime;
        }
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
        cin.ignore();
        cout << "\nPress Enter to continue...";
        string _tmp;
        getline(cin, _tmp);
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
    cin.ignore();
    cout << "\nPress Enter to continue...";
    string _tmp;
    getline(cin, _tmp);
}