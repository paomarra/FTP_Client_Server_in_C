#include "ftp.h"
#include <stdio.h>
#include <stdlib.h> /* getenv, exit */
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>

#define EMPTY "-"
#define UNKNOWN_CMD 99

enum command_type {
	EXIT_CMD = 0,
	LS_CMD = 1,
	CD_CMD = 2,
	CP_CMD = 3,
	GET_CMD = 4,
	CD_BACK_CMD = 5
};
typedef enum command_type command_type;

void eval_return_info(int info);
command_type eval_command(char* input, file_info* arguments);
int Pthread_create(pthread_t *tid, const pthread_attr_t *attr, void * (*func)(void *), void *arg);

char pwd[MAX_SIZE];

pthread_t client_pthread;
pthread_mutex_t  mutex;

CLIENT *clnt;
CLIENT *clnt_data;

enum clnt_stat retval_1;
int result_1;

enum clnt_stat retval_2;
catalog_state result_2;

enum clnt_stat retval_3;
int result_3;

enum clnt_stat retval_4;
int result_4;

enum clnt_stat retval_5;
catalog_state result_5;
char *  init_5_arg;

command_type command;
file_info* arguments;	

enum clnt_stat retval_6;
file_content_info result_6;

enum clnt_stat retval_7;
int result_7;
file_content_info  send_file_1_arg;

void *
send_file_thread(void *argp)
{
	struct stat st;
	char pwd[MAX_SIZE];
	char remote_pwd[MAX_SIZE];
	int fin, size, count;

	file_info *arg = (file_info *)malloc(sizeof(file_info));
	arg->file_name = (char *)malloc(sizeof(char)*MAX_SIZE);
	arg->path = (char *)malloc(sizeof(char)*MAX_SIZE);

	strcpy(arg->file_name, arguments->file_name);
	strcpy(arg->path, arguments->path);

	pthread_mutex_unlock(&mutex);

	memset(pwd, 0, MAX_SIZE);
	memset(remote_pwd, 0, MAX_SIZE);

	FILE *p = popen("pwd", "r");

	fscanf(p, "%s", pwd);

	sprintf(pwd, "%s/%s", pwd, arg->file_name);
	sprintf(remote_pwd, "%s/%s", arg->path, arg->file_name);

	if (stat(pwd, &st) == 0)
	{
		if(S_ISDIR(st.st_mode)) 
		{ 
			printf("ERROR: It is a catalog, not a file.\n");
			return;
		}

	}

	fin = open(pwd, O_RDONLY);

	send_file_1_arg.name = (char *)malloc(sizeof(char)*MAX_SIZE);
	send_file_1_arg.name = remote_pwd;

	if (fin == -1)
	{
		printf("ERROR: Cannot open file %s\n", pwd);
		close(fin);
		return;
	}

	size = lseek(fin, 0, SEEK_END);
	close(fin);

	fin = open(pwd, O_RDONLY);

	send_file_1_arg.data.data_len = size;
	send_file_1_arg.data.data_val = (char *)malloc(sizeof(char) * size);

	read(fin, send_file_1_arg.data.data_val, size);

	close(fin);

	retval_7 = send_file_1(&send_file_1_arg, &result_7, clnt_data);
	if (retval_7 != RPC_SUCCESS) {
		clnt_perror(clnt_data, "send call failed");
	}

	eval_return_info(result_7);
}

void *
get_file_thread(void *argp)
{
	file_info *arg = (file_info *)malloc(sizeof(file_info));
	arg->file_name = (char *)malloc(sizeof(char)*MAX_SIZE);
	arg->path = (char *)malloc(sizeof(char)*MAX_SIZE);

	strcpy(arg->file_name, arguments->file_name);
	strcpy(arg->path, arguments->path);

	pthread_mutex_unlock(&mutex);

	result_6.data.data_val = (char *)malloc(sizeof(char) * MAX_BINARY_FILE);
	result_6.data.data_len = MAX_BINARY_FILE;
	result_6.name = (char *)malloc(sizeof(char) * MAX_SIZE);

	retval_6 = get_file_1(arg, &result_6, clnt_data);
	if (retval_6 != RPC_SUCCESS) {
		clnt_perror(clnt_data, "get call failed");
	}

	if (result_6.data.data_len <= 0)
	{
		printf("Error in downloading file\n");
		return;
	}

	FILE *pipe = fopen(result_6.name, "wb");

	fwrite(result_6.data.data_val, 1, result_6.data.data_len, pipe);
	fclose(pipe);	
	free(arg->file_name);
	free(arg->path);
	free(arg);
}

void
ftp_prog_1(char *host)
{
	char user_input[MAX_SIZE];

	pthread_mutex_init (&mutex , NULL);	

	arguments = (file_info *)malloc(sizeof(file_info));
	arguments->file_name = (char *)malloc(sizeof(char)*MAX_SIZE);
	arguments->path = (char *)malloc(sizeof(char)*MAX_SIZE);

#ifndef	DEBUG
	clnt = clnt_create(host, FTP_PROG, FTP_VERS, "tcp");
	if (clnt == (CLIENT *) NULL) {
		clnt_pcreateerror(host);
		exit(1);
	}

	clnt_data = clnt_create(host, FTP_PROG, FTP_VERS, "tcp");
	if (clnt_data == (CLIENT *) NULL) {
		clnt_pcreateerror(host);
		exit(1);
	}
#endif	/* DEBUG */

	auth_destroy(clnt->cl_auth);
	clnt->cl_auth = authunix_create_default();

	auth_destroy(clnt_data->cl_auth);
	clnt_data->cl_auth = authunix_create_default();

	result_5.content_view = (char *)malloc(sizeof(char)*MAX_SIZE);

	retval_5 = init_1((void *)&init_5_arg, &result_5, clnt);
	if (retval_5 != RPC_SUCCESS) {
		clnt_perror(clnt, "init call failed");
	}
	
	memset(pwd, 0, MAX_SIZE);

	strcpy(pwd, result_5.content_view);

	while(1)
	{
		pthread_mutex_lock(&mutex);

		// SET UP DEFAULT ARGUMENTS
		memset(arguments->file_name, 0 , MAX_SIZE);
		memset(arguments->path, 0 , MAX_SIZE);

		strcpy(arguments->file_name, EMPTY);		
		strcpy(arguments->path, pwd);

		memset(&user_input, 0, strlen(user_input));

		printf("client@host:%s$ ", pwd);

		fgets(&user_input, sizeof(user_input), stdin);
		
		pthread_mutex_unlock(&mutex);

		command = eval_command(user_input, arguments);

		switch (command)
		{
		case EXIT_CMD:
			printf("Shut down.\n");
			free(arguments->file_name);
			free(arguments->path);
			free(arguments);
#ifndef	DEBUG
				clnt_destroy(clnt);
#endif		/* DEBUG */

			exit(0);
		case LS_CMD:
			result_2.content_view = (char *)malloc(sizeof(char)*MAX_LS_CONTENT);

			retval_2 = ls_1(arguments, &result_2, clnt);

			if (retval_2 != RPC_SUCCESS) {
				clnt_perror(clnt, "ls call failed");
			}

			printf("%s\n", result_2.content_view);

			free(result_2.content_view);

			break;
		case CD_CMD:
			retval_1 = cd_1(arguments, &result_1, clnt);
			if (retval_1 != RPC_SUCCESS) {
				clnt_perror(clnt, "cd call failed");
			}
			
			if (result_1 == SUCCESS_OP)
			{
				sprintf(pwd, "%s/%s", pwd, arguments->file_name);
			}

			eval_return_info(result_1);
			break;
		case CP_CMD:
			retval_3 = scp_1(arguments, &result_3, clnt);
			if (retval_3 != RPC_SUCCESS) {
				clnt_perror(clnt, "scp call failed");
			}
			
			eval_return_info(result_3);

			if (result_3 == SUCCESS_OP)
			{
				pthread_mutex_lock(&mutex);

				if ((Pthread_create(&client_pthread, NULL, send_file_thread, NULL)) == 0) 
				{
					printf("SEND FILE: %s/%s\n", arguments->path, arguments->file_name);
				}
			}
			break;
		case GET_CMD:
			retval_4 = wget_1(arguments, &result_4, clnt);
			if (retval_4 != RPC_SUCCESS) {
				clnt_perror(clnt, "wget call failed");
			}
			
			eval_return_info(result_4);

			if (result_3 == SUCCESS_OP)
			{
				pthread_mutex_lock(&mutex);
				
				if ((Pthread_create(&client_pthread, NULL, get_file_thread, NULL)) == 0) 
				{
					printf("GET FILE: %s/%s\n", arguments->path, arguments->file_name);
				}
			}
			break;
		case CD_BACK_CMD:
			break;
		default:
			printf("Invalid command!\n");
			break;
		}
	}
}


main(int argc, char *argv[])
{
	char *host;

	if (argc < 2) {
		printf("usage:  %s server_host\n", argv[0]);
		exit(1);
	}
	host = argv[1];
	printf("########JACEK SPOLNIK########\n");
	printf("Welcome to simple ftp client\n");
	printf("Options: exit, ls, cd, scp, wget\n\n");

	ftp_prog_1(host);
}

command_type
eval_command(char* input, file_info* arguments)
{
	char* exit_cmd = "exit";
	char* ls_cmd = "ls";
	char* cd_cmd = "cd";
	char* wget_dir_cmd = "wget -r";
	char* scp_dir_cmd = "scp -r";
	char* wget_cmd = "wget";
	char* scp_cmd = "scp";

	char path[MAX_SIZE];
	char file_name[MAX_SIZE];
	char temp_pwd[MAX_SIZE];

	memset(path, 0, MAX_SIZE);
	memset(file_name, 0, MAX_SIZE);
	memset(temp_pwd, 0, MAX_SIZE);

	if (!strncmp(exit_cmd, input, strlen(exit_cmd)))
	{
		return EXIT_CMD;
	}

	//LS
	if (!strncmp(ls_cmd, input, strlen(ls_cmd)))
	{
		return LS_CMD;
	}

	//CD
	if (!strncmp(cd_cmd, input, strlen(cd_cmd)))
	{
		if (strlen(input) < 5)
		{
			return UNKNOWN_CMD;
		}		

		if (!strcmp("cd ..\n", input))
		{
			if (strlen(pwd) < 2)
			{
				return CD_BACK_CMD;
			}

			strcpy(temp_pwd, pwd);

			char* to_delete = strrchr(temp_pwd, '/');
			memset(to_delete, 0, strlen(to_delete));

			memset(pwd, 0, MAX_SIZE);
			strcpy(pwd, temp_pwd);

			return CD_BACK_CMD;
		}

		sscanf(input, "cd %s\n", file_name);

		memset(arguments->file_name, 0, MAX_SIZE);
		strcpy(arguments->file_name, file_name);

		return CD_CMD;
	}

	//WGET
	if (!strncmp(wget_cmd, input, strlen(wget_cmd)))
	{
		if (strlen(input) < 5)
		{
			return UNKNOWN_CMD;
		}

		sscanf(input, "wget %s\n", file_name);

		memset(arguments->file_name, 0, MAX_SIZE);
		strcpy(arguments->file_name, file_name);

		return GET_CMD;
	}

	//SCP
	if (!strncmp(scp_cmd, input, strlen(scp_cmd)))
	{
		if (strlen(input) < 5)
		{
			return UNKNOWN_CMD;
		}
		
		sscanf(input, "scp %s\n", file_name);
		
		memset(arguments->file_name, 0, MAX_SIZE);
		strcpy(arguments->file_name, file_name);

		return CP_CMD;
	}

	return UNKNOWN_CMD;
}

void eval_return_info(int info)
{
	if (info != SUCCESS_OP)
	{
		if (info == NO_CATALOG_ERR)
		{
			printf("Error: file is not a catalog.\n");
		} 
		else if (info == NOT_EXIST_ERR)
		{
			printf("Error: file does not exist.\n");
		}
		else if (info == ACCESS_DENIED_ERR)
		{
			printf("Access denied\n");
		}
	}
}

int 
Pthread_create(pthread_t *tid, const pthread_attr_t *attr, void * (*func)(void *), void *arg) 
{
	int n;
	if ( (n = pthread_create(tid, attr, func, arg)) == 0) {
		return 0;
	}
	
	printf("pthread_create error");
	return 1;
}
