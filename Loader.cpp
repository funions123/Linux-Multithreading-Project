#include "json.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include "Account.h"
#include <queue>
#include <vector>
#include <list>

using json = nlohmann::json;

extern std::list<std::shared_ptr<Account>> accounts;
extern std::mutex logMutex;
extern std::string logString;

//helper function to load a json file
static inline json loadJSONFile(const std::string& filename)
{
	std::ifstream file(filename);
	json j;
	if(file.is_open())
	{
		try
		{
			file >> j;
		}
		catch(const std::exception& e)
		{
			std::cout << e.what() << '\n';
		}
		catch(...)
		{
			std::cerr << "Some exception while parsing the file json!\n";
		}
	}
	else
	{
		//Could not find file
		std::cout << "File: " << filename << " could not be found while loading.\n";
	}
	return j;
}

//function to load all accounts from the provided json file
static inline std::list<std::shared_ptr<Account>> loadAccountsFromJSON(const std::string& filename)
{
	json j = loadJSONFile(filename);
	std::list<std::shared_ptr<Account>> accountlist;
	for (auto& [key, account] : j.items())
	{
        std::string account_name = account["name"].get<std::string>();
        int account_number = account["number"].get<int>();
        double account_balance = account["balance"].get<double>();

		accountlist.emplace_back(std::make_shared<Account>(account_name, account_number, account_balance));
	}
	return accountlist;
}

//function to load all transactions from the provided json file
static inline std::queue<Transaction> loadTransactionsFromJSON(const std::string& filename)
{
    json j = loadJSONFile(filename);
    std::queue<Transaction> transactions;
    for (auto& [key, transaction] : j.items())
    {
        short transaction_type = transaction["type"].get<short>();
        int sender_account_number = transaction["senderAccountNumber"].get<int>();
        int recipient_account_number = transaction["recipientAccountNumber"].get<int>();
        double transaction_amount = transaction["amount"].get<double>();

		//search accounts for sender and recipient
		auto senderAccountIter = std::find_if(accounts.begin(), accounts.end(), [&](const std::shared_ptr<Account>& account)
		{
			return account->getAccountNumber() == sender_account_number;
		});
		auto recipientAccountIter = std::find_if(accounts.begin(), accounts.end(), [&](const std::shared_ptr<Account>& account)
		{
			return account->getAccountNumber() == recipient_account_number;
		});

		//ensure both accounts exist
		if(!(senderAccountIter == accounts.end() && recipientAccountIter == accounts.end())) 
		{
			Transaction t = Transaction(transaction_type, sender_account_number, recipient_account_number, transaction_amount);
			transactions.push(t);
		}
		else
		{
			//write error to log
			std::lock_guard<std::mutex> lock(logMutex);
			logString += "[ERROR] Transaction " + key + " could not be processed because one or both accounts could not be found.\n";
		}

        Transaction t = Transaction(transaction_type, sender_account_number, recipient_account_number, transaction_amount);
        transactions.push(t);
    }
    return transactions;
}

