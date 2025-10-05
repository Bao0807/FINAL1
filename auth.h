#pragma once
#include <iostream>
#include <string>
using namespace std;

struct authuser{
    string username;
    string password;
};

authuser users[] = {
    {"admin", "admin123"},
    {"user1", "password1"},
    {"0123456789", "password2"}
};
int curUsers = sizeof(users) / sizeof(users[0]);

void login(){
    string login, password;
    system("cls");
    cout << "\t\t\x1b[34mHE THONG QUAN LY NHA HANG\t\t\x1b[0m\n\n";
    cout << "Ten tai khoan: \n";
    cout << "Mat khau: \n";

    while (true){
        cout << "\x1b[2F\x1b[15C";
        cin >> login;
        cout << "\x1b[10C";
        cin >> password;
        cout << "\n";

        bool found = false;
        for (int i = 0; i < curUsers; i++){
            if (users[i].username == login && users[i].password == password){ //linear search
                found = true;
                break;
            }
        }
        if (found){
            cout << "\x1b[32mLogin thanh cong! Nhan phim bat ky de tiep tuc.\x1b[0m\n";
            cin.ignore();
            cin.get();
            break;
        }
        else{
            cout << "\x1b[31mTai khoan hoac mat khau sai, vui long thu lai.\x1b[0m\n";
            cout << "\x1b[2F\x1b[15C";
        }
    }
}