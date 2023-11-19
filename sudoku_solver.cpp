#include <iostream>
#include <string>
#include <unordered_set> 
#include <algorithm>
#include <type_traits>
#include <fstream>
#include <set>
#include <functional>   
#include <list>
#include <memory>
#include <chrono>
#include <ctime>
#include "constraints.h"
using namespace std;

template <typename T>
using matrix = vector<vector<T>>;

vector<matrix<int>> read_sudoku_file()
{
    ifstream file = ifstream("sudoku.txt");
    string line_iterator = "";
    vector<matrix<int>> sudoku_grids = vector<matrix<int>>();
    matrix<int> sudoku_grid = matrix<int>();
    bool first_grid = true;

    while (getline(file, line_iterator)) {
        if (line_iterator[0] == 'G') 
        {
            if (first_grid) {
                first_grid = false;
            }
            else {
                sudoku_grids.push_back(sudoku_grid);
                sudoku_grid.clear();
            }
            continue;
        }
        vector<int> sudo_line = vector<int>();
        for (auto entry : line_iterator) 
            sudo_line.push_back(entry - '0'); 

        sudoku_grid.push_back(sudo_line);
    }
    sudoku_grids.push_back(sudoku_grid);
    sudoku_grid.clear();
    return sudoku_grids;
}


void print_grid(const matrix<int>& sudoku_grid) 
{
    for (auto row : sudoku_grid) {
        for (auto elem : row) 
            cout << elem << " ";
        cout << endl;
    } 
    cout << endl;
}

bool deep_search_solution(vector<Variable>& variables, vector<Constraint> & constraints) 
{
    using namespace ranges;
    using ranges::for_each;
    stack<pair<Variable*, int>> retired_values = stack<pair<Variable*, int>>(); 

    //Filters
    auto is_candidate = views::filter([](const Variable& var) { return var.domaine.size() > 1; });
    auto forward_checkable = views::filter([](const Constraint &C) { return C.is_forward_checkable(); });
    while (true) {  
        auto contraintes_a_verifier = constraints | forward_checkable;
        auto contraintes_id_a_verifier = unordered_set<int>();

        list<Constraint> list_contraintes = list<Constraint>{contraintes_a_verifier.begin(), contraintes_a_verifier.end()};
        for (const Constraint& constraint : list_contraintes) {
            contraintes_id_a_verifier.insert(constraint.get_id());
        }

        while (!list_contraintes.empty()) {
            set<int> values_to_remove = set<int>();
            Constraint& current_constraint = list_contraintes.front();
            Variable* targeted_var = current_constraint.get_unassigned_var();

            for (int domain_value : targeted_var->domaine) {
                *targeted_var->var = domain_value;
                if (!current_constraint()) {
                    values_to_remove.insert(domain_value);
                }
            }
          
            for (int val : values_to_remove) {
                targeted_var->domaine.remove(val);
                retired_values.push(make_pair(targeted_var, val));
            }

            if (targeted_var->domaine.empty()) 
            {
                while (!retired_values.empty()) { 
                    auto & [var, value] = retired_values.top();
                    retired_values.pop();
                    var->domaine.undo_change(value);
                } 
                for_each(variables | is_candidate, [](auto & variable) { *variable.var = 0; });
                return false;
            }
            else if (targeted_var->domaine.size() == 1) {
                *targeted_var->var = *targeted_var->domaine.begin();
                auto variable_in_range = views::filter([&](const Constraint &C) { 
                    return C.in_var_range(targeted_var) && contraintes_id_a_verifier.find(C.get_id()) == contraintes_id_a_verifier.end(); 
                }); 

                auto contraintes_affecte = constraints | forward_checkable | variable_in_range;
                list_contraintes.insert(list_contraintes.end(), contraintes_affecte.begin(), contraintes_affecte.end());
                for_each(contraintes_affecte, [&](const auto & C) { contraintes_id_a_verifier.insert(C.get_id()); });
            } 
            else *targeted_var->var = 0;

            list_contraintes.pop_front();
            contraintes_id_a_verifier.erase(contraintes_id_a_verifier.find(current_constraint.get_id()));
        }
        
        auto variables_candidates = variables | is_candidate; 

        if (variables_candidates.empty()) {
            return true;
        }

        int min_domain_size = 100;
        Variable* selected_variable = nullptr; 
        for (Variable & cand_variable : variables_candidates) {
            selected_variable = (cand_variable.domaine.size() < min_domain_size) ? &cand_variable : selected_variable;
            min_domain_size = (cand_variable.domaine.size() < min_domain_size) ? cand_variable.domaine.size() : min_domain_size;
        }

        int value = *selected_variable->domaine.begin();
        *selected_variable->var = value;

        EnumDomain tmp_domaine = selected_variable->domaine;
        selected_variable->domaine = EnumDomain({value});

        if  (!deep_search_solution(variables, constraints)) {
            selected_variable->domaine = tmp_domaine;
            selected_variable->domaine.remove(value);
            retired_values.push(make_pair(selected_variable, value)); 
            
            if (selected_variable->domaine.size() == 1) { 
                *selected_variable->var = *selected_variable->domaine.begin();
            }
            else *selected_variable->var = 0;
        }
    }
}


void solve_sudoku(matrix<int> & sudoku_grid) 
{
    using ranges::for_each;
    vector<Variable> variables = vector<Variable>();
    vector<Constraint> constraints = vector<Constraint>();

    int row = 0;
    auto make_all_diff_constraint = [&constraints, &variables](ranges::input_range auto&& variable_range) {
          for (auto var1 = variable_range.begin(); var1 != ranges::prev(variable_range.end()); var1 = ranges::next(var1)) {
            for (auto var2 = ranges::next(var1); var2 != variable_range.end(); var2 = ranges::next(var2)) {
                const auto& varv1 = *var1;
                const auto& varv2 = *var2;
                auto vals = views::filter([&varv1, &varv2](const Variable& var) { return varv1 == var || varv2 == var; });
                Constraint new_constraint = Constraint(variables | vals);
                constraints.push_back(new_constraint);
            } 
        } 
    };

    for (auto & grid_line : sudoku_grid){
        int col = 0;
        for (auto & grid_entry : grid_line)
        {
            //It's an unassigned grid case so we create a variable for it
            if (grid_entry == 0) 
                variables.push_back(Variable{&grid_entry, make_pair(row,col), EnumDomain({1,2,3,4,5,6,7,8,9})});
            col++;
        }
        row++;
    }

    //Predicate to get the constant values in the sudoku grid
    auto constant = views::filter([](int* a) { return *a != 0; });

    //The constraint that every rows are all_different
    for (int i = 0; i < sudoku_grid.size(); i++) {
        vector<int*> row = vector<int*>();
        for (int i2 = 0; i2 < 9; i2++)  {
            row.push_back(&sudoku_grid[i][i2]);
        }

        auto in_row = ranges::filter_view(variables, [i](const Variable& v) { return v.grid_pos.first == i; });
        for (int* val_to_filter : row | constant) {
            for_each (in_row, [&val_to_filter](auto &var) {var.domaine.remove(*val_to_filter); }); 
        }
        make_all_diff_constraint(in_row);
    }

    //The constraint that every columns are all_different
    for (int i = 0; i < sudoku_grid[0].size(); i++) {
        vector<int*> col = vector<int*>();
        for (int i2 = 0; i2 < 9; i2++)  {
            col.push_back(&sudoku_grid[i2][i]);
        }

        auto in_col = ranges::filter_view(variables, [i](Variable& v) { return v.grid_pos.second == i; });
        for (int* val_to_filter : col | constant) {
            for_each (in_col, [&val_to_filter](auto &var) {var.domaine.remove(*val_to_filter); }); 
        }
        make_all_diff_constraint(in_col);
    }

    for (int i = 0; i < sudoku_grid.size(); i+=3) { 
        for (int j = 0; j < sudoku_grid[0].size(); j += 3)  
        {
            matrix<int*> subgrid = matrix<int*>();
            for (int z = 0; z < 3; z++) {
                vector<int*> subrow = vector<int*>();
                for (int z2 = 0; z2 < 3; z2++) {
                    subrow.push_back(&sudoku_grid[i+z][j+z2]);
                }
                subgrid.push_back(subrow);
            }
            auto in_grid_pred = [i, j](Variable& v){  
                auto [row, col] = v.grid_pos;
                return row >= i && row < i + 3 &&  col >= j && col < j + 3;
            };
            auto in_grid = ranges::filter_view(variables, in_grid_pred);
            for (auto subrow : subgrid) 
                for (int* val_to_filter : subrow | constant) 
                    for_each(in_grid, [&val_to_filter](auto &var) {var.domaine.remove(*val_to_filter); }); 

            
            for (auto var1 = in_grid.begin(); var1 != ranges::prev(in_grid.end()); var1 = ranges::next(var1)) {
                for (auto var2 = ranges::next(var1); var2 != in_grid.end(); var2 = ranges::next(var2)) {
                    const auto& varv1 = *var1;
                    const auto& varv2 = *var2;
                    auto [row1,col1] = varv1.grid_pos;
                    auto [row2,col2] = varv2.grid_pos;
                    if (row1 == row2 || col1 == col2 ) {
                        continue;
                    }

                    auto vals = views::filter([&varv1, &varv2](const Variable& var) { return varv1 == var || varv2 == var; });
                    Constraint new_constraint = Constraint(variables | vals);
                    constraints.push_back(new_constraint);
                } 
            } 
        }
    }
    deep_search_solution(variables, constraints);   
}


int main() 
{
    int sum_left_corner = 0;
    
    vector<matrix<int>> sudoku_grids = vector<matrix<int>>();
    sudoku_grids = read_sudoku_file();

    chrono::time_point<chrono::system_clock> start;
    start = chrono::system_clock::now();

    for (auto& grid : sudoku_grids) {
        print_grid(grid);
        solve_sudoku(grid);
        print_grid(grid);
        int _digit_number = grid[0][0] * 100 + grid[0][1] * 10 + grid[0][2];
        sum_left_corner += _digit_number; 
    }
    
    chrono::duration<double> elapsed_seconds = chrono::system_clock::now() - start;
    cout << "La somme totale des nombres dans le coin gauche est : " << sum_left_corner << endl;
    cout << "Execution Time : " << elapsed_seconds << endl;
}