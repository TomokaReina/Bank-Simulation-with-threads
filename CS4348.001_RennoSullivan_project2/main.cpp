#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <cstdlib>
#include <stdio.h>

using namespace std;

#define NUM_CUST_THREADS 5
#define NUM_BANK_THREADS 2
#define NUM_LOAN_THREADS 1

//function prototypes
int getrandomtask();
void exit_bank();
void bankservecust();
void loanservecust();
//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
//variables
sem_t semaphore;        //=0
sem_t customer;         //=5
sem_t bank_teller;      //=2
sem_t loan_officer;     //=1
sem_t cust_ready1, cust_ready2, finished_bank, finished_loan, tellerprint1, loanprint1, custreqteller, custreqloan;   //=0 all
sem_t mutex1, mutex2, mutex3, mutex4, mutex5;   //=1
//mutex1 is for customer enqueue for loan officer, mutex2 is for customer enqueueing for bank teller
//mutex3 is for teller dequeueing, mutex4 if for loanofficer dequeueing
//mutex5 is for entering the for loop in customer

//ending balance for each customer (used for printing at the end)
int balance [5] = {1000, 1000, 1000, 1000, 1000};
//total loan amount to print for each customer (used for printing at the end)
int printloanamount [5] = {0, 0, 0, 0, 0};

//array for customer to get bank teller number (used for printing)
int banktellernumber [5] = {7, 7, 7, 7, 7};
//requested amount array (used when printing)
int requestedamount [5] = {0, 0, 0, 0, 0};

//array with the specified task for each customer
int task [5] = {3, 3, 3, 3, 3};
//array with all the options available for a customer to request when Depositing or Withdrawing
int dorwamount [10] = {-500, -400, -300, -200, -100, 100, 200, 300, 400, 500};
//array with all the options available for a customer to request when getting a Loan
int loanamount [5] {100 , 200, 300, 400, 500};


//queue for customers to wait in for bank and loan
queue<int> bankq;
queue<int> loanq;
//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
//customer thread
void* customerstart(void * arg)
{
    srand(time(0));
    int *customer_num = (int *) arg;
    sem_wait(&customer);

    //loop to let each customer make 3 visits to bank
    sem_wait(&mutex5);
    for(int x=0; x<3; x++){

        task [*customer_num] = rand()%2;    //task used to decide whether going to bank teller or loan officer
        if(task[*customer_num] == 0){          //assigned to go to bank teller

            //int reqamount = dorwamount[rand()%10];
            //printf("\n%d\n", reqamount);



            //wait for bank teller to be ready
            sem_wait(&bank_teller);

            //wait mutex2
            sem_wait(&mutex2);
            //enqueue(customer_num)
            bankq.push(*customer_num);
            //signal bank teller that customer is ready
            sem_post(&cust_ready1);
            //signal mutex2
            sem_post(&mutex2);
            //wait for teller to store its number in array
            sem_wait(&tellerprint1);

            //put requested amount in array so bank teller can see it
            requestedamount[*customer_num] = dorwamount[rand()%10];
            //print request amount
            if(requestedamount[*customer_num] > 0)           //> 0 means making a deposit
            {
                printf("Customer %d requests of teller %d to make a deposit of $%d\n", *customer_num, banktellernumber[*customer_num], requestedamount[*customer_num]);
            }
            else if(requestedamount[*customer_num] < 0)      //< 0 means making a withdraw
            {
                int wamount = abs(requestedamount[*customer_num]);
                printf("Customer %d requests of teller %d to make a withdrawal of $%d\n", *customer_num, banktellernumber[*customer_num], wamount);
            }
            //signal bank teller that customer has made request
            sem_post(&custreqteller);

            sem_wait(&finished_bank);   //wait for bank teller to finish processing customer
            //print getting receipt
            if(requestedamount[*customer_num] > 0)           //> 0 means making a deposit
            {
                printf("Customer %d gets receipt from teller %d\n", *customer_num, banktellernumber[*customer_num]);
            }
            else if(requestedamount[*customer_num] < 0)      //< 0 means making a withdraw
            {
                printf("Customer %d gets cash and receipt from teller %d\n", *customer_num, banktellernumber[*customer_num]);
            }
        }
        else if(task[*customer_num] == 1){			        //assigned to go to loan officer

            //int reqamount = loanamount[rand()%5];


            sem_wait(&loan_officer);        //wait for loan officer to be ready

            //wait mutex1
            sem_wait(&mutex1);
            //enqueue(customer_num)
            loanq.push(*customer_num);
            //signal loan officer that customer is ready
            sem_post(&cust_ready2);
            //signal mutex1
            sem_post(&mutex1);

            //put requested amount in array so loan officer can see it
            requestedamount[*customer_num] = loanamount[rand()%5];
            //wait for loan officer to print its line1
            sem_wait(&loanprint1);
            //print request amount
            printf("Customer %d requests of loan officer to apply for a loan of $%d\n", *customer_num, requestedamount[*customer_num]);
            //signal loan officer that customer has made request
            sem_post(&custreqloan);

            //wait for loan officer to finish processing customer
            sem_wait(&finished_loan);
            //print getting receipt
            printf("Customer %d gets loan from loan officer\n", *customer_num);
        }

    }//end for loop
    sem_post(&mutex5);


    //exit bank
    printf("Customer %d departs the bank\n", *customer_num);
    sem_post(&customer);

    return NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
//bank teller thread
void* banktellerstart(void * arg)
{
    int frontnum;
    int *teller_num = (int *) arg;
    while(true)
    {
        sem_wait(&cust_ready1);         //wait for customer to be ready
        //wait mutex3
        sem_wait(&mutex3);
        //get front number in queue to store customer thread number into array
        frontnum = bankq.front();
        banktellernumber[frontnum] = *teller_num;
        //dequeue
        bankq.pop();

        printf("Teller %d begins serving customer %d\n", *teller_num, frontnum);
        //signal that teller printed its line
        sem_post(&tellerprint1);
        //wait for customer to make request
        sem_wait(&custreqteller);
        int dorwnumber = requestedamount[frontnum];        //dorw used to decide whether deposit or withdraw
        if(dorwnumber > 0)              //>0 means the customer requested a deposit
        {
            //loop to process order by increments of $100
            for(int b = 0; b < (dorwnumber/100); b++)
            {
                balance[frontnum] = balance[frontnum] + 100;
            }
            printf("Teller %d processes deposit of $%d for customer %d\n", *teller_num, dorwnumber, frontnum);
        }
        else if(dorwnumber < 0)         //<0 means the customer requested a withdraw
        {
            //because we are withdrawing, dorwnumber is negative, we need the absolute value of it
            int wamount = abs(dorwnumber);
            //loop to process order by increments of $100
            for(int a = 0; a < (wamount/100); a++)
            {
                balance[frontnum] = balance[frontnum] - 100;
            }
            printf("Teller %d processes withdrawal of $%d for customer %d\n", *teller_num, wamount, frontnum);
        }
        //signal mutex3
        sem_post(&mutex3);
        //signal process is complete
        sem_post(&finished_bank);
        //signal so another customer can go to bank teller
        sem_post(&bank_teller);
    }
}
//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
//loan officer thread
void* loanofficerstart(void * arg)
{
    int frontnum;
    while(true)
    {
        sem_wait(&cust_ready2);         //wait for customer to be ready
        //wait mutex4
        sem_wait(&mutex4);
        //get front number in queue to store customer thread number into array
        frontnum = loanq.front();
        //dequeue
        loanq.pop();

        printf("Loan Officer begins serving customer %d\n", frontnum);
        //signal that loan officer has printed line 1
        sem_post(&loanprint1);
        //wait for customer to make request
        sem_wait(&custreqloan);
        int loannumber = requestedamount[frontnum];     //used to store the number requested
        printf("Loan Officer approves loan for customer %d\n", frontnum);

        //loop to process order by increments of $100
        for(int y = 0; y < (loannumber/100); y++)
        {
            printloanamount[frontnum] = printloanamount[frontnum] + 100;
        }

        //signal mutex4
        sem_post(&mutex4);
        //signal process is complete
        sem_post(&finished_loan);
        //signal so another customer can go to loan officer
        sem_post(&loan_officer);
    }
}
//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------
int main()
{
    cout << "Thread activity" << endl << endl;

    //variables
    int rc;
    //create pthreads
    pthread_t cust_threads[NUM_CUST_THREADS];
    pthread_t bank_threads[NUM_BANK_THREADS];
    pthread_t loan_threads[NUM_LOAN_THREADS];

    //initialize semaphores
    if (sem_init(&semaphore,0,0) == -1)
    {
        printf("initialize semaphore\n");
        exit(1);
    }
    if (sem_init(&customer,0,5) == -1)
    {
        printf("initialize customer\n");
        exit(1);
    }
    if (sem_init(&bank_teller,0,2) == -1)
    {
        printf("initialize bank_teller\n");
        exit(1);
    }
    if (sem_init(&loan_officer,0,1) == -1)
    {
        printf("initialize loan_officer\n");
        exit(1);
    }
    if (sem_init(&cust_ready1,0,0) == -1)
    {
        printf("initialize cust_ready1\n");
        exit(1);
    }
    if (sem_init(&cust_ready2,0,0) == -1)
    {
        printf("initialize cust_ready2\n");
        exit(1);
    }
    if (sem_init(&finished_bank,0,0) == -1)
    {
        printf("initialize finished_bank\n");
        exit(1);
    }
    if (sem_init(&finished_loan,0,0) == -1)
    {
        printf("initialize finished_loan\n");
        exit(1);
    }
    if (sem_init(&tellerprint1,0,0) == -1)
    {
        printf("initialize tellerprint1\n");
        exit(1);
    }
    if (sem_init(&loanprint1,0,0) == -1)
    {
        printf("initialize loanprint1\n");
        exit(1);
    }
    if (sem_init(&custreqteller,0,0) == -1)
    {
        printf("initialize custreqteller\n");
        exit(1);
    }
    if (sem_init(&custreqloan,0,0) == -1)
    {
        printf("initialize custreqloan\n");
        exit(1);
    }
    if (sem_init(&mutex1,0,1) == -1)
    {
        printf("initialize mutex1\n");
        exit(1);
    }
    if (sem_init(&mutex2,0,1) == -1)
    {
        printf("initialize mutex2\n");
        exit(1);
    }
    if (sem_init(&mutex3,0,1) == -1)
    {
        printf("initialize mutex3\n");
        exit(1);
    }
    if (sem_init(&mutex4,0,1) == -1)
    {
        printf("initialize mutex4\n");
        exit(1);
    }
    if (sem_init(&mutex5,0,1) == -1)
    {
        printf("initialize mutex5\n");
        exit(1);
    }



    //create 2 bank threads
    for(int i = 0; i < NUM_BANK_THREADS; i++)
    {
        int *pnum = (int*)malloc(sizeof(int));
        *pnum = i;
        printf("teller %d created\n", i);
        rc = pthread_create(&bank_threads[i], NULL, banktellerstart, (void*)pnum);

        if(rc)
        {
            printf("Error:unable to create BANK thread, %d\n", rc);
            exit(-1);
        }
    }
    //create 1 loan officer thread
    for(int i = 0; i < NUM_LOAN_THREADS; i++)
    {
        int *pnum = (int*)malloc(sizeof(int));
        *pnum = i;
        printf("loan officer %d created\n", i);
        rc = pthread_create(&loan_threads[i], NULL, loanofficerstart, (void*)pnum);

        if(rc)
        {
            printf("Error:unable to create LOANOFFICER thread, %d\n", rc);
            exit(-1);
        }
    }
    //create 5 customer threads
    for(int i = 0; i < NUM_CUST_THREADS; i++)
    {
        int *pnum = (int*)malloc(sizeof(int));
        *pnum = i;
        printf("customer %d created\n", i);
        rc = pthread_create(&cust_threads[i], NULL, customerstart, (void*)pnum);
        if(rc)
        {
            printf("Error:unable to create CUSTOMER thread, %d\n", rc);
            exit(-1);
        }
    }



    //join customer
    int i = 0;
    while(i < NUM_CUST_THREADS)
    {
        rc = pthread_join(cust_threads[i], NULL);
        if(rc != 0)
        {
            printf("Error joining CUSTOMER thread\n");
            exit(-1);
        }
        printf("customer %d is joined by main\n", i);
        //increment i
        i++;
    }
    /*
    for(int i = 0; i < NUM_CUST_THREADS; i++)
    {
        rc = pthread_join(cust_threads[i], NULL);
        if(rc)
        {
            printf("Error joining CUSTOMER thread\n");
            exit(-1);
        }
        printf("customer %d is joined by main\n", i);
    }*/



    //destroy semaphores
    sem_destroy(&semaphore);
    sem_destroy(&customer);
    sem_destroy(&bank_teller);
    sem_destroy(&loan_officer);
    sem_destroy(&cust_ready1);
    sem_destroy(&cust_ready2);
    sem_destroy(&finished_bank);
    sem_destroy(&finished_loan);
    sem_destroy(&mutex1);
    sem_destroy(&mutex2);
    sem_destroy(&mutex3);
    sem_destroy(&mutex4);
    sem_destroy(&mutex5);

    //calculate total ending balance
    int totalendingbalance;
    for(int i = 0; i < 5; i++)
    {
        totalendingbalance += balance[i];
    }
    //calculate total loan amount
    int totalloanamount;
    for(int i = 0; i < 5; i++)
    {
        totalloanamount += printloanamount[i];
    }
    //print values at the end
    printf("\n\n\n");
    printf("\t\tBank Simulation Summary\n\n");
    printf("\t\tEnding balance	Loan Amount\n\n");
    printf("Customer 0\t%d\t\t%d\n", balance[0], printloanamount[0]);
    printf("Customer 1\t%d\t\t%d\n", balance[1], printloanamount[1]);
    printf("Customer 2\t%d\t\t%d\n", balance[2], printloanamount[2]);
    printf("Customer 3\t%d\t\t%d\n", balance[3], printloanamount[3]);
    printf("Customer 4\t%d\t\t%d\n\n", balance[4], printloanamount[4]);
    printf("Totals\t\t%d\t\t%d\n", totalendingbalance-1, totalloanamount);
    return 0;
}

