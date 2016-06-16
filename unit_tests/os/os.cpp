#include "os.hpp"
#include <utility>

extern "C" {
OS System;
}

static IOS* OSProxy = nullptr;

static OSResult TaskCreate(OSTaskProcedure entryPoint,
    const char* taskName,
    uint16_t stackSize,
    void* taskParameter,
    uint32_t priority,
    OSTaskHandle* taskHandle)
{
    if (OSProxy != nullptr)
    {
        return OSProxy->CreateTask(entryPoint, taskName, stackSize, taskParameter, priority, taskHandle);
    }
    else
    {
        return OSResultOutOfResources;
    }
}

static void RunScheduller(void)
{
    if (OSProxy != nullptr)
    {
        OSProxy->RunScheduller();
    }
}

static void TaskSleep(const OSTaskTimeSpan time)
{
    if (OSProxy != nullptr)
    {
        OSProxy->Sleep(time);
    }
}

static void TaskSuspend(OSTaskHandle task)
{
    if (OSProxy != nullptr)
    {
        OSProxy->SuspendTask(task);
    }
}

static void TaskResume(OSTaskHandle task)
{
    if (OSProxy != nullptr)
    {
        OSProxy->ResumeTask(task);
    };
}

OSReset::OSReset() : released(false)
{
}

OSReset::OSReset(OSReset&& arg) noexcept : released(arg.released)
{
    arg.released = true;
}

OSReset& OSReset::operator=(OSReset&& arg) noexcept
{
    OSReset tmp(std::move(arg));
    this->released = tmp.released;
    tmp.released = true;
    return *this;
}

OSReset::~OSReset()
{
    if (!released)
    {
        OSProxy = nullptr;
    }
}

OSReset InstallProxy(IOS* target)
{
    OSProxy = target;
    System.CreateTask = TaskCreate;
    System.ResumeTask = TaskResume;
    System.SuspendTask = TaskSuspend;
    System.SleepTask = TaskSleep;
    System.RunScheduler = RunScheduller;
    return OSReset();
}
