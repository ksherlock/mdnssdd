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
 * $Id: list.c 41 2008-07-03 00:21:57Z  $
 */

#include "list.h"
#include <string.h>
#include <strings.h>

struct listhead head = LIST_HEAD_INITIALIZER(head);

void init_list(void)
{
	LIST_INIT(&head);
}

struct entry *new_entry(const char *path)
{
	struct entry *e = malloc(sizeof(struct entry));
	
	if (e)
	{
		bzero(e, sizeof(e));
		e->fd = -1;
		e->ts = time(NULL);
		
		e->path = strdup(path);
	}
	return e;
}

void delete_entry(struct entry *e)
{
	if (e)
	{
		if (e->path) free(e->path);
		if (e->sr)  DNSServiceRefDeallocate(e->sr);
		free(e);
	}
}


void delete_list()
{
	while (!LIST_EMPTY(&head)) {
		struct entry *e = LIST_FIRST(&head);
		LIST_REMOVE(e, entries);
		delete_entry(e);
	}
}

/*
 * remove entries with errors.
 *
 */
void trim_list()
{
	struct entry *e;
	struct entry *tmp;
	
	LIST_FOREACH_SAFE(e, &head, entries, tmp)
	{
		if (e->status == STATUS_ERROR)
		{
			LIST_REMOVE(e, entries);
			delete_entry(e);
			continue;
		}
	}
}
