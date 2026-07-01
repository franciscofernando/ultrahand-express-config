/**
 * Express Config - Overlay Tesla / Ultrahand para Nintendo Switch
 *
 * Acceso rápido desde el menú Tesla para:
 *   - Controlar el brillo de la pantalla (servicio lbl)
 *   - Encender/apagar Bluetooth y emparejar/conectar auriculares y
 *     altavoces como AirPods (servicio btm:sys, requiere firmware 13.0.0+)
 *
 * Compatible con el cargador de overlays Ultrahand y con Tesla-Menu.
 */

#define TESLA_INIT_IMPL   // Solo se define en el archivo principal
#include <tesla.hpp>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// --------------------------------------------------------------------------
// Estado global de los servicios (inicializados en Overlay::initServices)
// --------------------------------------------------------------------------
static Result g_lblRc = 1;   // != 0  => aún no inicializado / falló
static Result g_btmRc = 1;

// Idioma de la interfaz: español solo si la consola está en español
// (España o Latinoamérica); en cualquier otro caso, inglés.
static bool g_es = false;

// Devuelve la cadena en español o en inglés según el idioma del sistema.
static inline const char *L(const char *es, const char *en) {
    return g_es ? es : en;
}

// --------------------------------------------------------------------------
// Utilidades de direcciones Bluetooth
// --------------------------------------------------------------------------
static inline bool addrEqual(const BtdrvAddress &a, const BtdrvAddress &b) {
    return std::memcmp(a.address, b.address, sizeof(a.address)) == 0;
}

static bool addrInList(const BtdrvAddress &a, const BtmAudioDevice *list, s32 count) {
    for (s32 i = 0; i < count; i++)
        if (addrEqual(a, list[i].addr))
            return true;
    return false;
}

static std::string addrToStr(const BtdrvAddress &a) {
    char buf[18];
    std::snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                  a.address[0], a.address[1], a.address[2],
                  a.address[3], a.address[4], a.address[5]);
    return std::string(buf);
}

static inline bool btmAudioAvailable() {
    // Los comandos de audio de btm:sys existen desde el firmware 13.0.0
    return R_SUCCEEDED(g_btmRc) && hosversionAtLeast(13, 0, 0);
}

// ==========================================================================
//  Pantalla: Buscar dispositivos Bluetooth nuevos (modo emparejamiento)
// ==========================================================================
class DiscoveryGui : public tsl::Gui {
    tsl::elm::List     *m_list   = nullptr;
    tsl::elm::ListItem *m_status = nullptr;
    std::string         m_sig;          // firma de la lista actual para evitar redibujar
    u32                 m_tick   = 0;
    bool                m_ok     = false;

    void buildHeader(const std::string &statusText) {
        m_list->addItem(new tsl::elm::CategoryHeader(
            L("Pon el dispositivo en modo emparejamiento", "Put the device in pairing mode")));
        m_status = new tsl::elm::ListItem(statusText);
        m_list->addItem(m_status);
        m_list->addItem(new tsl::elm::CategoryHeader(L("Dispositivos encontrados", "Devices found")));
    }

    void rebuild(const BtmAudioDevice *devs, s32 n) {
        m_list->clear();   // libera los elementos anteriores (incluido el foco)
        buildHeader(n > 0 ? (std::to_string(n) + L(" encontrado(s)", " found"))
                          : L("Buscando...", "Searching..."));

        tsl::elm::Element *firstFocus = nullptr;
        for (s32 i = 0; i < n; i++) {
            BtdrvAddress addr = devs[i].addr;
            auto *item = new tsl::elm::ListItem(std::string(devs[i].name), addrToStr(addr));
            item->setClickListener([addr](u64 keys) {
                if (keys & HidNpadButton_A) {
                    // Conecta (y empareja si hace falta) con el dispositivo
                    btmsysConnectAudioDevice(addr);
                    return true;
                }
                return false;
            });
            if (!firstFocus) firstFocus = item;
            m_list->addItem(item);
        }
        // Reasigna el foco a un elemento válido tras reconstruir la lista
        if (firstFocus)
            this->requestFocus(firstFocus, tsl::FocusDirection::None);
    }

public:
    virtual tsl::elm::Element *createUI() override {
        auto *frame = new tsl::elm::OverlayFrame("Express Config", L("Buscar Bluetooth", "Find Bluetooth"));
        m_list = new tsl::elm::List();

        if (btmAudioAvailable()) {
            buildHeader(L("Buscando...", "Searching..."));
            btmsysEnableRadio();                            // asegura la radio encendida
            m_ok = R_SUCCEEDED(btmsysStartAudioDeviceDiscovery());
            if (!m_ok)
                m_status->setText(L("Error al iniciar la búsqueda", "Failed to start search"));
        } else {
            buildHeader(L("Requiere firmware 13.0.0+", "Requires firmware 13.0.0+"));
        }

        frame->setContent(m_list);
        return frame;
    }

    virtual void update() override {
        if (!m_ok) return;
        if ((m_tick++ % 60) != 0) return;   // refresca ~1 vez por segundo

        BtmAudioDevice devs[15];
        s32 total = 0;
        if (R_FAILED(btmsysGetDiscoveredAudioDevice(devs, 15, &total)))
            return;

        std::string sig;
        for (s32 i = 0; i < total; i++)
            sig += addrToStr(devs[i].addr) + ";";

        if (sig != m_sig) {   // solo redibuja si cambió el conjunto de dispositivos
            m_sig = sig;
            rebuild(devs, total);
        }
    }

    virtual ~DiscoveryGui() {
        if (m_ok)
            btmsysStopAudioDeviceDiscovery();
    }
};

// ==========================================================================
//  Pantalla: Bluetooth (radio + dispositivos emparejados)
// ==========================================================================
class BluetoothGui : public tsl::Gui {
    std::vector<std::pair<BtdrvAddress, tsl::elm::ListItem *>> m_rows;
    u32 m_tick = 0;

    static void toggleConnect(const BtdrvAddress &addr) {
        BtmAudioDevice conn[8];
        s32 count = 0;
        btmsysGetConnectedAudioDevices(conn, 8, &count);
        if (addrInList(addr, conn, count))
            btmsysDisconnectAudioDevice(addr);
        else
            btmsysConnectAudioDevice(addr);
    }

public:
    virtual tsl::elm::Element *createUI() override {
        auto *frame = new tsl::elm::OverlayFrame("Express Config", "Bluetooth");
        auto *list  = new tsl::elm::List();

        if (!btmAudioAvailable()) {
            list->addItem(new tsl::elm::CategoryHeader(L("No disponible", "Not available")));
            list->addItem(new tsl::elm::ListItem(
                L("El audio Bluetooth requiere", "Bluetooth audio requires"), "firmware 13.0.0+"));
            frame->setContent(list);
            return frame;
        }

        // ---- Interruptor de la radio Bluetooth ----
        bool radioOn = false;
        btmsysGetRadioOnOff(&radioOn);
        auto *radioToggle = new tsl::elm::ToggleListItem("Bluetooth", radioOn);
        radioToggle->setStateChangedListener([](bool state) {
            if (state) btmsysEnableRadio();
            else       btmsysDisableRadio();
        });
        list->addItem(new tsl::elm::CategoryHeader("Radio"));
        list->addItem(radioToggle);

        // ---- Dispositivos emparejados ----
        list->addItem(new tsl::elm::CategoryHeader(
            L("Emparejados  (A: conectar/desconectar   Y: olvidar)",
              "Paired  (A: connect/disconnect   Y: forget)")));

        BtmAudioDevice paired[10];
        s32 ptotal = 0;
        Result rc = btmsysGetPairedAudioDevices(paired, 10, &ptotal);

        if (R_SUCCEEDED(rc) && ptotal > 0) {
            BtmAudioDevice conn[8];
            s32 ctotal = 0;
            btmsysGetConnectedAudioDevices(conn, 8, &ctotal);

            for (s32 i = 0; i < ptotal; i++) {
                BtdrvAddress addr = paired[i].addr;
                bool isConn = addrInList(addr, conn, ctotal);
                auto *item = new tsl::elm::ListItem(std::string(paired[i].name),
                                                    isConn ? L("Conectado", "Connected") : "");
                item->setClickListener([addr](u64 keys) {
                    if (keys & HidNpadButton_A) { toggleConnect(addr);                return true; }
                    if (keys & HidNpadButton_Y) { btmsysRemoveAudioDevicePairing(addr); return true; }
                    return false;
                });
                m_rows.emplace_back(addr, item);
                list->addItem(item);
            }
        } else {
            list->addItem(new tsl::elm::ListItem(L("(ninguno todavía)", "(none yet)")));
        }

        // ---- Buscar nuevos ----
        list->addItem(new tsl::elm::CategoryHeader(L("Añadir", "Add")));
        auto *scan = new tsl::elm::ListItem(
            L("Buscar auriculares / altavoces...", "Search headphones / speakers..."));
        scan->setClickListener([](u64 keys) {
            if (keys & HidNpadButton_A) {
                tsl::changeTo<DiscoveryGui>();
                return true;
            }
            return false;
        });
        list->addItem(scan);

        frame->setContent(list);
        return frame;
    }

    virtual void update() override {
        if (m_rows.empty()) return;
        if ((m_tick++ % 30) != 0) return;   // refresca el estado ~2 veces por segundo

        BtmAudioDevice conn[8];
        s32 count = 0;
        if (R_FAILED(btmsysGetConnectedAudioDevices(conn, 8, &count)))
            return;

        bool connecting = false;
        btmsysIsConnectingAudioDevice(&connecting);

        for (auto &row : m_rows) {
            if (addrInList(row.first, conn, count))
                row.second->setValue(L("Conectado", "Connected"));
            else if (connecting)
                row.second->setValue("...");
            else
                row.second->setValue("");
        }
    }
};

// ==========================================================================
//  Pantalla: Brillo
// ==========================================================================
class BrightnessGui : public tsl::Gui {
    tsl::elm::TrackBar *m_bar = nullptr;

    static void applyBrightness(float value) {
        if (R_FAILED(g_lblRc)) return;
        bool autoOn = false;
        lblIsAutoBrightnessControlEnabled(&autoOn);
        if (autoOn)                                   // el brillo manual desactiva el automático
            lblDisableAutoBrightnessControl();
        if (value < 0.0f) value = 0.0f;
        if (value > 1.0f) value = 1.0f;
        lblSetCurrentBrightnessSetting(value);
    }

public:
    virtual tsl::elm::Element *createUI() override {
        auto *frame = new tsl::elm::OverlayFrame("Express Config", L("Brillo", "Brightness"));
        auto *list  = new tsl::elm::List();

        if (R_FAILED(g_lblRc)) {
            list->addItem(new tsl::elm::CategoryHeader(L("No disponible", "Not available")));
            list->addItem(new tsl::elm::ListItem(
                L("No se pudo abrir el servicio lbl", "Could not open lbl service")));
            frame->setContent(list);
            return frame;
        }

        float current = 0.5f;
        lblGetCurrentBrightnessSetting(&current);
        bool autoOn = false;
        lblIsAutoBrightnessControlEnabled(&autoOn);

        // ---- Barra deslizante de brillo ----
        list->addItem(new tsl::elm::CategoryHeader(L("Nivel de brillo", "Brightness level")));
        m_bar = new tsl::elm::TrackBar("☀");   // icono de sol
        m_bar->setProgress(static_cast<u8>(current * 100.0f + 0.5f));
        m_bar->setValueChangedListener([](u8 value) {
            applyBrightness(static_cast<float>(value) / 100.0f);
        });
        list->addItem(m_bar);

        // ---- Ajustes rápidos ----
        list->addItem(new tsl::elm::CategoryHeader(L("Ajustes rápidos", "Quick presets")));
        for (int preset : {10, 25, 50, 75, 100}) {
            auto *item = new tsl::elm::ListItem(std::to_string(preset) + " %");
            item->setClickListener([this, preset](u64 keys) {
                if (keys & HidNpadButton_A) {
                    applyBrightness(static_cast<float>(preset) / 100.0f);
                    if (m_bar) m_bar->setProgress(static_cast<u8>(preset));
                    return true;
                }
                return false;
            });
            list->addItem(item);
        }

        // ---- Opciones ----
        list->addItem(new tsl::elm::CategoryHeader(L("Opciones", "Options")));
        auto *autoToggle = new tsl::elm::ToggleListItem(L("Brillo automático", "Auto brightness"), autoOn);
        autoToggle->setStateChangedListener([](bool state) {
            if (R_FAILED(g_lblRc)) return;
            if (state) lblEnableAutoBrightnessControl();
            else       lblDisableAutoBrightnessControl();
        });
        list->addItem(autoToggle);

        frame->setContent(list);
        return frame;
    }
};

// ==========================================================================
//  Pantalla principal (menú)
// ==========================================================================
class MainGui : public tsl::Gui {
public:
    virtual tsl::elm::Element *createUI() override {
        auto *frame = new tsl::elm::OverlayFrame("Express Config", L("Acceso rápido", "Quick access"));
        auto *list  = new tsl::elm::List();

        list->addItem(new tsl::elm::CategoryHeader(L("Ajustes rápidos", "Quick settings")));

        auto *brightness = new tsl::elm::ListItem(L("Brillo", "Brightness"), "☀");
        brightness->setClickListener([](u64 keys) {
            if (keys & HidNpadButton_A) { tsl::changeTo<BrightnessGui>(); return true; }
            return false;
        });
        list->addItem(brightness);

        auto *bluetooth = new tsl::elm::ListItem("Bluetooth", "");
        bluetooth->setClickListener([](u64 keys) {
            if (keys & HidNpadButton_A) { tsl::changeTo<BluetoothGui>(); return true; }
            return false;
        });
        list->addItem(bluetooth);

        frame->setContent(list);
        return frame;
    }
};

// ==========================================================================
//  Overlay
// ==========================================================================
class ExpressConfig : public tsl::Overlay {
public:
    // libtesla ya inicializa fs, hid, pl, pmdmnt, hid:sys y set:sys.
    // Esto se ejecuta dentro de una sesión sm abierta por libtesla.
    virtual void initServices() override {
        g_lblRc = lblInitialize();
        g_btmRc = btmsysInitialize();

        // Detecta el idioma del sistema una sola vez al arrancar.
        if (R_SUCCEEDED(setInitialize())) {
            u64 code = 0;
            SetLanguage lang = SetLanguage_ENUS;
            if (R_SUCCEEDED(setGetSystemLanguage(&code)))
                setMakeLanguage(code, &lang);
            g_es = (lang == SetLanguage_ES || lang == SetLanguage_ES419);
            setExit();
        }
    }

    virtual void exitServices() override {
        if (R_SUCCEEDED(g_btmRc)) btmsysExit();
        if (R_SUCCEEDED(g_lblRc)) lblExit();
    }

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<MainGui>();
    }
};

int main(int argc, char **argv) {
    return tsl::loop<ExpressConfig>(argc, argv);
}
