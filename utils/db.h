#include <sqlite3.h>
#include <stdio.h>

int count_rows(void *data, int argc, char **argv, char **azColName)
{
    int *count = (int *)data;
    (*count)++;
    return 0;
}

int verify_user_exist(sqlite3 *db, char* username ,char *password) 
{
    char *errMsg = 0;
    char query[100];
    int rows = 0;
    strcpy(query,"SELECT * FROM USERS WHERE USERNAME='");
    strcat(query,username);
    strcat(query,"' AND PASSWORD='");
    strcat(query,password);
    strcat(query,"';");

    int response = sqlite3_exec(db,query,count_rows,&rows, &errMsg);

    if(response)
    {
        perror("Get rows count error");
        exit(EXIT_FAILURE);
    }

    return rows;
}

static int display(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

void create_table(sqlite3 *db, char*query)
{
    char *errMsg = 0;
    int response = sqlite3_exec(db,query,NULL,0,&errMsg);
    if( response ) {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
}

void select_table(sqlite3 *db, char *query)
{
    char *errMsg = 0;
    const char *data = "Result:\n";
    int rows = 0;

    int response = sqlite3_exec(db,query,display,(void*)data,&errMsg);
    if( response ) {
        fprintf(stderr, "SELECT error: %s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    } 
    else {
        fprintf(stderr, "SELECT was succesfully executed\n");
    }
}

int insert(sqlite3 *db, char *username, char *password)
{
    char *errMsg = 0;
    char query[100];

    strcpy(query,"INSERT INTO USERS (USERNAME,PASSWORD) VALUES ('");
    strcat(query,username);
    strcat(query,"','");
    strcat(query,password);
    strcat(query,"');");

    //Preparation statement for query
    // sqlite3_stmt *statement;
    // int response_stmt = sqlite3_prepare_v2(db,query,-1,&statement, NULL);

    // if(response_stmt != SQLITE_OK)
    // {
    //     perror("Preparation failed");
    // }

    // int count = 0;
    // int response_step = sqlite3_step(statement);
    // if (response_step == SQLITE_ROW) {
    //     count = sqlite3_column_int(statement, 0);
    // }
    // sqlite3_finalize(statement);

    int response = sqlite3_exec(db,query,NULL,0,&errMsg);
    if( response ) {
        fprintf(stderr, "Error : %s\n", sqlite3_errmsg(db));
    } 
    else {
        fprintf(stderr, "INSERT was succesfully executed\n");
    }

    return response;
}

void update()
{

}

void delete_account(sqlite3 *db, char *query)
{
    char *errMsg = 0;
    int response = sqlite3_exec(db,query,NULL,0,&errMsg);
    if( response ) {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    } 
    else {
        fprintf(stderr, "DELETE was succesfully executed\n");
    }
}
