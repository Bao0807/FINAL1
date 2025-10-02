#include <iostream>
#include "Functions.h"

using namespace std;

void waitEnter()
{
    cout << "\nPress Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

int main()
{
    Queue q;
    int choice;
    int idCounter = getLastOrderID();
    initTables();

    do
    {
        clearScreen();

        cout << "\n===== RESTAURANT =====\n";
        cout << "1. Quan ly du lieu (Add & Display)\n";
        cout << "2. Tien do don:\n";
        cout << "3. Tim kiem theo so ban\n";
        cout << "4. Doanh thu & so luong\n";
        cout << "5. Thanh toan (hoi so ban -> chon hinh thuc)\n";
        cout << "0. EXIT\n";
        cout << "Choice: ";
        cin >> choice;

        // Clear screen again so mỗi thao tác được hiển thị trên màn hình sạch
        clearScreen();

        switch (choice)
        {
        case 1:
        {
            manageData(q, idCounter);
            break;
        }
        case 2:
        {
            // Hiển thị toàn bộ đơn trong queue (tiến độ, trạng thái, items)
            displayQueue(q);
            break;
        }
        case 3:
        {
            cout << "Enter table number: ";
            int tb;
            cin >> tb;
            searchByTable(q, tb);
            break;
        }
        case 4:
        {
            viewRevenueSummary();
            break;
        }
        case 5:
        {
            payment(q);
            break;
        }
        case 0:
        {
            cout << "Exit.\n";
            break;
        }
        default:
            cout << "Invalid.\n";
        }

        if (choice != 0)
            waitEnter();

    } while (choice != 0);
    return 0;
}
