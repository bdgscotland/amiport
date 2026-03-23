/*
 * queue.h -- BSD LIST macros vendored for AmigaOS 3.x port of patch
 * amiport: sys/queue.h not available on AmigaOS; vendor the required macros.
 *
 * Originally from OpenBSD sys/queue.h, ISC license.
 * Only LIST_ macros are included (ed.c uses: LIST_HEAD, LIST_ENTRY,
 * LIST_INIT, LIST_INSERT_HEAD, LIST_INSERT_AFTER, LIST_INSERT_BEFORE,
 * LIST_REMOVE, LIST_FOREACH, LIST_EMPTY, LIST_FIRST, LIST_NEXT).
 */

#ifndef QUEUE_H
#define QUEUE_H

/*
 * Singly-linked List definitions — not used here, kept for completeness.
 */

/*
 * List definitions.
 */
#define LIST_HEAD(name, type)                                           \
struct name {                                                           \
    struct type *lh_first;  /* first element */                        \
}

#define LIST_HEAD_INITIALIZER(head)                                     \
    { NULL }

#define LIST_ENTRY(type)                                                \
struct {                                                                \
    struct type *le_next;   /* next element */                         \
    struct type **le_prev;  /* address of previous next element */     \
}

/*
 * List functions.
 */
#define LIST_EMPTY(head)        ((head)->lh_first == NULL)

#define LIST_FIRST(head)        ((head)->lh_first)

#define LIST_FOREACH(var, head, field)                                  \
    for ((var) = LIST_FIRST((head));                                    \
        (var);                                                          \
        (var) = LIST_NEXT((var), field))

#define LIST_INIT(head)         ((head)->lh_first = NULL)

#define LIST_INSERT_AFTER(listelm, elm, field)                          \
do {                                                                    \
    if (((elm)->field.le_next = (listelm)->field.le_next) != NULL)     \
        (listelm)->field.le_next->field.le_prev =                      \
            &(elm)->field.le_next;                                      \
    (listelm)->field.le_next = (elm);                                   \
    (elm)->field.le_prev = &(listelm)->field.le_next;                  \
} while (0)

#define LIST_INSERT_BEFORE(listelm, elm, field)                         \
do {                                                                    \
    (elm)->field.le_prev = (listelm)->field.le_prev;                   \
    (elm)->field.le_next = (listelm);                                   \
    *(listelm)->field.le_prev = (elm);                                  \
    (listelm)->field.le_prev = &(elm)->field.le_next;                  \
} while (0)

#define LIST_INSERT_HEAD(head, elm, field)                              \
do {                                                                    \
    if (((elm)->field.le_next = (head)->lh_first) != NULL)             \
        (head)->lh_first->field.le_prev = &(elm)->field.le_next;       \
    (head)->lh_first = (elm);                                           \
    (elm)->field.le_prev = &(head)->lh_first;                          \
} while (0)

#define LIST_NEXT(elm, field)   ((elm)->field.le_next)

#define LIST_REMOVE(elm, field)                                         \
do {                                                                    \
    if ((elm)->field.le_next != NULL)                                   \
        (elm)->field.le_next->field.le_prev =                          \
            (elm)->field.le_prev;                                       \
    *(elm)->field.le_prev = (elm)->field.le_next;                      \
} while (0)

#endif /* QUEUE_H */
