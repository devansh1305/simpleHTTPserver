#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/dir.h>
int main(void)
{
	struct dirent **namelist;
	int n;
	int i=0;
	n = scandir(".", &namelist,NULL,alphasort);
	if (n < 0)
		perror("scandir");
	else {
		while (i<n) {
			printf("%s\n", namelist[i]->d_name);
			free(namelist[i]);
			++i;
		}
		free(namelist);
	}
	return 0;
}
