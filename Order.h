#pragma once
#include <string>
#include"Menu.h"
using namespace std;

const int MAX_ITEMS = 20;

struct OrderDetail
{
    string foodName; // món ăn
    int quantity = 0; // số lượng
    long long price = 0; //giá
    long long subtotal = 0; //thành tiền
    int prepTime = 0; //thời gian
    int remainingTime = 0; //
};

struct Order
{
    int id = 0;
    string customerName; // tên khách
    OrderDetail items[MAX_ITEMS]; //danh sách món
    int itemCount = 0; //số món
    long long total = 0; //tổng tiền
    string status; // "Pending" | "Wait" | "Cooking" | "Ready" | "Completed" | "Saved"
    int tableNumber = 0;
    string tableStatus = "Empty";
    bool isOnHold = false;
    bool progressLogged = false;
    int totalRemainingTime; //tổng thời gian còn lại
};

void showMainMenu()
{
    cout << "\x1b[36m"; // Mau cyan
    cout << "\n==============================================\n";
    cout << "       N H A   H A N G   V I E T   N A M\n";
    cout << "==============================================\x1b[0m\n";

    cout << "\x1b[33m1.\x1b[0m Them don hang\n";
    cout << "\x1b[33m2.\x1b[0m Hien thi\n";
    cout << "\x1b[33m3.\x1b[0m Tra cuu\n";
    cout << "\x1b[33m4.\x1b[0m Bao cao doanh thu & so luong mon ban ra\n";
    cout << "\x1b[33m5.\x1b[0m Thanh toan hoa don\n";
    cout << "\x1b[33m6.\x1b[0m Cap nhat thuc don\n";
    cout << "\x1b[31m0.\x1b[0m Thoat chuong trinh\n";

    cout << "\x1b[36m----------------------------------------------\x1b[0m\n";
    cout << "\x1b[32mNhap lua chon cua ban: \x1b[0m";
}

