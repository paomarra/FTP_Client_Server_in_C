const SUCCESS_OP = 0;
const NO_CATALOG_ERR = 1;
const NOT_EXIST_ERR = 2;
const ACCESS_DENIED_ERR = 3;

const MAX_SIZE = 128;
const MAX_LS_CONTENT = 10240;
const MAX_BINARY_FILE = 10485760;

struct file_info
{
	string path<MAX_SIZE>;
	string file_name<MAX_SIZE>;
};

struct catalog_state
{
	string content_view<MAX_LS_CONTENT>;
};

struct file_content_info
{
	char data<>;
	string name<MAX_SIZE>;	
};

program FTP_PROG
{
	version FTP_VERS
	{
		catalog_state init(void)	= 0;
		int cd(file_info)	= 1;
		catalog_state ls(file_info)	= 2;
		int scp(file_info)	= 3;
		int wget(file_info)	= 4;
		file_content_info get_file(file_info) = 5;
		int send_file(file_content_info) = 6;
	} = 1;
} = 0x33224358;
