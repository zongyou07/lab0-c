#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */


/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *head =
        (struct list_head *) malloc(sizeof(struct list_head));
    if (head)
        INIT_LIST_HEAD(head);
    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (!l)
        return;
    element_t *ele, *safe;
    list_for_each_entry_safe (ele, safe, l, list)
        q_release_element(ele);
    free(l);
}

/*Create new element with value s*/
element_t *new_element(char *s)
{
    element_t *ele = (element_t *) malloc(sizeof(element_t));
    if (!ele)
        return NULL;
    ele->value = strdup(s);
    if (!ele->value) {
        free(ele);
        return NULL;
    }
    return ele;
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *ele = new_element(s);
    if (!ele)
        return false;
    list_add(&ele->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *ele = new_element(s);
    if (!ele)
        return false;
    list_add_tail(&ele->list, head);
    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *ele = list_first_entry(head, element_t, list);
    list_del(head->next);
    if (sp) {
        strncpy(sp, ele->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    return ele;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *ele = list_last_entry(head, element_t, list);
    list_del(head->prev);
    if (sp) {
        strncpy(sp, ele->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    return ele;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *li;
    list_for_each (li, head)
        len++;
    return len;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;
    struct list_head **node = &head->next, *tmp = head->next;
    for (; tmp != head && tmp->next != head; tmp = tmp->next->next)
        node = &(*node)->next;
    tmp = *node;
    list_del(tmp);
    q_release_element(list_entry(tmp, element_t, list));
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head)
        return false;
    element_t *ele, *safe;
    bool isdup = false;
    list_for_each_entry_safe (ele, safe, head, list) {
        if (ele->list.next != head &&
            !strcmp(ele->value,
                    list_entry(ele->list.next, element_t, list)->value)) {
            list_del(&ele->list);
            q_release_element(ele);
            isdup = true;
        } else if (isdup) {
            list_del(&ele->list);
            q_release_element(ele);
            isdup = false;
        }
    }

    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head)
        return;
    struct list_head *node = head->next;
    for (; node != head && node->next != head; node = node->next) {
        struct list_head *next = node->next;
        list_del(node);
        list_add(node, next);
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    struct list_head *pre = head->prev, *cur = head->next, *next = NULL;
    for (; cur != pre; cur = next) {
        next = cur->next;
        list_del(cur);
        list_add(cur, pre);
    }
}

typedef unsigned char u8;
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
typedef int
    __attribute__((nonnull(2, 3))) (*list_cmp_func_t)(void *,
                                                      const struct list_head *,
                                                      const struct list_head *);
int sort_comp(void *p, const struct list_head *a, const struct list_head *b)
{
    return strcmp(list_entry(a, element_t, list)->value,
                  list_entry(b, element_t, list)->value);
}

/*
 * Returns a list organized in an intermediate format suited
 * to chaining of merge() calls: null-terminated, no reserved or
 * sentinel head node, "prev" links not maintained.
 */
__attribute__((nonnull(2, 3, 4))) static struct list_head *
merge(void *priv, list_cmp_func_t cmp, struct list_head *a, struct list_head *b)
{
    struct list_head *head = NULL, **tail = &head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            *tail = a;
            tail = &a->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }
    return head;
}

/*My q_sort function*/
void my_q_sort(void *priv, struct list_head *head, list_cmp_func_t cmp)
{
    struct list_head *node = head->prev, *pending = NULL;
    int count = 0;

    if (node == head->next) /* Zero or one elements */
        return;

    head->next->prev = NULL;

    do {
        int bits;
        struct list_head **tail = &pending;

        /* Find the least-significant clear bit in count */
        for (bits = count; bits & 1; bits >>= 1)
            tail = &(*tail)->prev;
        /* Do the indicated merge */
        if (likely(bits)) {
            struct list_head *a = *tail, *b = a->prev;

            a = merge(priv, cmp, b, a);
            /* Install the merged result in place of the inputs */
            a->prev = b->prev;
            *tail = a;
        }

        struct list_head *cur = node, *curhead = node;
        while (node && node->prev) {
            if (cmp(priv, node, node->prev) <= 0) {
                cur->next = node->prev;
                cur = cur->next;
                node = node->prev;
            } else
                break;
        }
        cur->next = NULL;
        if (node)
            node = node->prev;
        curhead->prev = pending;
        pending = curhead;
        count++;
    } while (node);

    /* End of input; merge together all the pending lists. */
    node = pending;
    pending = pending->prev;
    for (;;) {
        if (!pending)
            break;
        struct list_head *next = pending->prev;

        node = merge(priv, cmp, pending, node);
        pending = next;
    }

    /* Rebuilding prev links */
    struct list_head *tail = head;
    while (node) {
        tail->next = node;
        node->prev = tail;
        tail = node;
        node = node->next;
    }
    tail->next = head;
    head->prev = tail;
}

/* Sort elements of queue in ascending order */
void q_sort(struct list_head *head)
{
    if (!head || list_empty(head))
        return;

    my_q_sort(NULL, head, sort_comp);
}
