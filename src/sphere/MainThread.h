#include "../sphere/threads.h"

class MainThread : public AbstractSphereThread
{
public:
    MainThread();
    virtual ~MainThread() { };

    MainThread(const MainThread& copy) = delete;
    MainThread& operator=(const MainThread& other) = delete;

public:
    // we increase the access level from protected to public in order to allow manual execution when
    // configuration disables using threads
    // TODO: in the future, such simulated functionality should lie in AbstractThread inself instead of hacks
    virtual void tick() override;

protected:
    virtual void onStart() override;
    virtual bool shouldExit() noexcept override;
};
