#include <iostream>
//#include <array>

#include "state_machine.h"
#include <limits.h>

using helper::Location;
using helper::StateMachine;

struct StateMachineStruct{
    std::string name;
    uint32_t id=0;
};



using getValueFuncType =std::function<uint32_t(const Location & loc)>;
using getStructValueFuncType =std::function <StateMachineStruct(const Location & loc)>;
using modifyPtrValueFuncType = std::function< bool(const Location & loc, uint32_t * val)>;
using modifyReferenceFuncType = std::function<bool(const Location & loc, uint32_t & val)>;
using RightvalueFuncType = std::function<void(const Location & loc, uint32_t && rVal)>;
using Event_Event1FuncType = std::function<void(const Location & loc, uint32_t code , const std::string & msg)>;
using Event_EventFuncType = std::function<void(const Location& loc, const std::string& msg)>;
using Event_FinalFuncType = std::function<void(const Location& loc, const std::string& msg)>;
using Event_ParallelFuncType = std::function<void(const Location& loc, const std::string& msg)>;
using Event_Any = std::function <void(const Location& loc)>;
using Response_Response1FuncType = std::function<void(const Location & loc, uint32_t code, std::string msg, std::string  msg2, void * user_data)>;

class StateMachineTest : public StateMachine {
public:
    StateMachineTest(const std::string& name) :StateMachine(name) {
        this->root.onentry[0]=([this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; });
        this->root.onexit[0]=([this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; });

        this->root.cond[0]= CONDITION(
            MessageType::REQUEST, getValueFuncType, [this](const Location& loc) {
                auto ret = this->getValue();
                std::cout << __FILE__ << ":" << __LINE__ << " Transition to children state children1" << std::endl;
                this->Transition("children1");
                return ret;
            }
        );

        this->root.cond[1]= CONDITION(
            MessageType::EVENT, Event_Event1FuncType,[this](const Location& loc, uint32_t code , const std::string& msg)->void {
					this->Event1(code , msg);
                    std::cout << __FILE__ << ":" << __LINE__ << " Transition to children state children2-1" << std::endl;
					this->Transition("children2-1");
                    return;
            }
        );

        this->root.cond[2] = CONDITION(
            MessageType::EVENT, Event_FinalFuncType, [this](const Location& loc, const std::string& msg)->void {
                std::cout << __FILE__ << ":" << __LINE__ << " Transition to final " << msg << std::endl;
				this->TransitionFinal();
                return;
            }
        );
        this->root.cond[3] = CONDITION(
            MessageType::EVENT, Event_FinalFuncType, [this](const Location& loc, const std::string& msg)->void {
                std::cout << __FILE__ << ":" << __LINE__ << " Transition to init " << msg << std::endl;
				this->TransitionRoot();
                return;
            }
        );
        this->root.cond[4] = CONDITION(
            MessageType::EVENT, Event_ParallelFuncType, [this](const Location& loc, const std::string& msg)->void {
                std::cout << __FILE__ << ":" << __LINE__ << " Transition to parallel-1-1 " << msg << std::endl;
				this->Transition("parallel-1-1");
                return;
            }
        );
        this->root.cond[5] =CONDITION(
            MessageType::EVENT, Event_Any, [this](const Location& loc)->void {
                std::cout << __FILE__ << ":" << __LINE__ << " anyFunction " << std::endl;
                return;
            }
        );

        this->root["children1"].onentry[0] = [this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; };
        this->root["children1"].onexit[0] = [this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; };
        this->root["children1"].cond[0] = CONDITION(
                  MessageType::REQUEST, getStructValueFuncType, [this](const Location& loc) {
                      auto ret = this->getStructValue();
                      std::cout << __FILE__ << ":" << __LINE__ << " Transition to Brother State children2" << std::endl;
                      this->Transition("children2");
                      return ret;
                  }
        );

        this->root["children1"].cond[1] = CONDITION(
            MessageType::REQUEST, modifyReferenceFuncType, [this](const Location& loc, uint32_t& refVal) {
                      auto ret = this->modifyReference(refVal);
                      std::cout << __FILE__ << ":" << __LINE__ << " Transition to current state children1" << std::endl;
					  this->Transition("children1");
                      return ret;
                  } 
        );
        this->root["children1"].cond[2] = CONDITION(
            MessageType::REQUEST, RightvalueFuncType, [this](const Location& loc, uint32_t&& rVal)->void {
					  this->Rightvalue(std::forward<uint32_t>(rVal));
                      std::cout << __FILE__ << ":" << __LINE__ << " Transition to parent state init" << std::endl;
					  this->TransitionRoot();
                      return;
                  }
        );

        this->root["children1"]["children1-1"].onentry[0] = [this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; };
        this->root["children1"]["children1-1"].onexit[0] = [this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; };


        this->root["children2"].onentry[0] =[this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; };
        this->root["children2"].cond[0] = CONDITION(
            MessageType::RESPONSE, Response_Response1FuncType, [this](const Location& loc, uint32_t code , std::string  msg,std::string  msg2, void* user_data)->void
                {
					this->Response1(code, msg, msg2);
                    std::cout << __FILE__ << ":" << __LINE__ << " Transition to branch state of sibling state children1-1" << std::endl;
					this->Transition("children1-1");
                }
        );
        this->root["children2"].cond[1] = CONDITION(
            MessageType::REQUEST, modifyPtrValueFuncType, [this](const Location& loc, uint32_t* ptrVal)
                {
                    auto ret = this->modifyPtrValue(ptrVal);
                    std::cout << __FILE__ << ":" << __LINE__ << " Transition to Brother State children1" << std::endl;
					this->Transition("children1");
                    return ret;
                }
        );
        this->root["children2"].cond[2] = CONDITION(
            MessageType::EVENT, Event_ParallelFuncType, [this](const Location& loc, const std::string& msg)->void {
                std::cout << __FILE__ << ":" << __LINE__ << " Transition to parallel " << msg << std::endl;
				this->Transition("parallel");
                return;
            }
        );

        this->root["children2"].onexit[0] = [this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; };

        this->root["children2"]["children2-1"].onentry[0] = [this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; };
        this->root["children2"]["children2-1"].onexit[0] = [this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; };

        this->root.parallel["parallel"]["parallel-1"].onentry[0] = [this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; };
        this->root.parallel["parallel"]["parallel-1"].onexit[0] = [this]() {std::cout << " onexit " << this->GetCurStateId() << std::endl; };

        this->root.parallel["parallel"]["parallel-1"]["parallel-1-1"].onentry[0] = [this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; };
        this->root.parallel["parallel"]["parallel-1"]["parallel-1-1"].onexit[0] = [this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; };

        this->root.parallel["parallel"]["parallel-1"]["parallel-1-1"].cond[0]= CONDITION(
            MessageType::EVENT, Event_EventFuncType, [this](const Location& loc, const std::string& msg)->void {
              std::cout << __FILE__ << ":" << __LINE__ << " Transition to init " << msg << std::endl;
			  this->TransitionRoot();
              return;
            }
        );

        this->root.parallel["parallel"]["parallel-2"].onentry[0] = [this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; };
        this->root.parallel["parallel"]["parallel-2"].onexit[0] = [this]() {std::cout << " onexit " << this->GetCurStateId() << std::endl; };
        this->root.parallel["parallel"]["parallel-2"].cond[0] = CONDITION(
            MessageType::EVENT, Event_EventFuncType, [this](const Location& loc, const std::string& msg)->void {
              std::cout << __FILE__ << ":" << __LINE__ << " Transition to init " << msg << std::endl;
			  this->TransitionRoot();
              return;
            }
        );

        this->final.onentry[0] = [this]() {std::cout << " onentry " << this->GetCurStateId() << " final" << std::endl; };
    }

    virtual ~StateMachineTest() {

    };

private:
    uint32_t getValue() {
        std::cout << __FUNCTION__ << " return :" << UINT_MAX << std::endl;
        return UINT_MAX;
    }

    StateMachineStruct getStructValue() {
        StateMachineStruct stut;
        stut.name = "StateMachineStruct";
        stut.id = UINT_MAX;
        std::cout << __FUNCTION__ << " return :{" << stut.name << "," << stut.id << "}" << std::endl;
        return stut;
    }

    bool modifyPtrValue(uint32_t* ptrVal) {
        *ptrVal = INT_MIN;
        std::cout << __FUNCTION__ << ":" << *ptrVal << std::endl;
        return true;
    }

    bool modifyReference(uint32_t& refVal) {
        refVal = 1;
        std::cout << __FUNCTION__ << ":" << refVal << std::endl;
        return true;
    }

    void Rightvalue(uint32_t&& rVal) {
        std::cout << __FUNCTION__ << " address:" << &rVal << std::endl;
        return;
    }

    void Event1(uint32_t code, const std::string& msg) {
        std::cout << __FUNCTION__ << ": " << code << "------->" << msg << std::endl;
        return;
    }

    void Response1(uint32_t code, const std::string& msg, const std::string& msg2) {
        std::cout << __FUNCTION__ << ": " << code << "------->" << msg << "          " << msg2 << std::endl;
        return;
    }
};




int main()
{

    StateMachineTest stateMachine("state_machine_demo");
    stateMachine.Start();

    {
        auto ret = stateMachine.ADD_REQUEST_TASK(getValueFuncType);
        std::cout << "getValue return :" << ret << std::endl;
    }

    {
        std::cout << std::endl << std::endl << std::endl;
			auto ret = stateMachine.ADD_REQUEST_TASK(getStructValueFuncType);
        std::cout << "getStructValue return :{" << ret.name << "," << ret.id << "}" << std::endl;
    }
  
    {
        std::cout << std::endl << std::endl << std::endl;
        uint32_t val = 0;
        auto ret = stateMachine.ADD_REQUEST_TASK(modifyPtrValueFuncType, &val);
        std::cout << "modifyPtrValue  :" << val << std::endl;
    }

    {
        std::cout << std::endl << std::endl << std::endl;
        uint32_t val = 0;
        auto ret = stateMachine.ADD_REQUEST_TASK(modifyReferenceFuncType, val);
        std::cout << "modifyReference " << ":" << val << std::endl;
    }

    {
        std::cout << std::endl << std::endl << std::endl;
        uint32_t val = 2;
        std::cout << "Rightvalue address:" << &val << std::endl;
         stateMachine.ADD_REQUEST_TASK(RightvalueFuncType, std::move(val));
    }

    {
        std::cout << std::endl << std::endl << std::endl;
        uint32_t code  = 10000;
        stateMachine.ADD_EVENT_TASK(Event_Event1FuncType,  code ,"First Event test");
    }

    {
      std::thread([&]() {
        std::cout << std::endl << std::endl << std::endl;
        uint32_t code = 20000;
        std::string str1("Response test param1");
        std::string str2("Response test param2");
        stateMachine.ADD_RESPONSE_TASK(Response_Response1FuncType, code, "Response test param1", str2, nullptr);
        }
        ).detach();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    {
      std::cout << std::endl << std::endl << std::endl;
      uint32_t code = 10000;
      stateMachine.ADD_EVENT_TASK(Event_Event1FuncType, code ,  "Second Event test");
    }

    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.ADD_EVENT_TASK(Event_ParallelFuncType, "Parallel Event test");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.ADD_EVENT_TASK(Event_EventFuncType, "Event2 test");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.ADD_EVENT_TASK(Event_ParallelFuncType, "Parallel2 Event test");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.ADD_EVENT_TASK(Event_EventFuncType, "Event3 test");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.ADD_EVENT_TASK(Event_EventFuncType, "Event3 test");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.ADD_EVENT_TASK(Event_FinalFuncType, "Final Event test");
    }


    stateMachine.Stop();
    std::getchar();
};
