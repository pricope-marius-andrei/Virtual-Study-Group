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

int create_group(sqlite3 *db, int admin_id, char *name, char *password)
{
    char *err_msg = 0;

    char id[1024];
    int unique_id = get_unique_id(db);
    sprintf(id, "%d", unique_id);

    char adm_id[1024];
    sprintf(adm_id, "%d", admin_id);

    char query[1024];
    strcpy(query,"INSERT INTO GROUPS (ID_GROUP,ADMIN_ID,NAME,PASSWORD) VALUES('");
    strcat(query,id);
    strcat(query,"','");
    strcat(query,adm_id);
    strcat(query,"','");
    strcat(query,name);
    strcat(query,"','");
    strcat(query,password);
    strcat(query,"');");

    int response = sqlite3_exec(db,query,NULL,0,&err_msg);

    if(response)
    {
        perror("CREATE GROUP ERROR!");
        exit(EXIT_FAILURE);
    }

    return unique_id;
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

int verify_group_exist(sqlite3 *db, char* id_group ,char *password) 
{
    char *errMsg = 0;
    char query[100];
    int rows = 0;
    strcpy(query,"SELECT * FROM GROUPS WHERE ID_GROUP='");
    strcat(query,id_group);
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

int verify_file_exist(sqlite3 *db, int group_id, char *file_name)
{
    char *errMsg = 0;
    char query[100];
    int rows = 0;
    sprintf(query,"SELECT * FROM FILES WHERE GROUP_ID='%d' AND FILE_NAME='%s';",group_id,file_name);

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

// static int display(void *data, int argc, char **argv, char **azColName) {
//    for(int i = 0; i<argc; i++) {
//     char *line;
//     sprintf(line,"%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
//     strcpy(data,line);
//    }
//    printf("\n");
//    return 0;
// }

char* select_table(sqlite3 *db, char *query)
{
    struct sqlite3_stmt *stmt;
    char *errMsg = 0;
    char *data;
    char current_data[1024]="";

    char line[100] = "";
    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int columns_count = sqlite3_column_count(stmt);
            for (int index = 0; index < 2; index++)
            {
                strcpy(line,sqlite3_column_name(stmt,index));
                strcat(line, " = ");
                strcat(line, sqlite3_column_text(stmt,index));
                strcat(line, "\n");
                strcat(current_data,line);
            }
            strcat(current_data, "\n");
        }

        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
    }

    data = (char*)malloc(strlen(current_data)+1);
    strcpy(data,current_data);

    return data;
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

void update_users_field(sqlite3 *db , char* field, int user_id, char*value_field) 
{
    char *errMsg = 0;
    char query[254];

    char uid[1024];
    sprintf(uid,"%d",user_id); 

    strcpy(query,"UPDATE USERS set ");
    strcat(query,field);
    strcat(query,"='");
    strcat(query,value_field);
    strcat(query,"' WHERE ID='");
    strcat(query,uid);
    strcat(query,"';");

    int response = sqlite3_exec(db,query,NULL,0,&errMsg);
    if( response ) {
        fprintf(stderr, "Error : %s\n", sqlite3_errmsg(db));
    } 
    else {
        fprintf(stderr, "UPDATE status was succesfully executed\n");
    }
}

int get_field_value(sqlite3 *db, const char *condition, const char* field, const char* table_name)
{
    sqlite3_stmt *stmt;
    char *errMsg = 0;
    char query[254];
    int status = -1;

    strcpy(query,"SELECT ");
    strcat(query,field);
    strcat(query," FROM ");
    strcat(query, table_name);
    strcat(query, " ");
    strcat(query,condition);
    strcat(query,";");


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

void save_message(sqlite3 *db, int user_id, int group_id, char *message)
{
    char *errMsg = 0;
    char query[254];

    sprintf(query,"INSERT INTO MESSAGES (USER_ID,GROUP_ID,MESSAGE,DATE,TIME) VALUES('%d','%d','%s',date(),time());",user_id,group_id,message);

    int response = sqlite3_exec(db,query,NULL,0,&errMsg);
    if( response ) {
        fprintf(stderr, "Error : %s\n", sqlite3_errmsg(db));
    } 
    else {
        fprintf(stderr, "INSERT was succesfully executed\n");
    }
}

void save_file(sqlite3 *db, int group_id, int user_id, char *file_name)
{
    char *errMsg = 0;
    char query[254];

    sprintf(query,"INSERT INTO FILES (FILE_NAME,GROUP_ID,USER_ID) VALUES('%s','%d','%d');",file_name,group_id,user_id);

    int response = sqlite3_exec(db,query,NULL,0,&errMsg);
    if( response ) {
        fprintf(stderr, "Error : %s\n", sqlite3_errmsg(db));
    } 
    else {
        fprintf(stderr, "INSERT was succesfully executed\n");
    }
}

char* get_group_messages(sqlite3 *db, int group_id)
{
    struct sqlite3_stmt *stmt;
    char *errMsg = 0;
    char *data;
    char current_data[1024]="";
    char query[254];
    sprintf(query,"SELECT USERNAME,MESSAGE FROM USERS JOIN MESSAGES ON USERS.ID=MESSAGES.USER_ID WHERE GROUP_ID='%d' ORDER BY DATE,TIME",group_id);

    char line[100] = "";
    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            strcpy(line,sqlite3_column_text(stmt,0));
            strcat(line, ":");
            strcat(line, sqlite3_column_text(stmt,1));
            strcat(current_data,line);
        }

        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
    }
    data = (char*)malloc(strlen(current_data)+1);
    if(strlen(current_data) == 0)
    {
        strcpy(data,"\n");
    }
    else {
        strcpy(data,current_data);
    }
    return data;
}