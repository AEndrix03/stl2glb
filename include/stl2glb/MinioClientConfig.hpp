#pragma once
#include <miniocpp/client.h>
#include <string>

namespace stl2glb {

/**
 * @struct MinioClientConfig
 * @brief Configurazione per il client MinIO
 * 
 * Contiene tutte le impostazioni configurabili per il client MinIO,
 * come timeout, certificati SSL, ecc.
 */
    struct MinioClientConfig {
        // Timeout in millisecondi per la connessione
        unsigned int connect_timeout_ms = 30000;  // 30 secondi

        // Timeout in millisecondi per la lettura
        unsigned int read_timeout_ms = 30000;     // 30 secondi

        // Percorso del file di certificato SSL (vuoto per disabilitare SSL)
        std::string ssl_cert_file = "";

        // Numero di tentativi per le operazioni
        unsigned int retry_count = 3;

        // Protocollo predefinito se non specificato nell'endpoint
        std::string default_protocol = "http";

        // Converte questa configurazione in un oggetto minio::s3::ClientConfig
        minio::s3::ClientConfig toMinioConfig() const {
            minio::s3::ClientConfig config;
            config.connect_timeout_ms = connect_timeout_ms;
            config.read_timeout_ms = read_timeout_ms;
            config.ssl_cert_file = ssl_cert_file;
            return config;
        }
    };

} // namespace stl2glb