#pragma once

#include <atomic>
#include <iostream>
#include <memory>

#include "aot/common/mem_pool.h"  // Boost pool for memory management

template <typename Type>
struct LockFreeNode {
    Type* session;  // The session associated with this node
    std::atomic<LockFreeNode*> next;  // Pointer to the next node
    std::atomic<LockFreeNode*> prev;  // Pointer to the previous node
};

template <typename Type>
class LockFreeLinkedList {
public:
    // Constructor: Initializes the list with a memory pool of given size
    LockFreeLinkedList(size_t poolSize) 
        : memoryPool_(poolSize) {
        // Create dummy head and tail nodes
        head_ = AllocateNode();
        tail_ = AllocateNode();
        head_->next.store(tail_);
        tail_->prev.store(head_);
    }

    // Destructor: Clears the list and deallocates the head and tail nodes
    ~LockFreeLinkedList() {
        Clear();
        DeallocateNode(head_);
        DeallocateNode(tail_);
    }

    // Adds a session to the end of the list
    void PushBack(Type* session) {
        LockFreeNode<Type>* newNode = AllocateNode();
        newNode->session = session;
        LockFreeNode<Type>* prevNode = tail_->prev.load();

        newNode->prev.store(prevNode);
        newNode->next.store(tail_);

        // Attempt to insert the node atomically
        while (!prevNode->next.compare_exchange_weak(prevNode->next, newNode)) {
            prevNode = tail_->prev.load();  // Update reference to the last node
        }

        prevNode->next.store(newNode);
        tail_->prev.store(newNode);
    }

    // Removes a session from the list
    bool Remove(Type* session) {
        LockFreeNode<Type>* currentNode = head_->next.load();

        while (currentNode != tail_) {
            if (currentNode->session == session) {
                LockFreeNode<Type>* prevNode = currentNode->prev.load();
                LockFreeNode<Type>* nextNode = currentNode->next.load();

                // Atomically remove the node
                prevNode->next.store(nextNode);
                nextNode->prev.store(prevNode);

                DeallocateNode(currentNode);
                return true;
            }
            currentNode = currentNode->next.load();
        }
        return false;  // Session not found
    }

    // Checks if the session exists in the list
    bool Contains(Type* session) {
        LockFreeNode<Type>* currentNode = head_->next.load();
        while (currentNode != tail_) {
            if (currentNode->session == session) {
                return true;
            }
            currentNode = currentNode->next.load();
        }
        return false;
    }

    // Clears the list
    void Clear() {
        LockFreeNode<Type>* currentNode = head_->next.load();
        while (currentNode != tail_) {
            LockFreeNode<Type>* nextNode = currentNode->next.load();
            DeallocateNode(currentNode);
            currentNode = nextNode;
        }
        head_->next.store(tail_);
        tail_->prev.store(head_);
    }

private:
    LockFreeNode<Type>* head_;  // Head node
    LockFreeNode<Type>* tail_;  // Tail node
    common::MemoryPool<LockFreeNode<Type>> memoryPool_;  // Memory pool for node management

    // Allocates a new node from the memory pool
    LockFreeNode<Type>* AllocateNode() {
        return memoryPool_.Allocate();
    }

    // Deallocates a node and returns it to the memory pool
    void DeallocateNode(LockFreeNode<Type>* node) {
        memoryPool_.Deallocate(node);
    }
};

