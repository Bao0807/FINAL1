
#include <iostream>
#include "Functions.h"

int main() {
    Queue q;
    int choice;
    int idCounter = getLastOrderID();
    initTables();

    do {
        std::cout << "\n===== RESTAURANT =====\n";
        std::cout << "1. Quan ly du lieu (Add & Display)\n";
        std::cout << "2. Tien do don: advance minutes + progress.txt + snapshot bill Pending\n";
        std::cout << "3. Tim kiem theo so ban\n";
        std::cout << "4. Doanh thu & so luong\n";
        std::cout << "5. Thanh toan (hoi so ban -> chon hinh thuc)\n";
        std::cout << "0. EXIT\n";
        std::cout << "Choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1: {
                manageData(q, idCounter);
                break;
            }
            case 2: {
                std::cout << "Enter minutes to advance: ";
                int m; std::cin >> m;
                advanceTime(q, m);
                for (int i=0;i<q.count;i++){ int idx=(q.front+i)%MAX; writeProgressTxt(q.orders[idx]); }
                snapshotReadyOrdersToBill(q);
                std::cout << "Advanced " << m << " minutes.\n";
                displayAll(q);
                break;
            }
            case 3: {
                std::cout << "Enter table number: ";
                int tb; std::cin >> tb;
                searchByTable(q, tb);
                break;
            }
            case 4: {
                viewRevenueSummary();
                break;
            }
            case 5: {
                payment(q);
                break;
            }
            case 0: {
                std::cout << "Exit.\n";
                break;
            }
            default:
                std::cout << "Invalid.\n";
        }
    } while (choice != 0);

    return 0;
}
