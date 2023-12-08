#include "ArchMemory.h"
#include "BDVirtualDevice.h"
#include "umap.h"
#include "types.h"
#include "mm/PageManager.h"
#include "Condition.h"
#include "debug.h"
#include "Thread.h"
#include "uqueue.h"

class SwapThread : public Thread {
    friend class UserThread;
public:
    SwapThread();
    ~SwapThread();
    void Run();
    static SwapThread *instance();

private:
    static SwapThread *instance_;
};