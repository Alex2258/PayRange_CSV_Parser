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
                for columns that are blank.

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

struct row* head;
int totalNodes = 1;

//Function Declaration(s)/Prototype(s)
void verifyFileName(void);
void parseHeaders(void);
struct row* parsePayRangeFile(void);
void sortLinkedList(int);
void writePayRangeFile(void);
void showHead(void);//used to debug
void showNode(struct row*);//used to debug
void showList(void);//used to debug
void countPause(int);
void alternativeSort(void);
char* parseToken(char*);
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

    strcpy(filename, "This_Week_OriginalPayRange.csv");

    printf("Verifying File Exists...\n");
    verifyFileName();

    printf("Reading Column Headers...\n");
    parseHeaders();

    printf("Parsing File Now...\n");
    parsePayRangeFile();

    printf("Sorting File Now...\n");
    alternativeSort();

    printf("\nCreating New File...\n");
    writePayRangeFile();


    printf("Completed! Check new file and rename it!\n");

    return;
}//END main

//Function(s)
void verifyFileName()
{
    //Local Variable(s)
    int tmp = NOTVERIFIED;
    char tempString[MAX_STRING_LEN];

    while(tmp == NOTVERIFIED)
    {
        FILE* stream = fopen(filename, "r"); //Attempt to Open File

        if(stream == NULL) //If failed to open file
        {
            fclose(stream); //Close the file

            printf("Error: File failing to open, verify the name is correct and try again: ");
            scanf("%s", &tempString); //Ask for correct filename && try again
            strcpy(filename, tempString);
            printf("\n");
        }
        else
        {
            printf("File Found...\n");
            fclose(stream); //Close the file
            tmp = VERIFIED;
        }
    }

    return;
}//END verifyFileName

void parseHeaders() //FGETS STOPS AT (N-) are read or newline char, or EOF,        return;   whichever comes first
{
    //Local Variable(s)
    char tempString[MAX_CSV_LEN];
    char *token;
    FILE* stream;
    int count = 0;

    stream = fopen(filename, "r"); //Open File
    fgets(tempString, MAX_CSV_LEN, stream); //Get First Line (column headers)

    token = strtok(tempString, comma); //Grab First Token from First Line String (delimited by commas)

    while(token != NULL) //while there are tokens remaining
    {
        count++;

        //printf("\nThis is Token %d: %s",count, token);

        if(strcmp(token, dName) == 0)
        {
            col_dName = count; //set desired col location
        }
        else if(strcmp(token, mobileAmt) == 0)
        {
            col_mobileAmt = count; //set desired col location
        }
        else if(strcmp(token, discountAmt) == 0)
        {
            col_discountAmt = count; //set desired col location
        }
        else if(strcmp(token, feeAmt) == 0)
        {
            col_feeAmt = count; //set desired col location
        }
        else if(strcmp(token, netAmt) == 0)
        {
            col_netAmt = count; //set desired col location
        }

        token = strtok(NULL, comma); //get next token
    }

    totalColumns = count;

    //TEST CODE
    /*
    printf("\nDisplay Name is Column: %d.", col_dName); //
    printf("\nMobile Amt is Column: %d.", col_mobileAmt); //
    printf("\nDiscount Amt is Column: %d.", col_discountAmt); //
    printf("\nFee Amt is Column: %d.", col_feeAmt); //
    printf("\nNet Amt is Column: %d.", col_netAmt); //
    printf("\nThere are %d total columns!\n", totalColumns); //
    */
    fclose(stream); //Close File


    return;
}//END parseHeaders

struct row* parsePayRangeFile()
{

    //Local Variable(s)
    char tempString[MAX_CSV_LEN];
    char *token;
    FILE* stream;
    int count = 2;
    int firstNode = NOTVERIFIED;
    struct row* temp = (row*)malloc(sizeof(struct row));

    stream = fopen(filename, "r"); //Open File
    fgets(tempString, MAX_CSV_LEN, stream); //Get First Line (discard, do not need as it is only headers)

    while(fgets(tempString, MAX_CSV_LEN, stream) != NULL)//while there are lines left to read, get one.
    {
        while(strlen(tempString) < MAX_STRING_LEN)
        {
            fgets(tempString, MAX_CSV_LEN, stream); //skip this line, not what we're looking for
        }

        if(firstNode == VERIFIED && totalNodes > 2)
        {
            temp->next = (row*)malloc(sizeof(row)); // allocation memory for next row
            temp = temp->next; //move temp to next row
        }
        token = zstring_strtok(tempString, comma); //Get First Token from newly grabbed line from file

        while(token != NULL) //while there are tokens remaining
        {
            if(col_dName == count)
            {
                token = parseToken(token);
                strcpy(temp->dName, token);
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

            if(count == totalColumns)//found last item for this node, reset counter
            {
                if(firstNode == NOTVERIFIED)
                {
                    head = (row*)malloc(sizeof(struct row));

                    strcpy(head->dName, temp->dName);
                    strcpy(head->mobileAmt, temp->mobileAmt);
                    strcpy(head->discountAmt, temp->discountAmt);
                    strcpy(head->feeAmt, temp->feeAmt);
                    strcpy(head->netAmt, temp->netAmt);

                    head->totalAmt = strToFloat(head->mobileAmt,head->discountAmt);
                    firstNode = VERIFIED;
                    head->next = temp;
                }
                else
                {
                    temp->totalAmt = strToFloat(temp->mobileAmt,temp->discountAmt);
                }

                count = 2; //reset counter
                token = NULL;
                totalNodes++;
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

char* parseToken(char* token)
{
    //Local Variable(s)
    int length = strlen(token);//Length of the Token
    int prevChar, currChar; //ASCII Value of the previous and current characters
    bool prevCharIsLetter, currCharIsLetter; //flag to denote the prev and curr char is a letter
    int i;

    token[0] = toupper(token[0]);//Capitalize First Letter

    for(i=1; i<length; i++)
    {
        prevCharIsLetter = isLetter(token[i-1]);
        currCharIsLetter = isLetter(token[i]);

        if(prevCharIsLetter && currCharIsLetter)//Both are Letters
        {
            token[i] = tolower(token[i]);
        }
        else if(prevCharIsLetter && (!currCharIsLetter))//Prev is Letter, Curr is not
        {
            //do nothing
        }
        else if((!prevCharIsLetter) && currCharIsLetter)//Curr is Letter, Prev is not
        {
            token[i] = toupper(token[i]);
        }
        else//Both are non-letters
        {
            //do nothing
        }
    }
    return token;
}//END parseToken

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

void sortLinkedList(int i)
{
    //Local Variable(s)
    struct row* temp;      //Used to avoid chance of losing HEAD
    struct row* prevHold;  //Used to retain location of the previous node to use during swapping
    struct row* hold;      //Used to store data that needs to be saved when swapping
    char strCurr[MAX_STRING_LEN]; //Curr Node dName
    char strNext[MAX_STRING_LEN]; //Next Node dName
    int firstRow = NOTVERIFIED;   //Starting Node Flag
    bool swapOccured = true;
    bool beginFlag;
    int n = 0;

    while(swapOccured == true)
    {
        printf("\nSort #: (%d)",++n);
        beginFlag = true;
        swapOccured = false; //Used to indicate if we're done sorting. If we do not make a swap, we're done
        temp = head; //Use TEMP instead of HEAD to minimize chance of losing the list

        while(temp->next != NULL) //While not at end of list
        {
            strcpy(strCurr, temp->dName); //this nodes Display
            strcpy(strNext, temp->next->dName); //next nodes Display Name

            //Case 1: Swap Needed
            if(strCurr[i] > strNext[i])
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
                prevHold = temp;//Retain location of prev node
                temp = temp->next;//Go to next node
                beginFlag = false; //Set head of list flag
            }
        }
    }
    return;
}//END sortLinkedList

void showHead()
{
    printf("\nHead Node: \n");
    printf("\nDisplay Name: (%s)", head->dName);
    printf("\nMobile Amt: (%s)", head->mobileAmt);
    printf("\nDiscount Amt: (%s)", head->discountAmt);
    printf("\nFee Amt: (%s)", head->feeAmt);
    printf("\nNet Amt: (%s)", head->netAmt);
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

float strToFloat(char mobileAmt[MAX_STRING_LEN],char discountAmt[MAX_STRING_LEN])
{
    float returnNet = 0.00;
    float amt;
    int i, j;
    int length = strlen(mobileAmt);

    /*
        mobileAmt in form:   '$0.00'
        discountAmt in form: '40.00'
    */

    //Determine where the '.' is:
    for(i = 0; i < length; i++)//loop through string
    {
        if(mobileAmt[i] == '.')//once we find our decimal marker
        {
            //Get & Add Hundredths Place
            amt = mobileAmt[i+2] - '0';
            returnNet += 0.01*amt;

            //Get & Add Tenths Place
            amt = mobileAmt[i+1] - '0';
            returnNet += 0.1*amt;

            //Get & Add Ones Place
            amt = mobileAmt[i-1] - '0';
            returnNet += 1.0*amt;

            if(i == 3)
            {
                //Get & Add Tens Place IF IT EXISTS
                amt = mobileAmt[i-2] - '0';
                returnNet += 10.0*amt;
            }

            if(i == 4)
            {
                //Get & Add Hundreds Place IF IT EXISTS
                amt = mobileAmt[i-3] - '0';
                returnNet += 100.0*amt;
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
            returnNet -= 0.01*amt;

            //Get & Sub Tenths Place
            amt = discountAmt[i+1] - '0';
            returnNet -= 0.1*amt;

            //Get & Sub Ones Place
            amt = discountAmt[i-1] - '0';
            returnNet -= 1.0*amt;

            if(i == 3)
            {
                //Get & Sub Tens Place IF IT EXISTS
                amt = discountAmt[i-2] - '0';
                returnNet -= 10.0*amt;
            }

            if(i == 4)
            {
                //Get & Sub Hundreds Place IF IT EXISTS
                amt = discountAmt[i-3] - '0';
                returnNet -= 100.0*amt;
            }
        }
    }

    return returnNet;
}//END strToInt

void writePayRangeFile()
{
    //Local Variable(s)
    FILE* stream;
    int flag = NOTVERIFIED;   //flag for end of list
    struct row* temp = head;  //temp used to avoid loss of head pointer
    float totalAmt= 0.00;     //used to store total amounts between nodes of the same account

    stream = fopen("parsed_payrange_file.csv", "w"); //Open a new file to write proper data into

    //PRINT HEADERS INTO FILE
    fprintf(stream, "Display Name"); fprintf(stream, ",");
    fprintf(stream, "Mobile"); fprintf(stream, ",");
    fprintf(stream, "Discounts"); fprintf(stream, ",");
    fprintf(stream, "Fee"); fprintf(stream, ",");
    fprintf(stream, "Net"); fprintf(stream, ",");
    fprintf(stream, "Total");fprintf(stream, "\n");

    while(flag != VERIFIED) //while there are still rows to go through
    {
        if(moneyExists(temp->mobileAmt)) //if there is money for this specific PayRange then include in file, else delete
        {
            fprintf(stream, temp->dName); fprintf(stream, ",");//Write Display Name & Comma
            fprintf(stream, temp->mobileAmt); fprintf(stream, ",");//Write Mobile Amt & Comma

            if(moneyExists(temp->discountAmt))//If there is a Discount Amt: Write it & Comma
            {
                fprintf(stream, temp->discountAmt);
            }fprintf(stream, ",");


            fprintf(stream, temp->feeAmt); fprintf(stream, ",");//Write Fee Amt & Comma

            strcpy(temp->netAmt, (removeNewLine(temp->netAmt)));//Remove New Line Char from Net Amt if it exists
            fprintf(stream, temp->netAmt); fprintf(stream, ",");//Write Net Amt & Comma

            if(isNextMatch(temp->dName,temp->next->dName))//CurrName & NextName Match
            {

                if(moneyExists(temp->next->mobileAmt))//If the next Node is actually gonna be written to the file && matches
                {
                    fprintf(stream, "\n");
                    totalAmt += temp->totalAmt;
                }
                else if(isNextMatch(temp->dName,temp->next->next->dName))//otherwise, names match but nextName is not being added, check 2 nodes ahead to see if match occurs
                {
                    //ERROR IS HERE------*******------- IF WE ARE END OF LIST AND ITEMS ARE NOT GOING TO BE WRITTEN BUT NEXT FEW NODES MATCH, we will never write
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
            else
            {
                totalAmt += temp->totalAmt;
                fprintf(stream, "$%.2f\n", totalAmt);
                totalAmt = 0.00;
            }
        }

        if(temp->next == NULL)
        {
            flag = VERIFIED; //We're at the end of list, using temp so we dont use head of list location
            system("pause");
        }
        else
        {
            temp = temp->next; //move to next row
        }
    }

    fprintf(stream, "\n");
    fprintf(stream, "\n");
    fprintf(stream, "Totals:");
    fclose(stream);
    return;

}//END writePayRangeFile

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

bool isNextMatch(char currName[MAX_STRING_LEN],char nextName[MAX_STRING_LEN])
{
    //printf("\nCurrName: (%s)", currName);
    //printf("\nNextName: (%s)", nextName);

    if(currName[1] == nextName[1])
    {
        if(currName[2] == nextName[2])
            if(currName[3] == nextName[3])
                if(currName[4] == nextName[4])
                {
                    //printf("\nReturning: True\n");
                    return true;

                }
    }
    else
    {
        //printf("\nReturning: False\n");
        return false;

    }

    //printf("\nReturning: False\n");
    return false;
}//END isNextMatch()

bool moneyExists(char mobileAmt[MAX_STRING_LEN])
{
    //printf("\nMobile Amt: (%s)", mobileAmt);
    int x = strcmp("$0.00", mobileAmt);
    int y = strcmp("$0.00 ", mobileAmt);
    int z = strcmp(" $0.00", mobileAmt);
    //printf("\nx: (%d) --- y: (%d) --- z: (%d)",x,y,z);
    //system("pause");

    if(x == 0)
        return false;
    else if(y == 0)
        return false;
    else if(z == 0)
        return false;


    return true;
}

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
    char strCurr[MAX_STRING_LEN]; //Curr Node dName
    char strNext[MAX_STRING_LEN]; //Next Node dName
    int firstRow = NOTVERIFIED;   //Starting Node Flag
    bool swapOccured = true;
    bool beginFlag;
    int n = 1;int m = 4; int i = 0;

    while(swapOccured == true)//while a swap was made in the prev iteration
    {
        /*
        ++m;
        if((m % 5) == 0)
        {
            printf("\n");
        }
        printf("Sort #: (%d) ",++n);
        */
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

   //showList();
    return;

}
