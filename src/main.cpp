#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>

namespace AutoEagleEye {
    class UpdateHandler {
    public:
        static void Update(RE::Main* a_main) {
            _Update(a_main);

            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) return;

            // 1. 检查是否正在拉弓/装弩
            auto attackingWeapon = player->GetAttackingWeapon();
            if (!attackingWeapon || !attackingWeapon->object) return;

            auto weaponData = attackingWeapon->object->As<RE::TESObjectWEAP>();
            if (!weaponData || (!weaponData->IsBow() && !weaponData->IsCrossbow())) return;

            // 2. 检查是否有鹰眼天赋 (FormID: 0x58F61)
            static auto* eagleEyePerk = RE::TESForm::LookupByID<RE::BGSPerk>(0x00058F61);
            if (!eagleEyePerk || !player->HasPerk(eagleEyePerk)) return;

            // 1.5.97 特有的底层拉弓进度获取
            float bowDrawnPercent = player->GetActorRuntimeData().bowDrawnPercent;

            auto controlMap = RE::ControlMap::GetSingleton();
            if (!controlMap) return;

            // 核心控制切换
            if (bowDrawnPercent > 0.1f) {
                // 强行向引擎发送按下防御键(Block)的标志，直接开启鹰眼
                player->SetActorValue(RE::ActorValue::kBlock, 1.0f);
            }
        }

        static inline REL::Relocation<decltype(&Update)> _Update;
    };

    void Install() {
        // 1.5.97 特定的 Main::Update 虚函数表地址注入
        REL::Relocation<std::uintptr_t> mainVtbl{ RE::VTABLE_Main[0] };
        UpdateHandler::_Update = mainVtbl.write_vfunc(0x5, UpdateHandler::Update);
        SKSE::log::info("1.5.97 自动鹰眼插件加载成功！");
    }
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info) {
    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = "AutoEagleEye";
    a_info->version = 1;

    if (a_skse->IsEditor()) {
        return false;
    }

    // 严格限制游戏版本为 1.5.97
    const auto runtimeVersion = a_skse->RuntimeVersion();
    if (runtimeVersion != SKSE::RUNTIME_1_5_97) {
        return false;
    }

    return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
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
