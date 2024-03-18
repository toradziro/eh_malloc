#include "eh_malloc.h"
#include <stdio.h>

typedef struct ListNode {
    int data;
    struct ListNode* prev;
    struct ListNode* next;
} ListNode;

ListNode* createNode(int data) {
    ListNode* newNode = (ListNode*)eh_malloc(sizeof(ListNode));
    if (newNode == NULL) {
        return NULL;
    }
    newNode->data = data;
    newNode->prev = NULL;
    newNode->next = NULL;
    return newNode;
}

void deleteNode(ListNode* node) {
    if (node != NULL)
    {
        eh_free(node);
    }
}

void appendNode(ListNode** head, int data) {
    ListNode* newNode = createNode(data);
    if (*head == NULL) {
        *head = newNode;
    } else {
        ListNode* lastNode = *head;
        while (lastNode->next != NULL) {
            lastNode = lastNode->next;
        }
        lastNode->next = newNode;
        newNode->prev = lastNode;
    }
}

#define trace printf("File: %s --- Function: %s --- Line: %d\n", __FILE__, __FUNCTION__, __LINE__);
void deleteList(ListNode** head) {
    ListNode* current = *head;
    while (current != NULL) {
        ListNode* next = current->next;
        deleteNode(current);
        current = next;
    }
    *head = NULL;
}

int main() {
    ListNode* head = NULL;

    for (int i = 0; i < 1000; i++) {
        appendNode(&head, i);
    }
    ListNode* current = head;
    int expectedValue = 0;
    while (current != NULL) {
        if (current->data != expectedValue) {
            printf("Data integrity check failed: expected %d, got %d\n", expectedValue, current->data);
            deleteList(&head);
            return 1;
        }
        current = current->next;
        expectedValue++;
    }

    printf("Data integrity check passed: all node values are correct.\n");

    
    deleteList(&head);

    printf("List has been created, filled, verified, and deleted successfully using custom allocator.\n");

    return 0;
}
