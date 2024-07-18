#include <iostream>
//#include <array>

#include "state_machine.h"
#include <limits.h>

using helper::Location;
using helper::StateMachine;

enum class MethodId {
    anyFunction = -1,
    getValue = 0,
    getStructValue,
    modifyPtrValue,
    modifyReference,
    Rightvalue,
    Event_Event1,
    Event_Parallel,
    Event_Final,
    Event_Event2,
    Event_Event3,
    Event_Parallel2,
    Response_Response1
};

struct StateMachineStruct{
    std::string name;
    uint32_t id;
};



typedef uint32_t(*getValueFuncType)(class StateMachineTest *, const Location & loc);
typedef StateMachineStruct(*getStructValueFuncType)(class StateMachineTest *, const Location & loc);
typedef bool (*modifyPtrValueFuncType)(class StateMachineTest *, const Location & loc, uint32_t * val);
typedef bool(*modifyReferenceFuncType)(class StateMachineTest *, const Location & loc, uint32_t & val);
typedef void(*RightvalueFuncType)(class StateMachineTest *, const Location & loc, uint32_t && rVal);
typedef void(*Event_Event1FuncType)(class StateMachineTest *, const Location & loc, uint32_t code , const std::string & msg);
typedef void(*Event_EventFuncType)(class StateMachineTest*, const Location& loc, const std::string& msg);
typedef void(*Event_FinalFuncType)(class StateMachineTest*, const Location& loc, const std::string& msg);
typedef void(*Event_ParallelFuncType)(class StateMachineTest*, const Location& loc, const std::string& msg);
typedef void(*Event_Any)(class StateMachineTest*, const Location& loc);
typedef void(*Response_Response1FuncType)(class StateMachineTest *, const Location & loc, uint32_t code, std::string msg, std::string  msg2, void * user_data);

class StateMachineTest : public StateMachine<StateMachineTest, MethodId> {
public:
    StateMachineTest(const std::string& name) :StateMachine(name) {

        this->Root().AddOnEntry([this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; });
        this->Root().AddOnExit([this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; });

        this->Root().AddCond({
            MessageType::REQUEST, MethodId::getValue, (void*)(getValueFuncType)[](StateMachineTest* pThis, const Location& loc) {
                auto ret = pThis->getValue();
                std::cout << __FILE__ << ":" << __LINE__ << " Transition to children state children1" << std::endl;
                pThis->Transition("children1");
                return ret;
            }
            });

        this->Root().AddCond({
            MessageType::EVENT, MethodId::Event_Event1, (void*)(Event_Event1FuncType)[](StateMachineTest* pThis, const Location& loc, uint32_t code , const std::string& msg)->void {
                    pThis->Event1(code , msg);
                    std::cout << __FILE__ << ":" << __LINE__ << " Transition to children state children2-1" << std::endl;
                    pThis->Transition("children2-1");
                    return;
            }
            });

        this->Root().AddCond({
            MessageType::EVENT, MethodId::Event_Final, (void*)(Event_FinalFuncType)[](StateMachineTest* pThis, const Location& loc, const std::string& msg)->void {
                std::cout << __FILE__ << ":" << __LINE__ << " Transition to final " << msg << std::endl;
                pThis->TransitionFinal();
                return;
            }
            });
        this->Root().AddCond({
            MessageType::EVENT, MethodId::Event_Event2, (void*)(Event_FinalFuncType)[](StateMachineTest* pThis, const Location& loc, const std::string& msg)->void {
                std::cout << __FILE__ << ":" << __LINE__ << " Transition to init " << msg << std::endl;
                pThis->TransitionRoot();
                return;
            }
            });
        this->Root().AddCond({
            MessageType::EVENT, MethodId::Event_Parallel2, (void*)(Event_ParallelFuncType)[](StateMachineTest* pThis, const Location& loc, const std::string& msg)->void {
                std::cout << __FILE__ << ":" << __LINE__ << " Transition to parallel-1-1 " << msg << std::endl;
                pThis->Transition("parallel-1-1");
                return;
            }
            });
        this->Root().AddCond({
            MessageType::EVENT, MethodId::anyFunction, (void*)(Event_Any)[](StateMachineTest* pThis, const Location& loc)->void {
                std::cout << __FILE__ << ":" << __LINE__ << " anyFunction " << std::endl;
                return;
            }
            });

        this->Root()["children1"].AddOnEntry([this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; });
        this->Root()["children1"].AddOnExit([this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; });
        this->Root()["children1"].AddCond({
                  MessageType::REQUEST, MethodId::getStructValue, (void*)(getStructValueFuncType)[](StateMachineTest* pThis, const Location& loc) {
                      auto ret = pThis->getStructValue();
                      std::cout << __FILE__ << ":" << __LINE__ << " Transition to Brother State children2" << std::endl;
                      pThis->Transition("children2");
                      return ret;
                  } });

        this->Root()["children1"].AddCond({
            MessageType::REQUEST, MethodId::modifyReference, (void*)(modifyReferenceFuncType)[](StateMachineTest* pThis, const Location& loc, uint32_t& refVal) {
                      auto ret = pThis->modifyReference(refVal);
                      std::cout << __FILE__ << ":" << __LINE__ << " Transition to current state children1" << std::endl;
                      pThis->Transition("children1");
                      return ret;
                  } });
        this->Root()["children1"].AddCond({
            MessageType::REQUEST, MethodId::Rightvalue, (void*)(RightvalueFuncType)[](StateMachineTest* pThis, const Location& loc, uint32_t&& rVal)->void {
                      pThis->Rightvalue(std::forward<uint32_t>(rVal));
                      std::cout << __FILE__ << ":" << __LINE__ << " Transition to parent state init" << std::endl;
                      pThis->TransitionRoot();
                      return;
                  } });

        this->Root()["children1"]["children1-1"].AddOnEntry([this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; });
        this->Root()["children1"]["children1-1"].AddOnExit([this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; });


        this->Root()["children2"].AddOnEntry([this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; });
        this->Root()["children2"].AddCond({
            MessageType::RESPONSE, MethodId::Response_Response1, (void*)(Response_Response1FuncType)[](StateMachineTest* pThis, const Location& loc, uint32_t code , std::string  msg,std::string  msg2, void* user_data)->void
                {
                    pThis->Response1(code, msg, msg2);
                    std::cout << __FILE__ << ":" << __LINE__ << " Transition to branch state of sibling state children1-1" << std::endl;
                    pThis->Transition("children1-1");
                }
            });
        this->Root()["children2"].AddCond({
            MessageType::REQUEST, MethodId::modifyPtrValue, (void*)(modifyPtrValueFuncType)[](StateMachineTest* pThis, const Location& loc, uint32_t* ptrVal)
                {
                    auto ret = pThis->modifyPtrValue(ptrVal);
                    std::cout << __FILE__ << ":" << __LINE__ << " Transition to Brother State children1" << std::endl;
                    pThis->Transition("children1");
                    return ret;
                }
            });
        this->Root()["children2"].AddCond({
            MessageType::EVENT, MethodId::Event_Parallel, (void*)(Event_ParallelFuncType)[](StateMachineTest* pThis, const Location& loc, const std::string& msg)->void {
                std::cout << __FILE__ << ":" << __LINE__ << " Transition to parallel " << msg << std::endl;
                pThis->Transition("parallel");
                return;
            } });

        this->Root()["children2"].AddOnExit([this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; });

        this->Root()["children2"]["children2-1"].AddOnEntry([this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; });
        this->Root()["children2"]["children2-1"].AddOnExit([this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; });

        this->Root().Parallel("parallel")["parallel-1"].AddOnEntry([this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; });
        this->Root().Parallel("parallel")["parallel-1"].AddOnExit([this]() {std::cout << " onexit " << this->GetCurStateId() << std::endl; });

        this->Root().Parallel("parallel")["parallel-1"]["parallel-1-1"].AddOnEntry([this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; });
        this->Root().Parallel("parallel")["parallel-1"]["parallel-1-1"].AddOnExit([this]() {std::cout << this->GetCurStateId() << " onexit " << std::endl; });

        this->Root().Parallel("parallel")["parallel-1"]["parallel-1-1"].AddCond({
            MessageType::EVENT, MethodId::Event_Event2, (void*)(Event_EventFuncType)[](StateMachineTest* pThis, const Location& loc, const std::string& msg)->void {
              std::cout << __FILE__ << ":" << __LINE__ << " Transition to init " << msg << std::endl;
              pThis->TransitionRoot();
              return;
            } });

        this->Root().Parallel("parallel")["parallel-2"].AddOnEntry([this]() {std::cout << " onentry " << this->GetCurStateId() << std::endl; });
        this->Root().Parallel("parallel")["parallel-2"].AddOnExit([this]() {std::cout << " onexit " << this->GetCurStateId() << std::endl; });
        this->Root().Parallel("parallel")["parallel-2"].AddCond({
            MessageType::EVENT, MethodId::Event_Event3, (void*)(Event_EventFuncType)[](StateMachineTest* pThis, const Location& loc, const std::string& msg)->void {
              std::cout << __FILE__ << ":" << __LINE__ << " Transition to init " << msg << std::endl;
              pThis->TransitionRoot();
              return;
            } }
        );

        this->Final().AddOnEntry([this]() {std::cout << " onentry " << this->GetCurStateId() << " final" << std::endl; });
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
        auto ret = stateMachine.AddRequetTask<getValueFuncType>(HELPER_FROM_HERE, MethodId::getValue);
        std::cout << "getValue return :" << ret << std::endl;
    }

    {
        std::cout << std::endl << std::endl << std::endl;
        auto ret = stateMachine.AddRequetTask<getStructValueFuncType>(HELPER_FROM_HERE, MethodId::getStructValue);
        std::cout << "getStructValue return :{" << ret.name << "," << ret.id << "}" << std::endl;
    }
  
    {
        std::cout << std::endl << std::endl << std::endl;
        uint32_t val = 0;
        auto ret = stateMachine.AddRequetTask<modifyPtrValueFuncType>(HELPER_FROM_HERE, MethodId::modifyPtrValue, &val);
        std::cout << "modifyPtrValue  :" << val << std::endl;
    }

    {
        std::cout << std::endl << std::endl << std::endl;
        uint32_t val = 0;
        auto ret = stateMachine.AddRequetTask<modifyReferenceFuncType>(HELPER_FROM_HERE, MethodId::modifyReference, val);
        std::cout << "modifyReference " << ":" << val << std::endl;
    }

    {
        std::cout << std::endl << std::endl << std::endl;
        uint32_t val = 2;
        std::cout << "Rightvalue address:" << &val << std::endl;
         stateMachine.AddRequetTask<RightvalueFuncType>(HELPER_FROM_HERE, MethodId::Rightvalue, std::move(val));
    }

    {
        std::cout << std::endl << std::endl << std::endl;
        uint32_t code  = 10000;
        stateMachine.AddEventTask<Event_Event1FuncType>(HELPER_FROM_HERE, MethodId::Event_Event1, code ,"First Event test");
    }

    {
      std::thread([&]() {
        std::cout << std::endl << std::endl << std::endl;
        uint32_t code = 20000;
        std::string str1("Response test param1");
        std::string str2("Response test param2");
        stateMachine.AddResponseTask<Response_Response1FuncType>(HELPER_FROM_HERE, MethodId::Response_Response1, code, "Response test param1", str2, nullptr);
        }
        ).detach();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    {
      std::cout << std::endl << std::endl << std::endl;
      uint32_t code = 10000;
      stateMachine.AddEventTask<Event_Event1FuncType>(HELPER_FROM_HERE, MethodId::Event_Event1, code, "Second Event test");
    }

    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.AddEventTask<Event_ParallelFuncType>(HELPER_FROM_HERE, MethodId::Event_Parallel, "Parallel Event test");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.AddEventTask<Event_EventFuncType>(HELPER_FROM_HERE, MethodId::Event_Event2, "Event2 test");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.AddEventTask<Event_ParallelFuncType>(HELPER_FROM_HERE, MethodId::Event_Parallel2, "Parallel2 Event test");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.AddEventTask<Event_EventFuncType>(HELPER_FROM_HERE, MethodId::Event_Event3, "Event3 test");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.AddEventTask<Event_EventFuncType>(HELPER_FROM_HERE, MethodId::Event_Event3, "Event3 test");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    {
      std::cout << std::endl << std::endl << std::endl;
      stateMachine.AddEventTask<Event_FinalFuncType>(HELPER_FROM_HERE, MethodId::Event_Final,"Final Event test");
    }


    stateMachine.Stop();
    std::getchar();
};
