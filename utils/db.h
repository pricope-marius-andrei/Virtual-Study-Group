#include <sqlite3.h>
#include <stdio.h>

int count_rows(void *data, int argc, char **argv, char **azColName)
{
    int *count = (int *)data;
    (*count)++;
    return 0;
}

int get_unique_id(sqlite3 *db)
{
    char *err_msg = 0;
    char query[1024];
    int id = 0;

    strcpy(query,"SELECT * FROM GROUPS;");

    int response = sqlite3_exec(db,query,count_rows,&id,&err_msg);

    if(response)
    {
        perror("GET UNIQUE ID ERROR!");
        exit(EXIT_FAILURE);
    }

    return id;
}

void create_group(sqlite3 *db, char *name, char *password)
{
    char *err_msg = 0;

    char id[1024];
    int unique_id = get_unique_id(db);
    printf("Unique id: %d\n",unique_id);
    sprintf(id, "%d", unique_id);

    char query[1024];
    strcpy(query,"INSERT INTO GROUPS (ID_GROUP,NAME,PASSWORD) VALUES('");
    strcat(query,id);
    strcat(query,"','");
    strcat(query,name);
    strcat(query,"','");
    strcat(query,password);
    strcat(query,"');");


    printf("Query: %s\n", query);

    int response = sqlite3_exec(db,query,NULL,0,&err_msg);

    if(response)
    {
        perror("CREATE GROUP ERROR!");
        exit(EXIT_FAILURE);
    }
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

void create_table(sqlite3 *db, char*query)
{
    char *errMsg = 0;
    int response = sqlite3_exec(db,query,NULL,0,&errMsg);
    if( response ) {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
}

static int display(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

void select_table(sqlite3 *db, char *query)
{
    char *errMsg = 0;
    const char *data = "Result:\n";

    int response = sqlite3_exec(db,query,display,(void*)data,&errMsg);
    if( response ) {
        fprintf(stderr, "SELECT error: %s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    } 
    else {
        fprintf(stderr, "SELECT was succesfully executed\n");
    }
}

int insert_user(sqlite3 *db, char* id , char *username, char *password, char* status, char *id_group)
{
    char *errMsg = 0;
    char query[254];

    strcpy(query,"INSERT INTO USERS (ID,USERNAME,PASSWORD,STATUS,ID_GROUP) VALUES ('");
    strcat(query,id);
    strcat(query,"','");
    strcat(query,username);
    strcat(query,"','");
    strcat(query,password);
    strcat(query,"','");
    strcat(query,status);
    strcat(query,"','");
    strcat(query,id_group);
    strcat(query,"');");

    int response = sqlite3_exec(db,query,NULL,0,&errMsg);
    if( response ) {
        fprintf(stderr, "Error : %s\n", sqlite3_errmsg(db));
    } 
    else {
        fprintf(stderr, "INSERT was succesfully executed\n");
    }

    return response;
}

void update_logged_status(sqlite3 *db, char *username, char *password, char*status) 
{
    char *errMsg = 0;
    char query[254];

    strcpy(query,"UPDATE USERS set STATUS=" );
    strcat(query,status);
    strcat(query," WHERE USERNAME='");
    strcat(query,username);
    strcat(query,"' AND PASSWORD='");
    strcat(query,password);
    strcat(query,"';");

    int response = sqlite3_exec(db,query,NULL,0,&errMsg);
    if( response ) {
        fprintf(stderr, "Error : %s\n", sqlite3_errmsg(db));
    } 
    else {
        fprintf(stderr, "UPDATE status was succesfully executed\n");
    }
}

int logged_status(void *data, int argc, char **argv, char **azColName)
{
    char status[100];
    strcpy(status,(char*)data);
    return 0;
}
int get_logged_status(sqlite3 *db, char *username)
{
    sqlite3_stmt *stmt;
    char *errMsg = 0;
    char query[254];
    int status = -1;

    strcpy(query,"SELECT STATUS FROM USERS WHERE USERNAME='");
    strcat(query,username);
    strcat(query,"';");

    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char *status_text = sqlite3_column_text(stmt, 0);
            if (status_text) {
                status = atoi((char *)status_text);
                printf("GET status was successfully executed. Status: %d\n", status);
            }
        } else {
            fprintf(stderr, "No data was find!");
        }

        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
    }

    return status;
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
