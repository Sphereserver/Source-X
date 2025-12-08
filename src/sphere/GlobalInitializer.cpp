#include "../common/sphereversion.h"
#include "../game/CObjBase.h"
#include "../game/uo_files/CUOItemTypeRec.h"
#include "GlobalInitializer.h"
#include <sstream>

extern std::string g_sServerDescription;

// Dynamic initialization of some static members of other classes, which are used very soon after the server starts
dword CObjBase::sm_iCount = 0;				// UID table.
#ifdef _WIN32
llong CSTime::_kllTimeProfileFrequency = 1; // Default value.
#endif


// This method MUST be the first code running when the application starts!
GlobalInitializer::GlobalInitializer()
{
    // The order of the instructions is important!

    // Overwriting for safety, but it is created/allocated in spheresvr.cpp!
    DummySphereThread::_instance = nullptr;

    std::stringstream ssServerDescription;
    ssServerDescription << SPHERE_TITLE << " Version " << SPHERE_BUILD_INFO_STR;
    ssServerDescription << " [" << get_target_os_str() << '-' << get_target_arch_str() << "]";
    ssServerDescription << " by www.spherecommunity.net";
    g_sServerDescription = ssServerDescription.str();

    //-- Time

    /*
     # *ifdef _WIN32
     // Ensure it's ACTUALLY necessary, before resorting to this.
     timeBeginPeriod(1); // from timeapi.h, we need also to link against Winmm.lib...
     #endif
     */
    PeriodicSyncTimeConstants();


    //--- Sphere threading system

    DummySphereThread::createInstance();

    {
        // Ensure i have this to have context for ADDTOCALLSTACK and other operations.
        const AbstractThread* curthread = ThreadHolder::get().current();
        ASSERT(curthread != nullptr);
        ASSERT(dynamic_cast<DummySphereThread const *>(curthread));
        UnreferencedParameter(curthread);
    }

    //--- Exception handling

    #ifdef WINDOWS_SPHERE_SHOULD_HANDLE_STRUCTURED_EXCEPTIONS
    SetWindowsStructuredExceptionTranslator();
    #endif

    // Set function to handle the invalid case where a pure virtual function is called.
    SetPurecallHandler();

    //--- Pre-startup sanity checks

    constexpr const char* m_sClassName = "GlobalInitializer";
    EXC_TRY("Pre-startup Init");

    static_assert(MAX_BUFFER >= sizeof(CCommand));
    static_assert(MAX_BUFFER >= sizeof(CEvent));
    static_assert(sizeof(int) == sizeof(dword));	// make this assumption often.
    static_assert(sizeof(ITEMID_TYPE) == sizeof(dword));
    static_assert(sizeof(word) == 2);
    static_assert(sizeof(dword) == 4);
    static_assert(sizeof(nword) == 2);
    static_assert(sizeof(ndword) == 4);
    static_assert(sizeof(wchar) == 2);	// 16 bits
    static_assert(sizeof(CUOItemTypeRec) == 37);	// is byte packing working ?

    EXC_CATCH;
}

void GlobalInitializer::InitRuntimeDefaultValues() // static
{
    CPointBase::InitRuntimeDefaultValues();
}

void GlobalInitializer::PeriodicSyncTimeConstants() // static
{
    // TODO: actually call it periodically!

    #ifdef _WIN32
    // Needed to get precise system time.
    LARGE_INTEGER liProfFreq;
    if (QueryPerformanceFrequency(&liProfFreq))
    {
        CSTime::_kllTimeProfileFrequency = liProfFreq.QuadPart;
    }
    #endif  // _WIN32
}
