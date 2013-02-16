#include "ftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>

#define ERNIE_HOST "ernie"
#define JAGULAR_HOST "jagular"

#define SPOLNIK_ERNIE_UID 1157
#define SPOLNIK_JAGULAR_UID 1598

bool_t
init_1_svc(void *argp, catalog_state *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;
	char pwd[MAX_SIZE];

	memset(pwd, 0, MAX_SIZE);

	FILE *p = popen("pwd", "r");

	fscanf(p, "%s", pwd);

	result->content_view = (char *)malloc(sizeof(char)*MAX_SIZE);

	strcpy(result->content_view, pwd);
	
	pclose(p);	

	return (retval);
}

bool_t
cd_1_svc(file_info *argp, int *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;
	struct stat st;
	char pwd[MAX_SIZE];

	if(rqstp->rq_cred.oa_flavor == AUTH_UNIX) 
	{
		struct authunix_parms *au;

		au= (struct authunix_parms *)rqstp->rq_clntcred;

		if(!((!strcmp(au->aup_machname, ERNIE_HOST) && au->aup_uid == SPOLNIK_ERNIE_UID) ||
			(!strcmp(au->aup_machname, JAGULAR_HOST) && au->aup_uid == SPOLNIK_JAGULAR_UID)))
		{
			*result = ACCESS_DENIED_ERR;
			return (retval);
		}
	}

	memset(pwd, 0, MAX_SIZE);

	sprintf(pwd, "%s/%s", argp->path, argp->file_name);

	if(stat(pwd, &st) < 0)
	{
		*result = NOT_EXIST_ERR;
		return (retval);
	}

	if(!S_ISDIR(st.st_mode)) 
	{ 
		*result = NO_CATALOG_ERR;
		return (retval);
	}

	*result = SUCCESS_OP;

	return (retval);
}

bool_t
ls_1_svc(file_info *argp, catalog_state *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;
	DIR *directory;
	struct dirent *dir;
	struct stat st;
	char content[MAX_LS_CONTENT];
	int i;
	char error[] = "Access denied";
	char pwd[MAX_SIZE];

	if(rqstp->rq_cred.oa_flavor == AUTH_UNIX) 
	{
		struct authunix_parms *au;

		au= (struct authunix_parms *)rqstp->rq_clntcred;

		if(!((!strcmp(au->aup_machname, ERNIE_HOST) && au->aup_uid == SPOLNIK_ERNIE_UID) ||
			(!strcmp(au->aup_machname, JAGULAR_HOST) && au->aup_uid == SPOLNIK_JAGULAR_UID)))
		{
			result->content_view = (char *)malloc(sizeof(char)*MAX_LS_CONTENT);
			result->content_view = error;

			return (retval);
		}
	}

	memset(content, 0, MAX_LS_CONTENT);

	directory = opendir(argp->path);

	if (directory != NULL)
	{
		i = 0;
		while ((dir = readdir(directory)) != NULL)
		{
			if(strncmp(dir->d_name, ".", 1) == 0) 
			{
					continue;
			}

			memset(pwd, 0, MAX_SIZE);

			sprintf(pwd, "%s/%s", argp->path, dir->d_name);

			if (stat(pwd, &st) < 0)
			{
				continue;
			}

			if (i == 0)
			{
				strcpy(content, dir->d_name);

				if (S_ISDIR(st.st_mode))
				{
					strcat(content, "/");
				}

				i = FALSE;
			}
			else
			{
				if (i > 100)
				{
					sprintf(content, "%s\n%s", content, "...");
					break;
				}				

				sprintf(content, "%s\t%s", content, dir->d_name);

				if (S_ISDIR(st.st_mode))
				{				
					strcat(content, "/");
				}
			}

			free(&st);

			++i;
		}		
	}

	closedir(directory);	

	result->content_view = (char *)malloc(sizeof(char)*MAX_LS_CONTENT);
	result->content_view = content;

	return (retval);
}

bool_t
scp_1_svc(file_info *argp, int *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	if(rqstp->rq_cred.oa_flavor == AUTH_UNIX) 
	{
		struct authunix_parms *au;

		au= (struct authunix_parms *)rqstp->rq_clntcred;

		if(!((!strcmp(au->aup_machname, ERNIE_HOST) && au->aup_uid == SPOLNIK_ERNIE_UID) ||
			(!strcmp(au->aup_machname, JAGULAR_HOST) && au->aup_uid == SPOLNIK_JAGULAR_UID)))
		{
			*result = ACCESS_DENIED_ERR;
			return (retval);
		}
	}

	*result = SUCCESS_OP;

	return (retval);
}

bool_t
wget_1_svc(file_info *argp, int *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;
	struct stat st;
	char pwd[MAX_SIZE];	

	if(rqstp->rq_cred.oa_flavor == AUTH_UNIX) 
	{
		struct authunix_parms *au;

		au= (struct authunix_parms *)rqstp->rq_clntcred;

		if(!((!strcmp(au->aup_machname, ERNIE_HOST) && au->aup_uid == SPOLNIK_ERNIE_UID) ||
			(!strcmp(au->aup_machname, JAGULAR_HOST) && au->aup_uid == SPOLNIK_JAGULAR_UID)))
		{
			*result = ACCESS_DENIED_ERR;
			return (retval);
		}
	}

	memset(pwd, 0, MAX_SIZE);

	sprintf(pwd, "%s/%s", argp->path, argp->file_name);

	if(stat(pwd, &st) < 0)
	{
		*result = NOT_EXIST_ERR;
		return (retval);
	}	

	*result = SUCCESS_OP;

	return (retval);
}

bool_t
get_file_1_svc(file_info *argp, file_content_info *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;
	struct stat st;
	char acces_denied[] = "Access denied";
	char pwd[MAX_SIZE];
	int fin, size, count;
	
	if(rqstp->rq_cred.oa_flavor == AUTH_UNIX) 
	{
		struct authunix_parms *au;

		au= (struct authunix_parms *)rqstp->rq_clntcred;

		if(!((!strcmp(au->aup_machname, ERNIE_HOST) && au->aup_uid == SPOLNIK_ERNIE_UID) ||
			(!strcmp(au->aup_machname, JAGULAR_HOST) && au->aup_uid == SPOLNIK_JAGULAR_UID)))
		{
			result->name = (char *)malloc(sizeof(char)*MAX_SIZE);
			result->name = acces_denied;

			return (retval);
		}
	}

	memset(pwd, 0, MAX_SIZE);

	sprintf(pwd, "%s/%s", argp->path, argp->file_name);

	if (stat(pwd, &st) == 0)
	{
		if(S_ISDIR(st.st_mode)) 
		{ 
			printf("ERROR: It is a catalog, not a file.\n");
			result->data.data_len = -1;
			result->data.data_val = NULL;
			return (retval);
		}
		
	}

	fin = open(pwd, O_RDONLY);

	result->name = (char *)malloc(sizeof(char)*MAX_SIZE);
	result->name = argp->file_name;

	if (fin == -1)
	{
		printf("ERROR: Cannot open file\n");
		result->data.data_len = -1;
		result->data.data_val = NULL;
		close(fin);
		return (retval);
	}
	
	size = lseek(fin, 0, SEEK_END);
	close(fin);

	fin = open(pwd, O_RDONLY);
	
	result->data.data_len = size;
	result->data.data_val = (char *)malloc(sizeof(char) * size);
	
	count = read(fin, result->data.data_val, size);

	printf("FILE UPLOADED: %s\n", pwd);

	close(fin);

	return (retval);
}

bool_t
send_file_1_svc(file_content_info *argp, int *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	if(rqstp->rq_cred.oa_flavor == AUTH_UNIX) 
	{
		struct authunix_parms *au;

		au= (struct authunix_parms *)rqstp->rq_clntcred;

		if(!((!strcmp(au->aup_machname, ERNIE_HOST) && au->aup_uid == SPOLNIK_ERNIE_UID) ||
			(!strcmp(au->aup_machname, JAGULAR_HOST) && au->aup_uid == SPOLNIK_JAGULAR_UID)))
		{
			*result = ACCESS_DENIED_ERR;
			return (retval);
		}
	}

	FILE *pipe = fopen(argp->name,"wb");

	fwrite(argp->data.data_val, 1, argp->data.data_len, pipe);
	fclose(pipe);

	printf("FILE DOWNLOADED: %s\n", argp->name);

	*result = SUCCESS_OP;

	return (retval);
}

int
ftp_prog_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	(void) xdr_free(xdr_result, result);

	return (TRUE);
}
