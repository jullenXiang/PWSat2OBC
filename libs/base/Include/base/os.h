#ifndef LIBS_BASE_OS_H
#define LIBS_BASE_OS_H

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <type_traits>
#include <utility>
#include "system.h"
#include "utils.h"

/**
 * @defgroup osal OS Abstraction layer
 * @{
 */

/**
 * @brief Maximal allowed operation timeout.
 */

static constexpr std::chrono::milliseconds InfiniteTimeout = std::chrono::milliseconds::max();

#ifndef ELAST
// newlib workaround
#if defined _NEWLIB_VERSION && defined __ELASTERROR
#define ELAST __ELASTERROR
#else
#error "stdlib does not define ELAST errno value."
#endif
#endif

/**
 * @brief Enumerator for all possible operating system error codes.
 */
enum class OSResult
{
    /** Success */
    Success = 0,
    /** @brief Requested operation is invalid. */
    InvalidOperation = ELAST,
    /** Power failure */
    PowerFailure = ELAST + 1,

    /** Requested element was not found. */
    NotFound = ENOENT,
    /** Interrupted system call */
    Interrupted = EINTR,
    /** I/O error */
    IOError = EIO,
    /** Argument list too long */
    ArgListTooLong = E2BIG,
    /** Bad file number */
    InvalidFileHandle = EBADF,
    /** No children */
    NoChildren = ECHILD,
    /** Not enough memory */
    NotEnoughMemory = ENOMEM,
    /** Permission denied */
    AccessDenied = EACCES,
    /** Bad address */
    InvalidAddress = EFAULT,
    /** Device or resource busy */
    Busy = EBUSY,
    /** File exists */
    FileExists = EEXIST,
    /** Cross-device link */
    InvalidLink = EXDEV,
    /** No such device */
    DeviceNotFound = ENODEV,
    /** Not a directory */
    NotADirectory = ENOTDIR,
    /** Is a directory */
    IsDirectory = EISDIR,
    /** Invalid argument */
    InvalidArgument = EINVAL,
    /** Too many open files in system */
    TooManyOpenFiles = ENFILE,
    /** File descriptor value too large */
    DescriptorTooLarge = EMFILE,
    /** File too large */
    FileTooLarge = EFBIG,
    /** No space left on device */
    OutOfDiskSpace = ENOSPC,
    /** Illegal seek */
    InvalidSeek = ESPIPE,
    /** Read-only file system */
    ReadOnlyFs = EROFS,
    /** Too many links */
    TooManyLinks = EMLINK,
    /** Result too large */
    OutOfRange = ERANGE,
    /** Deadlock */
    Deadlock = EDEADLK,
    /** No lock */
    NoLock = ENOLCK,
    /** A non blocking operation could not be immediately completed */
    WouldBlock = ENODATA,
    /** Operation timed out. */
    Timeout = ETIME,
    /** Protocol error */
    ProtocolError = EPROTO,
    /** Bad message */
    InvalidMessage = EBADMSG,
    /** Inappropriate file type or format */
    InvalidFileFormat = EFTYPE,
    /** Function not implemented */
    NotImplemented = ENOSYS,
    /** Directory not empty */
    DirectoryNotEmpty = ENOTEMPTY,
    /** File or path name too long */
    PathTooLong = ENAMETOOLONG,
    /** Too many symbolic links */
    LinkCycle = ELOOP,
    /** Operation not supported */
    NotSupported = EOPNOTSUPP,
    /** Protocol family not supported  */
    ProtocolNotSupported = EPFNOSUPPORT,
    /** No buffer space available */
    BufferNotAvailable = ENOBUFS,
    /** Protocol not available */
    ProtocolNotAvailable = ENOPROTOOPT,
    /** Unknown protocol */
    UnknownProtocol = EPROTONOSUPPORT,
    /** Illegal byte sequence */
    InvalidByteSequence = EILSEQ,
    /** Value too large for defined data type */
    Overflow = EOVERFLOW,
    /** Operation canceled */
    Cancelled = ECANCELED,
};

/**
 * @brief Macro for verification whether passed OSResult value indicates success.
 */
constexpr inline bool OS_RESULT_SUCCEEDED(OSResult x)
{
    return x == OSResult::Success;
}

/**
 * @brief Macro for verification whether passed OSResult value indicates failure.
 */
constexpr inline bool OS_RESULT_FAILED(OSResult x)
{
    return x != OSResult::Success;
}

/** @brief Type definition of handle to system task. */
using OSTaskHandle = void*;

/** @brief Type definition of semaphore handle. */
using OSSemaphoreHandle = void*;

/** @brief Type definition of event group handle. */
using OSEventGroupHandle = void*;

/** @brief Type definition of event group value. */
using OSEventBits = std::uint32_t;

/** @brief Type definition of queue handle */
using OSQueueHandle = void*;

/** @brief Type definition of pulse all handle */
using OSPulseHandle = void*;

/**
 * @brief Pointer to generic system procedure that operates on task.
 *
 * @param[in] task Task handle.
 */
using OSTaskProcedure = void (*)(OSTaskHandle task);

/**
 * Task priorities
 */
enum class TaskPriority
{
    Idle = 0, //!< Idle
    P1,       //!< P1
    P2,       //!< P2
    P3,       //!< P3
    P4,       //!< P4
    P5,       //!< P5
    P6,       //!< P6
    P7,       //!< P7
    P8,       //!< P8
    P9,       //!< P9
    P10,      //!< P10
    P11,      //!< P11
    P12,      //!< P12
    P13,      //!< P13
    P14,      //!< P14
    Highest   //!< Highest
};

/**
 * @brief Definition of operating system interface.
 */
class System final : public PureStatic
{
  public:
    /**
     * @brief Creates new task
     *
     * @param[in] entryPoint Pointer to task procedure.
     * @param[in] taskName Name of the new task.
     * @param[in] stackSize Size of the new task's stack in bytes.
     * @param[in] taskParameter Pointer to caller supplied object task context.
     * @param[in] priority New task priority.
     * @param[out] taskHandle Pointer to variable that will be filled with the created task handle.
     * @return Operation status.
     */
    static OSResult CreateTask(OSTaskProcedure entryPoint,
        const char* taskName,
        std::uint16_t stackSize,
        void* taskParameter,
        TaskPriority priority,
        OSTaskHandle* taskHandle);

    /**
     * @brief Suspends current task execution for specified time period.
     * @param[in] time Time period in ms.
     */
    static void SleepTask(const std::chrono::milliseconds time);

    /**
     * @brief Resumes execution of requested task.
     *
     * @param[in] task Task handle.
     * @remark This procedure should not be used from within interrupt service routine.
     */
    static void ResumeTask(OSTaskHandle task);

    /**
     * @brief Suspend execution of requested task.
     *
     * @param[in] task Task handle.
     * @remark This procedure should not be used from within interrupt service routine.
     */
    static void SuspendTask(OSTaskHandle task);

    /**
     * @brief Runs the system scheduler.
     */
    static void RunScheduler();

    /**
     * @brief Creates binary semaphore.
     *
     */
    static OSSemaphoreHandle CreateBinarySemaphore(std::uint8_t semaphoreId = 0);

    /**
     * @brief Acquires semaphore.
     *
     * @param[in] semaphore Handle to the semaphore that should be acquired.
     * @param[in] timeout Operation timeout.
     * @return Operation status.
     * @remark This procedure should not be used from within interrupt service routine.
     */
    static OSResult TakeSemaphore(OSSemaphoreHandle semaphore, std::chrono::milliseconds timeout);

    /**
     * @brief Releases semaphore.
     *
     * @param[in] semaphore Handle to semaphore that should be released.
     * @return Operation status.
     * @remark This procedure should not be used from within interrupt service routine.
     */
    static OSResult GiveSemaphore(OSSemaphoreHandle semaphore);

    /**
     * @brief Release semaphore from ISR
     * @param[in] semaphore Semaphore to release
     * @return Operation status
     */
    static OSResult GiveSemaphoreISR(OSSemaphoreHandle semaphore);

    /**
     * @brief Creates event group object.
     *
     * @return Event group handle on success, NULL otherwise.
     */
    static OSEventGroupHandle CreateEventGroup();

    /**
     * @brief Sets specific bits in the event group.
     *
     * @param[in] eventGroup Handle to the event group that should be updated.
     * @param[in] bitsToChange Bits that should be set.
     * @returns The value of the event group at the time the call to xEventGroupSetBits() returns.
     *
     * There are two reasons why the returned value might have the bits specified by the bitsToChange
     * parameter cleared:
     *  - If setting a bit results in a task that was waiting for the bit leaving the blocked state
     *  then it is possible the bit will have been cleared automatically.
     *  - Any unblocked (or otherwise Ready state) task that has a priority above that of the task
     *  that called EventGroupSetBits() will execute and may change the event group value before
     *  the call to EventGroupSetBits() returns.
     * @remark This procedure should not be used from within interrupt service routine.
     */
    static OSEventBits EventGroupSetBits(OSEventGroupHandle eventGroup, const OSEventBits bitsToChange);

    /**
     * @brief Sets specific bits in the event group from ISR
     *
     * @param[in] eventGroup Handle to the event group that should be updated.
     * @param[in] bitsToChange Bits that should be set.
     * @returns The value of the event group at the time the call to xEventGroupSetBits() returns.
     *
     * There are two reasons why the returned value might have the bits specified by the bitsToChange
     * parameter cleared:
     *  - If setting a bit results in a task that was waiting for the bit leaving the blocked state
     *  then it is possible the bit will have been cleared automatically.
     *  - Any unblocked (or otherwise Ready state) task that has a priority above that of the task
     *  that called EventGroupSetBits() will execute and may change the event group value before
     *  the call to EventGroupSetBits() returns.
     */
    static OSEventBits EventGroupSetBitsISR(OSEventGroupHandle eventGroup, const OSEventBits bitsToChange);

    /**
     * @brief Clears specific bits in the event group.
     *
     * @param[in] eventGroup Handle to the event group that should be updated.
     * @param[in] bitsToChange Bits that should be cleared.
     * @return The value of the event group before the specified bits were cleared.
     */
    static OSEventBits EventGroupClearBits(OSEventGroupHandle eventGroup, const OSEventBits bitsToChange);

    /**
     * @brief Gets current value of event group
     * @param eventGroup Event group handle
     * @return Value of event group from moment of call
     */
    static OSEventBits EventGroupGetBits(OSEventGroupHandle eventGroup);

    /**
     * @brief Suspends current task execution until the specific bits in the event group are set.
     *
     * @param[in] eventGroup The affected event group handle.
     * @param[in] bitsToWaitFor Bits that the caller is interested in.
     * @param[in] waitAll Flat indicating whether this procedure should return when all requested bits are set.
     * @param[in] autoReset Flag indicating whether the signaled bits should be cleared on function return.
     * @param[in] timeout Operation timeout in ms.
     *
     * @return The value of the event group at the time either the event bits being waited for became set,
     * or the block time expired.
     */
    static OSEventBits EventGroupWaitForBits(OSEventGroupHandle eventGroup,
        const OSEventBits bitsToWaitFor,
        bool waitAll,
        bool autoReset,
        const std::chrono::milliseconds timeout);

    /**
     * @brief Allocates block of memory from OS heap
     *
     * @param[in] size Size of the block to alloc
     */
    static void* Alloc(std::size_t size);

    /**
     * @brief Frees block of memory
     * @param[in] ptr Pointer to block to free
     * @see OSFree
     */
    static void Free(void* ptr);

    /**
     * @brief Creates queue
     *
     * @param[in] maxQueueElements Maximum number of elements in queue
     * @param[in] elementSize Size of single element
     * @return Queue handle on success, NULL otherwise
     */
    static OSQueueHandle CreateQueue(std::size_t maxQueueElements, std::size_t elementSize);

    /**
     * @brief Receives element form queue
     *
     * @param[in] queue Queue handle
     * @param[out] element Buffer for element
     * @param[in] timeout Operation timeout in ms.
     * @return TRUE if element was received, FALSE on timeout
     */
    static bool QueueReceive(OSQueueHandle queue, void* element, std::chrono::milliseconds timeout);

    /**
     * @brief Receives element form queue in interrupt handler
     *
     * @param[in] queue Queue handle
     * @param[out] element Buffer for element
     * @return TRUE if element was received, FALSE on timeout
     */
    static bool QueueReceiveFromISR(OSQueueHandle queue, void* element);

    /**
     * @brief Sends element to queue
     *
     * @param[in] queue Queue handle
     * @param[in] element Element to send to queue
     * @param[in] timeout Operation timeout in ms
     * @return TRUE if element was received, FALSE on timeout
     */
    static bool QueueSend(OSQueueHandle queue, const void* element, std::chrono::milliseconds timeout);

    /**
     * @brief Sends element to queue in interrupt handler
     *
     * @param[in] queue Queue handle
     * @param[in] element Element to send to queue
     * @return TRUE if element was received, FALSE on timeout
     */
    static bool QueueSendISR(OSQueueHandle queue, const void* element);

    /**
     * @brief Overwrites element in queue
     *
     * @param[in] queue QueueHandle
     * @param[in] element Element to send to queue
     */
    static void QueueOverwrite(OSQueueHandle queue, const void* element);

    /**
     * @brief Resets queue to empty state
     * @param queue Queue handle
     */
    static void QueueReset(OSQueueHandle queue);

    /**
     * @brief Procedure that should be called at the end of interrupt handler
     *
     */
    static void EndSwitchingISR();

    /**
     * @brief Creates pulse all event
     *
     */
    static OSPulseHandle CreatePulseAll();

    /**
     * @brief Waits for pulse
     *
     * @param[in] handle Pulse handle
     * @param[in] timeout Operation timeout in ms
     * @return Wait result
     */
    static OSResult PulseWait(OSPulseHandle handle, std::chrono::milliseconds timeout);

    /**
     * @brief Sets pulse event
     *
     * @param[in] handle Pulse handle
     */
    static void PulseSet(OSPulseHandle handle);

    /** @brief Yields task control */
    static void Yield();

    /**
     * @brief Gets number of miliseconds since system start
     * @return Number of miliseconds since system start
     */
    static std::chrono::milliseconds GetUptime();

    /**
     * @brief Enters critical section
     */
    static void EnterCritical();

    /**
     * @brief Leaves critical section
     */
    static void LeaveCritical();
};

/**
 * @brief RTOS Task wrapper
 * @tparam Param Type of parameter passed to task procedure
 * @tparam StackSize Size of stack in bytes
 * @tparam Priority Priority assigned to task
 *
 */
template <typename Param, std::uint16_t StackSize, TaskPriority Priority> class Task final
{
    static_assert(sizeof(Param) < 16, "WTF are you trying to do?");
    static_assert(StackSize % 2 == 0, "Stack size must even");

  public:
    /**
     * @brief Type of function that can be used as task handler
     */
    using HandlerType = void (*)(Param);

    /**
     * @brief Initializes (but not creates in RTOS) task
     * @param[in] name Task name
     * @param[in] param Parameter passed to task
     * @param[in] handler Function that will be executed in new task
     */
    Task(const char* name, Param param, HandlerType handler);

    /**
     * @brief Creates RTOS task
     * @return Operation status
     */
    OSResult Create();

  private:
    /**
     * @brief Wrapper function that dispatches newly started task to specified handler
     * @param param
     */
    static void EntryPoint(void* param);

    /** @brief Task name */
    const char* _taskName;
    /** @brief Parameter passed to task */
    Param _param;
    /** @brief Task handler */
    HandlerType _handler;
    /** @brief Task handle */
    OSTaskHandle _handle;
};

template <typename Param, std::uint16_t StackSize, TaskPriority Priority>
Task<Param, StackSize, Priority>::Task(const char* name, Param param, HandlerType handler)
    : _taskName(name), _param(std::move(param)), _handler(std::move(handler)), _handle(nullptr)
{
}

template <typename Param, std::uint16_t StackSize, TaskPriority Priority> OSResult Task<Param, StackSize, Priority>::Create()
{
    return System::CreateTask(EntryPoint, this->_taskName, StackSize, static_cast<void*>(this), Priority, &this->_handle);
}

template <typename Param, std::uint16_t StackSize, TaskPriority Priority> void Task<Param, StackSize, Priority>::EntryPoint(void* param)
{
    auto This = static_cast<Task*>(param);
    This->_handler(This->_param);
}

/**
 * @brief Semaphore lock class.
 *
 * When created it takes semaphore and releases it at the end of scope
 *
 * Usage:
 *
 * @code
 * {
 * 	Lock lock(this->_sem);
 *
 * 	if(!lock())
 * 	{
 * 		// take failed
 * 		return;
 * 	}
 *
 * 	// do something
 * } // semaphore release at the end of scope
 * @endcode
 */
class Lock final : private NotCopyable, private NotMoveable
{
  public:
    /**
     * @brief Constructs @ref Lock object and tries to acquire semaphore
     * @param[in] semaphore Semaphore to take
     * @param[in] timeout Timeout
     */
    Lock(OSSemaphoreHandle semaphore, std::chrono::milliseconds timeout);

    /**
     * @brief Releases semaphore (if taken) on object destruction
     */
    ~Lock();

    /**
     * @brief Checks if semaphore has been taken
     * @return true if taking semaphore was successful
     */
    bool operator()();

    /**
     * @brief Checks if semaphore has been taken
     * @return true if taking semaphore was successful
     */
    explicit operator bool();

  private:
    /** @brief Semaphore handle */
    const OSSemaphoreHandle _semaphore;
    /** @brief Flag indicating if semaphore is acquired */
    bool _taken;
};

inline bool Lock::operator()()
{
    return this->_taken;
}

inline Lock::operator bool()
{
    return this->_taken;
}

/**
 * @brief RTOS queue wrapper
 */
template <typename Element, std::size_t Capacity> class Queue final
{
    static_assert(std::is_pod<Element>::value, "Queue works only for POD/integral types");

  public:
    /**
     * @brief Creates underlying RTOS queue object
     * @return Operation result
     */
    OSResult Create();

    /**
     * @brief Pushes element to queue
     * @param[in] element Pointer to element that will be placed in queue
     * @param[in] timeout Timeout in ms
     * @return Operation result
     */
    OSResult Push(const Element& element, std::chrono::milliseconds timeout);

    /**
     * @brief Pushes element to queue from interrupt service routine
     * @param[in] element Pointer to element that will be placed in queue
     * @return Operation result
     */
    OSResult PushISR(const Element& element);

    /**
     * @brief Pops element from queue
     * @param[out] element Pointer to place where element from queue will be saved
     * @param[in] timeout Timeout in ms
     * @return Operation result
     */
    OSResult Pop(Element& element, std::chrono::milliseconds timeout);

    /**
     * @brief Overwrites last element in queue
     * @param element Element
     */
    void Overwrite(const Element& element);

    /**
     * @brief Resets queue to empty state
     */
    void Reset();

  private:
    /** @brief Queue handle */
    OSQueueHandle _handle;
};

template <typename Element, std::size_t Capacity> OSResult Queue<Element, Capacity>::Create()
{
    this->_handle = System::CreateQueue(Capacity, sizeof(Element));

    if (this->_handle == nullptr)
    {
        return OSResult::NotEnoughMemory;
    }

    return OSResult::Success;
}

template <typename Element, std::size_t Capacity>
OSResult Queue<Element, Capacity>::Push(const Element& element, std::chrono::milliseconds timeout)
{
    if (System::QueueSend(this->_handle, &element, timeout))
    {
        return OSResult::Success;
    }
    return OSResult::Timeout;
}

template <typename Element, std::size_t Capacity> OSResult Queue<Element, Capacity>::PushISR(const Element& element)
{
    if (System::QueueSendISR(this->_handle, &element))
    {
        return OSResult::Success;
    }
    return OSResult::Overflow;
}

template <typename Element, std::size_t Capacity>
OSResult Queue<Element, Capacity>::Pop(Element& element, std::chrono::milliseconds timeout)
{
    if (System::QueueReceive(this->_handle, &element, timeout))
    {
        return OSResult::Success;
    }
    return OSResult::Timeout;
}

template <typename Element, std::size_t Capacity> inline void Queue<Element, Capacity>::Overwrite(const Element& element)
{
    System::QueueOverwrite(this->_handle, &element);
}

template <typename Element, std::size_t Capacity> void Queue<Element, Capacity>::Reset()
{
    System::QueueReset(this->_handle);
}

/**
 * @brief Class that allows checking if specified number of miliseconds elapsed
 *
 * This class uses system tick count to measure elapsed time.
 *
 * Example usage:
 * @code
 * Timeout t(10); // start measuring 10ms timeout
 *
 * while(some_condition)
 * {
 * 	  // lengthy operation
 *
 * 	  if(t.Expired()) return Result::Timeout;
 * }
 * @endcode
 */
class Timeout final
{
  public:
    /**
     * @brief Constructs new Timeout object
     * @param[in] timeout Timeout in miliseconds
     */
    Timeout(std::chrono::milliseconds timeout);

    /**
     * @brief Checks is timeout is expired
     * @return[in] true if timeout specified during construction already expired
     */
    bool Expired();

  private:
    /**
     * @brief System uptime at which timeout will expire
     */
    const std::chrono::milliseconds _expireAt;
};

/**
 * @brief Wrapper around event group synchronization primitive
 */
class EventGroup final
{
  public:
    /**
     * @brief Default ctor
     */
    EventGroup();

    /** @brief Initializes event group */
    OSResult Initialize();

    /**
     * @brief Sets bits in event group
     * @param bits Bits to set
     * @remark Must not be used from ISR
     */
    void Set(OSEventBits bits);

    /**
     * @brief Sets bits in event group from ISR
     * @param bits Bits to set
     */
    void SetISR(OSEventBits bits);

    /**
     * @brief Clears bits in event group
     * @param bits Bits to clear
     */
    void Clear(OSEventBits bits);

    /**
     * @brief Waits until any bit specified by mask is set
     * @param bits Bits to wait for
     * @param clearOnExit Should bits be cleared on exit
     * @param timeout Timeout
     * @return Event group value at the exit of function
     */
    OSEventBits WaitAny(OSEventBits bits, bool clearOnExit, std::chrono::milliseconds timeout);

    /**
     * @brief Waits until all bits specified by mask are set
     * @param bits Bits to wait for
     * @param clearOnExit Should bits be cleared on exit
     * @param timeout Timeout
     * @return Event group value at the exit of function
     */
    OSEventBits WaitAll(OSEventBits bits, bool clearOnExit, std::chrono::milliseconds timeout);

    /**
     * @brief Checks if bit is currently set
     * @param bit Bit to check
     * @return true if bit is set
     */
    bool IsSet(OSEventBits bit);

  private:
    /** @brief Underlying event group handle */
    OSEventGroupHandle _handle;
};

/**
 * @brief Class providing 'universal' lock for all objects that provide `Lock` and `Unlock` methods
 * @tparam Type of object being locked
 */
template <typename T> class UniqueLock : private NotMoveable, private NotCopyable
{
  public:
    /**
     * @brief Ctor
     * @param target Object to lock
     * @param timeout Timeout
     */
    UniqueLock(T& target, std::chrono::milliseconds timeout);

    /**
     * @brief Destructor. Releases the lock if taken in ctor
     */
    ~UniqueLock();

    /**
     * @brief Checks if lock has been taken successfully
     * @return true if lock has been taken
     */
    bool operator()();

  private:
    /** @brief Object being locked */
    T& _target;
    /** @brief Flag indicating whether lock has been taken */
    bool _taken;
};

template <typename T> UniqueLock<T>::UniqueLock(T& target, std::chrono::milliseconds timeout) : _target(target)
{
    this->_taken = this->_target.Lock(timeout);
}

template <typename T> UniqueLock<T>::~UniqueLock()
{
    if (this->_taken)
    {
        this->_target.Unlock();
        this->_taken = false;
    }
}

template <typename T> bool UniqueLock<T>::operator()()
{
    return this->_taken;
}

/**
 * @brief RAII critical section
 */
class CriticalSection final
{
  public:
    /**
     * @brief Ctor
     */
    CriticalSection();
    /**
     * @brief Dtor
     */
    ~CriticalSection();
};

/** @}*/

#endif
