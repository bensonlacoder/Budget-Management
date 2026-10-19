// Balance model class.
// Responsibilities: set, view, add to, and subtract from the student's
// dining point balance.
// Knows: Balance

#pragma once

class Balance {
public:
    // Starts at the given balance (defaults to 0). Throws
    // std::invalid_argument if the starting balance is negative.
    explicit Balance(double startingBalance = 0.0);

    // Replaces the current balance. Throws std::invalid_argument if negative.
    void setBalance(double amount);

    // Returns the current remaining balance.
    double getBalance() const;

    // Adds dining points (e.g. a refill). Throws std::invalid_argument if
    // amount is negative.
    void add(double amount);

    // Subtracts dining points (e.g. a purchase). Throws std::invalid_argument
    // if amount is negative or larger than the current balance.
    void subtract(double amount);

private:
    double balance;
};
