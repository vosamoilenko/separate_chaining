#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace std;

// declaration ADS_SET
template <typename Key, size_t N = 32>
class ADS_set {
    class Bucket;
    class HashTable;
public:
    class Iterator;

    using value_type = Key;
    using key_type = Key;
    using reference = key_type&;
    using const_reference = const key_type&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = Iterator;
    using const_iterator = Iterator;
    using key_equal = std::equal_to<key_type>;
    using hasher = std::hash<key_type>;
    using key_compare = std::less<key_type>;
private:
    unsigned sz;
    unsigned table_size;

    HashTable** table;

    bool exist_table();
    size_type hash_index(const key_type& k) const;
    void insert_unchecked(const key_type& k);
    std::pair<typename ADS_set<Key,N>::Bucket*,bool> find_bucket(const key_type& k) const;
    void reserve(size_t old);
    void print_tails();
    void rehash();
    pair<Bucket*,int> getNextBucket(pair<Bucket*,int> start) const;
    pair<Bucket*, int>getFirstBucket() const;
    pair<Bucket*, size_t>getLastBucket() const;

public:

    ADS_set();
    ADS_set(std::initializer_list<key_type> ilist);
    template<typename InputIt> ADS_set(InputIt first, InputIt last);
    ADS_set(const ADS_set& other);

    ~ADS_set();

    iterator find(const key_type& key) const;

    size_type size() const;
    bool empty() const;
    size_type count(const key_type& key) const;
    std::pair<bool,size_type> count_private(const key_type& key) const;

    void insert(std::initializer_list<key_type> ilist);
    template<typename InputIt> void insert(InputIt first, InputIt last);
    std::pair<iterator,bool> insert(const key_type& key);

    void dump(std::ostream& o = std::cerr) const;

    ADS_set& operator=(const ADS_set& other);
    ADS_set& operator=(std::initializer_list<key_type> ilist);

    void clear();
    void swap(ADS_set& other);

    size_type erase(const key_type& key);

    const_iterator begin() const;
    const_iterator end() const;


//////////////////////////

    friend bool operator==(const ADS_set& lhs, const ADS_set& rhs) {
        if (lhs.sz != rhs.sz) return false;
        for (const auto& k: lhs)
            if (!rhs.count(k)) return false;
        return true;
    }
    friend bool operator!=(const ADS_set& lhs, const ADS_set& rhs) {
        return !(lhs==rhs);
    }
private:
    class Bucket{
    public:
    Key value;
    Bucket *next;

    Bucket();
    Bucket(const_reference _value, Bucket* _next = nullptr);
    ~Bucket();
    };
    class HashTable{
public:
        size_t length = 0;
      Bucket* head;
      Bucket *tail;
      HashTable();
      ~HashTable();
    };
};
template <typename Key, size_t N> void swap(ADS_set<Key,N>& lhs, ADS_set<Key,N>& rhs) { lhs.swap(rhs); }

template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {


public:
    Bucket *current;
    HashTable **table;
    size_t index;
    size_t table_size;

    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type&;
    using pointer = const value_type*;
    using iterator_category = std::forward_iterator_tag;

    explicit Iterator(Bucket *_current, HashTable** _table, size_t _index,size_t _table_size) : current(_current), table(_table), index(_index), table_size(_table_size) {}

    reference operator*() const {
        return current->value;
    }
    pointer operator->() const {
        return &(current->value);
    }

    Iterator& operator++() {

        size_t old = index;

        if (current->next != table[index]->tail) {
            current = current->next;
            return *this;
        } else {

            size_t counter = index+1;
            for (; counter < table_size; ++counter) {
                if (!(table[counter]->head == table[counter]->tail)) {
                    current = table[counter]->head;
                    index = counter;
                    return *this;
                }
            }
        }
        current = table[old]->tail;
        return *this;
    }

    Iterator operator++(int) {
        auto it = *this;
        ++*this;
        return it;
    }
    friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
        return lhs.current == rhs.current;
    }

    friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
        return lhs.current != rhs.current;
    }

};

////////////////////////////////////////////////////////////
// private functions ADS_set
////////////////////////////////////////////////////////////

template <typename Key, size_t N>
void ADS_set<Key,N>::print_tails() {
    Bucket *tail;
    for (int i = 0; i < N; ++i) {
        tail = table[i]->tail;
    }
}

template <typename Key, size_t N>
bool ADS_set<Key,N>::exist_table() {
    return table_size == 0;

}

template <typename Key, size_t N>
typename ADS_set<Key,N>::size_type ADS_set<Key,N>::hash_index(const key_type& k) const {
    return hasher{}(k) % table_size;
}

template <typename Key, size_t N>
void ADS_set<Key,N>::insert_unchecked(const key_type& key) {
    size_type hash_value = hash_index(key);

    Bucket *help = new Bucket(key,table[hash_value]->head);

    if (table[hash_value]->head == nullptr) {
        table[hash_value]->tail = help;
    }
    table[hash_value]->head = help;
    table[hash_value]->length += 1;
    sz += 1;

    if (sz > table_size*3) {rehash();}
}

template <typename Key, size_t N>
std::pair<typename ADS_set<Key,N>::Bucket*,bool> ADS_set<Key,N>::find_bucket(const key_type& k) const {
    size_type hash_value = hash_index(k);
    Bucket* s = table[hash_value]->head;
    while(s && s != table[hash_value]->tail) {
        if (key_equal()(s->value,k)) {
            return std::make_pair(s, true);
        }
        s = s->next;
    }
    return std::make_pair(s, false);
}

template <typename Key, size_t N>
void ADS_set<Key,N>::rehash() {
    unsigned old = table_size;
    //copy elements
    int counter = 0;
    key_type *arr = new key_type[sz];
    for (auto it = begin(); it != end(); ++it) {
        arr[counter++] = *it;
    }

    table_size*=5;
    reserve(old);

    for (int i = 0; i < counter; ++i) {
        insert_unchecked(arr[i]);
    }

    delete []arr;

}

template <typename Key, size_t N>
pair<typename ADS_set<Key,N>::Bucket*,int> ADS_set<Key,N>::getNextBucket(pair<Bucket*,int> start) const {
    // next - ok
    if(start.first->next != table[start.second]->tail) {
        start.first = start.first->next;
        return start;
    } else {
        //next = tail

        // i+1 == table_size
        if (start.second == table_size-1) {
            start.first = start.first->next;
            return start;
        }
        //
        if (table[start.second+1]->head!=table[start.second+1]->tail) {
            start.first = table[++start.second]->head;
            return start;
        } else {
            int i = start.second+=1;

            for (; i < table_size; ++i) {

                if(table[i]->head!=table[i]->tail) {
                    start.first = table[i]->head;
                    start.second = i;
                    return start;
                }
            }
        }
    }
    start.first = start.first->next;
    return start;

}

template <typename Key, size_t N>
pair<typename ADS_set<Key,N>::Bucket*, int> ADS_set<Key,N>::getFirstBucket() const {
    for (size_t i = 0; i < table_size; ++i) {

        if (table[i]->head != table[i]->tail) {
            return make_pair(table[i]->head, i);

        }
    }
    return make_pair(table[0]->head, -1);
}

template <typename Key, size_t N>
pair<typename ADS_set<Key,N>::Bucket*, size_t> ADS_set<Key,N>::getLastBucket() const {

    for (int i = table_size-1; i >= 0; --i) {

        if (table[i]->head != table[i]->tail) {
            return make_pair(table[i]->tail, i);
        }
    }
    return make_pair(table[0]->head, -1);
}

template <typename Key, size_t N>
void ADS_set<Key,N>::reserve(size_t old) {

    for (size_t i = 0; i < old; ++i) {
        delete table[i];
    }
    delete []table;

    table = new HashTable*[table_size]();
    for (size_t i = 0; i < table_size; ++i) {
        table[i] = new HashTable();
        Bucket* help = new Bucket();
        table[i]->head = help;
        table[i]->tail = help;
    }

    sz = 0;
}

template <typename Key, size_t N>
std::pair<bool,typename ADS_set<Key,N>::size_type> ADS_set<Key,N>::count_private(const key_type& key) const{
    size_type hash_value = hash_index(key);
    if (table[hash_value]->head!=table[hash_value]->tail) {
        auto current = table[hash_value]->head;

        while(current != table[hash_value]->tail) {
            if(key_equal()(current->value, key)) {
                return std::make_pair(true,hash_value);
            }
            current = current->next;
        }
    }
    return std::make_pair(false,hash_value);
}


////////////////////////////////////////////////////////////
// public ADS
////////////////////////////////////////////////////////////

template <typename Key, size_t N>
typename ADS_set<Key,N>::const_iterator ADS_set<Key,N>::begin() const {

    for (size_t i = 0; i < table_size; ++i) {

        if (table[i]->head != table[i]->tail) {
            return Iterator(table[i]->head,table, i, table_size);
        }
    }
    return Iterator(table[0]->head,table, 0,table_size);
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::const_iterator ADS_set<Key,N>::end() const {

    for (int i = table_size-1; i >= 0; --i) {

        if (table[i]->head != table[i]->tail) {
            return Iterator(table[i]->tail, table,i,table_size);
        }
    }
    return Iterator(table[0]->head,table, 0,table_size);
}

template <typename Key, size_t N>
void ADS_set<Key,N>::clear() {
    for (int i = 0; i < table_size; ++i) {
        delete table[i];
    }
    delete []table;

    table = new HashTable*[table_size]();
    for (int i = 0; i < table_size; ++i) {
        table[i] = new HashTable();
        Bucket* help = new Bucket();
        table[i]->head = help;
        table[i]->tail = help;
    }

    sz = 0;
}

template <typename Key, size_t N>
void ADS_set<Key,N>::swap(ADS_set& other) {
    using std::swap;
    swap(sz,other.sz);
    swap(table_size,other.table_size);
    swap(table,other.table);
}

template <typename Key, size_t N>
size_t ADS_set<Key,N>::erase(const key_type& key) {

    size_type hash_value = hash_index(key);

    Bucket *current = table[hash_value]->head;
    Bucket *next;
    if (!count(key)) {
        return 0;
    }

    if (sz != 0) {

        if (key_equal()(current->value,key)) {
            Bucket* next = current->next;
            table[hash_value]->head = next;

            current->next = nullptr;
            delete current;

            sz -= 1;

            if (table[hash_value]->head == nullptr) {
                table[hash_value]->tail = nullptr;
            }
            return 1;


        } else {

            while (current->next) {
                next = current->next;

                if (next == table[hash_value]->tail) {
                    table[hash_value]->tail = current;
                }

                if (key_equal()(next->value,key)) {
                    current->next = next->next;

                    next->next = nullptr;
                    delete next;
                    sz -=1;

                    return 1;
                }
                current = current->next;
            }
        }
    }

    return 0;

}

template <typename Key, size_t N>
ADS_set<Key,N>::ADS_set(){
    sz = 0;
    table_size = N;
    table = new HashTable*[N]();
    for (size_t i = 0; i < table_size; ++i) {
        table[i] = new HashTable();
        Bucket* help = new Bucket();
        table[i]->head = help;
        table[i]->tail = help;
    }
}

template <typename Key, size_t N>
ADS_set<Key,N>::ADS_set(std::initializer_list<key_type> ilist): ADS_set() {
    insert(ilist);
}

template <typename Key, size_t N>
template<typename InputIt> ADS_set<Key,N>::ADS_set(InputIt first, InputIt last): ADS_set() {
    insert(first, last); //
}

template <typename Key, size_t N>
ADS_set<Key,N>::ADS_set(const ADS_set& other) : ADS_set() {
    insert(other.begin(), other.end());
}

template <typename Key, size_t N>
ADS_set<Key,N>::~ADS_set(){
    for (size_t i = 0; i < table_size; ++i) {
        delete table[i];
    }
    delete []table;
}

template <typename Key, size_t N>
ADS_set<Key,N>& ADS_set<Key,N>::operator=(const ADS_set& other) {
    clear();
    insert(other.begin(), other.end());
    return *this;
}

template <typename Key, size_t N>
ADS_set<Key,N>& ADS_set<Key,N>::operator=(std::initializer_list<key_type> ilist) {
    clear();
    insert(ilist);
    return *this;
}

template <typename Key, size_t N>
std::pair<typename ADS_set<Key,N>::iterator,bool> ADS_set<Key,N>::insert(const key_type& key) {

    auto result = find_bucket(key);

    if (result.second) {

        return std::make_pair(Iterator(result.first, table,hash_index(key),table_size), false);
    } else {
        
        if (sz+1 > table_size*3) {rehash();}

        size_t hash_value = hash_index(key);

        Bucket *help = new Bucket(key,table[hash_value]->head);
        if (table[hash_value]->head == nullptr) {
            table[hash_value]->tail = help;
        }
        table[hash_value]->head = help;
        sz += 1;


        return std::make_pair(Iterator(help, table, hash_value, table_size), true);
    }
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::iterator ADS_set<Key,N>::find(const key_type& key) const {
    auto result = find_bucket(key);
    size_type hash_value = hash_index(key);
    if (result.second) {
        return Iterator(result.first, table, hash_value,table_size);
    }
    return end();
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::size_type ADS_set<Key,N>::size() const{
    return sz;
}

template <typename Key, size_t N>
bool ADS_set<Key,N>::empty() const{
    return !sz;
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::size_type ADS_set<Key,N>::count(const key_type& key) const{
    auto pair = count_private(key);
    if (pair.first) {return true;}
    return false;
}


template <typename Key, size_t N>
void ADS_set<Key,N>::insert(std::initializer_list<key_type> ilist) {
    for (const auto& k: ilist) {
        if (!count(k)) {
            insert_unchecked(k);

        }
    }
}

template <typename Key, size_t N>
template<typename InputIt> void ADS_set<Key,N>::insert(InputIt first, InputIt last) {

    for (auto it = first; it != last; ++it) {
        if (!count(*it)) {
            insert_unchecked(*it);

        }
    }
}

template <typename Key, size_t N>
void ADS_set<Key,N>::dump(std::ostream& o) const {
    for (size_t i = 0; i < table_size; ++i) {

        Bucket *help = table[i]->head;

        if (help == table[i]->tail || help == nullptr) {
            o << "[ ]\n";
        } else {

            while(help && help != table[i]->tail) {
                o << '[' << help->value << ']';
                if (help->next != table[i]->tail) {o << " -> ";} else {o <<"\n";}
                help = help->next;
            }

        }
    }
}


////////////////////////////////////////////////////////////
// HashTable
////////////////////////////////////////////////////////////

template <typename Key, size_t N>
ADS_set<Key,N>::HashTable::HashTable() : head(nullptr) {}

template <typename Key, size_t N>
ADS_set<Key,N>::HashTable::~HashTable() {
    delete head;
}

////////////////////////////////////////////////////////////
// Bucket
////////////////////////////////////////////////////////////

template <typename Key, size_t N>
ADS_set<Key,N>::Bucket::Bucket() {
    next = nullptr;
}

template <typename Key, size_t N>
ADS_set<Key,N>::Bucket::Bucket(const_reference _value, Bucket* _next) {
    value = _value;
    next = _next;
}

template <typename Key, size_t N>
ADS_set<Key,N>::Bucket::~Bucket() {
        delete next;
}



#endif // ADS_SET_H
