#include "PCFG.h"
#include <pthread.h>
using namespace std;

// // 用于线程参数传递的结构体
// typedef struct {
//     segment* seg;            // 指向segment的指针
//     int start;               // 线程处理的起始索引
//     int end;                 // 线程处理的结束索引
//     string prefix;           // 前缀（对多segment的PT有用）
//     vector<string>* guesses; // 指向结果集的指针
//     pthread_mutex_t* mutex;  // 互斥锁
//     int* total_guesses;      // 总猜测计数器
// } thread_data_t;

void PriorityQueue::CalProb(PT &pt)
{
    // 计算PriorityQueue里面一个PT的流程如下：
    // 1. 首先需要计算一个PT本身的概率。例如，L6S1的概率为0.15
    // 2. 需要注意的是，Queue里面的PT不是"纯粹的"PT，而是除了最后一个segment以外，全部被value实例化的PT
    // 3. 所以，对于L6S1而言，其在Queue里面的实际PT可能是123456S1，其中"123456"为L6的一个具体value。
    // 4. 这个时候就需要计算123456在L6中出现的概率了。假设123456在所有L6 segment中的概率为0.1，那么123456S1的概率就是0.1*0.15

    // 计算一个PT本身的概率。后续所有具体segment value的概率，直接累乘在这个初始概率值上
    pt.prob = pt.preterm_prob;

    // index: 标注当前segment在PT中的位置
    int index = 0;


    for (int idx : pt.curr_indices)
    {
        // pt.content[index].PrintSeg();
        if (pt.content[index].type == 1)
        {
            // 下面这行代码的意义：
            // pt.content[index]：目前需要计算概率的segment
            // m.FindLetter(seg): 找到一个letter segment在模型中的对应下标
            // m.letters[m.FindLetter(seg)]：一个letter segment在模型中对应的所有统计数据
            // m.letters[m.FindLetter(seg)].ordered_values：一个letter segment在模型中，所有value的总数目
            pt.prob *= m.letters[m.FindLetter(pt.content[index])].ordered_freqs[idx];
            pt.prob /= m.letters[m.FindLetter(pt.content[index])].total_freq;
            // cout << m.letters[m.FindLetter(pt.content[index])].ordered_freqs[idx] << endl;
            // cout << m.letters[m.FindLetter(pt.content[index])].total_freq << endl;
        }
        if (pt.content[index].type == 2)
        {
            pt.prob *= m.digits[m.FindDigit(pt.content[index])].ordered_freqs[idx];
            pt.prob /= m.digits[m.FindDigit(pt.content[index])].total_freq;
            // cout << m.digits[m.FindDigit(pt.content[index])].ordered_freqs[idx] << endl;
            // cout << m.digits[m.FindDigit(pt.content[index])].total_freq << endl;
        }
        if (pt.content[index].type == 3)
        {
            pt.prob *= m.symbols[m.FindSymbol(pt.content[index])].ordered_freqs[idx];
            pt.prob /= m.symbols[m.FindSymbol(pt.content[index])].total_freq;
            // cout << m.symbols[m.FindSymbol(pt.content[index])].ordered_freqs[idx] << endl;
            // cout << m.symbols[m.FindSymbol(pt.content[index])].total_freq << endl;
        }
        index += 1;
    }
    // cout << pt.prob << endl;
}

void PriorityQueue::init()
{
    // cout << m.ordered_pts.size() << endl;
    // 用所有可能的PT，按概率降序填满整个优先队列
    for (PT pt : m.ordered_pts)
    {
        for (segment seg : pt.content)
        {
            if (seg.type == 1)
            {
                // 下面这行代码的意义：
                // max_indices用来表示PT中各个segment的可能数目。例如，L6S1中，假设模型统计到了100个L6，那么L6对应的最大下标就是99
                // （但由于后面采用了"<"的比较关系，所以其实max_indices[0]=100）
                // m.FindLetter(seg): 找到一个letter segment在模型中的对应下标
                // m.letters[m.FindLetter(seg)]：一个letter segment在模型中对应的所有统计数据
                // m.letters[m.FindLetter(seg)].ordered_values：一个letter segment在模型中，所有value的总数目
                pt.max_indices.emplace_back(m.letters[m.FindLetter(seg)].ordered_values.size());
            }
            if (seg.type == 2)
            {
                pt.max_indices.emplace_back(m.digits[m.FindDigit(seg)].ordered_values.size());
            }
            if (seg.type == 3)
            {
                pt.max_indices.emplace_back(m.symbols[m.FindSymbol(seg)].ordered_values.size());
            }
        }
        pt.preterm_prob = float(m.preterm_freq[m.FindPT(pt)]) / m.total_preterm;
        // pt.PrintPT();
        // cout << " " << m.preterm_freq[m.FindPT(pt)] << " " << m.total_preterm << " " << pt.preterm_prob << endl;

        // 计算当前pt的概率
        CalProb(pt);
        // 将PT放入优先队列
        priority.emplace_back(pt);
    }
    // cout << "priority size:" << priority.size() << endl;
}

void PriorityQueue::PopNext()
{

    // 对优先队列最前面的PT，首先利用这个PT生成一系列猜测
    Generate(priority.front());

    // 然后需要根据即将出队的PT，生成一系列新的PT
    vector<PT> new_pts = priority.front().NewPTs();
    for (PT pt : new_pts)
    {
        // 计算概率
        CalProb(pt);
        // 接下来的这个循环，作用是根据概率，将新的PT插入到优先队列中
        for (auto iter = priority.begin(); iter != priority.end(); iter++)
        {
            // 对于非队首和队尾的特殊情况
            if (iter != priority.end() - 1 && iter != priority.begin())
            {
                // 判定概率
                if (pt.prob <= iter->prob && pt.prob > (iter + 1)->prob)
                {
                    priority.emplace(iter + 1, pt);
                    break;
                }
            }
            if (iter == priority.end() - 1)
            {
                priority.emplace_back(pt);
                break;
            }
            if (iter == priority.begin() && iter->prob < pt.prob)
            {
                priority.emplace(iter, pt);
                break;
            }
        }
    }

    // 现在队首的PT善后工作已经结束，将其出队（删除）
    priority.erase(priority.begin());
}

// 这个函数你就算看不懂，对并行算法的实现影响也不大
// 当然如果你想做一个基于多优先队列的并行算法，可能得稍微看一看了
vector<PT> PT::NewPTs()
{
    // 存储生成的新PT
    vector<PT> res;

    // 假如这个PT只有一个segment
    // 那么这个segment的所有value在出队前就已经被遍历完毕，并作为猜测输出
    // 因此，所有这个PT可能对应的口令猜测已经遍历完成，无需生成新的PT
    if (content.size() == 1)
    {
        return res;
    }
    else
    {
        // 最初的pivot值。我们将更改位置下标大于等于这个pivot值的segment的值（最后一个segment除外），并且一次只更改一个segment
        // 上面这句话里是不是有没看懂的地方？接着往下看你应该会更明白
        int init_pivot = pivot;

        // 开始遍历所有位置值大于等于init_pivot值的segment
        // 注意i < curr_indices.size() - 1，也就是除去了最后一个segment（这个segment的赋值预留给并行环节）
        for (int i = pivot; i < curr_indices.size() - 1; i += 1)
        {
            // curr_indices: 标记各segment目前的value在模型里对应的下标
            curr_indices[i] += 1;

            // max_indices：标记各segment在模型中一共有多少个value
            if (curr_indices[i] < max_indices[i])
            {
                // 更新pivot值
                pivot = i;
                res.emplace_back(*this);
            }

            // 这个步骤对于你理解pivot的作用、新PT生成的过程而言，至关重要
            curr_indices[i] -= 1;
        }
        pivot = init_pivot;
        return res;
    }

    return res;
}

// 单segment的PT线程函数
// void* generate_single_segment(void* arg) {
//     thread_data_t* data = (thread_data_t*)arg;
//     vector<string> local_guesses;
    
//     // 处理分配给该线程的segment值范围
//     for (int i = data->start; i < data->end; i++) {
//         string guess = data->seg->ordered_values[i];
//         local_guesses.push_back(guess);
//     }
    
//     // 临界区：将本地结果合并到全局结果
//     pthread_mutex_lock(data->mutex);
//     data->guesses->insert(data->guesses->end(), local_guesses.begin(), local_guesses.end());
//     *(data->total_guesses) += local_guesses.size();
//     pthread_mutex_unlock(data->mutex);
    
//     return NULL;
// }

// // 多segment的PT线程函数
// void* generate_multi_segment(void* arg) {
//     thread_data_t* data = (thread_data_t*)arg;
//     vector<string> local_guesses;
    
//     // 处理分配给该线程的segment值范围
//     for (int i = data->start; i < data->end; i++) {
//         string temp = data->prefix + data->seg->ordered_values[i];
//         local_guesses.push_back(temp);
//     }
    
//     // 临界区：将本地结果合并到全局结果
//     pthread_mutex_lock(data->mutex);
//     data->guesses->insert(data->guesses->end(), local_guesses.begin(), local_guesses.end());
//     *(data->total_guesses) += local_guesses.size();
//     pthread_mutex_unlock(data->mutex);
    
//     return NULL;
// }

// 这个函数是PCFG并行化算法的主要载体
// 尽量看懂，然后进行并行实现
void PriorityQueue::Generate(PT pt)
{
    // 确保线程池已初始化
    if (!threads_initialized) {
        init_thread_pool();
    }

    // 计算PT的概率
    CalProb(pt);

    // 对于只有一个segment的PT
    if (pt.content.size() == 1)
    {
        // 指向segment的指针
        segment *a;
        if (pt.content[0].type == 1) {
            a = &m.letters[m.FindLetter(pt.content[0])];
        } else if (pt.content[0].type == 2) {
            a = &m.digits[m.FindDigit(pt.content[0])];
        } else if (pt.content[0].type == 3) {
            a = &m.symbols[m.FindSymbol(pt.content[0])];
        }
        
        // 设置共享数据
        pthread_mutex_lock(&shared_data.mutex);
        shared_data.seg = a;
        shared_data.total_values = pt.max_indices[0];
        shared_data.guesses = &guesses;
        shared_data.total_guesses = &total_guesses;
        shared_data.task_type = TASK_SINGLE_SEGMENT;
        
        // 计算适当的块大小
        int total_values = pt.max_indices[0];
        shared_data.chunk_size = max(100, total_values / (NUM_THREADS * 10)); // 每个线程多次获取任务
        shared_data.next_index = 0;
        shared_data.active_threads = NUM_THREADS;
        shared_data.has_task = true;
        
        // 通知所有线程有新任务
        pthread_cond_broadcast(&shared_data.cond);
        
        // 等待所有任务完成
        while (shared_data.has_task) {
            pthread_cond_wait(&shared_data.done_cond, &shared_data.mutex);
        }
        
        pthread_mutex_unlock(&shared_data.mutex);
    }
    else // 多segment的PT
    {
        // 构建前缀（除最后一个segment外的所有值）
        string prefix;
        int seg_idx = 0;
        for (int idx : pt.curr_indices) {
            if (pt.content[seg_idx].type == 1) {
                prefix += m.letters[m.FindLetter(pt.content[seg_idx])].ordered_values[idx];
            } else if (pt.content[seg_idx].type == 2) {
                prefix += m.digits[m.FindDigit(pt.content[seg_idx])].ordered_values[idx];
            } else if (pt.content[seg_idx].type == 3) {
                prefix += m.symbols[m.FindSymbol(pt.content[seg_idx])].ordered_values[idx];
            }
            seg_idx += 1;
            if (seg_idx == pt.content.size() - 1) {
                break;
            }
        }
        
        // 获取最后一个segment
        segment *a;
        if (pt.content[pt.content.size() - 1].type == 1) {
            a = &m.letters[m.FindLetter(pt.content[pt.content.size() - 1])];
        } else if (pt.content[pt.content.size() - 1].type == 2) {
            a = &m.digits[m.FindDigit(pt.content[pt.content.size() - 1])];
        } else if (pt.content[pt.content.size() - 1].type == 3) {
            a = &m.symbols[m.FindSymbol(pt.content[pt.content.size() - 1])];
        }
        
        // 设置共享数据
        pthread_mutex_lock(&shared_data.mutex);
        shared_data.seg = a;
        shared_data.total_values = pt.max_indices[pt.content.size() - 1];
        shared_data.guesses = &guesses;
        shared_data.total_guesses = &total_guesses;
        shared_data.prefix = prefix;
        shared_data.task_type = TASK_MULTI_SEGMENT;
        
        // 计算适当的块大小
        int total_values = pt.max_indices[pt.content.size() - 1];
        shared_data.chunk_size = max(100, total_values / (NUM_THREADS * 10)); // 每个线程多次获取任务
        shared_data.next_index = 0;
        shared_data.active_threads = NUM_THREADS;
        shared_data.has_task = true;
        
        // 通知所有线程有新任务
        pthread_cond_broadcast(&shared_data.cond);
        
        // 等待所有任务完成
        while (shared_data.has_task) {
            pthread_cond_wait(&shared_data.done_cond, &shared_data.mutex);
        }
        
        pthread_mutex_unlock(&shared_data.mutex);
    }
}

// 工作线程函数
void* worker_thread_function(void* arg) {
    PriorityQueue* queue = (PriorityQueue*)arg;
    thread_shared_data_t* data = &queue->shared_data;
    vector<string> local_guesses;
    
    while (true) {
        // 等待任务
        pthread_mutex_lock(&data->mutex);
        while (!data->has_task && !data->shutdown) {
            pthread_cond_wait(&data->cond, &data->mutex);
        }
        
        // 检查是否要终止线程
        if (data->shutdown) {
            pthread_mutex_unlock(&data->mutex);
            break;
        }
        
        // 获取任务数据
        int start, end;
        // 获取下一个要处理的块
        start = data->next_index;
        data->next_index += data->chunk_size;
        end = min(data->next_index, data->total_values);
        
        // 如果已经没有块可处理，标记为空闲状态
        if (start >= data->total_values) {
            data->active_threads--;
            
            // 如果所有线程都空闲，通知主线程任务完成
            if (data->active_threads == 0) {
                data->has_task = false;
                pthread_cond_signal(&data->done_cond);
            }
            
            pthread_mutex_unlock(&data->mutex);
            continue;
        }
        
        pthread_mutex_unlock(&data->mutex);
        
        // 清空本地缓存，准备新任务
        local_guesses.clear();
        
        // 处理任务
        if (data->task_type == TASK_SINGLE_SEGMENT) {
            // 处理单segment任务
            for (int i = start; i < end; i++) {
                string guess = data->seg->ordered_values[i];
                local_guesses.push_back(guess);
            }
        } else if (data->task_type == TASK_MULTI_SEGMENT) {
            // 处理多segment任务
            for (int i = start; i < end; i++) {
                string temp = data->prefix + data->seg->ordered_values[i];
                local_guesses.push_back(temp);
            }
        }
        
        // 合并结果到全局猜测列表
        if (!local_guesses.empty()) {
            pthread_mutex_lock(&data->mutex);
            data->guesses->insert(data->guesses->end(), local_guesses.begin(), local_guesses.end());
            *(data->total_guesses) += local_guesses.size();
            pthread_mutex_unlock(&data->mutex);
        }
        
        pthread_mutex_lock(&data->mutex);
        // 如果已经没有更多的块要处理，减少活动线程计数
        if (data->next_index >= data->total_values) {
            data->active_threads--;
            
            // 如果所有线程都空闲，通知主线程任务完成
            if (data->active_threads == 0) {
                data->has_task = false;
                pthread_cond_signal(&data->done_cond);
            }
        }
        pthread_mutex_unlock(&data->mutex);
    }
    
    return NULL;
}

// 初始化线程池
void PriorityQueue::init_thread_pool() {
    if (threads_initialized) {
        return;
    }
    
    // 初始化共享数据
    pthread_mutex_init(&shared_data.mutex, NULL);
    pthread_cond_init(&shared_data.cond, NULL);
    pthread_cond_init(&shared_data.done_cond, NULL);
    shared_data.shutdown = false;
    shared_data.has_task = false;
    shared_data.active_threads = 0;
    
    // 创建工作线程
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&worker_threads[i], NULL, worker_thread_function, (void*)this) != 0) {
            cerr << "Error creating thread " << i << endl;
            // 清理已创建的线程
            shared_data.shutdown = true;
            pthread_cond_broadcast(&shared_data.cond);
            for (int j = 0; j < i; j++) {
                pthread_join(worker_threads[j], NULL);
            }
            return;
        }
    }
    
    threads_initialized = true;
}

// 销毁线程池
void PriorityQueue::destroy_thread_pool() {
    if (!threads_initialized) {
        return;
    }
    
    pthread_mutex_lock(&shared_data.mutex);
    shared_data.shutdown = true;
    pthread_cond_broadcast(&shared_data.cond);
    pthread_mutex_unlock(&shared_data.mutex);
    
    // 等待所有线程终止
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(worker_threads[i], NULL);
    }
    
    // 清理资源
    pthread_mutex_destroy(&shared_data.mutex);
    pthread_cond_destroy(&shared_data.cond);
    pthread_cond_destroy(&shared_data.done_cond);
    
    threads_initialized = false;
}