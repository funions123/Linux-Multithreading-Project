#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "Account.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "json.hpp"
#include "Loader.cpp"
#include <queue>
#include <mutex>
#include <list>

using json = nlohmann::json;

//global variables
//mutex to lock access to the main transaction queue so threads can write to it thread-safely
std::mutex mainQueueMutex;
std::queue<Transaction> mainTransactionQueue;

//global list of account pointers
std::list<std::shared_ptr<Account>> accounts;

//string to contain the log output
std::string logString = "Starting program...\n";
//mutex to lock access to the log string so threads can write to it thread-safely
std::mutex logMutex;

int pipefd[2];

void threadFunction(int threadNumber, std::queue<Transaction>& threadTransactionQueue)
{
    //iteration through the thread's transaction queue
    while (!threadTransactionQueue.empty())
    {
        Transaction transaction = threadTransactionQueue.front();
        threadTransactionQueue.pop();

        //0 for deposit, 1 for withdrawal
        std::string logOutput = "";
        if(transaction.type == 0)
        {
            //find the recipient account using shared_ptr
            auto accountIter = std::find_if(accounts.begin(), accounts.end(), [&](const std::shared_ptr<Account>& account)
            { 
                return account->getAccountNumber() == transaction.recipientAccountNumber; 
            });
            if (accountIter != accounts.end()) {
                logOutput = (*accountIter)->PerformTransaction(transaction, threadTransactionQueue, threadNumber);
            }
        }
        else
        {
            //find the sender account and process the withdrawal
            auto accountIter = std::find_if(accounts.begin(), accounts.end(), [&](const std::shared_ptr<Account>& account)
            {
                return account->getAccountNumber() == transaction.senderAccountNumber;
            });
            if (accountIter != accounts.end()) {
                logOutput = (*accountIter)->PerformTransaction(transaction, threadTransactionQueue, threadNumber);
            }
        }
        if(!logOutput.empty())
        {
            //lock log mutex
            std::lock_guard<std::mutex> lock(logMutex);
            //critical section
            logString += logOutput;

            //send logOutput to the child process
            write(pipefd[1], logOutput.c_str(), logOutput.length());
        }
    }
}

int main(int argc, char* argv[]) 
{
    std::ofstream outFile("output.txt");

    //load accounts from json file
    accounts = loadAccountsFromJSON("accounts.json");
    //load transactions from json file
    mainTransactionQueue = loadTransactionsFromJSON("transactions.json");
    
    const int numThreads = 4;
    std::vector<std::thread> threads;

    std::vector<std::queue<Transaction>> threadTransactionQueues;

    //assemble a transaction queue for each thread
    for(int i = 0; i < numThreads; ++i) 
    {   
        threadTransactionQueues.push_back(std::queue<Transaction>());
        if(mainTransactionQueue.empty())
        {
            continue;
        }
        else
        {
            threadTransactionQueues.at(i).push(mainTransactionQueue.front());
            mainTransactionQueue.pop();
        }
    }

    //create pipe and subproccess
    if(pipe(pipefd) == -1) 
    {
        std::cout << "pipe error" << std::endl;
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if(pid == -1) 
    {
        std::cout << "fork error" << std::endl;
        perror("fork");
        return 1;
    }

    if(pid == 0) 
    {
        // Child process
        close(pipefd[1]); // Close unused write end

        char buffer[1024];
        ssize_t nbytes;
        while ((nbytes = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) 
        {
            buffer[nbytes] = '\0';
            if (outFile.is_open()) 
            {
                outFile << buffer;
            } 
            else 
            {
                std::cout << "Unable to open file for writing.\n";
            }

            if (strcmp(buffer, "STOP") == 0) {
                outFile.close();
                break; // Exit the loop if the STOP string is received
            }
            std::cout << "Child received: " << buffer << std::endl;
        }
        close(pipefd[0]);
        _exit(0);
    }
    else //parent process
    {
        //start threads
        for(int i = 0; i < numThreads; ++i) 
        {
            threads.emplace_back(threadFunction, std::ref(i), std::ref(threadTransactionQueues.at(i)));
        }

        //join the threads with the main thread (wait for them to finish)
        for(auto& t : threads) 
        {
            t.join();
        }
        
        std::cout << "writing stop to pipe" << std::endl;
        write(pipefd[1], "STOP", 4);
        close(pipefd[1]); // Close write end after finishing writing
        wait(nullptr);    // Wait for child process to finish

        std::cout << "All threads have completed execution." << std::endl;
        
        std::cout << "Press any key to exit..." << std::endl;
        std::cin.ignore();

        return 0;
    }
}