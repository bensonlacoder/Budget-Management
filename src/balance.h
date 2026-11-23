#pragma once

// Dining points cannot go below zero, including after a purchase.
// Every stored value is rounded to the nearest hundredth of a point.
class Balance {
public:
    // Throws std::invalid_argument if startingBalance is negative or not finite.
    Balance(double startingBalance = 0.0);

    // Throws std::invalid_argument if amount is negative or not finite.
    void setBalance(double amount);

    double getBalance() const;

    // Throws std::invalid_argument if amount is negative or not finite.
    void add(double amount);

    // Throws std::invalid_argument if amount is negative, not finite, or exceeds the balance.
    // A rejected change leaves the balance unchanged.
    void subtract(double amount);

private:
    double balance;
};
