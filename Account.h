//class file to store bank account information
//owner name, account number, balance, and transaction history
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <queue>
#include <semaphore>
#include <list>

struct Transaction 
{
    short type; //0 for deposit, 1 for withdrawal
    int senderAccountNumber;
    int recipientAccountNumber;
    double amount;
};

class Account 
{
    public:
        Account(std::string name, int number, double balance);
        void deposit(double amount);
        void withdraw(double amount);

        Account(const Account&) = delete; // prevent mutex copy
        Account& operator=(const Account&) = delete; // prevent mutex assignment

        std::mutex accountMutex;

        std::string getOwner() const { return owner; };
        int getAccountNumber() const { return accountNumber; };
        double getBalance() const { return accountBalance; };
        std::string PerformTransaction(Transaction transaction, std::queue<Transaction>& threadTransactionQueue, int threadid);
                            
    private:
        std::string owner;
        int accountNumber;
        double accountBalance;
};