#include "catch_amalgamated.hpp"
#include "cheese-shop.h"

using Catch::Approx;

TEST_CASE("Cheese - Create new cheese with default values") {
    cheese_data cheese;
    cheese = new_cheese();
    CHECK(cheese.name == "");
    CHECK(cheese.weight == 0.0);
    CHECK(cheese.price_per_kg == 0);
}

TEST_CASE("Cheese - Provide name, weight, and price values for new cheese") {
    cheese_data cheese;
    cheese = new_cheese("Cheddar", 1.5, 2000);
    REQUIRE(cheese.name == "Cheddar");
    REQUIRE(cheese.weight == Approx(1.5));
    REQUIRE(cheese.price_per_kg == 2000);
}

TEST_CASE("Cheese - Create new cheese with partial values") {
    cheese_data cheese;
    cheese = new_cheese("Camembert", 2.0);
    REQUIRE(cheese.name == "Camembert");
    REQUIRE(cheese.weight == Approx(2.0));
    REQUIRE(cheese.price_per_kg == 0);
}

TEST_CASE("Cheese - Convert cheese data to string") {
    cheese_data cheese;
    cheese = new_cheese("Cheddar", 1.5, 2000);
    
    string result = cheese_to_string(cheese, true);
    string expected = "Cheddar: 1.50 kg, $20.00";
    REQUIRE(result == expected);
    
    result = cheese_to_string(cheese, false);
    expected = "Cheddar";
    REQUIRE(result == expected);
    
    cheese = new_cheese("Camembert");
    result = cheese_to_string(cheese, false);
    expected = "Camembert";
    REQUIRE(result == expected);
    
    result = cheese_to_string(cheese, true);
    expected = "Camembert: 0.00 kg, $0.00";
    REQUIRE(result == expected);
}

// ===== total_cost tests =====

TEST_CASE("Cheese - Calculate total cost") {
    cheese_data cheese;
    
    // Test basic calculation
    cheese = new_cheese("Cheddar", 1.5, 2000);
    REQUIRE(total_cost(cheese) == Approx(30.0));
    
    // Test with zero weight
    cheese = new_cheese("Brie", 0.0, 5000);
    REQUIRE(total_cost(cheese) == Approx(0.0));
    
    // Test with larger values
    cheese = new_cheese("Gouda", 5.0, 3000);
    REQUIRE(total_cost(cheese) == Approx(150.0));
}

// ===== reduce_weight tests =====

TEST_CASE("Cheese - Reduce weight normally") {
    cheese_data cheese;
    cheese = new_cheese("Cheddar", 1.5, 2000);
    
    double removed = reduce_weight(cheese, 0.5);
    REQUIRE(removed == Approx(0.5));
    REQUIRE(cheese.weight == Approx(1.0));
}

TEST_CASE("Cheese - Reduce weight to zero") {
    cheese_data cheese;
    cheese = new_cheese("Cheddar", 1.5, 2000);
    
    double removed = reduce_weight(cheese, 1.7);
    REQUIRE(removed == Approx(1.5));
    REQUIRE(cheese.weight == Approx(0.0));
}

TEST_CASE("Cheese - Reduce weight with negative value") {
    cheese_data cheese;
    cheese = new_cheese("Cheddar", 1.5, 2000);
    
    double removed = reduce_weight(cheese, -0.5);
    REQUIRE(removed == 0.0);
    REQUIRE(cheese.weight == Approx(1.5));
}

TEST_CASE("Cheese - Reduce weight from zero") {
    cheese_data cheese;
    cheese = new_cheese("Cheddar", 0.0, 2000);
    
    double removed = reduce_weight(cheese, 1.0);
    REQUIRE(removed == Approx(0.0));
    REQUIRE(cheese.weight == Approx(0.0));
}

// ===== increase_weight tests =====

TEST_CASE("Cheese - Increase weight normally") {
    cheese_data cheese;
    cheese = new_cheese("Cheddar", 1.5, 2000);
    
    increase_weight(cheese, 0.5);
    REQUIRE(cheese.weight == Approx(2.0));
}

TEST_CASE("Cheese - Increase weight with negative value") {
    cheese_data cheese;
    cheese = new_cheese("Cheddar", 1.5, 2000);
    
    increase_weight(cheese, -0.5);
    REQUIRE(cheese.weight == Approx(1.5));
}

TEST_CASE("Cheese - Increase weight from zero") {
    cheese_data cheese;
    cheese = new_cheese("Cheddar", 0.0, 2000);
    
    increase_weight(cheese, 2.5);
    REQUIRE(cheese.weight == Approx(2.5));
}

// ===== add_cheese tests =====

TEST_CASE("Shop - Add cheese to empty shop") {
    shop_data shop;
    cheese_data cheese = new_cheese("Cheddar", 1.5, 2000);
    
    add_cheese(shop, cheese);
    REQUIRE(shop.cheeses.length() == 1);
    REQUIRE(shop.cheeses[0].name == "Cheddar");
}

TEST_CASE("Shop - Add multiple cheeses") {
    shop_data shop;
    cheese_data cheese1 = new_cheese("Cheddar", 1.5, 2000);
    cheese_data cheese2 = new_cheese("Brie", 2.0, 3000);
    
    add_cheese(shop, cheese1);
    add_cheese(shop, cheese2);
    REQUIRE(shop.cheeses.length() == 2);
    REQUIRE(shop.cheeses[0].name == "Cheddar");
    REQUIRE(shop.cheeses[1].name == "Brie");
}

// ===== Order tests =====

TEST_CASE("Order - Create new order with default supplier") {
    order_data order = new_order();
    REQUIRE(order.supplier_name == "Supplier");
    REQUIRE(order.received == false);
    REQUIRE(order.items.length() == 0);
}

TEST_CASE("Order - Create new order with custom supplier") {
    order_data order = new_order("Dairy Farm Co.");
    REQUIRE(order.supplier_name == "Dairy Farm Co.");
    REQUIRE(order.received == false);
}

TEST_CASE("Order - Add item to order") {
    order_data order = new_order("Supplier");
    add_order_item(order, "Cheddar", 5.0);
    REQUIRE(order.items.length() == 1);
    REQUIRE(order.items[0].cheese_name == "Cheddar");
    REQUIRE(order.items[0].weight == Approx(5.0));
}

TEST_CASE("Order - Add multiple items to order") {
    order_data order = new_order("Supplier");
    add_order_item(order, "Cheddar", 5.0);
    add_order_item(order, "Brie", 2.5);
    REQUIRE(order.items.length() == 2);
    REQUIRE(order.items[1].cheese_name == "Brie");
}

TEST_CASE("Order - Edit item in order") {
    order_data order = new_order("Supplier");
    add_order_item(order, "Cheddar", 5.0);
    edit_order_item(order, 0, "Gouda", 3.0);
    REQUIRE(order.items[0].cheese_name == "Gouda");
    REQUIRE(order.items[0].weight == Approx(3.0));
}

TEST_CASE("Order - Remove item from order") {
    order_data order = new_order("Supplier");
    add_order_item(order, "Cheddar", 5.0);
    add_order_item(order, "Brie", 2.5);
    remove_order_item(order, 0);
    REQUIRE(order.items.length() == 1);
    REQUIRE(order.items[0].cheese_name == "Brie");
}

TEST_CASE("Order - Edit item with invalid index is ignored (Negative Test)") {
    order_data order = new_order("Supplier");
    add_order_item(order, "Cheddar", 5.0);
    
    // Trying to edit an index that doesn't exist (too high or negative)
    edit_order_item(order, 99, "Gouda", 3.0);
    edit_order_item(order, -1, "Gouda", 3.0);
    
    // It should remain completely unchanged
    REQUIRE(order.items[0].cheese_name == "Cheddar");
    REQUIRE(order.items[0].weight == Approx(5.0));
}

TEST_CASE("Order - Remove item with invalid index is ignored (Negative Test)") {
    order_data order = new_order("Supplier");
    add_order_item(order, "Cheddar", 5.0);
    
    // Trying to remove at an invalid index
    remove_order_item(order, 99); 
    remove_order_item(order, -1);
    
    // It should not delete anything
    REQUIRE(order.items.length() == 1);
}

TEST_CASE("Order - Receive order increases existing stock") {
    shop_data shop;
    shop.cheeses.add(new_cheese("Cheddar", 1.0, 2000));
    order_data order = new_order("Supplier");
    add_order_item(order, "Cheddar", 3.0);
    receive_order(shop, order);
    REQUIRE(shop.cheeses[0].weight == Approx(4.0));
    REQUIRE(order.received == true);
}

TEST_CASE("Order - Receive order adds new cheese if not in stock") {
    shop_data shop;
    order_data order = new_order("Supplier");
    add_order_item(order, "Gouda", 2.0);
    receive_order(shop, order);
    REQUIRE(shop.cheeses.length() == 1);
    REQUIRE(shop.cheeses[0].name == "Gouda");
    REQUIRE(shop.cheeses[0].weight == Approx(2.0));
}

TEST_CASE("Order - Already-received order is not applied twice") {
    shop_data shop;
    shop.cheeses.add(new_cheese("Cheddar", 1.0, 2000));
    order_data order = new_order("Supplier");
    add_order_item(order, "Cheddar", 3.0);
    receive_order(shop, order);
    receive_order(shop, order); // second call should be ignored
    REQUIRE(shop.cheeses[0].weight == Approx(4.0));
}

TEST_CASE("Shop - Add order to shop") {
    shop_data shop;
    order_data order1 = new_order("Supplier A");
    order_data order2 = new_order("Supplier B");
    
    add_order(shop, order1);
    add_order(shop, order2);
    
    REQUIRE(shop.orders.length() == 2);
    REQUIRE(shop.orders[0].supplier_name == "Supplier A");
    REQUIRE(shop.orders[1].supplier_name == "Supplier B");
}
