#pragma once
#include <string>

namespace stl2glb {

/**
 * @class MinioClient
 * @brief Classe per l'interazione con il servizio di storage MinIO
 *
 * Fornisce funzionalità per il download e upload di file da/verso bucket MinIO.
 * Implementa il pattern singleton per garantire un'unica istanza del client MinIO.
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
                         const std::string& localPath);

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
                       const std::string& localPath);

    // Non è possibile creare istanze dirette di questa classe
    MinioClient() = delete;
    MinioClient(const MinioClient&) = delete;
    MinioClient& operator=(const MinioClient&) = delete;
};

} // namespace stl2glb