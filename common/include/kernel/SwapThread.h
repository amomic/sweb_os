#include "ArchMemory.h"
#include "BDVirtualDevice.h"
#include "umap.h"
#include "types.h"
#include "mm/PageManager.h"
#include "Condition.h"
#include "debug.h"
#include "Thread.h"
#include "uqueue.h"
#include "Bitmap.h"

class SwapThread : public Thread {
    friend class UserThread;
public:
    SwapThread();
    ~SwapThread();
    void Run();
    static SwapThread *instance();
    size_t pages_number_;
    Bitmap* bitmap_;
    BDVirtualDevice *device_;
    size_t block_;
    size_t SwapInRequest();
    size_t SwapOutRequest();
    void SwapOut(PageTableEntry *pt, size_t pti);
    void SwapIn(PageTableEntry *pt, size_t pti);

private:
    static SwapThread *instance_;

};