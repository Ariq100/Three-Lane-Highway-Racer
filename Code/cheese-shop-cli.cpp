#include "splashkit.h"
#include "utilities.h"
#include "cheese-shop.h"

#include <format>
using std::format;

// ===== Enums =====

enum main_menu_option {
    EXIT_MAIN_MENU,
    ADD_CHEESE_MENU,
    EDIT_CHEESE_MENU,
    PRINT_STOCK_LIST_MENU,
    TEST_REDUCE_WEIGHT_MENU,
    TEST_INCREASE_WEIGHT_MENU,
    SUPPLIER_ORDERS_MENU
};

enum order_menu_option {
    BACK_ORDER_MENU,
    CREATE_ORDER_MENU,
    MANAGE_ITEMS_MENU,
    RECEIVE_ORDER_MENU,
    VIEW_ORDERS_MENU
};

enum item_menu_option {
    BACK_ITEM_MENU,
    ADD_ITEM_MENU,
    EDIT_ITEM_MENU,
    REMOVE_ITEM_MENU,
    PRINT_ITEMS_MENU
};

// ===== Cheese helpers =====

void print_cheese(const cheese_data &cheese) {
    write_line(cheese_to_string(cheese, true));
}

cheese_data read_cheese() {
    cheese_data cheese;
    cheese.name = read_string("Enter cheese name: ");
    cheese.weight = read_double("Enter weight in stock (kg): ");
    cheese.price_per_kg = read_integer("Enter price per kg (cents): ");
    return cheese;
}

void edit_cheese(cheese_data &cheese) {
    write_line("Editing cheese: " + cheese_to_string(cheese, true));
    if (read_boolean("Do you want to edit the name? (y/n): "))
        cheese.name = read_string("Enter new cheese name: ");
    if (read_boolean("Do you want to edit the weight? (y/n): "))
        cheese.weight = read_double("Enter new weight in stock (kg): ");
    if (read_boolean("Do you want to edit the price? (y/n): "))
        cheese.price_per_kg = read_integer("Enter new price per kg (cents): ");
}

void print_cheese_list(const dynamic_array<cheese_data> &cheeses, bool with_ids) {
    for (int i = 0; i < (int)cheeses.length(); i++) {
        cheese_data cheese = cheeses[i];
        if (with_ids) write(format("{}: ", i + 1));
        print_cheese(cheese);
    }
}

int select_cheese(const dynamic_array<cheese_data> &cheeses) {
    if (cheeses.length() == 0) {
        write_line("No cheese in stock.");
        return -1;
    }
    write_line("0: Select none");
    print_cheese_list(cheeses, true);
    int choice;
    while (true) {
        choice = read_integer(format("Select cheese (0 - {}): ", cheeses.length()));
        if (choice >= 0 && choice <= (int)cheeses.length()) break;
        write_line(format("Please enter a value between 0 and {}.", cheeses.length()));
    }
    return choice - 1;
}

// ===== Main cheese handlers =====

void handle_add_cheese(shop_data &shop) {
    cheese_data new_cheese_item = read_cheese();
    add_cheese(shop, new_cheese_item);
    write_line("Cheese added successfully!");
}

void handle_edit_cheese(shop_data &shop) {
    int index = select_cheese(shop.cheeses);
    if (index == -1) return;
    edit_cheese(shop.cheeses[index]);
    write_line("Cheese updated successfully!");
}

void print_stock_list(const shop_data &shop) {
    if (shop.cheeses.length() == 0) {
        write_line("No cheese in stock.");
        return;
    }
    write_line("\n===================================");
    write_line("Cheese stock list:");
    write_line("===================================");
    print_cheese_list(shop.cheeses, false);
    write_line("===================================\n");
}

void handle_test_reduce_weight(shop_data &shop) {
    int index = select_cheese(shop.cheeses);
    if (index == -1) return;
    double amount = read_double("Enter amount to reduce (kg): ");
    double removed = reduce_weight(shop.cheeses[index], amount);
    write_line(format("Removed: {:.2f} kg", removed));
    write_line("Updated cheese:");
    print_cheese(shop.cheeses[index]);
}

void handle_test_increase_weight(shop_data &shop) {
    int index = select_cheese(shop.cheeses);
    if (index == -1) return;
    double amount = read_double("Enter amount to increase (kg): ");
    increase_weight(shop.cheeses[index], amount);
    write_line("Updated cheese:");
    print_cheese(shop.cheeses[index]);
}

// ===== Order helpers =====

int select_order(shop_data &shop, bool pending_only) {
    int count = 0;
    for (int i = 0; i < (int)shop.orders.length(); i++) {
        if (!pending_only || !shop.orders[i].received) count++;
    }
    if (count == 0) {
        write_line(pending_only ? "No pending orders." : "No orders found.");
        return -1;
    }
    write_line("0: Select none");
    for (int i = 0; i < (int)shop.orders.length(); i++) {
        if (!pending_only || !shop.orders[i].received)
            write_line(format("{}: {}", i + 1, order_to_string(shop.orders[i])));
    }
    int choice;
    while (true) {
        choice = read_integer(format("Select order (0 - {}): ", shop.orders.length()));
        if (choice >= 0 && choice <= (int)shop.orders.length()) break;
        write_line(format("Please enter a value between 0 and {}.", shop.orders.length()));
    }
    return choice - 1;
}

void print_order_items(order_data &order) {
    if (order.items.length() == 0) {
        write_line("  (no items)");
        return;
    }
    for (int i = 0; i < (int)order.items.length(); i++) {
        write_line(format("  {}: {} - {:.2f} kg", i + 1, order.items[i].cheese_name, order.items[i].weight));
    }
}

int select_order_item(order_data &order) {
    if (order.items.length() == 0) {
        write_line("No items in this order.");
        return -1;
    }
    write_line("0: Select none");
    print_order_items(order);
    int choice;
    while (true) {
        choice = read_integer(format("Select item (0 - {}): ", order.items.length()));
        if (choice >= 0 && choice <= (int)order.items.length()) break;
        write_line(format("Please enter a value between 0 and {}.", order.items.length()));
    }
    return choice - 1;
}

// ===== Order item management sub-menu =====

item_menu_option read_item_menu() {
    write_line("\n=== Manage Order Items ===");
    write_line("0. Back");
    write_line("1. Add item");
    write_line("2. Edit item");
    write_line("3. Remove item");
    write_line("4. Print items");
    int choice;
    while (true) {
        choice = read_integer("Select an option (0-4): ");
        if (choice >= 0 && choice <= 4) break;
        write_line("Please enter a value between 0 and 4.");
    }
    return (item_menu_option)choice;
}

void handle_manage_items(order_data &order) {
    item_menu_option choice;
    do {
        write_line("\n--- " + order_to_string(order) + " ---");
        choice = read_item_menu();
        switch (choice) {
            case BACK_ITEM_MENU:
                break;
            case ADD_ITEM_MENU: {
                string name = read_string("Enter cheese name: ");
                double weight = read_double("Enter weight to order (kg): ");
                add_order_item(order, name, weight);
                write_line("Item added.");
                break;
            }
            case EDIT_ITEM_MENU: {
                int idx = select_order_item(order);
                if (idx == -1) break;
                string name = read_string("Enter new cheese name: ");
                double weight = read_double("Enter new weight (kg): ");
                edit_order_item(order, idx, name, weight);
                write_line("Item updated.");
                break;
            }
            case REMOVE_ITEM_MENU: {
                int idx = select_order_item(order);
                if (idx == -1) break;
                remove_order_item(order, idx);
                write_line("Item removed.");
                break;
            }
            case PRINT_ITEMS_MENU:
                write_line("\nItems in order:");
                print_order_items(order);
                break;
        }
    } while (choice != BACK_ITEM_MENU);
}

// ===== Supplier orders main menu =====

order_menu_option read_order_menu() {
    write_line("\n=== Supplier Orders Menu ===");
    write_line("0. Back to main menu");
    write_line("1. Create new order");
    write_line("2. Manage order items");
    write_line("3. Receive order (apply to stock)");
    write_line("4. View all orders");
    int choice;
    while (true) {
        choice = read_integer("Select an option (0-4): ");
        if (choice >= 0 && choice <= 4) break;
        write_line("Please enter a value between 0 and 4.");
    }
    return (order_menu_option)choice;
}

void handle_supplier_orders(shop_data &shop) {
    order_menu_option choice;
    do {
        choice = read_order_menu();
        switch (choice) {
            case BACK_ORDER_MENU:
                break;
            case CREATE_ORDER_MENU: {
                string supplier = read_string("Enter supplier name: ");
                order_data order = new_order(supplier);
                add_order(shop, order);
                write_line("Order created. Use 'Manage order items' to add items.");
                break;
            }
            case MANAGE_ITEMS_MENU: {
                int idx = select_order(shop, false);
                if (idx == -1) break;
                if (shop.orders[idx].received) {
                    write_line("This order has already been received and cannot be edited.");
                    break;
                }
                handle_manage_items(shop.orders[idx]);
                break;
            }
            case RECEIVE_ORDER_MENU: {
                int idx = select_order(shop, true);
                if (idx == -1) break;
                if (shop.orders[idx].items.length() == 0) {
                    write_line("Cannot receive an empty order.");
                    break;
                }
                receive_order(shop, shop.orders[idx]);
                write_line("Order received! Stock has been updated.");
                print_stock_list(shop);
                break;
            }
            case VIEW_ORDERS_MENU: {
                if (shop.orders.length() == 0) {
                    write_line("No orders yet.");
                    break;
                }
                write_line("\n=== All Orders ===");
                for (int i = 0; i < (int)shop.orders.length(); i++) {
                    write_line(format("{}. {}", i + 1, order_to_string(shop.orders[i], true)));
                }
                break;
            }
        }
    } while (choice != BACK_ORDER_MENU);
}

// ===== Main menu =====

main_menu_option read_main_menu() {
    write_line("\n=== Cheese Shop Menu ===");
    write_line("0. Exit");
    write_line("1. Add cheese");
    write_line("2. Edit cheese");
    write_line("3. Print cheese list");
    write_line("4. Test reduce weight");
    write_line("5. Test increase weight");
    write_line("6. Supplier orders");
    int choice;
    while (true) {
        choice = read_integer("Select an option (0-6): ");
        if (choice >= 0 && choice <= 6) break;
        write_line("Please enter a value between 0 and 6.");
    }
    return (main_menu_option)choice;
}

int main() {
    shop_data shop;
    main_menu_option choice;
    do {
        choice = read_main_menu();
        switch (choice) {
            case EXIT_MAIN_MENU:
                write_line("Exiting...");
                break;
            case ADD_CHEESE_MENU:
                handle_add_cheese(shop);
                break;
            case EDIT_CHEESE_MENU:
                handle_edit_cheese(shop);
                break;
            case PRINT_STOCK_LIST_MENU:
                print_stock_list(shop);
                break;
            case TEST_REDUCE_WEIGHT_MENU:
                handle_test_reduce_weight(shop);
                break;
            case TEST_INCREASE_WEIGHT_MENU:
                handle_test_increase_weight(shop);
                break;
            case SUPPLIER_ORDERS_MENU:
                handle_supplier_orders(shop);
                break;
        }
    } while (choice != EXIT_MAIN_MENU);
    return 0;
}
