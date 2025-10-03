#pragma once
#include <iostream>
#include <string>
using namespace std;

// ============ MENU (menu.h chỉ chứa menu) ============
// Các function có trong hàm:
// void displayMenu();
// MenuItem getMenuItem(const string &input);
// bool updateAvailability(const string &food, bool status);
// string formatPrice(long long price)


struct MenuItem {
    string foodName;
    long long price; // VND
    bool available;
    int prepTime; // second for one portion
};

MenuItem menuList[] = {
    {"Pho Ha Noi", 120000, true, 12},
    {"Com Tam Sai Gon", 130000, true, 10},
    {"Pho Kho Gia Lai", 120000, true, 12},
    {"Bun Dau Mam Tom", 140000, true, 15},
    {"Banh Xeo", 120000, true, 14},
    {"Bun Bo Hue", 130000, true, 13},
    {"Mi Quang Ga", 120000, true, 11},
    {"Bun Cha Ha Noi", 150000, true, 16},
    {"Lau Bo Nam", 450000, true, 25},
    {"Lau De Lang Son", 480000, true, 30},
    {"Coca cola", 20000, true, 30}
};
int menuSize = sizeof(menuList) / sizeof(menuList[0]);


string formatPrice(long long price){ // edit
    string Price = to_string(price);
    for (int i = Price.length() - 3; i >= 1; i -= 3)
        Price.insert(i, ".");
    return Price;
}

void displayMenu(){
    cout << "\x1b[36m\n================================== MENU ==================================\x1b[0m\n";
    cout << "\x1b[35mNo\tFood Name\t\tPrice\t\tPrep(min)\tStatus\x1b[0m\n";
    for (int i = 0; i < menuSize; i++){
        cout << "\x1b[33m" << i + 1 << ".\x1b[0m\t";
        cout << menuList[i].foodName << "\t\t";
        cout << "\x1b[34m" << formatPrice(menuList[i].price) << "\x1b[0m\t\t";
        cout << menuList[i].prepTime << "\t\t";
        if (menuList[i].available)
            cout << "\x1b[32mAvailable\x1b[0m";
        else
            cout << "\x1b[31mSold out\x1b[0m";
        cout << "\n";
    }
    cout << "\x1b[36m==========================================================================\x1b[0m\n";
}


MenuItem getMenuItem(string input){ //edit
    bool isDigit = true;
    for (int i = 0; i < input.length(); i++){
        if (!isdigit(input[i])){
            isDigit = false;
            break;
        }
    }

    if (isDigit){
        int idx = stoi(input);
        if (idx >= 1 && idx <= menuSize && menuList[idx - 1].available){
            return menuList[idx - 1];
        }
    }

    for (int i = 0; i < menuSize; i++){
        if (menuList[i].foodName == input && menuList[i].available)
            return menuList[i];
    }

    MenuItem none;
    none.foodName = "";
    none.price = 0;
    none.available = false;
    none.prepTime = 0;
    return none;
}

bool updateAvailability(const string &food, bool status){
    for (int i = 0; i < menuSize; i++){
        if (menuList[i].foodName == food){
            menuList[i].available = status;
            return true;
        }
    }
    return false;
}

