#include <windows.h>
#include <cstdint>

// 手动还原 SKSE 内部结构体定义，不依赖任何外界 .h 文件
struct PluginInfo {
    enum { kVersion = 1 };
    uint32_t infoVersion;
    const char* name;
    uint32_t version;
};

struct QueryInterface {
    uint32_t (*EditorVersion)();
    uint32_t (*RuntimeVersion)();
};

struct LoadInterface {
    uint32_t interfaceVersion;
    uint32_t runtimeVersion;
};

namespace AutoEagleEye {
    using Update_t = void(*)(void*);
    Update_t orgUpdate = nullptr;

    void HookedUpdate(void* a_main) {
        if (orgUpdate) {
            orgUpdate(a_main);
        }

        // 1.5.97 游戏版本的玩家单例基址
        uintptr_t skyrimBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        uintptr_t playerPtr = *reinterpret_cast<uintptr_t*>(skyrimBase + 0x2F3B660);
        
        if (!playerPtr) return;

        // 1.5.97 中拉弓精度的内存偏移量 (player + 0xB58)
        float* bowDrawnPercent = reinterpret_cast<float*>(playerPtr + 0xB58);
        
        if (bowDrawnPercent && *bowDrawnPercent > 0.1f) {
            // 调用玩家虚函数表中的 SetActorValue (虚表第 242 项，即偏移 0x790)
            // 24 代表 Block (防御/鹰眼) 的 ActorValue ID
            using SetAV_t = void(*)(void*, uint32_t, float);
            uintptr_t* vtable = *reinterpret_cast<uintptr_t**>(playerPtr);
            SetAV_t setAV = reinterpret_cast<SetAV_t>(vtable[242]); 
            if (setAV) {
                setAV(reinterpret_cast<void*>(playerPtr), 24, 1.0f);
            }
        }
    }

    void Install(uintptr_t skyrimBase) {
        // 1.5.97 中 Main::Update 所在的虚函数表地址 (SkyrimSE.exe + 0x1634888)
        uintptr_t vtblAddress = skyrimBase + 0x1634888;
        uintptr_t hookTarget = vtblAddress + (5 * sizeof(uintptr_t)); // 虚表第 5 项

        DWORD oldProtect;
        VirtualProtect(reinterpret_cast<LPVOID>(hookTarget), sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &oldProtect);
        orgUpdate = *reinterpret_cast<Update_t*>(hookTarget);
        *reinterpret_cast<uintptr_t*>(hookTarget) = reinterpret_cast<uintptr_t>(HookedUpdate);
        VirtualProtect(reinterpret_cast<LPVOID>(hookTarget), sizeof(uintptr_t), oldProtect, &oldProtect);
    }
}

// 统一标准 C 导出，彻底解决编译期符号报错
extern "C" __declspec(dllexport) bool SKSEPlugin_Query(const QueryInterface* a_skse, PluginInfo* a_info) {
    a_info->infoVersion = PluginInfo::kVersion;
    a_info->name = "AutoEagleEye";
    a_info->version = 1;

    // 严格限制游戏版本为 1.5.97 (0x01050610)
    if (a_skse->RuntimeVersion() != 0x01050610) {
        return false;
    }
    return true;
}

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const LoadInterface* a_skse) {
    // 延迟 5 秒初始化，保证游戏安全启动不冲突
    CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        Sleep(5000);
        uintptr_t skyrimBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        AutoEagleEye::Install(skyrimBase);
        return 0;
    }, nullptr, 0, nullptr);

    return true;
}
