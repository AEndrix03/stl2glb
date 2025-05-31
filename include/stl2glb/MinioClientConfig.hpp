#pragma once
#include <string>

namespace stl2glb {

/**
 * @struct MinioClientSettings
 * @brief Impostazioni per il client MinIO
 *
 * Contiene le impostazioni per il client MinIO che possono essere
 * configurate indipendentemente dalla presenza di ClientConfig nell'API.
 */
    struct MinioClientSettings {
        // Protocollo predefinito se non specificato nell'endpoint
        std::string default_protocol = "http";

        // Numero di tentativi per le operazioni
        unsigned int retry_count = 3;

        // Timeout in secondi per le operazioni HTTP (se supportato dalla libreria)
        unsigned int timeout_seconds = 30;
    };

} // namespace stl2glb