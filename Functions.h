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
Queue *g_queuePtr = nullptr;

// ===== Helpers dùng chung =====

// Có đơn đang "Nau" không?
bool hasCooking(Queue &q)
{
    for (int i = 0; i < q.count; ++i)
    {
        int idx = (q.front + i) % MAX;
        if (q.orders[idx].status == "Nau") return true;
    }
    return false;
}

// In 1 dòng tóm tắt đơn hàng (dùng lại ở display/search)
void printOrderRow(const Order &o)
{
    cout << "OrderID: " << o.id
         << " | Name: " << o.customerName
         << " | Table: " << o.tableNumber
         << " | Status: " << o.status
         << " | Total: " << formatPrice(o.total)
         << " | Remain: " << o.totalRemainingTime << "s\n";
}

// ===== Declarations =====
string formatPrice(long long price);
int getLastOrderID();

Order *findOrderByID(Queue &q, int id);
bool removeOrderByID(Queue &q, int id, Order &out);

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
    if (g_timerRunning.load()) return;
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
    if (!g_timerRunning.load()) return;
    g_timerRunning.store(false);
    if (g_timerThread.joinable()) g_timerThread.join();
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
        // Khi xong, tự động chuyển đơn "Cho" đầu tiên sang "Nau"
        for (int j = 0; j < q.count; ++j)
        {
            int idx2 = (q.front + j) % MAX;
            Order &next = q.orders[idx2];
            if (next.status == "Cho")
            {
                next.status = "Nau";
                break;
            }
        }
    }
}

// ===== getLastOrderID =====
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
            if (id > last) last = id;
        }
    }
    fin.close();
    return last;
}

// ===== findOrderByID =====
Order *findOrderByID(Queue &q, int id)
{
    for (int i = 0; i < q.count; i++)
    {
        int idx = (q.front + i) % MAX;
        if (q.orders[idx].id == id) return &q.orders[idx];
    }
    return nullptr;
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

    // Tổng thời gian của đơn
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

    cout << "\x1b[32mAdded order ID " << o.id << " for table " << o.tableNumber << "\x1b[0m\n";
    cout << "Do you want to print temporary bill (start cooking)? (y/n): ";

    char choice;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 'y' || choice == 'Y')
    {
        Order *f = &q.orders[q.rear];
        if (hasCooking(q))
            f->status = "Cho";
        else
            f->status = "Nau";

        printBill(*f);
        cout << "[Temp bill] -> " << f->status << " & progress logged.\n";
        startTimer(&q);
    }
    else
    {
        cout << "Order saved as Pending.\n";
    }

    enter();
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
                printOrderRow(q.orders[idx]); // dùng helper
            }
        }

        cout << "Tuy chinh don: 1-Xoa  2-Sua  3-Xuat hoa don tam thoi (Nau)  4.cap nhap lai  0-Quay lai\nChon: ";
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
            enter();
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
                if (hasCooking(q)) f->status = "Cho";
                else f->status = "Nau";

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
        else if (act == 0) break;
        else cout << "Invalid.\n";
    }
}

// ===== Edit Order =====
void editOrder(Queue &q)
{
    int id;
    cout << "Nhap ID can sua: ";
    cin >> id;

    Order *f = findOrderByID(q, id);
    if (!f)
    {
        cout << "Khong tim thay don hang.\n";
        return;
    }

    // --- Cho phep doi ten / doi ban bat ky luc nao ---
    cout << "Ten hien tai: " << f->customerName << "\n";
    cout << "Nhap ten moi (hoac '.' de giu nguyen): ";
    string name;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, name);
    if (name != ".") f->customerName = name;

    cout << "Doi ban? (y/n): ";
    char c; cin >> c;
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
            { hasOther = true; break; }
        }
        if (!hasOther && old >= 1 && old <= NUM_TABLES)
            gTableStatus[old - 1] = "Empty";
    }

    // --- Khong cho them mon neu dang Nau/Ready ---
    if (f->status == "Nau" || f->status == "Ready")
    {
        cout << "\nDon dang '" << f->status << "', khong the them mon moi.\n";
        cout << "Chi duoc phep doi ten/doi ban.\n";
        return;
    }

    // --- Chi khi Pending/Cho moi cho them mon ---
    cout << "Them mon moi? (y/n): ";
    char c2; cin >> c2;
    if (c2 == 'y' || c2 == 'Y')
    {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        char more = 'y';
        do {
            if (f->itemCount >= MAX_ITEMS) {
                cout << "Reached MAX_ITEMS, khong the them mon.\n";
                break;
            }

            displayMenu();
            cout << "Nhap ten mon hoac so thu tu: ";
            string inputFood;
            getline(cin, inputFood);
            MenuItem sel = getMenuItem(inputFood);

            if (!sel.available || sel.price == 0)
            {
                cout << "Lua chon khong hop le hoac mon het hang.\n";
                cout << "Them tiep? (y/n): ";
                cin >> more; cin.ignore(numeric_limits<streamsize>::max(), '\n');
                continue;
            }

            OrderDetail d;
            d.foodName = sel.foodName;
            cout << "So luong: ";
            if (!(cin >> d.quantity) || d.quantity <= 0) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "So luong khong hop le.\n";
                cout << "Them tiep? (y/n): ";
                cin >> more; cin.ignore(numeric_limits<streamsize>::max(), '\n');
                continue;
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            d.price = sel.price;
            d.subtotal = d.price * d.quantity;
            d.prepTime = sel.prepTime;
            d.remainingTime = sel.prepTime * d.quantity;

            // them vao don
            f->items[f->itemCount++] = d;
            f->total += d.subtotal;

            cout << "Them tiep? (y/n): ";
            cin >> more; cin.ignore(numeric_limits<streamsize>::max(), '\n');
        } while (more == 'y' || more == 'Y');

        // >>> CẬP NHẬT LẠI REMAINING SAU KHI THÊM MON <<<
        f->totalRemainingTime = 0;
        for (int j = 0; j < f->itemCount; ++j)
            f->totalRemainingTime += f->items[j].remainingTime;
    }

    cout << "Da cap nhat don hang.\n";
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
    if (!any) cout << "No orders.\n";
}

// ===== Search Customer (Naïve) =====
void searchCustomer(Queue &q)
{
    if (q.count == 0)
    {
        cout << "Khong co don hang nao trong he thong.\n";
        return;
    }

    string keyword;
    cout << "Nhap ten khach hang can tim: ";
    getline(cin, keyword);

    if (keyword.empty())
    {
        cout << "Tu khoa trong. Khong tim thay.\n";
        return;
    }

    bool found = false;
    cout << "\nKet qua tim kiem:\n";
    for (int i = 0; i < q.count; i++)
    {
        int idx = (q.front + i) % MAX;
        Order &o = q.orders[idx];
        if (containsNaive(o.customerName, keyword)) // hàm ở Menu.h
        {
            found = true;
            printOrderRow(o);
        }
    }
    if (!found) cout << "Khong tim thay khach hang nao phu hop.\n";
}
