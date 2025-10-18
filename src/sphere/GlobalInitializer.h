struct GlobalInitializer
{
    GlobalInitializer();
    ~GlobalInitializer() = default;
    static void InitRuntimeDefaultValues();
    static void PeriodicSyncTimeConstants();
};
