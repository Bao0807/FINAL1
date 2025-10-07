#include <iostream>
#include "Functions.h"
#include "auth.h"
using namespace std;

void showMainMenu();

int main()
{
    Queue q;
    int choice;
    int idCounter = getLastOrderID();
    initTables();

    login();

    do
    {
        clearScreen();

        showMainMenu();
        cin >> choice;

        clearScreen();

        switch (choice)
        {
        case 0:
        {
            cout << "Exit.\n";
            break;
        }
        case 1:
        {
            addOrder(q, idCounter);
            break;
        }
        case 2:
        {
            displayTables();
            displayQueue(q);
            break;
        }
        case 3:
        {
            cout << "1.Search Food    2.Search Customer    0.Back\n";
            cout << "Choice: ";
            int n;
            cin >> n;
            cin.ignore();
            if (n == 1){
                searchFoodNaive();
            }
            else if (n == 2)
            {
                searchCustomer(q);
                enter();
            }
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
        case 6:
        {
            while (true)
            {
                clearScreen();
                cout << "1.Sort menu ascending    2.Sort menu descending    3.Update status   0.Back\n";
                cout << "Choice: ";
                int n;
                cin >> n;
                cin.ignore();
                if (n == 1)
                {
                    sortMenuAscending();
                    continue;
                }
                else if (n == 2)
                {
                    sortMenuDescending();
                    continue;
                }
                else if (n == 3)
                {
                    updateFoodStatus();
                    continue;
                }
                else
                    break;
            }
            break;
        }

        default:
            cout << "Invalid.\n";
            cin.ignore();
            cin.get();
        }

    } while (choice != 0);
    return 0;
}