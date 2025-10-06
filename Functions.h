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
#include "bill.h"
#include "Revenue.h"

using namespace std;

// ===== timer globals =====
thread g_timerThread;
atomic<bool> g_timerRunning(false);
int g_secondsPer_minute = 60;
Queue *g_queuePtr = nullptr;

// ===== Declarations =====
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

// ===== Timer =====
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
            advanceTime(*g_queuePtr, 1);
        }
    });
}

void stopTimer()
{
    if (!g_timerRunning.load())
        return;
    g_timerRunning.store(false);
    if (g_timerThread.joinable())
        g_timerThread.join();
}

// ===== advanceTime =====
void advanceTime(Queue &q, int seconds)
{
    if (seconds <= 0 || q.count == 0) return;

    // tìm order đầu tiên đang Cooking
    int targetIdx = -1;
    for (int i = 0; i < q.count; ++i)
    {
        int idx = (q.front + i) % MAX;
        if (q.orders[idx].status == "Nau")
        {
            targetIdx = idx;
            break;
        }
    }
    if (targetIdx == -1) return;

    Order &o = q.orders[targetIdx];
    if (o.totalRemainingTime > 0)
    {
        o.totalRemainingTime -= seconds;
        if (o.totalRemainingTime < 0) o.totalRemainingTime = 0;
    }

    if (o.totalRemainingTime == 0 && o.status == "Nau")
    {
        o.status = "Ready";
        saveBillToFile(o);
        cout << "Order ID " << o.id << " finished -> saved as Ready.\n";

        // Chuyển đơn Wait đầu tiên sang Cooking
        for (int j = 0; j < q.count; ++j)
        {
            int idx2 = (q.front + j) % MAX;
            Order &next = q.orders[idx2];
            if (next.status == "Cho")
            {
                next.status = "Nau";
                cout << "Order ID " << next.id << " is now Cooking.\n";
                break;
            }
        }
    }
}

int getLastOrderID()
{
    ifstream fin("bill.txt");
    if (!fin) return 0;
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
    return nullptr;
}

const char *itemState(const OrderDetail &d)
{
    int full = d.prepTime * d.quantity;
    if (d.remainingTime <= 0)
        return "done";
    if (d.remainingTime >= full)
        return "Cho";
    return "cook";
}

// ===== Add Order =====
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

    o.totalRemainingTime = 0;
    for (int j = 0; j < o.itemCount; j++)
        o.totalRemainingTime += o.items[j].remainingTime;

    o.status = "Pending";

    if (!enqueue(q, o))
    {
        cout << "Queue full! Reverting table.\n";
        gTableStatus[o.tableNumber - 1] = "Empty";
        gTableOwner[o.tableNumber - 1] = 0;
        return;
    }

    startTimer(&q);

    cout << "Added order ID " << o.id << "\n";
    Order *f = &q.orders[q.rear];

    while (true)
    {
        clearScreen();
        cout << "Added order ID " << f->id << "\n";
        cout << "Tuy chinh don hien tai: 1-Xoa  2-Sua  3-Xuat hoa don tam thoi (Nau)  0-Quay lai\nChon: ";
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
            Order rem;
            if (removeOrderByID(q, f->id, rem))
            {
                cout << "Xoa.\n";
            }
            else cout << "Xoa that bai.\n";
            break;
        }
        else if (act == 2)
        {
            editOrder(q);
        }
        else if (act == 3)
        {
            bool cookingExists = false;
            for (int i = 0; i < q.count; i++)
            {
                int idx = (q.front + i) % MAX;
                if (q.orders[idx].status == "Nau")
                {
                    cookingExists = true;
                    break;
                }
            }
            if (cookingExists)
                f->status = "Cho";
            else
                f->status = "Nau";

            printBill(*f);
            cout << "[Temp bill] -> " << f->status << " & progress logged.\n";
            cout << "\nPress Enter to continue...";
            string _tmp;
            getline(cin, _tmp);
            startTimer(&q);
        }
        else if (act == 0)
            break;
        else
            cout << "Invalid.\n";
    }
}

// ===== Display Queue =====
void displayQueue(Queue &q)
{
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

        cout << "Tuy chinh don: 1-Xoa  2-Sua  3-Xuat hoa don tam thoi (Nau)  0-Quay lai\nChon: ";
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
                bool cookingExists = false;
                for (int i = 0; i < q.count; i++)
                {
                    int idx = (q.front + i) % MAX;
                    if (q.orders[idx].status == "Nau")
                    {
                        cookingExists = true;
                        break;
                    }
                }
                if (cookingExists)
                    f->status = "Cho";
                else
                    f->status = "Nau";

                printBill(*f);
                cout << "[Temp bill] -> " << f->status << "\n";
                cout << "\nPress Enter to continue...";
                string _tmp;
                getline(cin, _tmp);
            }
        }
        else if (act == 4)
        {
            clearScreen();
            displayTables();
            continue;
        }
        else if (act == 0)
            break;
        else
            cout << "Invalid.\n";
    }
}

// ===== Edit Order =====
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

        f->totalRemainingTime = 0;
        for (int j = 0; j < f->itemCount; j++)
            f->totalRemainingTime += f->items[j].remainingTime;
    }
    cout << "Edit done.\n";
}

// ===== Delete Order =====
void deleteOrder(Queue &q)
{
    int id;
    cout << "Enter ID to delete: ";
    cin >> id;
    Order *f = findOrderByID(q, id);
    if (!f)
    {
        cout << "Not found.\n";
        return;
    }

    if (f->status == "Nau" || f->status == "Ready")
    {
        cout << "Cannot delete order while it is being cooked or already done.\n";
        return;
    }

    Order rem;
    if (removeOrderByID(q, id, rem))
    {
        cout << "Deleted.\n";
        int t = rem.tableNumber;
        if (t >= 1 && t <= NUM_TABLES && !anyOrderForTable(q, t))
            gTableStatus[t - 1] = "Empty";
    }
    else cout << "Not found.\n";
}

// ===== Search by Table =====
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
