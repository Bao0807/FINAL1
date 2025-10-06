#pragma once
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>
#include <atomic>
#include <chrono>

#include "Queue.h"
#include "Menu.h"
#include "Table.h"
#include"bill.h"
#include"Revenue.h"

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
const char *itemState(const OrderDetail &d);

void addOrder(Queue &q, int &idCounter);
void displayQueue(Queue &q);
void editOrder(Queue &q);
void deleteOrder(Queue &q);
void advanceTime(Queue &q, int seconds);
void searchByTable(Queue &q, int tableNo);

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

const char *itemState(const OrderDetail &d)
{
    int full = d.prepTime * d.quantity;
    if (d.remainingTime <= 0)
        return "done";
    if (d.remainingTime >= full)
        return "wait";
    return "cook";
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

    // Tìm đơn hàng theo ID
    Order *f = findOrderByID(q, id);
    if (f == nullptr)
    {
        cout << "Not found.\n";
        return;
    }

    // Chặn xóa nếu trạng thái là COOK hoặc DONE
    if (f->status == "COOK" || f->status == "DONE")
    {
        cout << "Cannot delete order while it is being cooked or already done.\n";
        return;
    }

    // Nếu được phép, xóa khỏi hàng đợi
    if (removeOrderByID(q, id, rem))
    {
        cout << "Deleted.\n";a
        int t = rem.tableNumber;
        if (t >= 1 && t <= NUM_TABLES && !anyOrderForTable(q, t))
            gTableStatus[t - 1] = "Empty";
    }
    else
    {
        cout << "Not found.\n";
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

