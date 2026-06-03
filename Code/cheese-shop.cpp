#include "cheese-shop.h"
#include <format>
using std::format;

cheese_data new_cheese(string name, double weight, int price_per_kg) {
    cheese_data cheese;
    cheese.name = name;
    cheese.weight = weight;
    cheese.price_per_kg = price_per_kg;
    return cheese;
}

string cheese_to_string(const cheese_data &cheese, bool full_details) {
    if (full_details) {
        return format("{}: {:.2f} kg, ${:.2f}", cheese.name, cheese.weight, cheese.price_per_kg / 100.0);
    } 
    else {
        return cheese.name;
    }
}

double total_cost(const cheese_data &cheese) {
    return cheese.weight * (cheese.price_per_kg / 100.0);
}

double reduce_weight(cheese_data &cheese, double weight_to_reduce) {
    if (weight_to_reduce < 0) {
        return 0.0;
    }
    
    double actual_removed = 0.0;
    if (weight_to_reduce >= cheese.weight) {
        actual_removed = cheese.weight;
        cheese.weight = 0.0;
    } else {
        actual_removed = weight_to_reduce;
        cheese.weight -= weight_to_reduce;
    }
    
    return actual_removed;
}

void increase_weight(cheese_data &cheese, double weight_to_add) {
    if (weight_to_add > 0) {
        cheese.weight += weight_to_add;
    }
}

void add_cheese(shop_data &shop, const cheese_data &cheese) {
    shop.cheeses.add(cheese);
}

// ===== Order functions =====

order_data new_order(string supplier_name) {
    order_data order;
    order.supplier_name = supplier_name;
    order.received = false;
    return order;
}

string order_to_string(order_data &order, bool show_items) {
    string status = order.received ? "Received" : "Pending";
    string result = format("Order from '{}' [{}] - {} item(s)",
                           order.supplier_name, status, order.items.length());
    if (show_items) {
        for (int i = 0; i < (int)order.items.length(); i++) {
            result += format("\n  {}: {:.2f} kg", order.items[i].cheese_name, order.items[i].weight);
        }
    }
    return result;
}

void add_order_item(order_data &order, string cheese_name, double weight) {
    order_item_data item;
    item.cheese_name = cheese_name;
    item.weight = weight;
    order.items.add(item);
}

void edit_order_item(order_data &order, int index, string cheese_name, double weight) {
    if (index >= 0 && index < (int)order.items.length()) {
        order.items[index].cheese_name = cheese_name;
        order.items[index].weight = weight;
    }
}

void remove_order_item(order_data &order, int index) {
    if (index < 0 || index >= (int)order.items.length()) return;
    // Rebuild array without the item at index
    dynamic_array<order_item_data> new_items;
    for (int i = 0; i < (int)order.items.length(); i++) {
        if (i != index) new_items.add(order.items[i]);
    }
    order.items = new_items;
}

void receive_order(shop_data &shop, order_data &order) {
    if (order.received) return;
    for (int i = 0; i < (int)order.items.length(); i++) {
        order_item_data &item = order.items[i];
        bool found = false;
        for (int j = 0; j < (int)shop.cheeses.length(); j++) {
            if (shop.cheeses[j].name == item.cheese_name) {
                increase_weight(shop.cheeses[j], item.weight);
                found = true;
                break;
            }
        }
        if (!found) {
            shop.cheeses.add(new_cheese(item.cheese_name, item.weight));
        }
    }
    order.received = true;
}

void add_order(shop_data &shop, order_data &order) {
    shop.orders.add(order);
}