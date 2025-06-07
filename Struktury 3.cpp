#include <iostream>
#include <vector>
#include <list>
#include <chrono>
#include <random>
#include <algorithm>

using namespace std;
using namespace std::chrono;

class HashTable {
public:
    virtual void insert(int key, int value) = 0;
    virtual void remove(int key) = 0;
    virtual int getSize() const = 0;  
    virtual ~HashTable() {}
};

// ----------------- Separate Chaining -----------------
class SeparateChaining : public HashTable {
    vector<list<pair<int,int>>> table;
    int size;

    int hash(int key) { return key % size; }
public:
    SeparateChaining(int s) : size(s), table(s) {}

    void insert(int key, int value) override {
        int idx = hash(key);
        for(auto &kv : table[idx]){
            if(kv.first == key){
                kv.second = value; 
                return;
            }
        }
        table[idx].push_back({key, value});
    }

    void remove(int key) override {
        int idx = hash(key);
        auto &lst = table[idx];
        for(auto it = lst.begin(); it != lst.end(); ++it){
            if(it->first == key){
                lst.erase(it);
                return;
            }
        }
    }

    int getSize() const override { return size; }
};

// ----------------- Linear Probing -----------------
class LinearProbing : public HashTable {
    struct Entry {
        int key = -1, value = -1;
        bool occupied = false;
        bool deleted = false;

        Entry() = default;
        Entry(int k, int v, bool occ, bool del) : key(k), value(v), occupied(occ), deleted(del) {}
    };
    vector<Entry> table;
    int size;

    int hash(int key) { return key % size; }

public:
    LinearProbing(int s) : size(s), table(s) {}

    void insert(int key, int value) override {
        int idx = hash(key);
        for(int i=0; i<size; ++i){
            int probe = (idx + i) % size;
            if(!table[probe].occupied || table[probe].deleted){
                table[probe] = Entry(key,value,true,false);
                return;
            }
            else if(table[probe].occupied && table[probe].key == key){
                table[probe].value = value; // nadpisz
                return;
            }
        }
        
    }

    void remove(int key) override {
        int idx = hash(key);
        for(int i=0; i<size; ++i){
            int probe = (idx + i) % size;
            if(table[probe].occupied && !table[probe].deleted && table[probe].key == key){
                table[probe].deleted = true;
                return;
            }
        }
    }

    int getSize() const override { return size; }
};

// ----------------- Cuckoo Hashing -----------------
class CuckooHashing : public HashTable {
    vector<pair<int,int>> table1, table2;
    int size;
    int maxLoop = 50; // limit na próby przenoszenia

    int hash1(int key) { return key % size; }
    int hash2(int key) { return (key / size) % size; }

public:
    CuckooHashing(int s) : size(s), table1(s, {-1,-1}), table2(s, {-1,-1}) {}

    void insert(int key, int value) override {
        int curKey = key, curVal = value;
        for(int loop=0; loop < maxLoop; ++loop){
            int pos1 = hash1(curKey);
            if(table1[pos1].first == -1 || table1[pos1].first == curKey){
                table1[pos1] = {curKey, curVal};
                return;
            }
            swap(curKey, table1[pos1].first);
            swap(curVal, table1[pos1].second);

            int pos2 = hash2(curKey);
            if(table2[pos2].first == -1 || table2[pos2].first == curKey){
                table2[pos2] = {curKey, curVal};
                return;
            }
            swap(curKey, table2[pos2].first);
            swap(curVal, table2[pos2].second);
        }
        // jeśli nie udało się włożyć - pomijamy 
    }

    void remove(int key) override {
        int pos1 = hash1(key);
        if(table1[pos1].first == key){
            table1[pos1] = {-1,-1};
            return;
        }
        int pos2 = hash2(key);
        if(table2[pos2].first == key){
            table2[pos2] = {-1,-1};
            return;
        }
    }

    int getSize() const override { return size; }
};

// ------------ Testowanie ------------

void runTest(HashTable &ht, vector<int> &keys, string name) {
    cout << "Testowanie: " << name << endl;

    // Optymistyczny - dodawanie i usuwanie unikalnych kluczy, które nie kolidują 
    {
        auto start = high_resolution_clock::now();
        for(int k : keys){
            ht.insert(k, k+1);
        }
        auto mid = high_resolution_clock::now();
        for(int k : keys){
            ht.remove(k);
        }
        auto end = high_resolution_clock::now();

        auto insert_ms = duration_cast<microseconds>(mid - start).count();
        auto remove_ms = duration_cast<microseconds>(end - mid).count();
        cout << "Optymistyczny - dodanie: " << insert_ms << " us, usuniecie: " << remove_ms << " us\n";
    }

    // Średni - dodajemy i usuwamy losowe klucze (może kolizje)
    {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dist(1, keys.size()*10);

        vector<int> randomKeys;
        for(int i=0; i<(int)keys.size(); ++i){
            randomKeys.push_back(dist(gen));
        }

        auto start = high_resolution_clock::now();
        for(int k : randomKeys){
            ht.insert(k, k+1);
        }
        auto mid = high_resolution_clock::now();
        for(int k : randomKeys){
            ht.remove(k);
        }
        auto end = high_resolution_clock::now();

        auto insert_ms = duration_cast<microseconds>(mid - start).count();
        auto remove_ms = duration_cast<microseconds>(end - mid).count();
        cout << "Sredni - dodanie: " << insert_ms << " us, usuniecie: " << remove_ms << " us\n";
    }

    // Pesymistyczny - wstawiamy wiele kluczy o takim samym haśle (kolizje)
    {
        int badHash = 42;
        int sz = ht.getSize();
        vector<int> collisionKeys;
        for(size_t i=0; i<keys.size(); ++i){
            collisionKeys.push_back(badHash + i * sz);
        }

        auto start = high_resolution_clock::now();
        for(int k : collisionKeys){
            ht.insert(k, k+1);
        }
        auto mid = high_resolution_clock::now();
        for(int k : collisionKeys){
            ht.remove(k);
        }
        auto end = high_resolution_clock::now();

        auto insert_ms = duration_cast<microseconds>(mid - start).count();
        auto remove_ms = duration_cast<microseconds>(end - mid).count();
        cout << "Pesymistyczny - dodanie: " << insert_ms << " us, usuniecie: " << remove_ms << " us\n";
    }

    cout << endl;
}

int main() {
    vector<int> sizes = {1000, 5000, 10000};

    for(auto size : sizes){
        cout << "Rozmiar tablicy: " << size << endl;

        vector<int> keys(size);
        for(int i=0; i<size; ++i) keys[i] = i*2+1; 

        SeparateChaining sc(size);
        LinearProbing lp(size);
        CuckooHashing ch(size);

        cout << "=== Separate Chaining ===" << endl;
        runTest(sc, keys, "Separate Chaining");

        cout << "=== Linear Probing ===" << endl;
        runTest(lp, keys, "Linear Probing");

        cout << "=== Cuckoo Hashing ===" << endl;
        runTest(ch, keys, "Cuckoo Hashing");

        cout << "-------------------------------" << endl << endl;
    }

    return 0;
}
