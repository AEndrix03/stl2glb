#pragma once
#include <string>
#include "stl2glb/SimpleMinioClient.hpp"

namespace stl2glb {

/**
 * @class MinioClient
 * @brief Adattatore per SimpleMinioClient che mantiene l'API originale
 *
 * Questa classe mantiene l'interfaccia originale di MinioClient
 * ma utilizza l'implementazione SimpleMinioClient sotto il cofano.
 */
    class MinioClient {
    public:
        /**
         * @brief Scarica un oggetto da un bucket MinIO
         *
         * @param bucket Nome del bucket da cui scaricare
         * @param objectName Nome dell'oggetto da scaricare
         * @param localPath Percorso locale dove salvare il file
         * @throws std::runtime_error in caso di errori di download o creazione directory
         */
        static void download(const std::string& bucket,
                             const std::string& objectName,
                             const std::string& localPath) {
            SimpleMinioClient::download(bucket, objectName, localPath);
        }

        /**
         * @brief Carica un oggetto su un bucket MinIO
         *
         * @param bucket Nome del bucket su cui caricare
         * @param objectName Nome dell'oggetto da caricare
         * @param localPath Percorso locale del file da caricare
         * @throws std::runtime_error in caso di errori di upload o file non trovato
         */
        static void upload(const std::string& bucket,
                           const std::string& objectName,
                           const std::string& localPath) {
            SimpleMinioClient::upload(bucket, objectName, localPath);
        }

        // Non è possibile creare istanze dirette di questa classe
        MinioClient() = delete;
        MinioClient(const MinioClient&) = delete;
        MinioClient& operator=(const MinioClient&) = delete;
    };

} // namespace stl2glb