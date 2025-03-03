#include "Account.h"

extern std::list<Account> accounts;
extern std::string logString;
extern std::mutex logMutex;

Account::Account(std::string name, int number, double balance) :
    owner(name), accountNumber(number), accountBalance(balance) {}

std::string Account::PerformTransaction(Transaction transaction, std::queue<Transaction>& threadTransactionQueue, int threadid)
{
    //acquire account semaphore
    std::lock_guard<std::mutex> lock(accountMutex);
    //critical section
    if(transaction.type == 0) //deposits
    {
        accountBalance += transaction.amount;
        return "[DEPOSIT][" + std::to_string(threadid) + "] " + std::to_string(transaction.amount) + " deposited into account " + std::to_string(transaction.recipientAccountNumber) + ".\n";
    }
    else //withdrawals require actual error handling
    {
        if(accountBalance-transaction.amount < 0) //insufficient funds
        {
            return "[WITHDRAWAL][" + std::to_string(threadid) + "] Insufficient funds.\n";
        }
        else //withdrawal successful
        {
            //generate a new deposit transaction
            Transaction depositTransaction;
            depositTransaction.type = 0;
            depositTransaction.senderAccountNumber = transaction.senderAccountNumber;
            depositTransaction.recipientAccountNumber = transaction.recipientAccountNumber;
            depositTransaction.amount = transaction.amount;
        
            threadTransactionQueue.push(depositTransaction);
            //update the account's balance
            accountBalance -= transaction.amount;
        }
        return "[WITHDRAWAL][" + std::to_string(threadid) + "] " + std::to_string(transaction.amount) + " withdrawn from account " + std::to_string(transaction.senderAccountNumber) + ".\n";
    }
    //account mutex auto unlocks when out of scope
}