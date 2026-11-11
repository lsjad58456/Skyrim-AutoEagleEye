#include <windows.h>
#include <cstdint>

// 简单定义 SKSE 基础结构，彻底解耦第三方库依赖
namespace SKSE {
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
}

namespace AutoEagleEye {
    using Update_t = void(*)(void*);
    Update_t orgUpdate = nullptr;

    void HookedUpdate(void* a_main) {
        if (orgUpdate) {
            orgUpdate(a_main);
        }

        // 1.5.97 特定的 Player 单例基址指针：SkyrimSE.exe + 0x2F3B660
        uintptr_t skyrimBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        uintptr_t playerPtr = *reinterpret_cast<uintptr_t*>(skyrimBase + 0x2F3B660);
        
        if (!playerPtr) return;

        // 1.5.97 中拉弓精度的内存偏移量为 player + 0xB58
        float* bowDrawnPercent = reinterpret_cast<float*>(playerPtr + 0xB58);
        
        if (bowDrawnPercent && *bowDrawnPercent > 0.1f) {
            // 通过 1.5.97 特定的虚函数 SetActorValue (虚表第 242 项，即偏移 0x790)
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

extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info) {
    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = "AutoEagleEye";
    a_info->version = 1;

    // 严格限制游戏版本为 1.5.97 (0x01050610 即 1.5.97.0)
    if (a_skse->RuntimeVersion() != 0x01050610) {
        return false;
    }
    return true;
}

extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
    // 延迟 5 秒初始化，确保游戏底层单例加载完毕后再挂钩，防止闪退
    CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        Sleep(5000);
        uintptr_t skyrimBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        AutoEagleEye::Install(skyrimBase);
        return 0;
    }, nullptr, 0, nullptr);

    return true;
}
