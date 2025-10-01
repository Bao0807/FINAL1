
#pragma once
#include <iostream>
#include <fstream>
#include <limits>
#include "Queue.h"
#include "Menu.h"

using namespace std;

// ======= Globals =======
const int NUM_TABLES = 20;
static string gTableStatus[NUM_TABLES];

// Declarations
string formatPrice(long long price);
int getLastOrderID();
void initTables();
void displayTables();
int chooseTable();
bool anyOrderForTable(Queue &q, int tableNo);

Order* findOrderByID(Queue &q, int id);
bool removeOrderByID(Queue &q, int id, Order &out);

void printBill(const Order &o);
void saveBillToFile(const Order &o);

const char* itemState(const OrderDetail &d);
void writeProgressTxt(const Order &o);
void snapshotReadyOrdersToBill(Queue &q);
void updateRevenue(const Order& o);
void viewRevenueSummary();

void addOrder(Queue &q, int &idCounter);
void displayAll(Queue &q);
void editOrder(Queue &q);
void deleteOrder(Queue &q);
void advanceTime(Queue &q, int minutes);
void searchByTable(Queue &q, int tableNo);
void payment(Queue &q);
void manageData(Queue &q, int &idCounter);

// ======= Definitions =======
string formatPrice(long long price) {
    char tmp[32]; snprintf(tmp, sizeof(tmp), "%lld", (long long)price);
    string s = tmp, out; out.reserve(s.size()+s.size()/3+2);
    int c=0;
    for (int i=(int)s.size()-1;i>=0;--i) {
        out.insert(out.begin(), s[i]);
        c++;
        if (c==3 && i>0) { out.insert(out.begin(), '.'); c=0; }
    }
    return out;
}

int getLastOrderID() {
    ifstream fin("bill.txt"); if (!fin) return 0;
    string line; int last=0;
    while (getline(fin, line)) {
        if (line.rfind("OrderID:", 0)==0) {
            int id = atoi(line.c_str()+8);
            if (id > last) last = id;
        }
    }
    fin.close(); return last;
}

void initTables() { for (int i=0;i<NUM_TABLES;i++) gTableStatus[i] = "Empty"; }

void displayTables() {
    cout << "\n--- Table Status ---\n";
    for (int i=0;i<NUM_TABLES;i++) cout << "Table " << (i+1) << ": " << gTableStatus[i] << "\n";
    cout << "---------------------\n";
}

int chooseTable() {
    displayTables();
    int t=0;
    while (true) {
        cout << "Choose table (1-" << NUM_TABLES << "): ";
        if (!(cin >> t)) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); cout << "Invalid input.\n"; continue; }
        if (t<1 || t>NUM_TABLES) { cout << "Out of range.\n"; continue; }
        if (gTableStatus[t-1]=="Full") { cout << "Table is Full.\n"; continue; }
        break;
    }
    return t;
}

bool anyOrderForTable(Queue &q, int tableNo) {
    for (int i=0;i<q.count;i++){ int idx=(q.front+i)%MAX; if (q.orders[idx].tableNumber==tableNo) return true; }
    return false;
}

Order* findOrderByID(Queue &q, int id) {
    for (int i=0;i<q.count;i++){ int idx=(q.front+i)%MAX; if (q.orders[idx].id==id) return &q.orders[idx]; }
    return 0;
}

bool removeOrderByID(Queue &q, int id, Order &out) {
    if (q.count==0) return false;
    int rel=-1;
    for (int i=0;i<q.count;i++){ int idx=(q.front+i)%MAX; if (q.orders[idx].id==id){ rel=i; break; } }
    if (rel==-1) return false;
    int absIdx=(q.front+rel)%MAX;
    out = q.orders[absIdx];
    for (int k=rel;k<q.count-1;k++){ int from=(q.front+k+1)%MAX; int to=(q.front+k)%MAX; q.orders[to]=q.orders[from]; }
    q.count--;
    if (q.count==0){ q.front=0; q.rear=-1; } else { q.rear=(q.front+q.count-1)%MAX; }
    return true;
}

void printBill(const Order &o) {
    cout << "\x1b[36m\n=================================================================\n"
         << "                          RESTAURANT BILL           \n"
         << "=================================================================\n\x1b[0m";
    cout << "OrderID:\x1b[33m" << o.id << "\x1b[0m\n";
    cout << "Customer:\x1b[32m" << o.customerName << "\x1b[0m\n";
    cout << "Table:\x1b[33m" << o.tableNumber << "\x1b[0m\n";
    cout << "-----------------------------------------------------------------\n";
    cout << "\x1b[35mNo\tFood Name\t\tQuantity\tPrice\x1b[0m\n";
    for (int i=0;i<o.itemCount;i++) {
        cout << i+1 << ".\t" << o.items[i].foodName << "\t\t"
             << o.items[i].quantity << "\t\t" << formatPrice(o.items[i].subtotal) << "\n";
    }
    cout << "-----------------------------------------------------------------\n";
    cout << "\x1b[31mTOTAL: " << formatPrice(o.total) << " VND\x1b[0m\n";
    cout << "Status: " << o.status << "\n";
    cout << "\x1b[36m=================================================================\x1b[0m\n";
}

void saveBillToFile(const Order &o) {
    ofstream fout("bill.txt", ios::app);
    fout << "=====================================\n";
    fout << "           RESTAURANT BILL           \n";
    fout << "=====================================\n";
    fout << "OrderID:   " << o.id << "\n";
    fout << "Customer:  " << o.customerName << "\n";
    fout << "Table:     " << o.tableNumber << "\n";
    fout << "-------------------------------------\n";
    fout << "Food        Qty     Price\n";
    fout << "-------------------------------------\n";
    for (int i=0;i<o.itemCount;i++) {
        fout << o.items[i].foodName << "\t" << o.items[i].quantity << "\t" << o.items[i].subtotal << "\n";
    }
    fout << "-------------------------------------\n";
    fout << "TOTAL: " << o.total << " VND\n";
    fout << "Status: " << o.status << "\n";
    fout << "=====================================\n\n";
    fout.close();
}

const char* itemState(const OrderDetail &d) {
    int full = d.prepTime * d.quantity;
    if (d.remainingTime <= 0) return "done";
    if (d.remainingTime >= full) return "wait";
    return "cook";
}

void writeProgressTxt(const Order &o) {
    ofstream fout("progress.txt", ios::app);
    fout << "OrderID: " << o.id << " | Customer: " << o.customerName
         << " | Table: " << o.tableNumber << " | Status: " << o.status << "\n";
    for (int i=0;i<o.itemCount;i++) {
        const OrderDetail &d = o.items[i];
        fout << " - " << d.foodName << " x" << d.quantity
             << " [" << itemState(d) << "]"
             << " remaining=" << d.remainingTime << "m\n";
    }
    fout << "-------------------------------------\n";
    fout.close();
}

void snapshotReadyOrdersToBill(Queue &q) {
    for (int i=0;i<q.count;i++){
        int idx=(q.front+i)%MAX;
        Order &o=q.orders[idx];
        if (o.status=="Ready" && !o.progressLogged) {
            Order snap=o; snap.status="Pending";
            saveBillToFile(snap);
            o.progressLogged=true;
        }
    }
}

void updateRevenue(const Order& o) {
    ofstream fout("revenue.txt", ios::app);
    fout << "OrderID: " << o.id << " | Total: " << o.total << " | Items: ";
    for (int i=0;i<o.itemCount;i++){ if (i) fout << ", "; fout << o.items[i].foodName << "x" << o.items[i].quantity; }
    fout << "\n"; fout.close();
}

void viewRevenueSummary() {
    ifstream fin("revenue.txt"); long long grand=0;
    struct C { string name; int qty; } cnt[128]; int cc=0; for (int i=0;i<128;i++){ cnt[i].qty=0; }
    if (fin) {
        string line;
        while (getline(fin, line)) {
            size_t p=line.find("Total:"); if (p!=string::npos){ long long t=atoll(line.c_str()+p+6); grand+=t; }
            p=line.find("Items:"); if (p!=string::npos){
                string s=line.substr(p+6);
                string token;
                for (size_t i=0;i<=s.size();++i){
                    char ch=(i<s.size()? s[i]:',');
                    if (ch==','){
                        size_t start=0; while (start<token.size() && token[start]==' ') start++;
                        if (start<token.size()){
                            size_t x=token.rfind('x');
                            if (x!=string::npos){
                                string name=token.substr(start, x-start);
                                int q=atoi(token.c_str()+x+1);
                                bool f=false; for (int k=0;k<cc;k++) if (cnt[k].name==name){ cnt[k].qty+=q; f=true; break; }
                                if (!f && cc<128){ cnt[cc].name=name; cnt[cc].qty=q; cc++; }
                            }
                        }
                        token.clear();
                    } else token.push_back(ch);
                }
            }
        }
        fin.close();
    }
    cout << "\n=== REVENUE SUMMARY ===\n";
    cout << "Total revenue: " << formatPrice(grand) << " VND\n";
    cout << "Items sold:\n";
    for (int i=0;i<cc;i++) cout << " - " << cnt[i].name << ": " << cnt[i].qty << "\n";
    cout << "=======================\n";
}

void addOrder(Queue &q, int &idCounter) {
    Order o; o.id = ++idCounter;
    cout << "Customer name: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, o.customerName);

    int tableNum = chooseTable();
    o.tableNumber = tableNum;
    o.tableStatus = "Full";
    gTableStatus[o.tableNumber - 1] = "Full";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    char more='n';
    do {
        if (o.itemCount >= MAX_ITEMS) { cout << "Reached max items!\n"; break; }
        displayMenu();
        cout << "Choose food (number or exact name): ";
        string inputFood; getline(cin, inputFood);
        MenuItem sel = getMenuItem(inputFood);
        if (!sel.available || sel.price==0) { cout << "Invalid choice or sold out.\n"; continue; }

        OrderDetail d; d.foodName = sel.foodName;
        cout << "Quantity: ";
        if (!(cin >> d.quantity) || d.quantity<=0) { cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); cout << "Invalid quantity.\n"; continue; }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        d.price = sel.price; d.subtotal = d.price * d.quantity;
        d.prepTime = sel.prepTime; d.remainingTime = sel.prepTime * d.quantity;

        o.items[o.itemCount++] = d; o.total += d.subtotal;

        cout << "Add more food? (y/n): "; cin >> more;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    } while (more=='y' || more=='Y');

    o.status = "Pending";
    if (!enqueue(q, o)) { cout << "Queue full! Reverting table.\n"; gTableStatus[o.tableNumber-1]="Empty"; }
    else { cout << "Added order ID " << o.id << "\n"; }

    cout << "Actions after Add: 1-Delete  2-Edit  3-Temp bill (COOK)  4-Save (hold)  0-Back\nChoose: ";
    int act; cin >> act; cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (act==1) { deleteOrder(q); }
    else if (act==2) { editOrder(q); }
    else if (act==3) {
        cout << "Enter ID to temp-bill: "; int id; cin >> id; cin.ignore(numeric_limits<streamsize>::max(), '\n');
        Order *f=findOrderByID(q,id); if (!f){ cout<<"Not found.\n"; } else { f->status="Cooking"; writeProgressTxt(*f); printBill(*f); cout<<"[Temp bill] COOK & progress logged.\n"; }
    }
    else if (act==4) {
        cout << "Enter ID to Save (hold): "; int id; cin >> id; cin.ignore(numeric_limits<streamsize>::max(), '\n');
        Order *f=findOrderByID(q,id); if (!f){ cout<<"Not found.\n"; } else { f->isOnHold=true; f->status="Saved"; cout<<"Saved (on hold).\n"; }
    }
}

void displayAll(Queue &q) {
    cout << "\n===== TABLE STATUS =====\n"; displayTables();
    cout << "\n===== MENU =====\n"; displayMenu();
    cout << "\n===== ORDERS =====\n";
    if (q.count==0) cout << "No orders.\n";
    else {
        for (int i=0;i<q.count;i++){
            int idx=(q.front+i)%MAX; Order &o=q.orders[idx];
            cout << "OrderID: " << o.id << " | Name: " << o.customerName
                 << " | Table: " << o.tableNumber << " | Status: " << o.status
                 << " | Total: " << formatPrice(o.total) << "\n";
            for (int j=0;j<o.itemCount;j++){
                cout << "  - " << o.items[j].foodName << " x" << o.items[j].quantity
                     << " | remain " << o.items[j].remainingTime << "m"
                     << " [" << itemState(o.items[j]) << "]\n";
            }
        }
    }
    cout << "Actions in Display: 1-Delete  2-Edit  3-Temp bill (COOK)  0-Back\nChoose: ";
    int act; cin >> act; cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (act==1) deleteOrder(q);
    else if (act==2) editOrder(q);
    else if (act==3) {
        cout << "Enter ID to temp-bill: "; int id; cin >> id; cin.ignore(numeric_limits<streamsize>::max(), '\n');
        Order *f=findOrderByID(q,id); if (!f){ cout<<"Not found.\n"; } else { f->status="Cooking"; writeProgressTxt(*f); printBill(*f); cout<<"[Temp bill] COOK & progress logged.\n"; }
    }
}

void editOrder(Queue &q) {
    int id; cout << "Enter ID to edit: "; cin >> id;
    Order *f = findOrderByID(q, id);
    if (!f){ cout << "Not found.\n"; return; }
    cout << "Current customer: " << f->customerName << "\n";
    cout << "New name (or '.' keep): ";
    string name; cin.ignore(numeric_limits<streamsize>::max(), '\n'); getline(cin, name);
    if (name != ".") f->customerName = name;

    cout << "Change table? (y/n): "; char c; cin >> c;
    if (c=='y'||c=='Y'){ int old=f->tableNumber; int nt=chooseTable(); f->tableNumber=nt; gTableStatus[nt-1]="Full";
        bool hasOther=false; for (int i=0;i<q.count;i++){ int idx=(q.front+i)%MAX; if (q.orders[idx].id!=f->id && q.orders[idx].tableNumber==old){ hasOther=true; break; } }
        if (!hasOther && old>=1 && old<=NUM_TABLES) gTableStatus[old-1]="Empty";
    }

    cout << "Add more items? (y/n): "; char c2; cin >> c2; cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (c2=='y'||c2=='Y'){
        char more='n';
        do {
            if (f->itemCount >= MAX_ITEMS) { cout<<"Reached max items!\n"; break; }
            displayMenu();
            cout << "Choose food (number or name): ";
            string in; getline(cin, in);
            MenuItem sel = getMenuItem(in);
            if (!sel.available || sel.price==0){ cout<<"Invalid.\n"; continue; }
            OrderDetail d; d.foodName=sel.foodName;
            cout << "Quantity: ";
            if (!(cin>>d.quantity)||d.quantity<=0){ cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); cout<<"Invalid quantity.\n"; continue; }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            d.price=sel.price; d.subtotal=d.price*d.quantity; d.prepTime=sel.prepTime; d.remainingTime=sel.prepTime*d.quantity;
            f->items[f->itemCount++]=d; f->total+=d.subtotal;
            cout << "Add more? (y/n): "; cin >> more; cin.ignore(numeric_limits<streamsize>::max(), '\n');
        } while (more=='y'||more=='Y');
    }
    cout << "Edit done.\n";
}

void deleteOrder(Queue &q) {
    int id; cout << "Enter ID to delete: "; cin >> id;
    Order rem; if (removeOrderByID(q,id,rem)){ cout<<"Deleted.\n"; int t=rem.tableNumber; if (t>=1 && t<=NUM_TABLES && !anyOrderForTable(q,t)) gTableStatus[t-1]="Empty"; }
    else cout<<"Not found.\n";
}

void advanceTime(Queue &q, int minutes) {
    if (minutes<=0) return;
    for (int i=0;i<q.count;i++){
        int idx=(q.front+i)%MAX; Order &o=q.orders[idx];
        bool allZero=true;
        for (int j=0;j<o.itemCount;j++){
            o.items[j].remainingTime -= minutes;
            if (o.items[j].remainingTime < 0) o.items[j].remainingTime = 0;
            if (o.items[j].remainingTime > 0) allZero=false;
        }
        if (o.isOnHold){ /* stay Saved */ }
        else {
            if (allZero && o.itemCount>0) o.status="Ready";
            else if (o.status!="Pending") o.status="Cooking";
        }
    }
}

void searchByTable(Queue &q, int tableNo) {
    cout << "\n-- Orders at Table " << tableNo << " --\n";
    bool any=false;
    for (int i=0;i<q.count;i++){ int idx=(q.front+i)%MAX; const Order &o=q.orders[idx]; if (o.tableNumber==tableNo){ any=true; printBill(o);} }
    if (!any) cout << "No orders.\n";
}

void payment(Queue &q) {
    int tableNo; cout << "Enter table number: "; cin >> tableNo;
    if (tableNo<1 || tableNo>NUM_TABLES){ cout<<"Invalid table.\n"; return; }
    int ids[32]; int n=0;
    cout << "Orders:\n";
    for (int i=0;i<q.count;i++){ int idx=(q.front+i)%MAX; Order &o=q.orders[idx]; if (o.tableNumber==tableNo){ cout<<" - ID "<<o.id<<" ["<<o.status<<"] Total="<<formatPrice(o.total)<<"\n"; if (n<32) ids[n++]=o.id; } }
    if (n==0){ cout<<"No orders.\n"; return; }
    int id; cout << "Choose Order ID to pay: "; cin >> id;
    bool ok=false; for (int i=0;i<n;i++) if (ids[i]==id){ ok=true; break; }
    if (!ok){ cout<<"ID not in this table.\n"; return; }
    cout << "Payment method (1-Cash, 2-QR, 3-Card): "; int pm; cin >> pm; (void)pm;

    Order rem; if (!removeOrderByID(q,id,rem)){ cout<<"Not found.\n"; return; }
    rem.status="Completed"; printBill(rem); saveBillToFile(rem); updateRevenue(rem);
    cout << "Payment done & saved.\n";
    int t=rem.tableNumber; if (t>=1 && t<=NUM_TABLES && !anyOrderForTable(q,t)) gTableStatus[t-1]="Empty";
}

void manageData(Queue &q, int &idCounter) {
    cout << "\n===== MANAGE DATA =====\n";
    cout << "1. Add Order\n";
    cout << "2. Display All\n";
    cout << "0. Back\n";
    cout << "Choose: ";
    int c; cin >> c;
    switch (c) {
        case 1: addOrder(q, idCounter); break;
        case 2: displayAll(q); break;
        default: break;
    }
}
