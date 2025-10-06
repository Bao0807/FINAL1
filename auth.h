#include <iostream>
#include <optional>
using namespace std;

struct Item {
    int hash;
    string value;

    Item() {}

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
    int n = 7;
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

    void print() {
        for (int i = 0; i < k; i++) {
            if (table[i].getSize() > 0) {
                cout << "Bucket " << i << ": ";
                for (int j = table[i].getSize() - 1; j >= 0; j--) {
                    Item v = table[i].getItem(j);
                    cout << "[hash=" << v.hash << " -> " << v.value;
                }
                cout << endl;
            }
        }
    }

};

void login(){
    HashTable registeredUsers;
    registeredUsers.add("admin", "admin"); //user mac dinh
    registeredUsers.add("nhanvien1", "nhanvien");
    registeredUsers.add("0123456789", "admin");
    registeredUsers.add("admin@gmail.com", "admin");
    
    string login, pwd;
    optional<string> auth;

    //registeredUsers.print();
    //return;

    system("cls");
    cout << "\t\t\x1b[34mHE THONG QUAN LY NHA HANG\t\t\x1b[0m\n\n";
    cout << "Ten tai khoan: \n";
    cout << "Mat khau: \n";

    while (true){
        cout << "\x1b[2F\x1b[15C";
        cin >> login;
        cout << "\x1b[10C";
        cin >> pwd;
        cout << "\n";

        auth = registeredUsers.getValue(login);

        if (auth && auth.value() == pwd){
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