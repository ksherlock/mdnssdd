/*
 * Copyright (c) 2008, Kelvin W Sherlock
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: main.c 43 2008-07-13 15:51:32Z  $ 
 */


#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/param.h>
#include <dirent.h>

#include <errno.h>
#include <signal.h>
#include <ctype.h>


#include <dns_sd.h>


#include "list.h"

volatile int fStop = 0;
volatile int fReload = 0;
volatile int fTrim = 0;

int vFlag = 0;


void catch_int(int sig_num)
{
	switch(sig_num)
	{
		case SIGINT:
			fStop = 1;
			break;
		case SIGHUP:
			fReload = 1;
			signal(SIGHUP, catch_int);
			break;
	}		
	
}






char * ltrim(char *cp)
{
	if (cp == NULL) return NULL;
	
	while (isspace(*cp)) cp++;
	if (!*cp) return NULL;
	
	return cp;
}

/* Macros for min/max. */
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif /* MIN */
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif  /* MAX */


char *load_file(const char *path, const char *name, off_t *fileSize, int *error)
{
	int fd;
	off_t size;
	char *cp = NULL;
	
	char buffer[MAXPATHLEN];
	
	int a,b;
	*error = 0;
	
	a = strlen(path);
	b = strlen(name);
	if (a + b + 2 > MAXPATHLEN)
	{
		*error = 1;
		return NULL;
	}
	
	strcpy(buffer, path);
	buffer[a] = '/';
	strcpy(buffer + a + 1, name);
	

	
	
	fd = open(buffer, O_RDONLY);
	if (fd < 0) 
	{
		if (errno != ENOENT) *error = 1;
		return NULL;
	}
	
	size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	
	if (size > 0)
	{
		cp = malloc(size + 1);
		if (cp)
		{
			if (read(fd, cp, size) == size)
			{
				
				while (isspace(cp[size-1]) && size > 0) size--;
				cp[size] = 0;
				if (size == 0)
				{
					free(cp);
					return NULL;
				}
				
				
				if (fileSize) *fileSize = size;
			}
			else {
				free(cp);
				cp = NULL;
				*error = 1;
			}
		}
		else
		{
			*error = 1;
		}
	}
	else if (size < 0) *error = 1;
	
	close(fd);

	return cp;
}


static void register_callback(
							  DNSServiceRef client, 
							  const DNSServiceFlags flags, 
							  DNSServiceErrorType errorCode,
							  const char *name, 
							  const char *regtype, 
							  const char *domain, 
							  void *context
							)
{
	// this is a callback, so destroying the serviceref here might be a bad idea.

	struct entry * e = (struct entry *)context;

	printf("%s.%s%s: ", name, regtype, domain);
	
	if (errorCode == kDNSServiceErr_NoError)
	{
		if (e) e->status = STATUS_OK;
		printf("Registered.\n");
	}
	else if (errorCode == kDNSServiceErr_NameConflict)
	{
		printf("Name in use.");
		e->status = STATUS_ERROR;
		fTrim = 1;
	}
	else
	{
		printf("Error %d\n", errorCode);
		e->status = STATUS_ERROR;
		fTrim = 1;
	}

	fflush(stdout);
}

#define str_coalesce(x) x ? x : "[null]"

void load_one(const char *path)
{
	char *name;
	char *type;
	char *domain;
	char *hostname;
	char *port;
	char *txt;
	off_t txtLength;
	//int error;
	int i;
	
	int iport;
		
	DNSServiceErrorType err;
	
	struct entry *e;
		
	name = load_file(path, "name", NULL, &i);
	type = load_file(path, "type", NULL, &i);
	domain = load_file(path, "domain", NULL, &i);
	hostname = load_file(path, "hostname", NULL, &i);
	port = load_file(path, "port", NULL, &i);
	txt = load_file(path, "txt", &txtLength, &i);
	
	iport = port ? atoi(port) : 0;
	
	
	e = new_entry(path);
	
	err = DNSServiceRegister(
					   &e->sr,
					   0,
					   kDNSServiceInterfaceIndexAny,
					   ltrim(name),
					   ltrim(type),
					   ltrim(domain),
					   ltrim(hostname),
					   htons(iport),
					   txt ? txtLength : 0,
					   txt,
					   register_callback,
					   e
	);
					   
	

		
	if (err != kDNSServiceErr_NoError)
	{
		printf("%s %s %s %s %s %d: error: %d\n", 
			   path, 
			   str_coalesce(name),
			   str_coalesce(type),
			   str_coalesce(domain),
			   str_coalesce(hostname),
			   iport,
			   err);
		
		delete_entry(e);	
	}
	else
	{
		e->fd = DNSServiceRefSockFD(e->sr);
		LIST_INSERT_HEAD(&head, e, entries);
	}
	
	if (name) free(name);
	if (type) free(type);
	if (domain) free(domain);
	if (hostname) free(hostname);
	if (port) free(port);
	if (txt) free(txt);
	
	return;
	
}


void load_dir()
{
	DIR *dp;
	struct dirent *d;
	
	dp = opendir(".");
	if (!dp) return;
	
	while (d = readdir(dp))
	{
		if (d->d_type != DT_DIR) continue;
		
		if (d->d_name[0] == '.')
		{
			if (d->d_name[1] == 0) continue;
			if (d->d_name[1] == '.' && d->d_name[2] == 0) continue;
		}
		load_one(d->d_name);
	}
	
	
	closedir(dp);
}









void loop(void)
{
	fd_set fds;	
	//struct timeval tv;
	
	struct entry * e;
	struct entry * tmp;
	int nfd;
	int result;
	
	for(;;)
	{

		FD_ZERO(&fds);	
		nfd = 0;
		
		LIST_FOREACH_SAFE(e, &head, entries, tmp)
		{
			if (e->status != STATUS_ERROR)
			{
				if (e->fd >= 0) 
				{
					FD_SET(e->fd, &fds);
					nfd = MAX(e->fd, nfd);
				}
			}
		}
		
		nfd++;
		//tv.tv_sec = 60;
		//tv.tv_usec = 0;
		
		result = select(nfd, &fds, (fd_set *)NULL, (fd_set *)NULL,NULL);
		if (result > 0)
		{
			LIST_FOREACH_SAFE(e, &head, entries, tmp)
			{

				if (e->status != STATUS_ERROR)
				{
					if (e->fd >= 0 && FD_ISSET(e->fd, &fds))
						DNSServiceProcessResult(e->sr);
				}
			}
			
		}
		if (result < 0) 
		{
			//if (errno != EINTR) break;
		}
		
		
		
		
		if (fStop) break;

		
		if (fReload)
		{
			printf("Reloading...\n");
			delete_list();
			
			load_dir();
			
			fReload = 0;
			fTrim = 0;
		}
		
		if (fTrim)
		{
			trim_list();
			fTrim = 0;
		}
	

	}
	printf("stopping...\n");
	fflush(stdout);

}

#ifndef CONFIG_DIR
#define CONFIG_DIR "/usr/local/etc/mdnssdd.d"
#endif

int main (int argc, char * const argv[])
{
	int c;
	const char *root = CONFIG_DIR;
	
	init_list();
	
	while ( (c = getopt(argc, argv, "r:vh")) != -1)
	{
		switch (c)
		{
			case 'v':
				vFlag++;
				break;
			case 'h':
				//...
				break;
				
			case 'r':
				root = optarg;
				break;
			default:
				printf("unknown option: ``%c''\n", c);
		}
		
	}
	
	
	
	if (chdir(root) != 0)
	{
		exit(1);
	}
	// chroot prevents DNSRegisterService from working.
	// chroot will fail if not root, so ignore errors.
	//chroot(".");
	
	load_dir();
	
	fStop = 0;
	fReload = 0;
	
	signal(SIGINT, catch_int);
	signal(SIGHUP, catch_int);
	
	loop();
	
	delete_list();
	
	exit(0);
}
