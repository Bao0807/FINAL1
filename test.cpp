#include <string>
#include <iostream>
using namespace std;

int hashFunc(string key, int k)
{
    int r = 0;
    for (int i = 0; i < key.length(); i++)
    {
        r += int(key[i]) * (i + 1);
    }
    return r % k;
}
struct CountMinSketch
{
    int table[3][10] = {};

    void add(string key)
    {
        int h1 = hashFunc(key, 10);
        int h2 = hashFunc(key, 9);
        int h3 = hashFunc(key, 8);
        table[0][h1]++;
        table[1][h2]++;
        table[2][h3]++;
    }

    int count(string key)
    {
        int r = INT_MAX;
        int h1 = hashFunc(key, 10);
        int h2 = hashFunc(key, 9);
        int h3 = hashFunc(key, 8);
        int h[] = {h1, h2, h3};
        for (int i = 0; i < 3; i++)
        {
            int v = table[i][h[i]];
            if (v < r)
            {
                r = v;
            }
        }
        return r;
    }

    void print()
    {
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 10; j++)
            {
                cout << table[i][j] << "\t";
            }
            cout << endl;
        }
    }
};

int main()
{

    string names[] = {"Tan", "Toan", "Taj"};

    for (string name : names)
    {
        cout << hashFunc(name, 10) << endl;
        cout << hashFunc(name, 9) << endl;
        cout << hashFunc(name, 8) << endl;
    }
}
