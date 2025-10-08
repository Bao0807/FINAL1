#include <iostream>
#include <optional>
using namespace std;

struct Item {
    int hash;
    string value;

	Item() : hash(0), value("") {}

    Item(int hash, string value){
        Item::hash = hash;
        Item::value = value;
    }
};

struct ItemList {
    Item values[10];
    int right = -1;
    
    Item getItem(int ind) {
        return values[ind];
    }
    
    int getSize() {
        return right + 1;
    }
    
    void add(Item v) {
        right++;
        values[right] = v;
    }
};

struct HashTable {
    int k = 7;
    ItemList table[10] = {};
    
    int hash(string key) {
        int r = 0;
        for (int i = 0; i < key.length(); i++) {
            r += int(key[i]) * (i + 1);
        }
        return r;
    }
    
    optional<string> getValue(string key) {
        int h = hash(key);
        int ind = h % k;
        
        for (int j = 0; j < k; j++){
            int quadInd = (ind + j * j) % k;
            for (int i = 0; i < table[quadInd].getSize(); i++) {
                Item v = table[quadInd].getItem(i);
                if (v.hash == h) {
                    return v.value;
                }
            }
        }
        
        return {};
    }
    
    void staffList() {
        for (int i = 0; i < k; i++) {
            if (table[i].getSize() > 0) {
                cout << "Hash " << i << ": ";
                for (int j = table[i].getSize() - 1; j >= 0; j--) {
                    Item v = table[i].getItem(j);
                    cout << "[ID: " << v.hash << " - " << v.value <<"] ";
                }
                cout << endl;
            }
        }
    }

    void add(string key, string value) {
        int h = hash(key);
        int ind = h % k;

        if (table[ind].getSize() < 10) {
            table[ind].add(Item(h, value));
            return;
        }

        for (int i = 0; i < k; i++) {
            int quadInd = (h + i * i) % k;
            if (table[quadInd].getSize() < 10) {
                table[quadInd].add(Item(h, value));
                return;
            }
        }
    }

    
    void loadData(){
        add("admin", "admin"); //user mac dinh
        add("nhanvien1", "nhanvien");
        add("0123456789", "admin2");
        add("admin@gmail.com", "admin3");
    }
};

void login(){
    bool debug = true;
    if (debug) return;
    
    HashTable registeredUsers;
    registeredUsers.loadData();
    
    string login, pwd;
    optional<string> auth;


    system("cls");
    cout << "\t\t\x1b[34mHE THONG QUAN LY NHA HANG\t\t\x1b[0m\n\n";
    cout << "Ten tai khoan: \n";
    cout << "Mat khau: \n";
    cout << "\n\n\x1b[3mMac dinh: admin - admin\nDe xem danh sach ma (ID) nhan vien: nhap admin::list o phan Ten tai khoan\n\x1b[0m\n\x1b[5F";

    while (true){
        cout << "\x1b[2F\x1b[15C";
        cin >> login;
        cout << "\x1b[10C";
        
        if (login == "admin::list"){
            system("cls");
            cout << "\t\tSTAFF ID LIST\t\t\n";
            registeredUsers.staffList();
            cout << "\nNhan phim bat ki de tiep tuc.\n";
            cin.ignore();
            cin.get();
            return;
        }

        cin >> pwd;
        cout << "\n";
        
        auth = registeredUsers.getValue(login);

        if (auth && auth.value() == pwd){
            cout << "\x1b[32mLogin thanh cong! Nhan phim bat ky de tiep tuc.\x1b[0m";
            cin.ignore();
            cin.get();
            return;
        }
        else{
            cout << "\x1b[31mTai khoan hoac mat khau sai, vui long thu lai.\x1b[0m";
            cout << "\x1b[1F\x1b[15C";
        }
    }
}