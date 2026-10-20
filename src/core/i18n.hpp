// ============================================================================
//  RAW UI - user-visible strings (English / Russian).
//
//  A tiny hand-rolled table instead of a gettext-style runtime: the whole
//  string set is a few kilobytes of flash, both languages are compiled in, and
//  the build must stay free of external data files (everything goes into the
//  FPKG).  The X-macro keeps the two columns structurally in sync - adding an
//  entry gives you both languages at once and the enum follows automatically.
//
//      ui::drawText(ctx, tr(Str::kSettings), ...);
//
//  Language lives in AppConfig::lang and is applied on startup and whenever
//  the Settings row changes it.
// ============================================================================
#pragma once

#include <cstdint>

namespace rawui {

enum class Lang : uint8_t { English = 0, Russian = 1 };

// name, english, russian
#define RAWUI_UI_STRINGS(X)                                                    \
    /* ---- screen titles ------------------------------------------------ */ \
    X(kFiles,            "File manager",       "Файловый менеджер")            \
    X(kInstall,          "Install packages",   "Установка пакетов")            \
    X(kCheats,           "Cheats & patches",   "Читы и патчи")                 \
    X(kHen,              "GoldHEN",            "GoldHEN")                      \
    X(kSettings,         "Settings",           "Настройки")                    \
    X(kSettingsSubtitle, "Appearance, typography and motion",                   \
                         "Внешний вид, шрифты и анимации")                      \
    /* ---- settings sections --------------------------------------------- */ \
    X(kThemes,           "Themes",             "Темы")                         \
    X(kAppearance,       "Appearance",         "Оформление")                   \
    X(kTuning,           "Tuning",             "Тонкая настройка")             \
    X(kStorage,          "Storage",            "Память")                       \
    X(kLanguage,         "Language",           "Язык")                         \
    X(kGeneral,          "General",            "Общие")                        \
    X(kNight,            "Night mode",         "Ночной режим")                 \
    X(kBackup,           "Backup",             "Резервная копия")              \
    X(kThemesUsb,        "Themes on USB",      "Темы на USB")                   \
    X(kAbout,            "About",              "О программе")                  \
    /* ---- settings rows ------------------------------------------------- */ \
    X(kDark,             "Dark theme",         "Тёмная тема")                  \
    X(kLight,            "Light theme",        "Светлая тема")                 \
    X(kVibrant,          "Vibrant palette",    "Яркая палитра")                \
    X(kChromaBoost,      "Boost colour chroma","Усиленная насыщенность")       \
    X(kSeedColour,       "Seed colour",        "Базовый цвет")                 \
    X(kContrast,         "Contrast",           "Контраст")                     \
    X(kContrastHint,     "Colour contrast (-0.3 to +0.3)",                     \
                         "Контраст цвета (-0.3…+0.3)")                         \
    X(kTypeScale,        "Type scale",         "Размер шрифта")                \
    X(kTypeScaleHint,    "Text size (0.9x - 1.2x)",                            \
                         "Размер текста (0.9–1.2x)")                           \
    X(kMotionScale,      "Motion scale",       "Скорость анимаций")            \
    X(kMotionScaleHint,  "Animation speed (0x - 1.5x)",                        \
                         "Скорость анимаций (0–1.5x)")                         \
    X(kNightHint,        "Dark theme on a schedule",                           \
                         "Тёмная тема по расписанию")                          \
    X(kNightFrom,        "Dark from",          "Тёмная с")                     \
    X(kNightTo,          "to",                 "до")                           \
    X(kNightNow,         "Night mode is active right now",                     \
                         "Ночной режим активен сейчас")                        \
    X(kRestoreDefaults,  "Restore defaults",   "Сбросить настройки")           \
    X(kOn,               "On",                 "Вкл")                          \
    X(kOff,              "Off",                "Выкл")                         \
    X(kActive,           "ACTIVE",             "АКТИВНО")                      \
    /* ---- backup / usb --------------------------------------------------- */ \
    X(kBackupToUsb,      "Backup to USB",      "Скопировать на USB")            \
    X(kBackupToUsbHint,  "Write config.ini to a USB drive",                    \
                         "Записать config.ini на USB")                         \
    X(kRestoreFromUsb,   "Restore from USB",   "Восстановить с USB")            \
    X(kRestoreFromUsbHint, "Read config.ini back from USB",                    \
                         "Прочитать config.ini с USB")                         \
    X(kExportTheme,      "Export theme",       "Экспорт темы")                 \
    X(kExportThemeHint,  "Save the current theme to USB",                      \
                         "Сохранить текущую тему на USB")                      \
    X(kImportThemes,     "Import themes",      "Импорт тем")                   \
    X(kImportThemesHint, "Load themes from USB or /data/RAWUI",                \
                         "Загрузить темы с USB или /data/RAWUI")               \
    /* ---- toasts --------------------------------------------------------- */ \
    X(kThemeApplied,     "Theme: ",            "Тема: ")                       \
    X(kDefaultsRestored, "Defaults restored",  "Настройки сброшены")           \
    X(kSeedUpdated,      "Seed colour updated","Базовый цвет обновлён")        \
    X(kBackupOk,         "Settings copied to USB",                             \
                         "Настройки скопированы на USB")                       \
    X(kBackupFail,       "No USB drive found", "USB-устройство не найдено")     \
    X(kRestoreOk,        "Settings restored",  "Настройки восстановлены")      \
    X(kRestoreFail,      "No backup found on USB",                             \
                         "Резервная копия на USB не найдена")                  \
    X(kExportOk,         "Theme exported",     "Тема экспортирована")          \
    X(kExportFail,       "No USB drive found", "USB-устройство не найдено")     \
    X(kImportOk,         "Imported theme: ",   "Импортирована тема: ")         \
    X(kImportFail,       "Could not read theme file",                          \
                         "Не удалось прочитать файл темы")                     \
    X(kNothingToImport,  "No theme files found",                               \
                         "Файлы тем не найдены")                               \
    X(kThemesFound,      "%d theme files found",                               \
                         "Найдено файлов тем: %d")                             \
    /* ---- about ---------------------------------------------------------- */ \
    X(kHomebrewShell,    "Homebrew shell",     "Homebrew-оболочка")            \
    X(kFreeOf,           " free of ",          " свободно из ")                \
    X(kOnData,           " on /data",          " на /data")                    \
    X(kDataUnavailable,  "/data unavailable",  "/data недоступен")             \
    X(kAboutLine,        "Title ID %s · %s · %s",                              \
                         "Title ID %s · %s · %s")                              \
    /* ---- shared hints --------------------------------------------------- */ \
    X(kHintConfirm,      "Confirm",            "Выбрать")                      \
    X(kHintToggle,       "Toggle",             "Вкл/выкл")                     \
    X(kHintBack,         "Back",               "Назад")                        \
    X(kHintSelect,       "Select",             "Выбрать")                      \
    X(kHintAdjust,       "Navigate/Adjust",    "Переход/Настройка")            \
    X(kHintScreens,      "Screens",            "Экраны")                       \
    X(kHintRescan,       "Rescan",             "Обновить список")              \
    X(kHintInstall,      "Install",            "Установить")                   \
    X(kHintOpen,         "Open",               "Открыть")                      \
    X(kHintNavigate,     "Navigate",           "Навигация")                    \
    X(kHintDelete,       "Delete",             "Удалить")                      \
    X(kHintUp,           "Up",                 "Вверх")                        \
    X(kHintRefresh,      "Refresh",            "Обновить")                     \
    X(kHintDetails,      "Details",            "Подробности")                  \
    X(kHintProbe,        "Probe again",        "Проверить снова")              \
    X(kFabProbe,         "Probe",              "Проверить")                    \
    /* ---- dashboard ------------------------------------------------------ */ \
    X(kQuickActions,     "Quick actions",      "Быстрые действия")             \
    X(kCheatsCard,       "Cheats",             "Читы")                         \
    X(kHenActive,        "GoldHEN active",     "GoldHEN активен")               \
    X(kHenMissing,       "GoldHEN not detected", "GoldHEN не обнаружен")        \
    /* ---- install -------------------------------------------------------- */ \
    X(kNoPackages,       "No packages found",  "Пакеты не найдены")            \
    X(kScanning,         "Scanning drives...", "Сканирование носителей...")     \
    X(kScanningVolumes,  "Scanning mounted volumes...",                        \
                         "Сканирование смонтированных носителей...")           \
    X(kSearching,        "Searching: USB drives, /data, /mnt/ext0",            \
                         "Поиск: USB, /data, /mnt/ext0")                       \
    X(kNoPkgFiles,       "No .pkg files were found.",                          \
                         "Файлы .pkg не найдены.")                             \
    X(kCopyPkgHint,      "Copy a package to a USB drive, then press Triangle " \
                         "to rescan.",                                         \
                         "Скопируйте пакет на USB и нажмите Triangle, чтобы "   \
                         "повторить поиск.")                                   \
    X(kInstalling,       "Installing",         "Установка")                    \
    X(kInstallRejected,  "Install refused",    "Установка отклонена")          \
    X(kFinishing,        "Finishing up",       "Завершение")                   \
    /* ---- confirm sheets -------------------------------------------------- */ \
    X(kAlreadyInstalled, "Already installed",  "Уже установлено")              \
    X(kTitleIsInstalled, "Title %s is already installed.",                     \
                         "Титул %s уже установлен.")                           \
    X(kOverwrite,        "Overwrite",          "Перезаписать")                 \
    X(kWillOverwrite,    "The installer will overwrite it.",                    \
                         "Установщик перезапишет его.")                        \
    X(kDelete,           "Delete",             "Удалить")                      \
    /* ---- screen subtitles ------------------------------------------------ */ \
    X(kFilesUnreadable,  "This folder could not be read",                       \
                         "Не удалось прочитать этот каталог")                  \
    X(kPatches,          "Patches",            "Патчи")                        \
    X(kInstallingName,   "Installing %s",      "Установка %s")                  \
    X(kPayloadDetected,  "Payload detected",   "Пейлоад обнаружен")             \
    X(kPayloadFolder,    "Payload folder found, no servers answering",          \
                         "Папка пейлоада найдена, серверы не отвечают")         \
    X(kPayloadMissing,   "Payload not found",  "Пейлоад не найден")             \
    /* ---- shell dialog ---------------------------------------------------- */ \
    X(kExitTitle,        "Exit RAW UI",        "Выйти из RAW UI")               \
    X(kExitBody,         "Return to the PS4 menu?",                            \
                         "Вернуться в меню PS4?")                              \
    X(kExit,             "Exit",               "Выход")                         \
    /* ---- countable units (picked by trn) ---------------------------------- */ \
    X(kUnitFileOne,      "file",               "файл")                         \
    X(kUnitFileFew,      "files",              "файла")                        \
    X(kUnitFileMany,     "files",              "файлов")                       \
    X(kUnitPackageOne,   "package",            "пакет")                        \
    X(kUnitPackageFew,   "packages",           "пакета")                       \
    X(kUnitPackageMany,  "packages",           "пакетов")                      \
    X(kUnitServerOne,    "server",             "сервер")                       \
    X(kUnitServerFew,    "servers",            "сервера")                      \
    X(kUnitServerMany,   "servers",            "серверов")                     \
    X(kUnitItemOne,      "item",               "элемент")                      \
    X(kUnitItemFew,      "items",              "элемента")                     \
    X(kUnitItemMany,     "items",              "элементов")                    \
    /* ---- shell ------------------------------------------------------------ */ \
    X(kCancel,           "Cancel",             "Отмена")                       \
    X(kFree,             " free",              " свободно")                    \
    X(kNoHen,            "No HEN",             "Нет HEN")                      \
    /* ---- dashboard -------------------------------------------------------- */ \
    X(kTileInstallSub,   "Scan drives for .pkg files",                          \
                         "Поиск .pkg на носителях")                             \
    X(kTileFilesSub,     "Browse and clean up /data",                           \
                         "Просмотр и очистка /data")                            \
    X(kTileHenSub,       "Payload status and servers",                          \
                         "Статус пейлоада и серверы")                           \
    X(kTileCheatsSub,    "Cheat files and patches",                             \
                         "Файлы читов и патчи")                                 \
    X(kTileSettingsSub,  "Theme, text and storage",                             \
                         "Тема, шрифт и память")                                \
    X(kActiveShort,      "Active",            "Активен")                        \
    X(kOffline,          "Offline",           "Офлайн")                        \
    X(kNoneFound,        "None found",        "Не найдено")                     \
    X(kInternalStorage,  "Internal storage",  "Внутреннее хранилище")           \
    X(kCheatFiles,       "Cheat files",       "Файлы читов")                    \
    /* ---- file manager ----------------------------------------------------- */ \
    X(kColName,          "Name",              "Имя")                           \
    X(kColSize,          "Size",              "Размер")                        \
    X(kColModified,      "Modified",          "Изменён")                        \
    X(kFolder,           "Folder",            "Папка")                         \
    X(kPermDenied,       "Permission denied, or the folder no longer exists.",  \
                         "Нет доступа, или папка больше не существует.")        \
    X(kFolderEmpty,      "This folder is empty.",                              \
                         "Эта папка пуста.")                                    \
    X(kDeleteConfirm,    "Delete %s?",         "Удалить %s?")                   \
    X(kDeleteBodyDir,    "The folder and everything inside it will be deleted.",\
                         "Папка и всё её содержимое будут удалены.")           \
    X(kDeleteBodyFile,   "The file will be removed.",                           \
                         "Файл будет удалён.")                                  \
    X(kDeleted,          "Deleted %s",         "Удалено: %s")                   \
    X(kDeleteFailed,     "Could not delete %s",                                 \
                         "Не удалось удалить %s")                               \
    X(kFreed,            "%s will be freed.", "Будет освобождено %s.")          \
    /* ---- cheats ------------------------------------------------------------ */ \
    X(kColFile,          "File",              "Файл")                          \
    X(kColStatus,        "Status",            "Статус")                        \
    X(kEnabled,          "Enabled",           "Включён")                       \
    X(kDisabled,         "Disabled",          "Выключен")                      \
    X(kNoCheatJson,      "No cheat JSON files found in GoldHEN/cheats",        \
                         "Файлы читов не найдены в GoldHEN/cheats")            \
    X(kNoPatchFiles,     "No patch files found in GoldHEN/patches",            \
                         "Файлы патчей не найдены в GoldHEN/patches")          \
    X(kLookedIn,         "Looked in: %s",     "Проверено: %s")                  \
    X(kToggleFailed,     "Failed to toggle %s",                                 \
                         "Не удалось переключить %s")                           \
    X(kToggledOn,        "%s enabled",       "%s включён")                      \
    X(kToggledOff,       "%s disabled",      "%s выключен")                     \
    /* ---- installer --------------------------------------------------------- */ \
    X(kNoPkgOnVolumes,   "No .pkg files found on the mounted volumes",         \
                         "Файлы .pkg не найдены на смонтированных носителях")   \
    X(kInstalled,        "Installed %s",      "Установлено: %s")                \
    X(kContentIdMissing, "content id not in file name",                         \
                         "content id нет в имени файла")                       \
    X(kStatusFailed,     "Failed",            "Ошибка")                        \
    X(kStatusNoData,     "Installing - no progress data available",            \
                         "Установка — нет данных о прогрессе")                 \
    X(kInstallFailedTitle, "Install failed",  "Ошибка установки")               \
    X(kInstallRefusedDetail,                                        \
        "The installer refused the package. Free space, a GoldHEN-enabled "     \
        "system and a valid FPKG are all required.",                            \
        "Установщик отклонил пакет. Нужны свободное место, система с "          \
        "GoldHEN и корректный FPKG.")                                           \
    X(kBytesOf,          "%s of %s",           "%s из %s")                     \
    X(kSecondsLeft,      "  ·  about %ds left", "  ·  около %d с")            \
    X(kWaitingInstaller, "Waiting for the installer service.",                  \
                         "Ожидание службы установки.")                         \
    X(kUnknownTitle,     "unknown title",     "неизвестный титул")             \
    /* ---- goldhen ----------------------------------------------------------- */ \
    X(kProbingPorts,     "Probing GoldHEN ports...",                            \
                         "Проверка портов GoldHEN...")                          \
    X(kHenRunning,       "GoldHEN is running", "GoldHEN работает")              \
    X(kHenSilent,        "GoldHEN is not answering",                            \
                         "GoldHEN не отвечает")                                 \
    X(kNoPayloadDir,     "No payload directory at /data/GoldHEN",               \
                         "Папки пейлоада нет в /data/GoldHEN")                  \
    X(kServices,         "Services",          "Сервисы")                        \
    X(kListening,        "listening",         "слушаёт")                       \
    X(kClosed,           "closed",            "закрыт")                       \
    X(kServicesLine,     "%d of %d services listening",                         \
                         "%d из %d сервисов слушают")                           \
    X(kCheatsAndPatches, "Cheats and patches", "Читы и патчи")                  \
    X(kCheatPatchCounts, "%d cheat files  ·  %d patch files",                   \
                         "Файлов читов: %d  ·  патчей: %d")

enum class Str : uint16_t {
#define RAWUI_UI_STR(name, en, ru) name,
    RAWUI_UI_STRINGS(RAWUI_UI_STR)
#undef RAWUI_UI_STR
        Count
};

// Resolves a string in the active language.  Never returns null: an unknown id
// yields "".
const char* tr(Str id);
const char* tr(Str id, Lang lang);

// Resolves with printf-style substitution taken from the table, e.g.
// trf(Str::kTitleIsInstalled, titleId).  Both language columns must therefore
// carry the same conversion specifiers in the same order.  The result points
// at a static buffer of kTrBuf bytes and is always NUL-terminated; it is
// invalidated by the next trf() call.
constexpr int kTrBuf = 256;
const char* trf(Str id, ...);

// Countable nouns need more than a binary choice: English is one/other, but
// Russian is one/few/many - 1 файл, 2 файла, 5 файлов, but also 11 файлов.
// `n` picks the form; the English column simply uses `one` for 1 and `many`
// otherwise, which is why `few` and `many` are the same word there.
const char* trn(Str one, Str few, Str many, int n);

Lang lang();
void setLang(Lang lang);

// Human-readable language name, always in its own language ("English", "Русский").
const char* langName(Lang lang);

// Storage tag for the config file ("en" / "ru"), and the inverse parse.
// Unknown tags resolve to English rather than failing, so a hand-edited
// config.ini can never lock the UI out of a language.
const char* langTag(Lang lang);
Lang langFromName(const char* tag);

}  // namespace rawui
