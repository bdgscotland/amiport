/*
 * scandir.c -- POSIX scandir/alphasort shim for AmigaOS
 *
 * amiport: implements scandir() using amiport_opendir/readdir/closedir.
 * Written in C89 for AmigaOS 3.x compatibility.
 */

#include <amiport/scandir.h>
#include <stdlib.h>
#include <string.h>

#define SCANDIR_INITIAL_SIZE 32

int
amiport_scandir(const char *dirname, struct amiport_dirent ***namelist,
    int (*sel)(const struct amiport_dirent *),
    int (*compar)(const struct amiport_dirent **, const struct amiport_dirent **))
{
	AMIPORT_DIR *dir;
	struct amiport_dirent *entry;
	struct amiport_dirent **list = NULL;
	struct amiport_dirent *copy;
	int count = 0;
	int alloc = 0;

	dir = amiport_opendir(dirname);
	if (dir == NULL)
		return -1;

	while ((entry = amiport_readdir(dir)) != NULL) {
		/* Apply filter if provided */
		if (sel != NULL && !sel(entry))
			continue;

		/* Grow the array if needed */
		if (count >= alloc) {
			struct amiport_dirent **newlist;
			alloc = (alloc == 0) ? SCANDIR_INITIAL_SIZE : alloc * 2;
			newlist = (struct amiport_dirent **)realloc(list,
			    (size_t)alloc * sizeof(struct amiport_dirent *));
			if (newlist == NULL) {
				/* Free everything allocated so far */
				int i;
				for (i = 0; i < count; i++)
					free(list[i]);
				free(list);
				amiport_closedir(dir);
				return -1;
			}
			list = newlist;
		}

		/* Copy the entry */
		copy = (struct amiport_dirent *)malloc(sizeof(struct amiport_dirent));
		if (copy == NULL) {
			int i;
			for (i = 0; i < count; i++)
				free(list[i]);
			free(list);
			amiport_closedir(dir);
			return -1;
		}
		memcpy(copy, entry, sizeof(struct amiport_dirent));
		list[count++] = copy;
	}

	amiport_closedir(dir);

	/* Sort if comparator provided */
	if (compar != NULL && count > 0) {
		qsort(list, (size_t)count, sizeof(struct amiport_dirent *),
		    (int (*)(const void *, const void *))compar);
	}

	*namelist = list;
	return count;
}

int
amiport_alphasort(const struct amiport_dirent **a, const struct amiport_dirent **b)
{
	return strcmp((*a)->d_name, (*b)->d_name);
}
