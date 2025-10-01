
#pragma once
#include <string>
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
    string status; // "Pending" | "Cooking" | "Ready" | "Completed" | "Saved"
    int tableNumber = 0;
    string tableStatus = "Empty";
    bool isOnHold = false;
    bool progressLogged = false;
};
