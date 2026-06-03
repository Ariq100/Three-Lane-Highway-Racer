# Cheese Shop Program

A comprehensive C++ application demonstrating test-driven development (TDD), modular code organization, and command-line interface design using SplashKit.

## Project Structure

### Core Files

**cheese-shop.h** - Header file defining all data structures and function declarations:
- `cheese_data` struct - Represents a cheese with name, weight (kg), and price per kg (cents)
- `shop_data` struct - Represents a shop containing multiple cheeses
- Function declarations for all cheese operations

**cheese-shop.cpp** - Implementation of the cheese model:
- `new_cheese()` - Create a new cheese with default or specified values
- `cheese_to_string()` - Convert cheese data to a formatted string
- `total_cost()` - Calculate the total cost of a cheese (weight × price per kg)
- `reduce_weight()` - Reduce cheese weight, ensuring it doesn't go negative
- `increase_weight()` - Increase cheese weight (ignores negative values)
- `add_cheese()` - Add a cheese to the shop

**cheese-shop-cli.cpp** - Command-line interface for user interaction:
- Main menu with 6 options
- Add cheese to inventory
- Edit existing cheese
- View stock list
- Test weight reduction functionality
- Test weight increase functionality
- Helper functions for user input (string, double, integer with validation)

**cheese-shop-test.cpp** - Comprehensive unit tests using Catch2:
- 14 test cases covering all functionality
- Tests for cheese initialization with default and custom values
- Tests for string conversion with full and partial details
- Tests for cost calculation
- Tests for weight reduction (normal, exceeding, negative values)
- Tests for weight increase
- Tests for adding cheeses to shop

## Compilation

### Test Executable
```bash
clang++ cheese-shop.cpp cheese-shop-test.cpp catch_amalgamated.cpp -l splashkit -o test -Wall
```

### CLI Executable
```bash
clang++ cheese-shop.cpp cheese-shop-cli.cpp -l splashkit -o cheese-shop -Wall
```

## Running the Program

### Run Tests
```bash
./test
```

Expected output: All 32 assertions in 14 test cases pass

### Run Interactive CLI
```bash
./cheese-shop
```

## Features Implemented

### Iteration 1 (From guided tour)
✓ Cheese data structure with name, weight, and price
✓ `new_cheese()` function with default parameters
✓ `cheese_to_string()` function for display
✓ `total_cost()` function to calculate cheese value
✓ `reduce_weight()` function with safety checks
✓ `increase_weight()` function
✓ Basic CLI to display cheese

### Iteration 2 (From guided tour)
✓ Shop data structure containing multiple cheeses
✓ Add cheese to shop functionality
✓ Edit existing cheese (name, weight, price)
✓ Display cheese list with/without indices
✓ Select cheese from list with validation
✓ Interactive main menu
✓ Comprehensive test cases

### Additional Features
✓ Input validation for user entries
✓ Error handling for invalid operations
✓ Formatted currency output (dollars and cents)
✓ Weight reduction with overage protection
✓ Complete test coverage via TDD approach

## Usage Examples

### Add Cheese
- Select option 1 from main menu
- Enter cheese name (e.g., "Cheddar")
- Enter weight in kg (e.g., 1.5)
- Enter price per kg in cents (e.g., 2000 for $20.00)

### Edit Cheese
- Select option 2 from main menu
- Choose a cheese from the list
- Select which fields to edit

### View Stock
- Select option 3 to display all cheeses in inventory
- Shows: Name, weight, price per kg, and total cost

### Test Weight Functions
- Options 4 and 5 allow you to test weight reduction and increase
- Shows before/after state of the cheese

## Testing Strategy

All functionality follows test-driven development (TDD):
1. Tests written first to define expected behavior
2. Implementation written to pass tests
3. All 14 test cases verify core functionality
4. 100% pass rate ensures reliability

## Build Requirements

- C++ compiler (clang++ tested)
- SplashKit library (-l splashkit)
- Catch2 testing framework (included as amalgamated files)

## Notes

- Uses only SplashKit as specified (no external libraries)
- All input is validated before processing
- Program uses pass-by-reference for efficiency where appropriate
- Code includes comprehensive documentation
