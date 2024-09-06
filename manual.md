Linux内核API中使用的主要数据结构包括：

1. **链表** (`struct list_head`)
   - 实现双向链表。

2. **哈希表** (`struct hlist_head`, `struct hlist_node`)
   - 用于快速查找和存储数据。

3. **红黑树** (`struct rb_root`, `struct rb_node`)
   - 自平衡的二叉搜索树，实现高效的键值存储和查找。

4. **位图** (`unsigned long bitmap`)
   - 处理位数据的高效结构，常用于资源管理。

5. **队列** (`struct kfifo`)
   - 先进先出（FIFO）队列，常用于内核中的数据流管理。

6. **时间管理** (`struct timer_list`)
   - 用于实现定时器。

7. **信号量** (`struct semaphore`)
   - 用于进程同步。

8. **互斥锁** (`struct mutex`)
   - 用于保护共享资源的互斥访问。

9. **条件变量** (`wait_queue_head_t`)
   - 用于进程等待条件满足。

10. **文件系统中的结构** (`struct file`, `struct inode`, `struct dentry`)
    - 用于文件及目录的管理。

这些数据结构在内核开发中广泛使用，为实现高效的资源管理、进程调度和数据存储提供了基础。