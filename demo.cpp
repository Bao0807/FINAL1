#include <iostream>
#include "Functions.h"



static void waitEnter()
{
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
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

        std::cout << "\n===== RESTAURANT =====\n";
        std::cout << "1. Quan ly du lieu (Add & Display)\n";
        std::cout << "2. Tien do don:\n";
        std::cout << "3. Tim kiem theo so ban\n";
        std::cout << "4. Doanh thu & so luong\n";
        std::cout << "5. Thanh toan (hoi so ban -> chon hinh thuc)\n";
        std::cout << "0. EXIT\n";
        std::cout << "Choice: ";
        std::cin >> choice;

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
            std::cout << "Enter minutes to advance: ";
            int m;
            std::cin >> m;
            advanceTime(q, m);
            for (int i = 0; i < q.count; i++)
            {
                int idx = (q.front + i) % MAX;
                writeProgressTxt(q.orders[idx]);
            }
            snapshotReadyOrdersToBill(q);
            std::cout << "Advanced " << m << " minutes.\n";
            displayAll(q);
            break;
        }
        case 3:
        {
            std::cout << "Enter table number: ";
            int tb;
            std::cin >> tb;
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
            std::cout << "Exit.\n";
            break;
        }
        default:
            std::cout << "Invalid.\n";
        }

        if (choice != 0)
            waitEnter();

    } while (choice != 0);

    return 0;
}
