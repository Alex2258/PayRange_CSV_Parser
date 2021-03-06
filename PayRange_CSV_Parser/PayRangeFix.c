/*
    This program was designed by Alex Arwin
    a current employee of Gator Vending
    for use solely by Gator Vending, Inc.
    It's sole purpose is designed to minimize the
    grunt work it currently takes to process a
    weekly PayRange Report.

    Current Method:
        Download File
        Manually extract useful information
        Manually adjust columns/rows to readily show information
        Manually total columns with same Display Names
        Enter into our accounting software

    Goal Method:
        Download File
        Run .exe
        Enter into our accounting software

    Assumptions Made:
        The Downloaded PayRange File does not significantly change format styles over time
        The Number of Rows in the PayRange File is not extremely large(excess of 1000)
            --This is mentioned because the methods used to extract, parse, and sort
              the data is not exactly efficient and I am okay with this as the data set
              is never expected to grow much larger than 500 lines.
        No Line in the PayRange File is longer than 200 characters
        No Display Name is longer than 80 characters.
            --These numbers were chosen after carefully examining the file being parsed in
              its original form. They allow some minor expansion to occur without causing
              a parsing problem.

    Note(s):
        The Data Structure being implemented is Linked Lists
        The Sorting Algorithm is Bubble Sort
        The <zstring.h> is a custom library created with a sole
            purpose of fixing many of the pitfalls the more
            common <string.h> standard library experiences. More specifically
            I make use of it to avoid skipping tokens with no values
            (like strtok()) currently does. This allows me to account
            for columns that are blank/empty.

*/
//Header & Library File(s)
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zstring.h"

//Constant(s)
#define NOTVERIFIED -1
#define VERIFIED 1
#define MAX_STRING_LEN 80
#define MAX_CSV_LEN 200

//Struct(s)
typedef struct row
{
    char dName[MAX_STRING_LEN]; //Name of PayRange Location
    char mobileAmt[MAX_STRING_LEN]; //Raw Amount Before Reductions
    char feeAmt[MAX_STRING_LEN]; //Fee Amt (%-baseD)
    char discountAmt[MAX_STRING_LEN];//Amt of Discounts Given
    char netAmt[MAX_STRING_LEN]; //Net Amt after Discount & Fee
    float totalAmt; //Total Amt used to total accounts (Mobile minus Discounts)

    struct row* next; //pointer to next item in LL
} row;

//Global Variable(s)
char filename[MAX_STRING_LEN];
const char comma[2] = ",";
const char dName[13] = "Display Name";
const char mobileAmt[7] = "Mobile";
const char discountAmt[10] = "Discounts";
const char feeAmt[4] = "Fee";
const char netAmt[6] = "Net\n";

/*
For this program to work dynamically with PayRange
we need to search the headers and assign what column the data
we're looking for is in so we can skip extraneous data and only
store the desired data
*/
int col_pos_dName;
int col_pos_mobileAmt;
int col_pos_discountAmt;
int col_pos_feeAmt;
int col_pos_netAmt;
int totalColumns = 0;
int totalNodes = 0;

struct row* root = NULL;//Root of the Linked List. No Mobile,Discount, but Fee Exists, Keep these seperate.
struct row* head = NULL;//Head of the Linked List

//Function Declaration(s)/Prototype(s)
void run(void);
void verifyFileName(void);
void parseHeaders(void);
void parsePayRangeFile(void);
void alternativeSort(void);
void writePayRangeFile(void);
void printHeaders(FILE*);
void printRows(FILE*, struct row*);
void printTotal(FILE*);
void freeList(struct row*);

//--Used to Debug During Development
void showHead(void);
void showNode(struct row*);
void showList(void);
void countPause(int);

//--Helper Functions
void nullify(struct row*);
char* concat(const char*, const char*);
void removeNoCashNodes(void);
bool isLetter(char);
bool isNextMatch(char [MAX_STRING_LEN], char [MAX_STRING_LEN]);
bool moneyExists(struct row*);
bool moneyCheck(char [MAX_STRING_LEN]);
float strToFloat(char [MAX_STRING_LEN], char [MAX_STRING_LEN]);
char* removeNewLine(char [MAX_STRING_LEN]);

//Main
int main(int argc, char *argv[])
{
    // NOTE ARGC = Argument Count && Argv = Argument Vector
    //filename = *argv[0];

    /*
    strcpy(filename, "PayRange417to423");
    run();
    system("pause");

    strcpy(filename, "PayRange424to430");
    run();
    system("pause");

    strcpy(filename, "PayRange51to57");
    run();
    system("pause");

    strcpy(filename, "PayRange58to514");
    run();
    */

    strcpy(filename, "test1234");
    run();

    return;
}//END main

//Function(s)
void run()
{
    printf("Verifying File Exists...\n");
    verifyFileName();

    printf("Reading Column Headers...\n");
    parseHeaders();

    printf("Parsing File Now...\n");
    parsePayRangeFile();

    printf("Sorting File Now...\n");
    alternativeSort();

    printf("Creating New File...\n");
    writePayRangeFile();

    printf("Completed! Check new file and rename it!\n");

    return;
}

void verifyFileName()
{
    //Local Variable(s)
    int flag = NOTVERIFIED;
    char str[MAX_STRING_LEN];
    FILE* stream;
    char *s;

    while(flag == NOTVERIFIED)
    {
        s = concat(filename, ".csv");
        stream = fopen(s, "r"); //Attempt to Open File

        if(stream == NULL) //If failed to open file
        {
            fclose(stream); //Close the file

            printf("Error: File failing to open.\nPlease verify the name is correct and try again: ");
            gets(str); //Ask for correct filename && try again
            strcpy(filename, str);
            printf("\n");
        }
        else
        {
            printf("File Found...\n");
            fclose(stream); //Close the file
            flag = VERIFIED;
        }
    }

    return;
}//END verifyFileName

void parseHeaders()
{
    //Local Variable(s)
    char str[MAX_CSV_LEN];
    char *token, *s;
    FILE* stream;
    int col = 0;

    s = concat(filename, ".csv");
    stream = fopen(s, "r"); //Open File
    fgets(str, MAX_CSV_LEN, stream); //Get First Line (column headers)

    token = strtok(str, comma); //Grab First Token from First Line String (delimited by commas)

    while(token != NULL) //while there are tokens remaining
    {
        col++;

        if(strcmp(token, dName) == 0)
        {
            col_pos_dName = col; //retain col # of dName header
        }
        else if(strcmp(token, mobileAmt) == 0)
        {
            col_pos_mobileAmt = col; //retain col # of mobileAmt header
        }
        else if(strcmp(token, discountAmt) == 0)
        {
            col_pos_discountAmt = col; //retain col # of discountAmt header
        }
        else if(strcmp(token, feeAmt) == 0)
        {
            col_pos_feeAmt = col; //retain col # of feeAmt header
        }
        else if(strcmp(token, netAmt) == 0)
        {
            col_pos_netAmt = col; //retain col # of netAmt header
        }

        token = strtok(NULL, comma); //get next token
    }

    totalColumns = col;//Retain total amount of columns seen in the file

    fclose(stream); //Close File

    return;
}//END parseHeaders

void nullify(struct row* temp)
{
    strcpy(temp->dName,"\0");
    strcpy(temp->mobileAmt,"\0");
    strcpy(temp->discountAmt, "\0");
    strcpy(temp->feeAmt, "\0");
    strcpy(temp->netAmt,"\0");

    return;
}

void parsePayRangeFile()
{
    //Local Variable(s)
    FILE* stream;
    char str[MAX_CSV_LEN];
    char *token, *s;
    int currCol = 2;//Once we grab a line with data we're interested in, we'll actually be in column two (based on csv file format)
    int initialNode = NOTVERIFIED;
    struct row* temp = (struct row*)malloc(sizeof(struct row));

    s = concat(filename,".csv");
    stream = fopen(s, "r"); //Open File
    fgets(str, MAX_CSV_LEN, stream); //Get First Line (discard, do not need as it is only headers)

    while(fgets(str, MAX_CSV_LEN, stream) != NULL)//while there are lines left to read, get one.
    {
        while(strlen(str) < MAX_STRING_LEN)
        {
            fgets(str, MAX_CSV_LEN, stream); //skip this line, not what we're looking for
        }

        token = zstring_strtok(str, comma); //Get First Token from newly grabbed line from file

        while(token != NULL) //while there are tokens remaining
        {
                /* Determine if the current column is data we need, and store it accordingly */
                if(currCol == col_pos_dName)
                    strcpy(temp->dName, token);
                else if(currCol == col_pos_mobileAmt)
                    strcpy(temp->mobileAmt, token);
                else if(currCol == col_pos_discountAmt)
                    strcpy(temp->discountAmt, token);
                else if(currCol == col_pos_feeAmt)
                    strcpy(temp->feeAmt, token);
                else if(currCol == col_pos_netAmt)
                    strcpy(temp->netAmt, removeNewLine(token));

            if(currCol == totalColumns)//found last item for this node, save it and create new node
            {
                //showNode(temp);

                if(moneyExists(temp))//Only add node to list if it has money
                {
                    //Change the String: '$x.xx' to a float value, used when writing new csv file
                    temp->totalAmt = strToFloat(temp->mobileAmt,temp->discountAmt);

                    if(initialNode == NOTVERIFIED)
                    {
                        //printf("\nAdding Head Now!");
                        temp->next = (struct row*)malloc(sizeof(struct row)); //Allocate Mem for Next Node
                        head = temp;//link head to the first node created and assigned
                        temp = temp->next;//move temp to newly created blank node

                        initialNode = VERIFIED;//mark first(head) node as verified
                    }
                    else
                    {
                        //printf("\nAdding Middle of List Now!");
                        temp->next = (struct row*)malloc(sizeof(struct row)); //Allocate Mem for Next Node
                        temp = temp->next;
                    }
                }
                else
                {
                    //printf("\nNot Adding Node Now!");
                }

                currCol = 2; //reset counter
                token = NULL;//reset our token
                totalNodes++;//increment our totalNodes(total rows) counter

            }
            else
            {
                token = zstring_strtok(NULL, comma); //get next token
                currCol++; //adjust column over
            }
        }
    }


    return;
}//End parsePayRangeFile

void alternativeSort()
{
    showList();
    /*
    strcmp(s1,s2)

    strcmp returns neg int if stop char in s1 was LESS than in s2 (s1 < s2) s1 stopping char was closer to a than s2
           returns pos int if stop char in s1 was MORE than in s2 (s1 > s2) s2 stopping char was closer to a then s1
           returns 0 if equal

           ASCII
           65(A) to 90(Z)
           97(a) to 122(z)
           (difference of 32)
    */

    //Local Variable(s)
    struct row* temp;      //Used to avoid chance of losing HEAD
    struct row* prevHold;  //Used to retain location of the previous node to use during swapping
    struct row* hold;      //Used to store data that needs to be saved when swapping
    bool swapOccured = true;
    bool beginFlag;

    while(swapOccured == true)//while a swap was made in the prev iteration
    {
        beginFlag = true;
        swapOccured = false;
        temp = head;

        while(temp->next != NULL) //while not at end of list
        {
            //Case 1: Swap Needed
            if((strcmp(temp->dName,temp->next->dName)) > 0)//indicates the first node is lower in alphabet then second, swap needed
            {
                if(beginFlag == true)//Case 1: Head of List
                {
                    head = temp->next;//Move Head Pointer to newly swapped Node
                    prevHold = temp->next;//Maintain location of one node back so we can set its next ptr when swapping
                    temp->next = temp->next->next;//Curr Node now points to TWO Ahead
                    head->next = temp;//Ahead now points to Curr
                    swapOccured = true; //Set Swap Flag
                    beginFlag = false; //Set head of list flag
                }
                else if(temp->next->next == NULL)//Case 2: Next Node is the last node in list
                {
                    hold = temp->next;
                    prevHold->next = temp->next;
                    temp->next= NULL;
                    hold->next = temp;
                }
                else//Case 3: Middle of List
                {
                    hold = temp->next;
                    prevHold->next = temp->next;
                    prevHold = temp->next;//Maintain location of one node back so we can set its next ptr when swapping
                    temp->next = temp->next->next;//Curr Node now points to TWO Ahead
                    hold->next = temp;//Ahead now points to Curr
                    swapOccured = true; //Set Swap Flag
                }
            }
            //Case 2: Swap NOT Needed
            else
            {
                prevHold = temp;
                temp = temp->next;//Go to next node
                beginFlag = false; //Set head of list flag
            }
        }
    }

    return;
}

void writePayRangeFile()
{
    //Local Variable(s)
    FILE* stream;
    struct row* temp = head;  //temp used to avoid loss of head pointer
    char* s = concat(filename, "_parsed.csv");

    stream = fopen(s, "w"); //Open a new file to write proper data into

    //PRINT HEADERS INTO FILE
    printHeaders(stream);
    printRows(stream, temp);
    printTotal(stream);

    fclose(stream);

    freeList(head);
    free(s);
    return;

}//END writePayRangeFile

void printHeaders(FILE* stream)
{
    fprintf(stream, "Display Name,");
    fprintf(stream, "Mobile,");
    fprintf(stream, "Discounts,");
    fprintf(stream, "Fee,");
    fprintf(stream, "Net,");
    fprintf(stream, "Total\n");
    return;
}//END printHeaders

void printRows(FILE* stream, struct row* temp)
{
    showHead();
    system("pause");
    int flag = NOTVERIFIED;   //flag for end of list
    float totalAmt= 0.00;     //used to store total amounts between nodes of the same account

    while(flag == NOTVERIFIED)
    {
        fprintf(stream, temp->dName); fprintf(stream, ",");//Write Display Name & Comma
        fprintf(stream, temp->mobileAmt); fprintf(stream, ",");//Write Mobile Amt & Comma
        fprintf(stream, temp->discountAmt);fprintf(stream, ",");//Write Discount Amt & Comma
        fprintf(stream, temp->feeAmt); fprintf(stream, ",");//Write Fee Amt & Comma
        fprintf(stream, temp->netAmt); fprintf(stream, ",");//Write Net Amt & Comma


        if(temp->next != NULL)//If there are still more items to come &
        {
            if(isNextMatch(temp->dName,temp->next->dName))//CurrName & NextName Match, don't write total
            {
                fprintf(stream, "\n");
                totalAmt += temp->totalAmt;
            }
            else//not a match, write total
            {
                totalAmt += temp->totalAmt;
                fprintf(stream, "$%.2f\n", totalAmt);
                totalAmt = 0.00;
            }
        }
        else//at last node, write total
        {
            totalAmt += temp->totalAmt;
            fprintf(stream, "$%.2f\n", totalAmt);
        }


        if(temp->next == NULL)
            flag = VERIFIED; //We're at the end of list, using temp so we dont use head of list location
        else
            temp = temp->next; //move to next row
    }

    return;
}//END printRows

void printTotal(FILE* stream)
{
    struct row* temp = root;

    while(temp != NULL)
    {
        fprintf(stream, temp->dName); fprintf(stream, ",");//Write Display Name & Comma
        fprintf(stream, temp->mobileAmt); fprintf(stream, ",");//Write Mobile Amt & Comma
        fprintf(stream, temp->discountAmt);fprintf(stream, ",");//Write Discount Amt & Comma
        fprintf(stream, temp->feeAmt); fprintf(stream, ",");//Write Fee Amt & Comma
        fprintf(stream, temp->netAmt); fprintf(stream, ",");//Write Net Amt & Comma

        system("pause");
        temp->totalAmt =- (strToFloat(temp->feeAmt,"$0.00"));
        system("pause");

        fprintf(stream, "$%.2f\n", temp->totalAmt);

        temp = temp->next;
    }

    fprintf(stream, "\n");
    fprintf(stream, "\n");
    fprintf(stream, "Totals:");
    return;

}//END printTotal

void freeList(struct row* node)
{
    struct row* temp;

    while(node->next != NULL)//while there are nodes remaining to free
    {
        temp = node->next;//move temp to next node over
        free(node);//free curr node
        node = temp;//set root to next node
    }

    free(node);
    free(temp);
    return;
}

float strToFloat(char mobileAmt[MAX_STRING_LEN],char discountAmt[MAX_STRING_LEN])
{
    float net = 0.00;
    float amt;
    int i, j;
    int length = strlen(mobileAmt);

    /*
        mobileAmt in form:   '$0.00'
        discountAmt in form: '$0.00'
    */

    //Determine where the '.' is:
    for(i = 0; i < length; i++)//loop through string
    {
        if(mobileAmt[i] == '.')//once we find our decimal marker
        {
            //Get & Add Hundredths Place
            amt = mobileAmt[i+2] - '0';
            net += 0.01*amt;

            //Get & Add Tenths Place
            amt = mobileAmt[i+1] - '0';
            net += 0.1*amt;

            //Get & Add Ones Place
            amt = mobileAmt[i-1] - '0';
            net += 1.0*amt;

            if(i == 3)
            {
                //Get & Add Tens Place IF IT EXISTS
                amt = mobileAmt[i-2] - '0';
                net += 10.0*amt;
            }

            if(i == 4)
            {
                //Get & Add Hundreds Place IF IT EXISTS
                amt = mobileAmt[i-3] - '0';
                net += 100.0*amt;
            }
        }
    }

    //Repeat same process for discountAmt
    length = strlen(discountAmt);

    //Determine where the '.' is:
    for(i = 0; i < length; i++)//loop through string
    {
        if(discountAmt[i] == '.')//once we find our decimal marker
        {
            //Get & Sub Hundredths Place
            amt = discountAmt[i+2] - '0';
            net -= 0.01*amt;

            //Get & Sub Tenths Place
            amt = discountAmt[i+1] - '0';
            net -= 0.1*amt;

            //Get & Sub Ones Place
            amt = discountAmt[i-1] - '0';
            net -= 1.0*amt;

            if(i == 3)
            {
                //Get & Sub Tens Place IF IT EXISTS
                amt = discountAmt[i-2] - '0';
                net -= 10.0*amt;
            }

            if(i == 4)
            {
                //Get & Sub Hundreds Place IF IT EXISTS
                amt = discountAmt[i-3] - '0';
                net -= 100.0*amt;
            }
        }
    }

    return net;
}//END strToInt

char* removeNewLine(char netAmt[MAX_STRING_LEN])
{
    char* pos;

    if((pos=strchr(netAmt, '\n')) != NULL)//Case 1: Has NewLineChar, must remove it
    {
         *pos = '\0';
         return netAmt;
    }
    else//Case 2: Has no newline char
    {
        return netAmt;
    }
}//END removeNewLine

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator

    strcpy(result, s1);
    strcat(result, s2);

    return result;
}

bool isLetter(char ch)
{
    /*
       ASCII
       65(A) to 90(Z)
       97(a) to 122(z)
       (difference of 32)
    */
        if((64 < ch) && (ch < 91))//lowercase
        {
            return true;
        }
        else if((96 < ch) && (ch < 123))//uppercase
        {
            return true;
        }
        else//not a char
        {
            return false;
        }
}//END isLetter

bool isNextMatch(char currName[MAX_STRING_LEN],char nextName[MAX_STRING_LEN])
{
    int i;

    int currLength = strlen(currName);//get length of current node's dName
    int currSeperatorLoc;

    int nextLength = strlen(nextName);//get length of next node's dName
    int nextSeperatorLoc;

    /*
    find the '-' seperator. Characters before the '-' will be
    compared to see if they are from the same account so
    they can be grouped together. used for totaling amts for a single account
    with multiple machines
    */
    for(i = 0; i<currLength ;i++)
    {
        if(currName[i] == '-')
        {
            currSeperatorLoc = i;
        }
    }

    for(i = 0; i<nextLength ;i++)
    {
        if(nextName[i] == '-')
        {
            nextSeperatorLoc = i;
        }
    }

    if(currSeperatorLoc != nextSeperatorLoc)//'-' not in same position in string, automatically reject
    {
        return false;
    }

    for(i = 0; i<currSeperatorLoc; i++)
    {
        if(currName[i] != nextName[i]) //if something in the string differs before the seperator, reject
            return false;
    }

    return true;//all chars in string matched before seperator
}//END isNextMatch()

bool moneyExists(struct row* node)
{
    //printf("\nMade in to moneyExists and mobileAmt is: (%s)\n", node->mobileAmt); system("pause");
    if(moneyCheck(node->mobileAmt))
    {
        //printf("\nCheck #1\n");
        return true;
    }
    else if(moneyCheck(node->discountAmt))
    {
         //printf("\nCheck #2\n");
         return true;
    }
    else if(moneyCheck(node->feeAmt))
    {
        //printf("\nCheck #3\n");
        return true;
    }
    else
    {
        //printf("\nCheck#4\n");
        return false;
    }

}

bool moneyCheck(char amt[MAX_STRING_LEN])
{
    //system("pause");
    //printf("\nCompare Started");
    int x = strcmp("$0.00", amt);
    int y = strcmp("$0.00 ", amt);
    int z = strcmp(" $0.00", amt);
    //printf("\nCompare Finished\n");

    if(x == 0)
        return false;
    else if(y == 0)
        return false;
    else if(z == 0)
        return false;
    else
        return true;
}

void showHead()
{
    printf("\nHead Node: \n");
    printf("\nDisplay Name: (%s)", head->dName);
    printf("\nMobile Amt: (%s)", head->mobileAmt);
    printf("\nDiscount Amt: (%s)", head->discountAmt);
    printf("\nFee Amt: (%s)", head->feeAmt);
    printf("\nNet Amt: (%s)", head->netAmt);

    printf("\nHead Node: \n");
    printf("\nDisplay Name: (%s)", head->next->dName);
    printf("\nMobile Amt: (%s)", head->next->mobileAmt);
    printf("\nDiscount Amt: (%s)", head->next->discountAmt);
    printf("\nFee Amt: (%s)", head->next->feeAmt);
    printf("\nNet Amt: (%s)", head->next->netAmt);
    system("pause");
    return;
}//END showHead

void showNode(struct row* node)
{
    printf("\nNode Information: \n");
    printf("\nDisplay Name: (%s)", node->dName);
    printf("\nMobile Amt: (%s)", node->mobileAmt);
    printf("\nDiscount Amt: (%s)", node->discountAmt);
    printf("\nFee Amt: (%s)", node->feeAmt);
    printf("\nNet Amt: (%s)\n", node->netAmt);
    return;
}//END showNode

void showList()
{
    struct row* temp = head;
    int count = 1;

    while(temp->next != NULL)
    {
        printf("\n--------------------------------------");
        printf("\nNode (%d)", count++);
        printf("\nDisplay Name:  (%s)",temp->dName);
        printf("\nMobile Amt:    (%s)",temp->mobileAmt);
        printf("\nFee Amt:       (%s)",temp->feeAmt);
        printf("\nDiscount Amt:  (%s)",temp->discountAmt);
        printf("\nNet Amt:       (%s",temp->netAmt);

        if((count % 20) == 0)
            system("pause");

        temp = temp->next;
    }


    return;
}//END showList

void countPause(int n)
{
    if(n > 240)
    {
        system("pause");
    }
    return;
}
