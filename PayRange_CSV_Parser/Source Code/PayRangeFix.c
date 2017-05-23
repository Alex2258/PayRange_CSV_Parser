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
int col_dName;
int col_mobileAmt;
int col_discountAmt;
int col_feeAmt;
int col_netAmt;
int totalColumns;

struct row* root;//Root of the Linked List. No Mobile,Discount, but Fee Exists, Keep these seperate.
struct row* head;//Head of the Linked List
int totalNodes = 1;

//Function Declaration(s)/Prototype(s)
void run(void);
void verifyFileName(void);
void parseHeaders(void);
struct row* parsePayRangeFile(void);
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
char* concat(const char*, const char*);
char* iHeartCheck(char [MAX_STRING_LEN]);
void removeNoCashNodes(void);
bool isLetter(char);
bool isNextMatch(char [MAX_STRING_LEN], char [MAX_STRING_LEN]);
bool moneyExists(char [MAX_STRING_LEN]);
float strToFloat(char [MAX_STRING_LEN], char [MAX_STRING_LEN]);
char* removeNewLine(char [MAX_STRING_LEN]);

//Main
int main(int argc, char *argv[])
{
    // NOTE ARGC = Argument Count && Argv = Argument Vector
    //filename = *argv[0];

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

    printf("Removing Empty Nodes Now...\n");
    removeNoCashNodes();

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
            scanf("%s", &str); //Ask for correct filename && try again
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
    int count = 0;

    s = concat(filename, ".csv");
    stream = fopen(s, "r"); //Open File
    fgets(str, MAX_CSV_LEN, stream); //Get First Line (column headers)

    token = strtok(str, comma); //Grab First Token from First Line String (delimited by commas)

    while(token != NULL) //while there are tokens remaining
    {
        count++;

        if(strcmp(token, dName) == 0)
        {
            col_dName = count; //retain col # of dName header
        }
        else if(strcmp(token, mobileAmt) == 0)
        {
            col_mobileAmt = count; //retain col # of mobileAmt header
        }
        else if(strcmp(token, discountAmt) == 0)
        {
            col_discountAmt = count; //retain col # of discountAmt header
        }
        else if(strcmp(token, feeAmt) == 0)
        {
            col_feeAmt = count; //retain col # of feeAmt header
        }
        else if(strcmp(token, netAmt) == 0)
        {
            col_netAmt = count; //retain col # of netAmt header
        }

        token = strtok(NULL, comma); //get next token
    }

    totalColumns = count;//Retain total amount of columns seen in the file

    fclose(stream); //Close File

    return;
}//END parseHeaders

struct row* parsePayRangeFile()
{
    //Local Variable(s)
    char str[MAX_CSV_LEN];
    char *token, *s;
    FILE* stream;
    int count = 2;//Once we grab a line with data we're interested in, we'll actually be in column two (based on csv file format)
    int seenFirstNode = NOTVERIFIED;
    struct row* temp = (row*)malloc(sizeof(struct row));

    s = concat(filename,".csv");
    stream = fopen(s, "r"); //Open File
    fgets(str, MAX_CSV_LEN, stream); //Get First Line (discard, do not need as it is only headers)

    while(fgets(str, MAX_CSV_LEN, stream) != NULL)//while there are lines left to read, get one.
    {
        while(strlen(str) < MAX_STRING_LEN)
        {
            fgets(str, MAX_CSV_LEN, stream); //skip this line, not what we're looking for
        }

        if(seenFirstNode == VERIFIED && totalNodes > 2)
        {
            if(moneyExists(temp->mobileAmt))//ONLY ADD NODES THAT HAVE MONEY
            {
                temp->next = (row*)malloc(sizeof(row)); // allocation memory for next row
                temp = temp->next; //move temp to next row
            }
        }

        token = zstring_strtok(str, comma); //Get First Token from newly grabbed line from file

        while(token != NULL) //while there are tokens remaining
        {
            if(col_dName == count)
            {
                strcpy(temp->dName, iHeartCheck(token));
            }
            else if(col_mobileAmt == count)
            {
                strcpy(temp->mobileAmt, token);
            }
            else if(col_discountAmt == count)
            {
                strcpy(temp->discountAmt, token);
            }
            else if(col_feeAmt == count)
            {
                strcpy(temp->feeAmt, token);
            }
            else if(col_netAmt == count)
            {
                strcpy(temp->netAmt, token);
            }

            if(count == totalColumns)//found last item for this node
            {
                if((seenFirstNode == NOTVERIFIED) && moneyExists(temp->mobileAmt))
                {
                    head = (row*)malloc(sizeof(struct row));

                    strcpy(head->dName, iHeartCheck(temp->dName));
                    strcpy(head->mobileAmt, temp->mobileAmt);
                    strcpy(head->discountAmt, temp->discountAmt);
                    strcpy(head->feeAmt, temp->feeAmt);
                    strcpy(head->netAmt, temp->netAmt);

                    //Change the String: '$x.xx' to a float value, used when writing new csv file
                    head->totalAmt = strToFloat(head->mobileAmt,head->discountAmt);

                    seenFirstNode = VERIFIED;
                    head->next = temp;//link next node to the head of the list
                }
                else if(moneyExists(temp->mobileAmt))
                {
                    //Change the String: '$x.xx' to a float value, used when writing new csv file
                    temp->totalAmt = strToFloat(temp->mobileAmt,temp->discountAmt);
                }

                count = 2; //reset counter
                token = NULL;//reset our token
                totalNodes++;//increment our totalNodes(total rows) counter
            }
            else
            {
                token = zstring_strtok(NULL, comma); //get next token
                count++; //adjust column over
            }
        }
    }

    return;
}//End parsePayRangeFile

void alternativeSort()
{
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
            if((strcmp(temp->dName,temp->next->dName)) > 0)
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
    printRows(stream,temp);
    printTotal(stream);

    fclose(stream);

    freeList(head);
    free(s);
    return;

}//END writePayRangeFile

void printHeaders(FILE* stream)
{
    fprintf(stream, "Display Name"); fprintf(stream, ",");
    fprintf(stream, "Mobile"); fprintf(stream, ",");
    fprintf(stream, "Discounts"); fprintf(stream, ",");
    fprintf(stream, "Fee"); fprintf(stream, ",");
    fprintf(stream, "Net"); fprintf(stream, ",");
    fprintf(stream, "Total");fprintf(stream, "\n");
    return;
}//END printHeaders

void printRows(FILE* stream, struct row* temp)
{
    int flag = NOTVERIFIED;   //flag for end of list
    float totalAmt= 0.00;     //used to store total amounts between nodes of the same account

    while(flag == NOTVERIFIED)
    {
        fprintf(stream, iHeartCheck(temp->dName)); fprintf(stream, ",");//Write Display Name & Comma
        fprintf(stream, temp->mobileAmt); fprintf(stream, ",");//Write Mobile Amt & Comma
        fprintf(stream, temp->discountAmt);fprintf(stream, ",");//Write Discount Amt & Comma
        fprintf(stream, temp->feeAmt); fprintf(stream, ",");//Write Fee Amt & Comma
        fprintf(stream, temp->netAmt); fprintf(stream, ",");//Write Net Amt & Comma


        if(temp->next != NULL)//If there are still more items to come &
        {
            if(isNextMatch(temp->dName,temp->next->dName))//CurrName & NextName Match
            {
                if(temp->next->next != NULL)//CASE 1: NOT AT second to last node
                {
                    if(moneyExists(temp->next->mobileAmt))//If the next Node is actually gonna be written to the file && matches
                    {
                        fprintf(stream, "\n");
                        totalAmt += temp->totalAmt;
                    }
                    else if(isNextMatch(temp->dName,temp->next->next->dName))//otherwise, names match but nextName is not being added, check 2 nodes ahead to see if match occurs
                    {
                        fprintf(stream, "\n");
                        totalAmt += temp->totalAmt;
                    }
                    else//names match, but next node isnt being written and the following node is not a match
                    {
                        totalAmt += temp->totalAmt;
                        fprintf(stream, "$%.2f\n", totalAmt);
                        totalAmt = 0.00;
                    }
                }
                else//Case 2: AT second to last node
                {
                    if(moneyExists(temp->next->mobileAmt))//If the next Node is actually gonna be written to the file && matches
                    {
                        fprintf(stream, "\n");
                        totalAmt += temp->totalAmt;
                    }
                    else//names match
                    {
                        totalAmt += temp->totalAmt;
                        fprintf(stream, "$%.2f\n", totalAmt);
                        totalAmt = 0.00;
                    }
                }
            }
            else
            {
                totalAmt += temp->totalAmt;
                fprintf(stream, "$%.2f\n", totalAmt);
                totalAmt = 0.00;
            }
        }
        else
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
        fprintf(stream, iHeartCheck(temp->dName)); fprintf(stream, ",");//Write Display Name & Comma
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

void freeList(struct row* root)
{
    struct row* temp;

    while(root->next != NULL)//while there are nodes remaining to free
    {
        temp = root->next;//move temp to next node over
        free(root);//free curr node
        root = temp;//set root to next node
    }

    free(root);
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

char* iHeartCheck(char dName[MAX_STRING_LEN])
{
    char iHeart[6] = "iHeart";
    char IHeart[6] = "IHeart";
    int i;
    int count = 0; int counter = 0;
    int checkLength = 6;

    for(i = 0; i < checkLength; i++)
    {
        if(iHeart[i] == dName[i])//first char was Lowercase, we must Uppercase it for sorting purposes
            count++;
        else if(IHeart[i] == dName[i])//first char was Uppercase, lowercase it for writing file
            counter++;
    }

    if(count == 6)
        dName[0] = 'I';
    else if(counter == 6)
        dName[0] = 'i';

    return dName;
}//END iHeartCheck

void removeNoCashNodes()
{
    struct row* temp = head;
    struct row* prevNode;
    struct row* temp2;
    bool eraseRoot = true;
    bool firstRootNode = true;

    while(eraseRoot)//Case 1: Remove Root Node until we have a node whose mobileAmt > $0.00
    {
        if(!moneyExists(temp->discountAmt))
        {
            strcpy(temp->discountAmt,"");
        }

        strcpy(temp->netAmt, (removeNewLine(temp->netAmt)));

        if(!moneyExists(temp->mobileAmt))//Erasing Start Node
        {
            if(moneyExists(temp->feeAmt))
            {
                if(firstRootNode)
                {
                    root = temp;

                    head = temp->next;
                    temp = head;

                    root->next = NULL;

                    firstRootNode = false;
                }
                else
                {
                    temp2->next = temp;
                    temp2 = temp;

                    head = temp->next;
                    temp = head;

                    temp2->next = NULL;
                }
            }
            else
            {
                head = temp->next;//Move Head to Next Node (now this is the new start node)
                free(temp);//free the previous/old head node
                temp = head;//reset temp to head
            }

        }
        else//Not Erasing Start Node
        {
            eraseRoot = false;
            prevNode = temp;//retain this node's location &
            temp = temp->next;//Move to next node
        }
    }

    while(temp->next != NULL)//Case2: Loop thru remainder of list to remove nodes with $0.00 mobileAmts
    {
        if(!moneyExists(temp->discountAmt))
        {
            strcpy(temp->discountAmt,"");
        }

        strcpy(temp->netAmt, (removeNewLine(temp->netAmt)));

        if(!moneyExists(temp->mobileAmt))//Erase this node
        {
            if(moneyExists(temp->feeAmt))
            {
                if(firstRootNode)
                {
                    root = temp;

                    prevNode->next = temp->next;
                    temp = temp->next;

                    root->next = NULL;

                    firstRootNode = false;
                }
                else
                {
                    temp2 = temp;

                    head = temp->next;
                    temp = head;

                    temp2->next = NULL;

                    if(root->next == NULL)
                    {
                        root->next = temp2;
                    }
                }
            }
            else
            {
                prevNode->next = temp->next;
                free(temp);
                temp = prevNode->next;
            }
        }
        else//money exists, retain node and shift over by one node
        {
            prevNode = temp;
            temp = temp->next;
        }
    }


    if(!moneyExists(temp->mobileAmt))
    {//At TAIL NODE, remove tail if money doesn't exist
        prevNode->next = NULL;
        free(temp);
    }
    else
    {
        if(!moneyExists(temp->discountAmt))
        {
            strcpy(temp->discountAmt,"");
        }
        strcpy(temp->netAmt, (removeNewLine(temp->netAmt)));
    }

    return;
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

bool moneyExists(char mobileAmt[MAX_STRING_LEN])
{
    int x = strcmp("$0.00", mobileAmt);
    int y = strcmp("$0.00 ", mobileAmt);
    int z = strcmp(" $0.00", mobileAmt);

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
