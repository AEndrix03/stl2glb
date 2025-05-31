#pragma once
#include <string>
#include <miniocpp/client.h>
#include "stl2glb/Logger.hpp"

namespace stl2glb {

/**
 * @class MinioConnectionCheck
 * @brief Classe di utilità per verificare la connessione a MinIO
 *
 * Fornisce metodi per verificare la connessione a un server MinIO
 * e diagnosticare eventuali problemi di connessione.
 */
    class MinioConnectionCheck {
    public:
        /**
         * @brief Verifica la connessione a un server MinIO
         *
         * @param endpoint Endpoint del server MinIO
         * @param accessKey Chiave di accesso
         * @param secretKey Chiave segreta
         * @param timeout_ms Timeout in millisecondi (default: 5000)
         * @return bool True se la connessione è riuscita, False altrimenti
         */
        static bool checkConnection(const std::string& endpoint,
                                    const std::string& accessKey,
                                    const std::string& secretKey,
                                    unsigned int timeout_ms = 5000) {
            try {
                Logger::info("Testing connection to MinIO endpoint: " + endpoint);

                // Controlla se l'endpoint include già il protocollo
                std::string fullEndpoint = endpoint;
                if (endpoint.find("http://") != 0 && endpoint.find("https://") != 0) {
                    fullEndpoint = "http://" + endpoint;
                }

                // Crea credenziali
                minio::creds::StaticProvider credentials(accessKey, secretKey);

                // Crea client (senza config che non è disponibile nella tua versione)
                minio::s3::BaseUrl baseUrl{fullEndpoint};
                minio::s3::Client client(baseUrl, &credentials);

                // Tenta di elencare i bucket
                minio::s3::ListBucketsArgs args;
                auto result = client.ListBuckets(args);

                if (!result) {
                    Logger::error("Connection test failed: " + result.message);
                    return false;
                }

                // Conta i bucket
                size_t numBuckets = result.buckets.size();
                Logger::info("Connection successful. Found " + std::to_string(numBuckets) + " buckets.");

                return true;
            } catch (const std::exception& e) {
                Logger::error("Exception in connection test: " + std::string(e.what()));
                return false;
            }
        }

        /**
         * @brief Verifica la raggiungibilità di un endpoint senza autenticazione
         *
         * @param endpoint Endpoint da verificare
         * @param timeout_ms Timeout in millisecondi (default: 5000)
         * @return bool True se l'endpoint è raggiungibile, False altrimenti
         */
        static bool checkEndpointReachable(const std::string& endpoint,
                                           unsigned int timeout_ms = 5000) {
            try {
                Logger::info("Testing if endpoint is reachable: " + endpoint);

                // Rimuovi protocollo se presente
                std::string cleanEndpoint = endpoint;
                if (cleanEndpoint.find("http://") == 0) {
                    cleanEndpoint = cleanEndpoint.substr(7);
                } else if (cleanEndpoint.find("https://") == 0) {
                    cleanEndpoint = cleanEndpoint.substr(8);
                }

                // Estrai host e porta
                std::string host;
                int port = 80;

                size_t colonPos = cleanEndpoint.find(":");
                if (colonPos != std::string::npos) {
                    port = std::stoi(cleanEndpoint.substr(colonPos + 1));
                    host = cleanEndpoint.substr(0, colonPos);
                } else {
                    host = cleanEndpoint;
                }

                // Utilizziamo direttamente un socket per verificare la connessione
                // anziché httplib che potrebbe non essere disponibile

                // Creiamo un endpoint fittizio per Minio con credenziali vuote
                // per verificare solo la connettività di base
                minio::s3::BaseUrl baseUrl("http://" + host + ":" + std::to_string(port));
                minio::creds::StaticProvider creds("", "");
                minio::s3::Client client(baseUrl, &creds);

                // Tentiamo un'operazione semplice come ListBuckets per verificare la connettività
                minio::s3::ListBucketsArgs args;
                auto result = client.ListBuckets(args);

                // Non ci interessa se l'operazione è riuscita (probabilmente fallirà per credenziali),
                // ma se siamo riusciti a stabilire una connessione
                if (result.code == "AccessDenied" || result.code == "InvalidAccessKeyId") {
                    // Questi errori indicano che il server è raggiungibile, ma le credenziali sono sbagliate
                    Logger::info("Endpoint is reachable (auth failed but connection worked)");
                    return true;
                } else if (!result.message.empty()) {
                    // Abbiamo ricevuto una risposta dal server
                    Logger::info("Endpoint is reachable with response: " + result.code);
                    return true;
                }

                Logger::error("Endpoint is not reachable: No valid response from server");
                return false;
            } catch (const std::exception& e) {
                Logger::error("Exception in endpoint reachability test: " + std::string(e.what()));
                return false;
            }
        }
    };

} // namespace stl2glb