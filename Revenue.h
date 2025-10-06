#pragma once
#include <iostream>
#include <fstream>
#include"Functions.h"
using namespace std;

void updateRevenue(const Order &o);
void viewRevenueSummary();

void updateRevenue(const Order &o)
{
    ofstream fout("revenue.txt", ios::app);
    fout << "OrderID: " << o.id << " | Total: " << o.total << " | Items: ";
    for (int i = 0; i < o.itemCount; i++)
    {
        if (i)
            fout << ", ";
        fout << o.items[i].foodName << "x" << o.items[i].quantity;
    }
    fout << "\n";
    fout.close();
}

void viewRevenueSummary()
{
    ifstream fin("revenue.txt");
    long long grand = 0;
    struct C
    {
        string name;
        int qty;
    } cnt[128];
    int cc = 0;
    for (int i = 0; i < 128; i++)
    {
        cnt[i].qty = 0;
    }
    if (fin)
    {
        string line;
        while (getline(fin, line))
        {
            size_t p = line.find("Total:");
            if (p != string::npos)
            {
                long long t = atoll(line.c_str() + p + 6);
                grand += t;
            }
            p = line.find("Items:");
            if (p != string::npos)
            {
                string s = line.substr(p + 6);
                string token;
                for (size_t i = 0; i <= s.size(); ++i)
                {
                    char ch = (i < s.size() ? s[i] : ',');
                    if (ch == ',')
                    {
                        size_t start = 0;
                        while (start < token.size() && token[start] == ' ')
                            start++;
                        if (start < token.size())
                        {
                            size_t x = token.rfind('x');
                            if (x != string::npos)
                            {
                                string name = token.substr(start, x - start);
                                int q = atoi(token.c_str() + x + 1);
                                bool f = false;
                                for (int k = 0; k < cc; k++)
                                    if (cnt[k].name == name)
                                    {
                                        cnt[k].qty += q;
                                        f = true;
                                        break;
                                    }
                                if (!f && cc < 128)
                                {
                                    cnt[cc].name = name;
                                    cnt[cc].qty = q;
                                    cc++;
                                }
                            }
                        }
                        token.clear();
                    }
                    else
                        token.push_back(ch);
                }
            }
        }
        fin.close();
    }
    cout << "\n=== REVENUE SUMMARY ===\n";
    cout << "Total revenue: " << formatPrice(grand) << " VND\n";
    cout << "Items sold:\n";
    for (int i = 0; i < cc; i++)
        cout << " - " << cnt[i].name << ": " << cnt[i].qty << "\n";
    cout << "=======================\n";
    cin.ignore();
    enter();
}