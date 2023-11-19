#pragma once
#include <initializer_list>
#include <unordered_map>
#include <vector>
#include <iterator>
#include <stack>
#include <numeric>
#include <ranges>
#include <memory>
#include <utility>

class EnumDomain 
{
    struct Node 
    {
        // part which will store data
        int data;
        // pointer to the previous node
        struct std::shared_ptr<Node> prev;
        // pointer to the next node
        struct std::shared_ptr<Node> next;
        Node(int val): data(val), next(nullptr), prev(nullptr) {}
    }; 

    using node_ptr = std::shared_ptr<Node>;

    struct T_Node 
    {
        bool flag; 
        node_ptr target;
    };

public: 
    struct Iterator 
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = Node;
        using pointer           = node_ptr;  // or also value_type*
        using reference         = int&;  // or also value_type&
        
        Iterator(pointer ptr) : m_ptr(ptr) {}
        reference operator*() const { return m_ptr->data; }
    
        // Prefix increment
        Iterator& operator++() { m_ptr = m_ptr->next; return *this; }  
    
        // Postfix increment
        Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

        friend bool operator== (const Iterator& a, const Iterator& b) { return a.m_ptr == b.m_ptr; };
        friend bool operator!= (const Iterator& a, const Iterator& b) { return a.m_ptr != b.m_ptr; };   

    private:
        pointer m_ptr;
    };
private:
    std::unordered_map<int, T_Node> _flag_table {};
    node_ptr _head; 
    node_ptr _end;
    
    node_ptr operator[](int i) {
        return _flag_table[i].target;
    }
    size_t length = 0;

public:
    EnumDomain() : _head(nullptr), _end(nullptr) {}

    ~EnumDomain() {
        node_ptr tmp = nullptr;
        while (_head) {
            tmp = _head;
            _head = _head->next;
            tmp.reset();
        }
        _head = nullptr;
        _end = nullptr;
        _flag_table.clear();
    }
    
    EnumDomain(const std::initializer_list<int>& init_list) : _head(nullptr), _end(nullptr) {
        for (int elem : init_list) 
            push_back(elem);
    }


    void push_back(int val) {
        node_ptr new_node = std::make_shared<Node>(val);
        if (!_end  && !_head) {
            _head = new_node;
            _end = new_node;
        }
        else {
            _end->next = new_node;
            new_node->prev = _end;
            _end = new_node;
        }
        _flag_table[val] = (T_Node{true, new_node});
        length++;
    }

    void remove(int val) {
        if (!this->in_domain(val)) 
            return;
        node_ptr node = this->operator[](val); 
        if (node == this->_head && node == this->_end) {
            _head = nullptr;
            _end = nullptr;
        }
        else if (node == this->_head) {
            node->next->prev = nullptr;
            this->_head = node->next;
        }
        else if (node == this->_end) {
            node->prev->next = nullptr; 
            this->_end = node->prev;
        }
        else {
            node->next->prev = node->prev;
            node->prev->next = node->next; 
        }
        _flag_table[val].flag = false;
        length--;
        node = nullptr;
    }

    void undo_change(int val) {
        node_ptr last_removed_node = _flag_table.at(val).target;
        if (empty()) {
            _head = last_removed_node; 
            _end = last_removed_node;
        }
        else if (last_removed_node->next == _head) {
            _head->prev = last_removed_node;
            _head = last_removed_node;
        }
        else if (last_removed_node->prev == _end) {
            _end->next = last_removed_node;
            _end = last_removed_node;
        }
        else {
            last_removed_node->prev->next = last_removed_node;
            last_removed_node->next->prev = last_removed_node;
        }
        _flag_table[val].flag = true;
        length++;
        last_removed_node = nullptr;
    }

    bool in_domain(int val) const {
        return (_flag_table.find(val) != _flag_table.end()) ? _flag_table.at(val).flag : false;
    }

    bool empty() const { return _head == nullptr; }
    size_t size() const { return length; }

    Iterator begin() { return Iterator(_head); }
    Iterator end()   { return Iterator(nullptr); }
};

class domaine_borne { 
    std::stack<std::pair<int,int>> domain_stack; 
    int min;
    int max;
    
public:
    domaine_borne(int pmin, int pmax) {
        min = pmin;
        pmax = pmax;
        stack_node();
    }
    
    void setMin(int new_min) {
        domain_stack.push(std::make_pair(min, max)); 
        min = new_min;
    }

    void setMax(int new_max) { 
        max = new_max; 
    }

    void stack_node() {
        domain_stack.push(std::make_pair(min,max)); 
    }

    void setMinMax(int new_min, int new_max) {
        min = new_min;
        max = new_max;
    }

    void revert() {
        auto [p_min, p_max] = domain_stack.top();
        min = p_min;
        max = p_max; 
        domain_stack.pop();
    }
 
    std::list<int> getDomainRange()    {
        std::list<int> l(9);
        std::iota(l.begin(), l.end(), 1);
        return l;
    }
};


