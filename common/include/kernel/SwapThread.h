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

struct SwapRequest{
    size_t swap_type_;
    size_t ppn_;
    size_t vpn_;
    size_t block_number_;
    bool is_done = false;
    SwapRequest(size_t sw_type, size_t ppn, size_t vpn, size_t block_number, [[maybe_unused]]Mutex* swap_lock_) :
            swap_type_(sw_type), ppn_(ppn), vpn_(vpn), block_number_(block_number){
    };
};

class SwapThread : public Thread {
    friend class UserThread;

public:
    SwapThread();
    ~SwapThread();
    void Run() override;
    static SwapThread *instance();
    size_t pages_number_;
    Bitmap* bitmap_;
    BDVirtualDevice *device_;
    size_t block_;
    size_t SwapOut(SwapRequest* request);
    void SwapIn();
    ustl::queue <SwapRequest*> swap_request_map_;
    Mutex swap_lock_;
    Condition swap_wait;
    size_t lowest_unreserved_page_;

    void addCond(size_t found);
    static size_t randomPRA();
    size_t number_of_blocks_;

private:
    static SwapThread *instance_;


};

