#pragma once
#include <iostream>
#include <string>
using namespace std;

struct MenuItem {
    string foodName;
    long long price; // VND
    bool available;
    int prepTime; // second for one portion
};

string formatPrice(long long price);
void displayMenu();
MenuItem getMenuItem(string input);
bool updateAvailability(const string &food, bool status);
void sortMenuAscending();
void sortMenuDescending();
void updateFoodStatus();
bool containsNaive(const string &text, const string &pattern);
void searchFoodNaive();




void enter(){
    cout << "\nPress Enter to continue...";
    string _tmp;
    getline(cin, _tmp);
}

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
    {"Coca cola", 20000, true, 3}
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
    cout << "\x1b[35mNo\tFood Name\t\tPrice\t\tPrep(second)\tStatus\x1b[0m\n";
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

void sortMenuAscending() { // Sắp xếp tăng dần theo giá
    for (int i = 1; i < menuSize; i++) {
        MenuItem key = menuList[i];
        int j = i - 1;
        while (j >= 0 && menuList[j].price > key.price) {
            menuList[j + 1] = menuList[j];
            j--;
        }
        menuList[j + 1] = key;
    }

    cout << "\x1b[36m\nMenu sorted in ascending order by price\x1b[0m\n";
    displayMenu();
    enter();
}

void sortMenuDescending() { // Sắp xếp giảm dần theo giá
    for (int i = 1; i < menuSize; i++) {
        MenuItem key = menuList[i];
        int j = i - 1;
        while (j >= 0 && menuList[j].price < key.price) {
            menuList[j + 1] = menuList[j];
            j--;
        }
        menuList[j + 1] = key;
    }

    cout << "\x1b[36m\nMenu sorted in descending order by price\x1b[0m\n";
    displayMenu();
    enter();
}

void updateFoodStatus() {
    cout << "\x1b[36m\n========= UPDATE FOOD STATUS =========\x1b[0m\n";
    displayMenu();

    cout << "\x1b[33mEnter the food name or number to update status:\x1b[0m ";
    string input;
    getline(cin, input);

    // Xác định món ăn (bằng số hoặc tên)
    MenuItem found = getMenuItem(input);
    if (found.foodName == "") {
        cout << "\x1b[31mFood not found or currently unavailable!\x1b[0m\n";
        return;
    }

    // Tìm vị trí món ăn trong mảng menuList
    int index = -1;
    for (int i = 0; i < menuSize; i++) {
        if (menuList[i].foodName == found.foodName) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        cout << "\x1b[31mError: could not locate food in menuList!\x1b[0m\n";
        return;
    }

    // Hiện trạng thái hiện tại
    cout << "\nCurrent status of \x1b[33m" << menuList[index].foodName << "\x1b[0m: ";
    if (menuList[index].available)
        cout << "\x1b[32mAvailable\x1b[0m\n";
    else
        cout << "\x1b[31mSold out\x1b[0m\n";

    // Chọn trạng thái mới
    cout << "Enter new status (1 = Available, 0 = Sold out): ";
    int newStatus;
    cin >> newStatus;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (newStatus != 0 && newStatus != 1) {
        cout << "\x1b[31mInvalid input. Must be 1 or 0.\x1b[0m\n";
        return;
    }

    menuList[index].available = (newStatus == 1);
    cout << "\x1b[32mStatus updated successfully!\x1b[0m\n";

    displayMenu();
    enter();
}

// tìm kiếm string search
bool containsNaive(const string &text, const string &pattern) {
    if (pattern.empty()) return false;
    int n = text.length();
    int m = pattern.length();

    // Duyệt qua từng vị trí của text
    for (int i = 0; i <= n - m; i++) {
        int j = 0;
        while (j < m && tolower(text[i + j]) == tolower(pattern[j])) {
            j++;
        }
        if (j == m) return true; // tìm thấy chuỗi con
    }
    return false;
}

// ======================= SEARCH FOOD BY NAME ===========================
void searchFoodNaive() {
    cout << "\x1b[36m\n================ SEARCH FOOD ================\x1b[0m\n";
    cout << "Enter keyword to search: ";
    string keyword;
    getline(cin, keyword);

    bool found = false;
    cout << "\x1b[35m\nResults for keyword: " << keyword << "\x1b[0m\n";
    cout << "\x1b[35mNo\tFood Name\t\tPrice\t\tPrep(second)\tStatus\x1b[0m\n";

    for (int i = 0; i < menuSize; i++) {
        if (containsNaive(menuList[i].foodName, keyword)) {
            found = true;
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
    }

    if (!found) {
        cout << "\x1b[31mNo food found with keyword: \x1b[33m" << keyword << "\x1b[0m\n";
    }

    cout << "\x1b[36m=============================================================\x1b[0m\n";
    enter();
}

