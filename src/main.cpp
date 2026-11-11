#include <SKSE/SKSE.h>
#include <REL/Relocation.h>

namespace RE {
    class Main;
    class PlayerCharacter {
    public:
        static PlayerCharacter* GetSingleton() {
            using func_t = decltype(&PlayerCharacter::GetSingleton);
            REL::Relocation<func_t> func{ REL::ID(39339) }; // 1.5.97 Player ID
            return func();
        }
        
        void SetActorValue(uint32_t a_akValue, float a_value) {
            using func_t = void(PlayerCharacter*, uint32_t, float);
            REL::Relocation<func_t> func{ REL::ID(37515) }; // SetActorValue ID
            func(this, a_akValue, a_value);
        }
    };
}

namespace AutoEagleEye {
    using Update_t = void(*)(RE::Main*);
    Update_t orgUpdate = nullptr;

    void HookedUpdate(RE::Main* a_main) {
        if (orgUpdate) {
            orgUpdate(a_main);
        }

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        // 获取拉弓进度的内存偏移量 (1.5.97 中 player + 0xB58 是 bowDrawnPercent)
        float* bowDrawnPercent = reinterpret_cast<float*>(reinterpret_cast<char*>(player) + 0xB58);
        
        if (bowDrawnPercent && *bowDrawnPercent > 0.1f) {
            // 24 是 Block (防御/鹰眼) 的 ActorValue ID
            player->SetActorValue(24, 1.0f);
        }
    }

    void Install() {
        // 1.5.97 中 Main::Update 的虚函数表地址
        REL::Relocation<std::uintptr_t> vtbl{ REL::ID(515636) }; 
        
        // 安全地修改内存并替换虚函数
        DWORD oldProtect;
        VirtualProtect(reinterpret_cast<LPVOID>(vtbl.address() + 0x28), sizeof(std::uintptr_t), PAGE_EXECUTE_READWRITE, &oldProtect);
        orgUpdate = *reinterpret_cast<Update_t*>(vtbl.address() + 0x28);
        *reinterpret_cast<std::uintptr_t*>(vtbl.address() + 0x28) = reinterpret_cast<std::uintptr_t>(HookedUpdate);
        VirtualProtect(reinterpret_cast<LPVOID>(vtbl.address() + 0x28), sizeof(std::uintptr_t), oldProtect, &oldProtect);
    }
}

extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info) {
    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = "AutoEagleEye";
    a_info->version = 1;
    if (a_skse->IsEditor() || a_skse->RuntimeVersion() != SKSE::RUNTIME_1_5_97) {
        return false;
    }
    return true;
}

extern "C" __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);
    auto messaging = SKSE::GetMessagingInterface();
    if (messaging) {
        messaging->RegisterListener([](SKSE::MessagingInterface::Message* a_msg) {
            if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
                AutoEagleEye::Install();
            }
        });
    }
    return true;
}
