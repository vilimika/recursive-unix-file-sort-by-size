#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

//struct that'll be used to build link list
struct statz 
{
	int size;
	char * path;
	struct statz *next;
} *head;

//function prototype
int check_mode(char*);
struct statz* append(struct statz*, struct statz*);
void scan_files(char*, int);
void split_line(char*, char**);
struct statz *build_lst(int);
int get_size(char*);
void print_lst(struct statz*);


//dat main
int main(int argc, char **argv)
{
	//make da pipe
	int pfds[2];
	pipe(pfds);

	//if parent wait on info comin thru pipe
	if(fork()) {build_lst(pfds[0]);/* print_lst(head);*/}
	//child - scan files and pass thru pipe to parent
	else {scan_files(argv[1], pfds[1]);}

	return 0;
}

void print_lst(struct statz *cp)
{
	while(cp)
	{
		printf("%i\t%s\n",cp -> size, cp -> path);
		cp = cp -> next;
	}
}

//parent main fuction. will wait for info comin thru da pipe
struct statz *build_lst(int fd)
{
	char buf[100];
	char *token;
	struct statz *temp, *hp = NULL;

int i = 0;
	while(i <= 5)
	{
		if(read(fd, buf, sizeof(buf)))
		{
			
			//printf("%s\n",  buf);
			temp = (struct statz *) malloc(sizeof(struct statz));
			token = strtok(buf, " ");
			temp -> size = atoi(token);
			//printf("%i\t", temp -> size);
			token = strtok(NULL, " ");
			temp -> path = token;
			temp -> next = NULL;
			//printf("%s\n", temp -> path);
			if(hp) hp = append(temp, hp);
			else hp = temp;
		
		}
		i++;
		printf("%i\n",i);
	}
	
	return hp;
}


//function adds new items to the list
struct statz *append(struct statz *s, struct statz *hp)
{
	struct statz *cp, *pp;
	cp = hp;
	//printf("%i\t%s\n", s -> size, s -> path);
//	printf("%i %i\n", cp -> size, s -> size);
	
	if(cp -> size > s -> size)
	{
//	printf("1\n");
		s -> next = cp;
		hp = s;
		return hp;
	}
	else
	{
//	printf("2\n");
		while(s -> size >= cp -> size)
		{
		printf("poop\n");
			pp = cp;
//	printf("%i %i\n", cp -> size, s -> size);
			if(!cp -> next) break;
			cp = cp -> next;
		}
	
	
	//	printf("3\n");
		if(cp -> next)
		{
			pp -> next = s;
			s -> next = cp;
		}
		else cp -> next = s;
		return hp;
	}
}

//main child function. will find files and pass em thru the pipe
void scan_files(char *file_name, int p_fd)
{
	struct dirent ent;
	int chrd, fd, mode, size;
	char buf[100];
	char buf1[100];

	fd = open(file_name, O_RDONLY);
	if(fd == -1) {perror("open file error"); exit(1);}
	while(1)
	{
	printf("poop2\n");
		//read dir entries
		chrd = syscall(SYS_getdents64, fd, &ent, sizeof(struct dirent));
		//check if any errors present
		if(chrd == -1) {perror("error reading dir entries");}
		//check if eof
		if(chrd == 0){printf("end\n"); break;}
		//if no errors process entries
		if(strcmp(ent.d_name ,".") != 0 && strcmp(ent.d_name, "..") != 0 )
		{
			sprintf(buf, "%s/%s", file_name, ent.d_name);
			mode = check_mode(buf);
			//its a file - write da pipe
			if(mode == 1)
			{	
				size = get_size(buf);
				sprintf(buf1, "%i %s", size, buf); 
				write(p_fd, buf1, sizeof(buf1));
			}
			//its a dir - process
			else if(mode == 2) {scan_files(buf, p_fd);}
		}
		
		//move over file pointer
		lseek(fd, ent.d_off, SEEK_SET);
	}
}

//function returns size of file 
int get_size(char *file_name)
{
	struct stat sa;
	int result, size;

	result = lstat(file_name, &sa);
	if(result == -1) {printf("file info unavailable for %s", file_name); exit(1);}
		
	size = sa.st_size;
	return size;
}

//function checks if file is a regualr file or dir
int check_mode(char *file_name)
{
	struct stat sa;
	mode_t mode;
	int result, fd;

	fd = open(file_name, O_RDONLY);
	if(fd == -1) {perror("error opening file"); exit(1);}

	result = fstat(fd, &sa);
	if(result == -1) {printf("file info unavailable for %s", file_name); exit(1);}
	
	mode = sa.st_mode;

	close(fd);

	if(S_ISREG(mode)) return 1;
	if(S_ISDIR(mode)) return 2;
	return 0;
}

