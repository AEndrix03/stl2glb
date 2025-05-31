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

                // Implementazione semplice con httplib
                // Questo è solo un esempio, potrebbe essere necessario usare altra libreria
                // o implementare un test più robusto

                // Estrai host e porta dall'endpoint
                std::string host = endpoint;
                int port = 80;

                // Rimuovi http:// o https:// se presente
                if (host.find("http://") == 0) {
                    host = host.substr(7);
                } else if (host.find("https://") == 0) {
                    host = host.substr(8);
                    port = 443;
                }

                // Estrai porta se specificata
                size_t colonPos = host.find(":");
                if (colonPos != std::string::npos) {
                    port = std::stoi(host.substr(colonPos + 1));
                    host = host.substr(0, colonPos);
                }

                // Usa httplib per verificare la connessione
                httplib::Client cli(host, port);
                cli.set_connection_timeout(timeout_ms / 1000); // Converte da ms a sec

                auto res = cli.Get("/");
                if (res) {
                    Logger::info("Endpoint is reachable");
                    return true;
                } else {
                    Logger::error("Endpoint is not reachable: " + httplib::to_string(res.error()));
                    return false;
                }
            } catch (const std::exception& e) {
                Logger::error("Exception in endpoint reachability test: " + std::string(e.what()));
                return false;
            }
        }
    };

} // namespace stl2glb