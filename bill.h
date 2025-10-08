#pragma once
#include<iostream>
#include<fstream>
#include"Functions.h"
#include"Revenue.h"
using namespace std;

void printBill(const Order &o);
void saveBillToFile(const Order &o);
void payment(Queue &q);

bool removeOrderByID(Queue &q, int id, Order &out)
{
    if (isEmpty(q))
        return false;
    int foundIndex = -1;
    // tìm vị trí (offset i trong queue)
    for (int i = 0; i <= q.right; i++)
    {
        if (q.orders[i].id == id)
        {
            foundIndex = i;
            break;
        }
    }
    if (foundIndex == -1)
        return false;

    // sao chép order ra ngoài
    out = q.orders[foundIndex];

    // dịch trái các phần tử sau foundIndex
    for (int i = foundIndex; i < q.right; i++)
    {
        q.orders[i] = q.orders[i + 1];
    }
    // cập nhật right
    q.right--;

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
    // Header (ANSI colors)
    cout << "\x1b[36m\n=================================================================\n"
         << "                          RESTAURANT BILL           \n"
         << "=================================================================\n\x1b[0m";
    cout << "OrderID:\x1b[33m" << o.id << "\x1b[0m\n";
    cout << "Customer:\x1b[32m" << o.customerName << "\x1b[0m\n";
    cout << "Table:\x1b[33m" << o.tableNumber << "\x1b[0m\n";
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

    // Progress section: chỉ 1 dòng tổng thời gian còn lại
    cout << "\n\x1b[33mPROGRESS: \x1b[0m";
    if (o.itemCount == 0)
    {
        cout << "(No items)\n";
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

// Ghi bill ra file, KHÔNG còn merge bàn nữa
void saveBillToFile(const Order &o)
{
    ofstream fout("bill.txt", ios::app);
    if (!fout)
    {
        cerr << "Cannot open bill.txt\n";
        return;
    }

    fout << "=====================================\n";
    fout << "           RESTAURANT BILL           \n";
    fout << "=====================================\n";
    fout << "OrderID:   " << o.id << "\n";
    fout << "Customer:  " << o.customerName << "\n";
    fout << "Table:     " << o.tableNumber << "\n";
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


void payment(Queue &q)
{
    clearScreen();
    cout << "\x1b[36m===== THANH TOAN HOA DON =====\x1b[0m\n";
    displayTables();

    int tableNo;
    cout << "Nhap so ban can thanh toan: ";
    cin >> tableNo;

    if (tableNo < 1 || tableNo > NUM_TABLES)
    {
        cout << "Ban khong hop le!\n";
        return;
    }

    // Tim don hang theo ban
    Order *target = nullptr;
    for (int i = 0; i <= q.right; i++)
    {
        Order &o = q.orders[i];
        if (o.tableNumber == tableNo)
        {
            target = &o;
            break;
        }
    }

    if (target == nullptr)
    {
        cout << "Khong co don hang nao cho ban nay.\n";
        cin.ignore();
        cout << "\nNhan Enter de tiep tuc...";
        string tmp;
        getline(cin, tmp);
        return;
    }

    cout << "\nBan " << tableNo << " co don hang:\n";
    cout << " - ID: " << target->id
         << " | Ten khach: " << target->customerName
         << " | Tong tien: " << formatPrice(target->total)
         << " | Trang thai: " << target->status << "\n";

    // Xac nhan
    cout << "\nXac nhan thanh toan? (y/n): ";
    char confirm;
    cin >> confirm;

    if (confirm != 'y' && confirm != 'Y')
    {
        cout << "Da huy thanh toan.\n";
        cin.ignore();
        enter();
        return;
    }

    // Cap nhat & in hoa don
    target->status = "Completed";
    printBill(*target);
    saveBillToFile(*target);
    updateRevenue(*target);
    cout << "\x1b[32mThanh toan thanh cong!\x1b[0m\n";

    // Xoa don khoi hang doi
    Order removed;
    removeOrderByID(q, target->id, removed);

    // Cap nhat trang thai ban
    if (!anyOrderForTable(q, tableNo))
        gTableStatus[tableNo - 1] = "Empty";

    cin.ignore();
    cout << "\nNhan Enter de tiep tuc...";
    string tmp;
    getline(cin, tmp);
}

