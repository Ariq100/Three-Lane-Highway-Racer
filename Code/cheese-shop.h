#include "splashkit.h"
#include <string>
#include "splashkit-arrays.h"

using std::string;

struct cheese_data {
    string name;
    double weight;
    int price_per_kg;
};

struct order_item_data {
    string cheese_name;
    double weight;
};

struct order_data {
    string supplier_name;
    dynamic_array<order_item_data> items;
    bool received;
};

struct shop_data {
    dynamic_array<cheese_data> cheeses;
    dynamic_array<order_data> orders;
};

cheese_data new_cheese(string name = "", double weight = 0.0, int price_per_kg = 0);
string cheese_to_string(const cheese_data &cheese, bool full_details = false);
double total_cost(const cheese_data &cheese);
double reduce_weight(cheese_data &cheese, double weight_to_reduce);
void increase_weight(cheese_data &cheese, double weight_to_add);
void add_cheese(shop_data &shop, const cheese_data &cheese);

// Order functions
order_data new_order(string supplier_name = "Supplier");
string order_to_string(order_data &order, bool show_items = false);
void add_order_item(order_data &order, string cheese_name, double weight);
void edit_order_item(order_data &order, int index, string cheese_name, double weight);
void remove_order_item(order_data &order, int index);
void receive_order(shop_data &shop, order_data &order);
void add_order(shop_data &shop, order_data &order);
