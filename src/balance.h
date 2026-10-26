#pragma once

// Dining points cannot go below zero, including after a purchase.
class Balance {
public:
    // Throws std::invalid_argument if startingBalance is negative.
    Balance(double startingBalance = 0.0);

    // Throws std::invalid_argument if amount is negative.
    void setBalance(double amount);

    double getBalance() const;

    // Throws std::invalid_argument if amount is negative.
    void add(double amount);

    // Throws std::invalid_argument if amount is negative or exceeds the balance.
    void subtract(double amount);

private:
    double balance;
};
