#pragma once
#include <functional>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <map>
#include <list>
#include <algorithm>
#include <exception>
#include "message_buffer.h"
#include "thread_helper.h"
#include "location.h"

namespace helper {


    template <typename C, typename MethodId>
    class StateMachine {
    public:
        enum class MessageType {
            ANYTYPE = -1,
            REQUEST,
            RESPONSE,
            EVENT,
        };
    public:
        using Task = std::function<void(C* class_, void* method, bool general)>;

        class MethodCallData {
        public:
            MethodCallData(const Location& loc, const MessageType& t, const MethodId& id)
                :loc_(loc), type_(t), id_(id) {}
            virtual ~MethodCallData() {}
        public:
            //在状态机中使用的信息
            Location loc_; //需要传递给状态机处理函数，定位问题需要。
            MessageType type_;
            MethodId id_;
            Task task_;
        };

        using Action = std::function<void()>;

        struct Condition
        {
            MessageType type_;
            MethodId id_;
            void* func_ptr_ = nullptr;
        };

        class BaseState {
        public:
            BaseState(const std::string& id_) :id(id_) {}
            BaseState() {}
            virtual ~BaseState() {}
        public:
            const std::string& GetId() const { return id; }
            std::map<uint32_t, Action> onentry;
            std::map<uint32_t, Action> onexit;
        private:
            std::string id;
            class BaseState* parent = nullptr;
            bool active_ = false;
            friend class StateMachine;
        };


        class  Final : public BaseState {
        public:
            Final() {}
        };



        class State : public BaseState {
        public:
            class Parallel : public BaseState {
            public:
                Parallel() {}
                State& operator[](const std::string& keyval) {
                    return children_[keyval];
                }
                std::map<std::string, State> children_;
            private:

                friend class StateMachine;
            };
        public:
            State(const std::string& id)
                :BaseState(id) {}
            State() {}
            State& operator[](const std::string& keyval) {
                return children[keyval];
            }
        public:
            std::map<uint32_t, Condition> cond;
            std::map<std::string, State> children;
            std::map<std::string, Parallel> parallel;
            friend class StateMachine;
        };

    public:
        StateMachine(const std::string& name)
            :name_(name) {
        }
        virtual ~StateMachine()
        {
            Stop();
        }
    public:
        void Start() {
            thread_is_run_ = new bool();
            *thread_is_run_ = true;
            this->thread_run_ = std::thread(&StateMachine::Run, this); //启动线程
        }
        void Stop(bool consume_all_at_exit = true) {
            if (thread_run_.joinable()) { //线程在运行中
                if (!consume_all_at_exit && thread_is_run_) {
                    *thread_is_run_ = false; //设置运行标志为false
                }
                task_queue_.Put(nullptr);

                if (thread_run_.get_id() == std::this_thread::get_id()) {
                    thread_run_.detach(); //自己线程调用停止，不能join
                }
                else if (thread_run_.joinable()) {
                    thread_run_.join(); //其他线程调用停止，等待结束
                }
            }
        }

        const std::string& GetCurStateId() {
            return this->current_state_->GetId();
        }

        std::thread::id GetWorkerThreadId() {
            return thread_run_.get_id();
        }

        template<typename Method, typename... Args>
        auto AddRequetTask(Location&& loc, MethodId&& id, Args&&... args)
        {
            auto future = AddTask<Method, Args...>(std::forward<Location>(loc), MessageType::REQUEST, std::forward<MethodId>(id), std::forward<Args>(args)...);
            return future.get();
        }

        template<typename Method, typename... Args> //不等待返回
        void AddResponseTask(Location&& loc, MethodId&& id, Args&&... args)
        {
            AsyncAddTask<Method>(std::forward<Location>(loc), MessageType::RESPONSE, std::forward<MethodId>(id), std::forward<Args>(args)...);
            return;
        }

        template<typename Method, typename... Args>//不等待返回
        void AddEventTask(Location&& loc, MethodId&& id, Args&&... args)
        {
            AsyncAddTask<Method>(std::forward<Location>(loc), MessageType::EVENT, std::forward<MethodId>(id), std::forward<Args>(args)...);
            return;
        }

        virtual bool TransitionRoot()final {
            return Transition(&this->root);
        }

        virtual bool TransitionFinal()final {
            return Transition(&this->final);
        }

        virtual bool Transition(const std::string& target) final {
            return Transition(GetState(target));
        }

    protected:
        State root;
        Final final;
    private:
        BaseState* current_state_ = nullptr;
        std::map<std::string, BaseState*> stateId_map_;
        std::thread thread_run_;
        bool* thread_is_run_ = nullptr;
        helper::MessageBuffer<std::shared_ptr<MethodCallData>> task_queue_;
        std::string name_; //状态机名称，也用作线程名称
    private:
        //添加任务，packaged_task 中返回的future，不调用get()不会阻塞
        template<typename Method, typename... Args>
        auto AddTask(Location&& loc, MessageType&& type, MethodId&& id, Args&&... args)
        {

            using RetType = decltype(((Method)nullptr)(nullptr, loc, std::forward<Args>(args)...)); //推导返回值类型
            auto task_data = std::make_shared<StateMachine::MethodCallData>(loc, type, id);

            typedef RetType(*GeneralMethod)(C* class_, const Location& loc);
            auto func = [&loc, &args...](C* class_, void* method_, bool general) {
                if (general)
                    return ((GeneralMethod)(method_))(class_, loc);
                else
                    return ((Method)(method_))(class_, loc, std::forward<Args>(args)...);
            };

            //在这里使用统一的packpaged类型
            auto taskPack = std::make_shared<std::packaged_task<RetType(C* class_, void* method_, bool general)>>(func);

            std::future<RetType> futureRet = taskPack->get_future();
            task_data->task_ = [taskPack](C* class_, void* method_, bool general)->void { (*taskPack)(class_, method_, general); };
            task_queue_.Put(task_data);

            return futureRet;
        }

        template<typename Method, typename... Args>
        auto AsyncAddTask(Location&& loc, MessageType&& type, MethodId&& id, Args&&... args)->void
        {
            auto task_data = std::make_shared<StateMachine::MethodCallData>(loc, type, id);

            typedef void(*GeneralMethod)(C* class_, const Location& loc);
            //参数使用临时变量
            auto func = [loc, args...](C* class_, void* method_, bool general)->void {
                if (general)
                    return ((GeneralMethod)(method_))(class_, loc);
                else
                    return ((Method)(method_))(class_, loc, std::move(args)...);
            };

            //在这里使用统一的packpaged类型
            auto taskPack = std::make_shared<std::packaged_task<void(C* class_, void* method_, bool general)>>(func);

            task_data->task_ = [taskPack](C* class_, void* method_, bool general)->void { (*taskPack)(class_, method_, general); };
            task_queue_.Put(task_data);
            return;

        }

        //解析状态机结构
        void ParseState(BaseState* baseState, BaseState* parent) {
            baseState->parent = parent;
            stateId_map_[baseState->GetId()] = baseState;
            State* state = dynamic_cast<State*>(baseState);
            if (state) {
                for (auto& parallel : state->parallel) {
                    parallel.second.id = parallel.first;
                    ParseState(&parallel.second, baseState);
                }

                for (auto& child : state->children) {
                    child.second.id = child.first;
                    ParseState(&child.second, baseState);
                }
            }

            auto parallel = dynamic_cast<typename State::Parallel*>(baseState);
            if (parallel) {
                for (auto& child : parallel->children_) {
                    child.second.id = child.first;
                    ParseState(&child.second, baseState);
                }
            }

        }

        virtual bool Transition(BaseState* target_state) final {

            if (target_state)
            {
                //从当前状态到目标状态找到最短路径
                //1、从当前状态->当前状态
                //2、从当前状态->当前状态的子孙状态
                //3、从当前状态->兄弟状态
                //4、从当前状态->父状态
                //5、从当前状态->父状态的兄弟子状态分支


                //构建根到current_state的双向链表
                std::list<BaseState*> current_state_path_list;
                {
                    auto current_state_path = this->current_state_;
                    current_state_path_list.push_front(current_state_path);
                    while (current_state_path) {
                        current_state_path_list.push_front(current_state_path->parent);
                        current_state_path = current_state_path->parent;
                    }
                }

                //构建根到target_state的双向链表
                std::list<BaseState*> target_state_path_list;
                {
                    auto target_state_path = target_state;
                    target_state_path_list.push_front(target_state_path);
                    while (target_state_path) {
                        target_state_path_list.push_front(target_state_path->parent);
                        target_state_path = target_state_path->parent;
                    }
                }

                for (auto rcurr = current_state_path_list.rbegin(); rcurr != current_state_path_list.rend(); ++rcurr)// 逆序 
                {
                    auto same_state = std::find(target_state_path_list.begin(), target_state_path_list.end(), *rcurr);
                    //找到相同的父节点
                    if (same_state != target_state_path_list.end())
                    {
                        //处理离开路径状态
                        auto leave_state = current_state_path_list.rbegin();
                        while (leave_state != rcurr) {
                            processExit(*leave_state);
                            ++leave_state;
                        }
                        //设置当前状态为最短路径根结点
                        this->current_state_ = *same_state;
                        if (this->current_state_) {
                            this->current_state_->active_ = true;
                        }

                        //处理进入路径状态
                        auto entry_state = ++same_state;
                        while (entry_state != target_state_path_list.end()) {
                            processEntry(*entry_state);
                            entry_state++;
                        }
                        break;
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }

        BaseState* GetState(const std::string& stateId) {
            const auto& state = this->stateId_map_.find(stateId);
            if (state != stateId_map_.end()) {
                return state->second;
            }
            return nullptr;
        }

        BaseState* findActiveState(State* state) {
            if (state->active_ == true) {
                return state;
            }
            else {
                for (auto& child : state->children) {
                    auto active_ = findActiveState(&child.second);
                    if (active_) {
                        return active_;
                    }
                }
                for (auto& parallel : state->parallel) {
                    if (parallel.second.active_ == true) {
                        return &parallel.second;
                    }
                }
                return nullptr;
            }
        }

        void processExit(BaseState* leave) {
            auto* parallel = dynamic_cast<typename State::Parallel*>(leave);
            if (parallel) {//是parallel状态，离开所有子状态
                for (auto& child : parallel->children_) {
                    BaseState* active_state = findActiveState(&child.second);
                    while (active_state != nullptr && active_state != parallel) {
                        processExit(active_state);
                        active_state = active_state->parent;
                    }
                }
            }
            this->current_state_ = leave;
            this->current_state_->active_ = false;
            processOnExit(this->current_state_);
        }

        void processEntry(BaseState* entry) {

            if (dynamic_cast<State*>(entry->parent)) {//如果父状态不是parallel是State，设置为非活跃
                entry->parent->active_ = false;
            }

            if (entry->active_ == true) { //parallel 状态子状态已经进入过，不需要重新进入
                return;
            }

            this->current_state_ = entry;
            this->current_state_->active_ = true;
            processOnEntry(this->current_state_);

            auto* parallel = dynamic_cast<typename State::Parallel*>(entry);
            if (parallel) {//是parallel状态，进入所有子状态
                for (auto& child : parallel->children_) {
                    this->current_state_ = &child.second;
                    child.second.active_ = true;
                    processOnEntry(&child.second);
                }
            }
        }

        void processOnEntry(const BaseState* entry_state) {
            for (const auto& action : entry_state->onentry) {
                if (action.second) {
                    action.second();
                }
            }
        }

        void processOnExit(BaseState* leave_state) {
            for (const auto& action : leave_state->onexit) {
                if (action.second) {
                    action.second();
                }
            }
        }

        bool processTask(BaseState* filterState, std::shared_ptr<MethodCallData> task_data) {
            auto state = dynamic_cast<State*>(filterState);
            if (state) {
                for (const auto& c : state->cond) {
                    if ((task_data->type_ == c.second.type_) && (task_data->id_ == c.second.id_)) {
                        //processCondtion
                        task_data->task_(dynamic_cast<C*>(this), c.second.func_ptr_, false);
                        return true;
                    }
                    else if ((task_data->type_ == c.second.type_) && (c.second.id_ == (MethodId)-1)) {
                        //processCondtion
                        task_data->task_(dynamic_cast<C*>(this), c.second.func_ptr_, true);
                        return true;
                    }
                }
            }

            auto parallel = dynamic_cast<typename State::Parallel*>(filterState);
            if (parallel) {//是parallel状态，匹配所有子分支
                for (auto& child : parallel->children_) {
                    auto filter_Parallel = findActiveState(&child.second);
                    while (filter_Parallel != nullptr && filter_Parallel != parallel) {
                        if (processTask(filter_Parallel, task_data)) {
                            return true;
                        }
                        else {
                            filter_Parallel = filter_Parallel->parent;
                        }
                    }
                }
            }

            return false;
        }

        void Run() {
            helper::SetCurrentThreadName(this->name_.c_str());
            this->ParseState(&this->root, nullptr);
            this->current_state_ = &this->root;
            /*
                在自己线程调用Stop时，不能等待线程结束，
                在执行完 task 后，this对象已经释放，thread_is_run_ 变的不可访问。
            */
            bool* tmp_thread_is_run = this->thread_is_run_;
            processEntry(this->current_state_);

            while (*tmp_thread_is_run) {
                std::shared_ptr<MethodCallData>  task_data;
                if (task_queue_.Get(task_data) && task_data != nullptr) {
                    auto filterState = this->current_state_;
                    bool foundMsg = false;
                    while (filterState != nullptr && foundMsg == false) {
                        foundMsg = processTask(filterState, task_data);
                        if (!foundMsg)
                        {
                            filterState = filterState->parent;
                        }
                    }
                }
                if (task_data == nullptr) {
                    break;
                }

            }

            delete tmp_thread_is_run;
            tmp_thread_is_run = nullptr;

        }
    };
}//end namespace helper

