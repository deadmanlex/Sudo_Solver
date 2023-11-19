#pragma once

#include "domain.h"
using namespace std;

template<class T>
struct is_container
{
    static const bool value = false;
};

template<class T, class Alloc>
struct is_container<std::vector<T, Alloc>>
{
    static const bool value = true; 
};

struct Variable 
{
    int *var;
    std::pair<int,int> grid_pos;
    EnumDomain domaine;

    size_t domaine_size() const { return (*var == 0) ? domaine.size() : 1; }
    bool operator==(const Variable& c2) const {
        auto [x1, y1] = this->grid_pos;
        auto [x2, y2] = c2.grid_pos;
        return x1 == x2 && y1 == y2;
    }
};

template<class T>
bool all_elements_differents(const T& list) {
    std::unordered_set<int> registered_entries = unordered_set<int>();
    std::vector vec{1, 2, 3, 4, 5, 6};
    auto v = std::views::filter(vec);
    bool valid = true;
    for (auto entry : list) {
        if constexpr (is_container<decltype(entry)>::value) 
        {
            if (!all_elements_differents(entry)) {
                valid = false;
                break;
            }
        }
        else if constexpr (is_integral<decltype(entry)>::value) 
        {
            if (!registered_entries.contains(entry)) {
                registered_entries.insert(entry);
            } 
            else {
                valid = false; 
                break;
            }
        } 
        else if constexpr (is_pointer<decltype(entry)>::value) 
        {
            if (!registered_entries.contains(*entry)) {
                registered_entries.insert(*entry);
            } 
            else {
                valid = false; 
                break;
            }
        }
    }
    return valid;
}

class Constraint 
{
    static int ID_GENERATOR;
    int ID = 0;
    std::vector<int*> _evaluation_range;
    std::vector<Variable*> _var_range;
public:
  
    Constraint(ranges::input_range auto&& variable_range)
    {
        ID = ID_GENERATOR;
        ID_GENERATOR++;

        _var_range = std::vector<Variable*>();
        _evaluation_range = std::vector<int*>();
        for (auto it = variable_range.begin(); it != variable_range.end(); it++) {
            _var_range.push_back(&(*it));
            _evaluation_range.push_back(it->var);
        }
    }

    bool range_assigned() {
        return std::find_if(_var_range.begin(), _var_range.end(), [](auto var) { return *var->var == 0; }) == _var_range.end();
    }

    bool operator()() { 
        return all_elements_differents(_evaluation_range);
    }
  
    std::vector<Variable*> var_range() const {
        return _var_range;
    }

    int get_id() const { return ID; }

    Variable* get_unassigned_var() const {
        auto find = std::find_if(_var_range.begin(), _var_range.end(), [](Variable* vari) { return *vari->var == 0; });
        return (find == _var_range.end()) ? *_var_range.begin() : *find;             
    }

    bool in_var_range(Variable* var) const {
        for (auto vi : _var_range) {
            if (*vi == *var) return true;
        }
        return false;
    }

    bool is_forward_checkable() const {
        int count = 0;
        for (auto vari : var_range()) {
            if (*vari->var == 0) count++;
        }
        return count == 1;
    }

    bool operator==(const Constraint& c2) const {
        return this->ID == c2.get_id();
    }
};


int Constraint::ID_GENERATOR = 0;

